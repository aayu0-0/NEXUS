/**
 * @file  fault_watchdog_reset.c
 * @brief NEXUS — Hardware watchdog (IWDG) reset detection + fault injection
 */

#include "fault_watchdog_reset.h"
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

/* CMSIS device header — provides RCC, IWDG register definitions.
 * Swap for your exact part if it isn't an F4 (e.g. stm32f1xx.h). */
#include "stm32f4xx.h"

#define WDR_NORMAL_CYCLES     20u
#define WDR_PERIOD_MS         30u

/* IWDG runs off the 32kHz LSI clock, independent of the core clock,
 * which is exactly why it's used for last-resort fault recovery —
 * it keeps working even if the main clock tree is wedged. */
#define IWDG_PRESCALER_64     0x04u
#define IWDG_RELOAD_VALUE     250u   /* ~500ms timeout at /64 prescaler */

#define IWDG_KEY_ENABLE       0x0000CCCCu
#define IWDG_KEY_WRITE_ACCESS 0x00005555u
#define IWDG_KEY_REFRESH      0x0000AAAAu

/* -----------------------------------------------------------
 * Boot-time reset cause detection
 *
 * RCC_CSR bit layout (STM32F4 family):
 *   bit 31 - LPWRRSTF  Low-power reset
 *   bit 30 - WWDGRSTF  Window watchdog reset
 *   bit 29 - IWDGRSTF  Independent watchdog reset   <-- this one
 *   bit 28 - SFTRSTF   Software reset
 *   bit 27 - PORRSTF   POR/PDR reset
 *   bit 26 - PINRSTF   NRST pin reset
 *   bit 25 - BORRSTF   Brown-out reset
 *   bit 24 - RMVF      Remove reset flags (write 1 to clear all of the above)
 * ----------------------------------------------------------*/

#define RCC_CSR_IWDGRSTF_Pos   29u
#define RCC_CSR_IWDGRSTF_Msk   (1u << RCC_CSR_IWDGRSTF_Pos)
#define RCC_CSR_RMVF_Pos       24u
#define RCC_CSR_RMVF_Msk       (1u << RCC_CSR_RMVF_Pos)

void WDT_CheckResetCause(void)
{
    uint32_t csr = RCC->CSR;

    if ((csr & RCC_CSR_IWDGRSTF_Msk) != 0u)
    {
        /* Payload carries the raw CSR value — every other reset flag
         * (POR, pin, brown-out, etc.) is preserved in it for later
         * analysis, in case more than one bit is set. */
        LOG_FAULT(EVT_WDT_HW_RESET, csr);

        Logger_DumpUART();
    }

    /* Always clear reset flags so the next reboot starts clean —
     * otherwise every future boot, watchdog or not, reports stale flags. */
    RCC->CSR |= RCC_CSR_RMVF_Msk;
}

/* -----------------------------------------------------------
 * IWDG fault injection task
 *
 * Strategy:
 *   Phase 1 — Normal operation, IWDG kicked on schedule.
 *   Phase 2 — Task deliberately stops kicking.
 *   Phase 3 — Hardware resets the chip. Nothing more executes here;
 *             the story continues in WDT_CheckResetCause() on next boot.
 * ----------------------------------------------------------*/

static void IWDG_Enable(void)
{
    /* Unlock write access to PR/RLR */
    IWDG->KR = IWDG_KEY_WRITE_ACCESS;

    IWDG->PR  = IWDG_PRESCALER_64;
    IWDG->RLR = IWDG_RELOAD_VALUE;

    /* Start the watchdog — once started it CANNOT be disabled
     * by software until the next reset. */
    IWDG->KR = IWDG_KEY_ENABLE;
}

static inline void IWDG_Kick(void)
{
    IWDG->KR = IWDG_KEY_REFRESH;
}

void Task_FaultWatchdogReset(void *pvParameters)
{
    (void)pvParameters;

    IWDG_Enable();

    /* -------------------------------------------------------
     * Normal operation phase — kick on schedule
     * ------------------------------------------------------*/

    for (uint32_t i = 0u; i < WDR_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        IWDG_Kick();

        vTaskDelay(pdMS_TO_TICKS(WDR_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* -------------------------------------------------------
     * Fault injection phase — stop kicking, IWDG resets the chip.
     *
     * No log call here: the buffer lives in normal RAM and is
     * wiped by the reset. The trace ends at the last EVT_TASK_END
     * above; the story resumes via WDT_CheckResetCause() on reboot.
     * ------------------------------------------------------*/

    for (;;)
    {
        __NOP();
    }
}
