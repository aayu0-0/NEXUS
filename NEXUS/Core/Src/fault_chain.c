/**
 * @file  fault_chain.c
 * @brief NEXUS — Cascading fault chain injection
 *
 * Stage 1 — Comm degradation  → EVT_COMM_TIMEOUT / EVT_COMM_ERROR
 * Stage 2 — Queue backup      → EVT_QUEUE_OVERFLOW
 * Stage 3 — Memory pressure   → EVT_LOW_HEAP → EVT_MEM_ALLOC_FAIL
 * Stage 4 — Watchdog timeout  → EVT_WDT_TIMEOUT  (terminal halt)
 */

#include "fault_chain.h"
#include "logger.h"

#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* -----------------------------------------------------------
 * Tunables
 * ----------------------------------------------------------*/

#define FC_NORMAL_CYCLES          20u    /* healthy operation before cascade starts */
#define FC_PERIOD_MS              30u

/* Stage 1 — comm */
#define FC_COMM_SUCCESS_LIMIT     10u    /* mock bus responds OK for this many sends */
#define FC_COMM_MAX_RETRIES        3u
#define FC_COMM_FAULT_THRESHOLD    3u    /* consecutive total failures before EVT_COMM_ERROR */
#define FC_COMM_PERIOD_MS         20u

/* Stage 2 — queue */
#define FC_QUEUE_DEPTH            10u
#define FC_QUEUE_SEND_COUNT       60u    /* send attempts; consumer is stalled so fills fast */
#define FC_QUEUE_DROP_THRESHOLD    5u    /* drops before stage 3 begins */
#define FC_QUEUE_PERIOD_MS        10u

/* Stage 3 — memory */
#define FC_MEM_CHUNK_BYTES       512u
#define FC_MEM_WARNING_BYTES    2048u
#define FC_MEM_PERIOD_MS          10u

/* Stage 4 — watchdog */
#define FC_WDT_TIMEOUT_MS        800u   /* recovery task must kick within this window */
#define FC_WDT_KICK_PERIOD_MS    200u

/* Sub-task parameters */
#define FC_PRODUCER_STACK        128u
#define FC_RECOVERY_STACK        256u
#define FC_TASK_PRIORITY         (tskIDLE_PRIORITY + 1u)

/* -----------------------------------------------------------
 * Shared state
 * ----------------------------------------------------------*/

static volatile uint32_t  s_comm_attempts   = 0u;
static volatile uint32_t  s_drop_count      = 0u;
static volatile uint32_t  s_alloc_count     = 0u;
static volatile uint32_t  s_kick_flag       = 0u;
static volatile uint32_t  s_kick_count      = 0u;

static QueueHandle_t      s_queue           = NULL;
static TimerHandle_t      s_wdt_timer       = NULL;

/* -----------------------------------------------------------
 * Stage 4 — software watchdog callback
 *
 * Fires every FC_WDT_TIMEOUT_MS. If the recovery task has not
 * set s_kick_flag in that window, the chain terminates here.
 * ----------------------------------------------------------*/

static void WatchdogCallback(TimerHandle_t xTimer)
{
    (void)xTimer;

    if (s_kick_flag == 0u)
    {
        LOG_FAULT(EVT_WDT_TIMEOUT, s_kick_count);

        Logger_DumpUART();

        taskDISABLE_INTERRUPTS();

        for (;;)
        {
            __NOP();
        }
    }

    s_kick_flag = 0u;
}

/* -----------------------------------------------------------
 * Stage 1 — mock communication peripheral
 *
 * Succeeds for the first FC_COMM_SUCCESS_LIMIT attempts, then
 * simulates a dead bus on every subsequent call.
 * ----------------------------------------------------------*/

static BaseType_t MockComm_Send(void)
{
    s_comm_attempts++;

    if (s_comm_attempts <= FC_COMM_SUCCESS_LIMIT)
    {
        return pdTRUE;
    }

    return pdFALSE;
}

static BaseType_t Comm_SendWithRetry(void)
{
    for (uint32_t r = 0u; r < FC_COMM_MAX_RETRIES; r++)
    {
        if (MockComm_Send() == pdTRUE)
        {
            return pdTRUE;
        }

        LOG_WARN(EVT_COMM_TIMEOUT, s_comm_attempts);
    }

    return pdFALSE;
}

/* -----------------------------------------------------------
 * Stage 2 — producer sub-task
 *
 * Sends to the queue as fast as possible. The consumer (the comm
 * layer) has stalled, so the queue fills and drops accumulate.
 * Once FC_QUEUE_DROP_THRESHOLD drops are recorded, signals the
 * main chain task by incrementing s_drop_count past the threshold
 * and then suspends itself — stage 3 proceeds from the chain task.
 * ----------------------------------------------------------*/

static void ProducerTask(void *pvParameters)
{
    (void)pvParameters;

    for (uint32_t i = 0u; i < FC_QUEUE_SEND_COUNT; i++)
    {
        BaseType_t result = xQueueSend(s_queue, &i, 0u);

        if (result != pdTRUE)
        {
            s_drop_count++;

            LOG_WARN(EVT_QUEUE_OVERFLOW, s_drop_count);
        }

        vTaskDelay(pdMS_TO_TICKS(FC_QUEUE_PERIOD_MS));
    }

    vTaskSuspend(NULL);
}

/* -----------------------------------------------------------
 * Stage 3 + 4 — recovery sub-task
 *
 * Simulates a recovery routine that allocates diagnostic buffers
 * and kicks the watchdog while doing so. Once heap is exhausted
 * (pvPortMalloc returns NULL), it logs EVT_MEM_ALLOC_FAIL and
 * then hangs — no more watchdog kicks. The timer fires Stage 4.
 * ----------------------------------------------------------*/

