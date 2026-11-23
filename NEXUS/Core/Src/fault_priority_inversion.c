/**
 * @file  fault_priority_inversion.c
 * @brief NEXUS — Priority inversion fault injection
 *
 * Strategy:
 *   Three tasks, three priorities:
 *     - LowTask    (lowest)  — takes the shared resource (semaphore),
 *                              then does long "work", then releases.
 *     - MedTask    (middle)  — pure CPU-bound busy task, no resource
 *                              needed. Repeatedly preempts LowTask.
 *     - HighTask   (highest) — wants the same resource. Blocks on it
 *                              as soon as LowTask is holding it.
 *
 *   Because a plain binary semaphore (not a FreeRTOS mutex) is used,
 *   there is no priority inheritance. MedTask can run indefinitely
 *   while LowTask is preempted and never releases the semaphore —
 *   so HighTask is starved despite having the highest priority.
 *
 *   EVT_PI_HIGH_WAITING marks "high task blocked, waiting"
 *   EVT_PI_HIGH_ACQUIRED marks "high task acquired resource" (recovery)
 *   EVT_PI_STARVATION (LVL_FAULT) fires if HighTask's wait exceeds
 *   HIGH_WAIT_TIMEOUT_MS — confirms genuine starvation rather than
 *   a normal short wait.
 *
 * Requirements:
 *   - configUSE_MUTEXES can be 0 or 1; this deliberately avoids
 *     xSemaphoreCreateMutex() and uses xSemaphoreCreateBinary()
 *     instead, specifically to bypass priority inheritance.
 */

#include "fault_priority_inversion.h"
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define PRIO_LOW                  (tskIDLE_PRIORITY + 1u)
#define PRIO_MED                  (tskIDLE_PRIORITY + 2u)
#define PRIO_HIGH                 (tskIDLE_PRIORITY + 3u)

#define LOW_HOLD_TIME_MS           2000u   /* how long LowTask holds the resource */
#define MED_BUSY_SLICE_MS          50u     /* MedTask busy-loop slice, yields briefly */
#define MED_YIELD_MS               5u
#define HIGH_WAIT_TIMEOUT_MS       5000u   /* how long HighTask waits before declaring fault */

#define PI_NORMAL_CYCLES           10u
#define PI_PERIOD_MS               30u

static SemaphoreHandle_t s_resource = NULL;
static volatile uint32_t s_high_wait_start = 0u;

/* -----------------------------------------------------------
 * Low-priority task — acquires the shared resource and holds it
 * for a long time, simulating real work.
 * ----------------------------------------------------------*/

static void LowTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        if (xSemaphoreTake(s_resource, portMAX_DELAY) == pdTRUE)
        {
            LOG_INFO(EVT_PI_LOW_ACQUIRED, 0u);   /* low task acquired resource */

            /* Simulate long critical-section work. Busy-loop instead of
             * vTaskDelay so that MedTask, once it preempts, keeps this
             * task off the CPU for the full duration rather than letting
             * the scheduler return control after a clean delay. */
            TickType_t start = xTaskGetTickCount();
            while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(LOW_HOLD_TIME_MS))
            {
                __NOP();
            }

            xSemaphoreGive(s_resource);
            LOG_INFO(EVT_PI_LOW_RELEASED, 0u);   /* low task released resource */
        }

        vTaskDelay(pdMS_TO_TICKS(100u));
    }
}

/* -----------------------------------------------------------
 * Medium-priority task — needs no shared resource. Runs CPU-bound
 * work that repeatedly preempts LowTask, starving it (and by
 * extension, HighTask waiting on LowTask's resource).
 * ----------------------------------------------------------*/

static void MedTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(MED_BUSY_SLICE_MS))
        {
            __NOP();
        }

        /* Brief yield so the system isn't completely locked, but short
         * enough that LowTask rarely gets meaningful CPU time. */
        vTaskDelay(pdMS_TO_TICKS(MED_YIELD_MS));
    }
}

/* -----------------------------------------------------------
 * High-priority task — wants the shared resource. Logs how long
 * it waits; declares a fault if the wait is unreasonably long.
 * ----------------------------------------------------------*/

static void HighTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        s_high_wait_start = (uint32_t)xTaskGetTickCount();

        LOG_INFO(EVT_PI_HIGH_WAITING, 0u);   /* high task begins waiting */

        BaseType_t got = xSemaphoreTake(
            s_resource,
            pdMS_TO_TICKS(HIGH_WAIT_TIMEOUT_MS));

        if (got == pdTRUE)
        {
            uint32_t waited = (uint32_t)xTaskGetTickCount() - s_high_wait_start;

            LOG_INFO(EVT_PI_HIGH_ACQUIRED, waited);   /* high task acquired, payload = wait time */

            xSemaphoreGive(s_resource);
        }
        else
        {
            uint32_t waited = (uint32_t)xTaskGetTickCount() - s_high_wait_start;

            /* HighTask never got the resource within the timeout despite
             * being the highest-priority task in the system — confirms
             * unbounded priority inversion, not just a slow turn. */
            LOG_FAULT(EVT_PI_STARVATION, waited);

            Logger_DumpUART();

            taskDISABLE_INTERRUPTS();

            for (;;)
            {
                __NOP();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50u));
    }
}

/* -----------------------------------------------------------
 * Fault injection entry task
 *
 * Spawns the three-task scenario and then deletes itself —
 * the three sub-tasks own the rest of the run.
 * ----------------------------------------------------------*/

void Task_FaultPriorityInversion(void *pvParameters)
{
    (void)pvParameters;

    for (uint32_t i = 0u; i < PI_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(PI_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* Binary semaphore, NOT a mutex — no priority inheritance */
    s_resource = xSemaphoreCreateBinary();

    if (s_resource == NULL)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

        taskDISABLE_INTERRUPTS();

        for (;;)
        {
            __NOP();
        }
    }

    /* Must give once — binary semaphores start "empty" */
    xSemaphoreGive(s_resource);

    xTaskCreate(LowTask,  "PI_Low",  256, NULL, PRIO_LOW,  NULL);
    xTaskCreate(MedTask,  "PI_Med",  256, NULL, PRIO_MED,  NULL);
    xTaskCreate(HighTask, "PI_High", 256, NULL, PRIO_HIGH, NULL);

    vTaskDelete(NULL);
}
