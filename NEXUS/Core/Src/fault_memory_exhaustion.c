/**
 * @file  fault_memory_exhaustion.c
 * @brief NEXUS — Memory fault injection: low-heap warning + full exhaustion
 *
 * Strategy:
 *   Phase 1 — Normal operation cycles.
 *   Phase 2 — Low-heap hold: allocate down to just below the warning
 *             threshold, then idle there for LOW_HEAP_HOLD_CYCLES,
 *             logging EVT_LOW_HEAP periodically. Heap never fully
 *             exhausts in this phase — produces a clean "sustained
 *             low heap" trace distinct from outright exhaustion.
 *   Phase 3 — Exhaustion: resume allocating until pvPortMalloc()
 *             returns NULL. Handled by vApplicationMallocFailedHook().
 */

#include "fault_memory_exhaustion.h"
#include "logger.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>

#define MEM_CHUNK_BYTES          512u
#define MEM_WARNING_BYTES        2048u
#define MEM_NORMAL_CYCLES        20u
#define MEM_PERIOD_MS            30u

#define LOW_HEAP_HOLD_CYCLES     15u   /* how long to sit in the low-heap zone */
#define LOW_HEAP_LOG_PERIOD_MS   200u
#define LOW_HEAP_TARGET_MARGIN   256u  /* stop just above 0 free, below warn */

static volatile uint32_t s_alloc_count = 0u;

/* -----------------------------------------------------------
 * Low-heap fault trigger
 *
 * Allocates down to just under MEM_WARNING_BYTES, then holds
 * there — never crossing into full exhaustion.
 * ----------------------------------------------------------*/

void TriggerFault_LowHeap(void)
{
    /* Drain heap down toward the warning threshold */
    while (xPortGetFreeHeapSize() > MEM_WARNING_BYTES)
    {
        void *p = pvPortMalloc(MEM_CHUNK_BYTES);

        if (p == NULL)
        {
            /* Hit exhaustion early — let the malloc-failed hook handle it */
            return;
        }

        s_alloc_count++;
        vTaskDelay(pdMS_TO_TICKS(10u));
    }

    /* Hold steady in the low-heap zone, logging periodically */
    for (uint32_t i = 0u; i < LOW_HEAP_HOLD_CYCLES; i++)
    {
        size_t free_heap = xPortGetFreeHeapSize();

        LOG_WARN(EVT_LOW_HEAP, (uint32_t)free_heap);

        vTaskDelay(pdMS_TO_TICKS(LOW_HEAP_LOG_PERIOD_MS));
    }
}

/* -----------------------------------------------------------
 * Heap exhaustion fault trigger
 * ----------------------------------------------------------*/

void TriggerFault_MemExhaustion(void)
{
    for (;;)
    {
        size_t free_heap = xPortGetFreeHeapSize();

        if (free_heap < MEM_WARNING_BYTES)
        {
            LOG_WARN(EVT_LOW_HEAP, (uint32_t)free_heap);
        }

        volatile void *p = pvPortMalloc(MEM_CHUNK_BYTES);

        /* Allocation failure is handled by vApplicationMallocFailedHook() */
        if (p == NULL)
        {
            break;
        }

        (void)p;

        s_alloc_count++;

        vTaskDelay(pdMS_TO_TICKS(10u));
    }
}

/* -----------------------------------------------------------
 * Fault injection task
 *
 * Runs the low-heap hold phase first, then proceeds into full
 * exhaustion — so a single boot/run captures both signatures
 * back to back, clearly separated in the dumped trace.
 * ----------------------------------------------------------*/

void Task_FaultMemExhaustion(void *pvParameters)
{
    (void)pvParameters;

    /* Normal operation phase */
    for (uint32_t i = 0u; i < MEM_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(MEM_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* Low-heap hold phase */
    TriggerFault_LowHeap();

    /* Exhaustion phase */
    TriggerFault_MemExhaustion();

    for (;;)
    {
        __NOP();
    }
}

/* -----------------------------------------------------------
 * FreeRTOS malloc failed hook
 * ----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    LOG_FAULT(EVT_MEM_ALLOC_FAIL, s_alloc_count);

    Logger_DumpUART();

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __NOP();
    }
}