static void RecoveryTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Kick the software watchdog */
        s_kick_flag = 1u;
        s_kick_count++;

        size_t free_heap = xPortGetFreeHeapSize();

        if (free_heap < FC_MEM_WARNING_BYTES)
        {
            LOG_WARN(EVT_LOW_HEAP, (uint32_t)free_heap);
        }

        void *p = pvPortMalloc(FC_MEM_CHUNK_BYTES);

        if (p == NULL)
        {
            /* Stage 3 terminal event — heap exhausted */
            LOG_FAULT(EVT_MEM_ALLOC_FAIL, s_alloc_count);

            /* Stop kicking — Stage 4 watchdog will fire next */
            for (;;)
            {
                __NOP();
            }
        }

        s_alloc_count++;

        vTaskDelay(pdMS_TO_TICKS(FC_MEM_PERIOD_MS));
    }
}

/* -----------------------------------------------------------
 * Fault chain entry task
 * ----------------------------------------------------------*/

void Task_FaultChain(void *pvParameters)
{
    (void)pvParameters;

    /* -------------------------------------------------------
     * Arm software watchdog before anything else.
     * Recovery task must kick it; if it hangs, Stage 4 fires.
     * ------------------------------------------------------*/

    s_wdt_timer = xTimerCreate(
        "FC_WDT",
        pdMS_TO_TICKS(FC_WDT_TIMEOUT_MS),
        pdTRUE,
        NULL,
        WatchdogCallback);

    if (s_wdt_timer == NULL)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

        taskDISABLE_INTERRUPTS();

        for (;;) { __NOP(); }
    }

    if (xTimerStart(s_wdt_timer, 0u) != pdPASS)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

        taskDISABLE_INTERRUPTS();

        for (;;) { __NOP(); }
    }

    /* -------------------------------------------------------
     * Normal operation phase
     * ------------------------------------------------------*/

    for (uint32_t i = 0u; i < FC_NORMAL_CYCLES; i++)
    {
        LOG_INFO(EVT_TASK_START, i);

        /* Kick watchdog during healthy operation */
        s_kick_flag = 1u;
        s_kick_count++;

        vTaskDelay(pdMS_TO_TICKS(FC_PERIOD_MS));

        LOG_INFO(EVT_TASK_END, i);
    }

    /* -------------------------------------------------------
     * Stage 1 — Communication degradation
     *
     * Bus goes silent after FC_COMM_SUCCESS_LIMIT sends.
     * Retries accumulate; EVT_COMM_ERROR fires when consecutive
     * failure count hits FC_COMM_FAULT_THRESHOLD.
     * Execution falls through into Stage 2 — the comm fault
     * is what causes the producer to keep pushing without a
     * working consumer to drain the queue.
     * ------------------------------------------------------*/

    {
        uint32_t consecutive = 0u;

        for (;;)
        {
            /* Kick watchdog — still healthy at this point */
            s_kick_flag = 1u;
            s_kick_count++;

            if (Comm_SendWithRetry() == pdTRUE)
            {
                consecutive = 0u;
            }
            else
            {
                consecutive++;

                if (consecutive >= FC_COMM_FAULT_THRESHOLD)
                {
                    LOG_FAULT(EVT_COMM_ERROR, s_comm_attempts);

                    /* Do NOT halt — cascade continues into Stage 2 */
                    break;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(FC_COMM_PERIOD_MS));
        }
    }

    /* -------------------------------------------------------
     * Stage 2 — Queue backup
     *
     * Create queue and producer. Consumer is absent (comm is
     * dead), so the queue fills. ProducerTask logs drops and
     * suspends after FC_QUEUE_SEND_COUNT attempts.
     * Main task waits until enough drops accumulate, then
     * moves into Stage 3 without halting.
     * ------------------------------------------------------*/

    s_queue = xQueueCreate(FC_QUEUE_DEPTH, sizeof(uint32_t));

    if (s_queue == NULL)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 0u);

        taskDISABLE_INTERRUPTS();

        for (;;) { __NOP(); }
    }

    if (xTaskCreate(ProducerTask,
                    "FC_Prod",
                    FC_PRODUCER_STACK,
                    NULL,
                    FC_TASK_PRIORITY,
                    NULL) != pdPASS)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 1u);

        taskDISABLE_INTERRUPTS();

        for (;;) { __NOP(); }
    }

    /* Wait for queue drop threshold to be crossed */
    while (s_drop_count < FC_QUEUE_DROP_THRESHOLD)
    {
        /* Kick watchdog while waiting */
        s_kick_flag = 1u;
        s_kick_count++;

        vTaskDelay(pdMS_TO_TICKS(FC_QUEUE_PERIOD_MS));
    }

    /* -------------------------------------------------------
     * Stage 3 + 4 — Memory pressure and watchdog timeout
     *
     * Spawn the recovery task. It allocates diagnostic buffers
     * and kicks the watchdog each iteration. Once heap is
     * exhausted it logs EVT_MEM_ALLOC_FAIL and hangs, stopping
     * all watchdog kicks. The software watchdog timer then fires
     * EVT_WDT_TIMEOUT and halts — Stage 4 terminal event.
     * ------------------------------------------------------*/

    if (xTaskCreate(RecoveryTask,
                    "FC_Recv",
                    FC_RECOVERY_STACK,
                    NULL,
                    FC_TASK_PRIORITY,
                    NULL) != pdPASS)
    {
        LOG_FAULT(EVT_TASK_CREATE_FAIL, 2u);

        taskDISABLE_INTERRUPTS();

        for (;;) { __NOP(); }
    }

    /* Entry task has nothing left to do — RecoveryTask and
     * WatchdogCallback own the rest of the run. */
    vTaskDelete(NULL);
}
