/**
 * @file  fault_task_create_fail.c
 * @brief NEXUS — Task creation failure fault injection
 *
 * Strategy:
 *   Phase 1 — Normal operation cycles.
 *   Phase 2 — Repeatedly create dummy tasks without deleting them.
 *             Each creation consumes heap (stack + TCB).
 *             EVT_LOW_HEAP is logged once when the threshold is crossed.
 *   Phase 3 — xTaskCreate() fails, fault declared.
 */

#include "fault_task_create_fail.h"
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

#define TCF_NORMAL_CYCLES        20u
#define TCF_PERIOD_MS            30u
#define TCF_DUMMY_STACK_WORDS   128u
#define TCF_LOW_HEAP_THRESHOLD 2048u
#define TCF_CREATE_PERIOD_MS      5u

static volatile uint32_t s_created_count = 0u;
static uint8_t s_low_heap_warned = 0u;

/* -----------------------------------------------------------
 * Dummy task
 *
 * Exists only to consume heap (stack + TCB).
 * Suspends itself immediately.
 * ----------------------------------------------------------*/

static void DummyTask(void *pvParameters)
{
    (void)pvParameters;

    vTaskSuspend(NULL);

    for (;;)
    {
        /* Should never execute */
    }
}

/* -----------------------------------------------------------
 * Fault injection task
 * ----------------------------------------------------------*/

void Task_FaultTaskCreateFail(void *pvParameters)
{
    (void)pvParameters;

    /* -------------------------------------------------------
     * Normal operation phase
     * ------------------------------------------------------*/

    for (uint32_t i = 0u; i < TCF_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(TCF_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* -------------------------------------------------------
     * Fault injection phase
     * ------------------------------------------------------*/

    for (;;)
    {
        uint32_t free_heap = (uint32_t)xPortGetFreeHeapSize();

        /* Log low-heap warning only once */
        if ((free_heap < TCF_LOW_HEAP_THRESHOLD) &&
            (s_low_heap_warned == 0u))
        {
            LOG_WARN(EVT_LOW_HEAP, free_heap);

            s_low_heap_warned = 1u;
        }

        BaseType_t result = xTaskCreate(
            DummyTask,
            "DUMMY",
            TCF_DUMMY_STACK_WORDS,
            NULL,
            tskIDLE_PRIORITY + 1u,
            NULL);

        if (result != pdPASS)
        {
            /* Payload = successful task creations */
            LOG_FAULT(EVT_TASK_CREATE_FAIL,
                      s_created_count);

            taskDISABLE_INTERRUPTS();

            for (;;)
            {
                __NOP();
            }
        }

        s_created_count++;

        /* EVT_TASK_CREATE_OK = successful dummy task creation */
        LOG_INFO(EVT_TASK_CREATE_OK,
                 s_created_count);

        /* Allow scheduler to run */
        vTaskDelay(pdMS_TO_TICKS(TCF_CREATE_PERIOD_MS));
    }
}
