#include "fault_stack_overflow.h"
#include "logger.h"
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

#define SO_WARNING_WORDS   64u
#define SO_NORMAL_CYCLES   30u
#define SO_PERIOD_MS       20u

static volatile uint32_t s_depth = 0u;

/* -----------------------------------------------------------
 * Recursive fault trigger
 * ----------------------------------------------------------*/

__attribute__((noinline, optimize("O0")))
void TriggerFault_StackOverflow(void)
{
    volatile uint8_t frame[64];
    (void)frame;

    s_depth++;

    UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);

    if (watermark < SO_WARNING_WORDS)
    {
        LOG_WARN(EVT_STACK_WARNING, (uint32_t)watermark);
    }

    TriggerFault_StackOverflow();      /* Intentional recursion */
}

/* -----------------------------------------------------------
 * Fault injection task
 * ----------------------------------------------------------*/

void Task_FaultStackOverflow(void *pvParameters)
{
    (void)pvParameters;

    for (uint32_t i = 0u; i < SO_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(SO_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    TriggerFault_StackOverflow();

    /* Never reached */
    vTaskDelete(NULL);
}

/* -----------------------------------------------------------
 * FreeRTOS Stack Overflow Hook
 * ----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    LOG_FAULT(EVT_STACK_OVERFLOW, s_depth);

    Logger_DumpUART();

    for (;;)
    {
        __NOP();
    }
}
