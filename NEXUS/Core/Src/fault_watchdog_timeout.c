/**

* @file  fault_watchdog_timeout.c
* @brief NEXUS — Watchdog timeout fault injection
*
* Strategy:
* Phase 1 — Normal operation with periodic watchdog kicks.
* Phase 2 — Task deliberately hangs and stops kicking.
* Phase 3 — Software watchdog detects timeout and raises fault.
  */

#include "fault_watchdog_timeout.h"

#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#define WDT_TIMEOUT_MS       500u
#define WDT_KICK_PERIOD_MS   100u
#define WDT_NORMAL_CYCLES    20u
#define WDT_LOG_INTERVAL     5u

static TimerHandle_t s_wdt_timer = NULL;

static volatile uint32_t s_kick_flag  = 0u;
static volatile uint32_t s_kick_count = 0u;

/* -----------------------------------------------------------

* Software watchdog timer callback
* ----------------------------------------------------------*/

static void WatchdogTimerCallback(TimerHandle_t xTimer)
{
(void)xTimer;


if (s_kick_flag == 0u)
{
    LOG_FAULT(EVT_WDT_TIMEOUT, s_kick_count);

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __NOP();
    }
}

s_kick_flag = 0u;


}

/* -----------------------------------------------------------

* Fault injection task
* ----------------------------------------------------------*/

void Task_FaultWatchdogTimeout(void *pvParameters)
{
(void)pvParameters;


s_wdt_timer = xTimerCreate(
    "NEXUS_WDT",
    pdMS_TO_TICKS(WDT_TIMEOUT_MS),
    pdTRUE,
    NULL,
    WatchdogTimerCallback);

if (s_wdt_timer == NULL)
{
    LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __NOP();
    }
}

if (xTimerStart(s_wdt_timer, 0u) != pdPASS)
{
    LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __NOP();
    }
}

/* -------------------------------------------------------
 * Normal operation phase
 * ------------------------------------------------------*/

for (uint32_t i = 0u; i < WDT_NORMAL_CYCLES; i++)
{
    LOG_INFO(EVT_TASK_START, i);

    s_kick_flag = 1u;
    s_kick_count++;

    if ((s_kick_count % WDT_LOG_INTERVAL) == 0u)
    {
        LOG_DEBUG(EVT_WDT_KICK, s_kick_count);
    }

    vTaskDelay(pdMS_TO_TICKS(WDT_KICK_PERIOD_MS));

    LOG_INFO(EVT_TASK_END, i);
}

/* -------------------------------------------------------
 * Fault injection phase
 * -------------------------------------------------------
 *
 * Simulate a hung task.
 * No more watchdog kicks occur.
 * Timer callback will eventually detect timeout.
 */

for (;;)
{
    __NOP();
}
}
