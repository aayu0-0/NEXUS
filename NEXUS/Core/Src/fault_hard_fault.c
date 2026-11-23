/**
 * @file  fault_hard_fault.c
 * @brief NEXUS — Cortex-M HardFault fault injection
 *
 * Strategy:
 *   Phase 1 — Normal operation cycles.
 *   Phase 2 — Dereference a NULL function pointer, triggering a
 *             genuine Cortex-M HardFault exception.
 *   Phase 3 — HardFault_Handler (defined in stm32f4xx_it.c, inside the
 *             USER CODE BEGIN/END HardFault_IRQn 0 markers so it
 *             survives CubeMX regeneration) branches into
 *             HardFault_LogAndHalt() below, which extracts CFSR and
 *             logs EVT_HARD_FAULT before halting.
 *
 * Requirements:
 *   - stm32f4xx_it.c must contain the asm trampoline inside
 *     HardFault_Handler (see project notes). This file does NOT
 *     define HardFault_Handler itself — only HardFault_LogAndHalt,
 *     which the trampoline branches to.
 */

#include "fault_hard_fault.h"
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"

#define HF_NORMAL_CYCLES     20u
#define HF_PERIOD_MS         30u

/* -----------------------------------------------------------
 * Fault trigger — NULL function pointer call
 *
 * Volatile + function-pointer indirection prevents the compiler
 * from optimizing away or diagnosing the NULL deref at build time.
 * ----------------------------------------------------------*/

typedef void (*FaultFunc_t)(void);

void TriggerFault_HardFault(void)
{
    volatile FaultFunc_t bad_func = (FaultFunc_t)NULL;

    bad_func();   /* Branches to address 0x00000000 -> HardFault */
}

/* -----------------------------------------------------------
 * Fault injection task
 * ----------------------------------------------------------*/

void Task_FaultHardFault(void *pvParameters)
{
    (void)pvParameters;

    for (uint32_t i = 0u; i < HF_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(HF_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    TriggerFault_HardFault();

    /* Never reached */
    for (;;)
    {
        __NOP();
    }
}

/* -----------------------------------------------------------
 * HardFault landing function
 *
 * Called from the asm trampoline in stm32f4xx_it.c's
 * HardFault_Handler, with the stacked frame pointer in r0.
 * Frame layout: r0,r1,r2,r3,r12,LR,PC,xPSR
 * ----------------------------------------------------------*/

void HardFault_LogAndHalt(uint32_t *stacked_frame)
{
    uint32_t faulting_pc = stacked_frame[6];
    uint32_t cfsr        = SCB->CFSR;

    (void)faulting_pc;

    /* Payload = CFSR; faulting PC is recoverable from this if you
     * also want to log it via a second EVT_USER_x entry. */
    LOG_FAULT(EVT_HARD_FAULT, cfsr);

    Logger_DumpUART();

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __NOP();
    }
}
