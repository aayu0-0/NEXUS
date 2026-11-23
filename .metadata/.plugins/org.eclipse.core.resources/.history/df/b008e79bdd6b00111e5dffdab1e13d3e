/**
 * @file  fault_comm_timeout.c
 * @brief NEXUS — Communication timeout fault injection
 *
 * Strategy:
 *   Phase 1 — Normal operation, mock communication succeeds.
 *   Phase 2 — Mock bus stops responding, retries begin.
 *   Phase 3 — Consecutive transaction failures hit threshold,
 *             communication fault is declared.
 */

#include "fault_comm_timeout.h"

#include "logger.h"

#include "FreeRTOS.h"
#include "task.h"

#define COMM_NORMAL_CYCLES       20u
#define COMM_SUCCESS_LIMIT       10u
#define COMM_MAX_RETRIES          3u
#define COMM_FAULT_THRESHOLD      3u
#define COMM_PERIOD_MS           30u

/* Counts total communication attempts */
static volatile uint32_t s_attempt_count = 0u;

/* -----------------------------------------------------------
 * Mock communication peripheral
 *
 * Returns success for the first COMM_SUCCESS_LIMIT attempts.
 * After that, simulates a dead bus by always timing out.
 * ----------------------------------------------------------*/

static BaseType_t MockComm_Transmit(void)
{
    s_attempt_count++;

    if (s_attempt_count <= COMM_SUCCESS_LIMIT)
    {
        return pdTRUE;
    }

    return pdFALSE;
}

/* -----------------------------------------------------------
 * Attempt one transaction with retries
 *
 * Returns:
 *   pdTRUE  - transaction successful
 *   pdFALSE - all retries exhausted
 * ----------------------------------------------------------*/

static BaseType_t Comm_TransmitWithRetry(void)
{
    for (uint32_t attempt = 0u;
         attempt < COMM_MAX_RETRIES;
         attempt++)
    {
        if (MockComm_Transmit() == pdTRUE)
        {
            return pdTRUE;
        }

        LOG_WARN(EVT_COMM_TIMEOUT,
                 attempt + 1u);
    }

    return pdFALSE;
}

/* -----------------------------------------------------------
 * Fault injection task
 * ----------------------------------------------------------*/

void Task_FaultCommTimeout(void *pvParameters)
{
    (void)pvParameters;

    uint32_t consecutive_failures = 0u;

    /* -------------------------------------------------------
     * Normal operation phase
     * ------------------------------------------------------*/

    for (uint32_t i = 0u;
         i < COMM_NORMAL_CYCLES;
         i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        vTaskDelay(pdMS_TO_TICKS(COMM_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* -------------------------------------------------------
     * Fault injection phase
     * ------------------------------------------------------*/

    for (;;)
    {
        if (Comm_TransmitWithRetry() == pdTRUE)
        {
            consecutive_failures = 0u;
        }
        else
        {
            consecutive_failures++;

            if (consecutive_failures >= COMM_FAULT_THRESHOLD)
            {
                LOG_FAULT(EVT_COMM_ERROR,
                          s_attempt_count);

                taskDISABLE_INTERRUPTS();

                for (;;)
                {
                    __NOP();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_PERIOD_MS));
    }
}
