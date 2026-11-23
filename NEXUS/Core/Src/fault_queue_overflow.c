#include <fault_queue_overflow.h>
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define QO_QUEUE_DEPTH            10u
#define QO_SEND_ATTEMPTS          50u
#define QO_NORMAL_CYCLES          20u
#define QO_PERIOD_MS              30u
#define QO_DROP_FAULT_THRESHOLD    5u

static volatile uint32_t s_drop_count = 0u;
static QueueHandle_t s_queue = NULL;

/* -----------------------------------------------------------
 * Queue overflow fault injection task
 * ----------------------------------------------------------*/

void Task_FaultQueueOverflow(void *pvParameters)
{
    (void)pvParameters;

    /* Create queue */
    s_queue = xQueueCreate(QO_QUEUE_DEPTH,
                           sizeof(uint32_t));

    if (s_queue == NULL)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

        Logger_DumpUART();

        taskDISABLE_INTERRUPTS();

        for (;;)
        {
            __NOP();
        }
    }

    /* -------------------------------------------------------
     * Normal operation phase
     * ------------------------------------------------------*/

    for (uint32_t i = 0u; i < QO_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(QO_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* -------------------------------------------------------
     * Fault injection phase
     * ------------------------------------------------------*/

    for (uint32_t i = 0u; i < QO_SEND_ATTEMPTS; i++)
    {
        BaseType_t result;

        result = xQueueSend(s_queue,
                            &i,
                            0u);     /* No blocking */

        if (result != pdTRUE)
        {
            s_drop_count++;

            LOG_WARN(EVT_QUEUE_OVERFLOW,
                     s_drop_count);

            if (s_drop_count >= QO_DROP_FAULT_THRESHOLD)
            {
                LOG_FAULT(EVT_QUEUE_OVERFLOW,
                          s_drop_count);

                Logger_DumpUART();

                taskDISABLE_INTERRUPTS();

                for (;;)
                {
                    __NOP();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5u));
    }

    /* Cleanup (only reached if threshold never triggers) */

    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }

    vTaskDelete(NULL);
}
