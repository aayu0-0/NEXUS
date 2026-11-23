#include "logger.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "usart.h"
#include "semphr.h"
#include "task.h"




#define LOG_BUF_MASK       (LOG_BUFFER_SIZE - 1u)   // Fast ring buffer wrap mask

typedef char _log_pow2_check[
    ((LOG_BUFFER_SIZE & LOG_BUF_MASK) == 0u) ? 1 : -1
];                   // Verify buffer size is power-of-2

#define UART_ROW_BUF_SIZE  48u                      // CSV formatting scratch buffer
#define NEXUS_FW_VERSION   "1.0"                    // Dump header firmware version

static LogBuffer_t s_log;                           // Global logger state



																// Weak function in case you don't use UART2 and choose smth else
/*__attribute__((weak))
*void Logger_UART_Transmit(const uint8_t *data, uint16_t len)
*{
*    (void)data;
*    (void)len;
*
*}
*/


//Example UART2 implementation   Uncomment after USART2 is configured in CubeMX.


void Logger_UART_Transmit(const uint8_t *data, uint16_t len)
{
     HAL_UART_Transmit(&huart2,
                       (uint8_t *)data,
                       len,
                       HAL_MAX_DELAY);
}



															// Send null-terminated string through logger transport
static inline void uart_send_str(const char *s)
{
    uint16_t len = 0;

    while (s[len]) {
        len++;
    }

    Logger_UART_Transmit((const uint8_t *)s, len);
}



																				// This part is the one to fill the ring buffer properly
static void write_entry_locked(LogLevel_t       level,
                               LogEventID_t     event_id,
                               uint32_t         payload)
{
    LogEntry_t *slot = &s_log.buffer[s_log.head & LOG_BUF_MASK];

    slot->timestamp = HAL_GetTick();
    slot->level     = (uint8_t)level;
    slot->event_id  = (uint8_t)event_id;
    slot->payload   = payload;

    s_log.head = (s_log.head + 1u) & LOG_BUF_MASK;

    if (s_log.count < LOG_BUFFER_SIZE) {
        s_log.count++;
    } else {
        /* Buffer full — advance tail, oldest entry is overwritten */
        s_log.tail = (s_log.tail + 1u) & LOG_BUF_MASK;
        s_log.overflow_count++;
    }
}


																					// Logger initialization
BaseType_t Logger_Init(void)
{
    memset(&s_log, 0, sizeof(s_log));

    s_log.mutex = xSemaphoreCreateMutex();
    if (s_log.mutex == NULL) {
        return pdFALSE;   															// Exhausted Heap, cannot initialize
    }

    s_log.min_level = LVL_DEBUG;   													// Lowest level, log everything by default


    Logger_Log(LVL_INFO, EVT_BOOT, 0u);												// Record successful boot

    return pdTRUE;
}


																					// Logging of Data
void Logger_Log(LogLevel_t level, LogEventID_t event_id, uint32_t payload)
{
    BaseType_t do_dump = pdFALSE;


    if ((level < s_log.min_level) && (level != LVL_FAULT)) {
        return;
    }

    if (xSemaphoreTake(s_log.mutex,
                       pdMS_TO_TICKS(LOG_MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        write_entry_locked(level, event_id, payload);

        if (level == LVL_FAULT) {
            do_dump = pdTRUE;   													// always dump fault immediately
        }

        xSemaphoreGive(s_log.mutex);
    }
    else
    {
        s_log.dropped_count++;														// lost entry — mutex contention, not buffer wraparound
    }

    if (do_dump == pdTRUE) {
        Logger_DumpUART();
    }
}


																					// Logging in case of ISR
void Logger_Log_FromISR(LogLevel_t       level,
                        LogEventID_t     event_id,
                        uint32_t         payload,
                        BaseType_t      *pxHigherPriorityTaskWoken)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((level < s_log.min_level) && (level != LVL_FAULT)) {
        return;
    }

    if (xSemaphoreTakeFromISR(s_log.mutex, &xHigherPriorityTaskWoken) == pdTRUE)
    {
        write_entry_locked(level, event_id, payload);
        xSemaphoreGiveFromISR(s_log.mutex, &xHigherPriorityTaskWoken);
    }
    else
    {
        s_log.dropped_count++;														// lost entry — mutex held by task context
    }

    if (pxHigherPriorityTaskWoken != NULL) {
        *pxHigherPriorityTaskWoken = xHigherPriorityTaskWoken;
    }
}


																					// A function to dump everything in buffer when you require it
void Logger_StreamUART(LogLevel_t level,
                       LogEventID_t event_id,
                       uint32_t payload)
{
    char row[UART_ROW_BUF_SIZE];

    snprintf(row,
             sizeof(row),
             "%lu,%u,%u,%lu\r\n",
             (unsigned long)HAL_GetTick(),
             (unsigned int)level,
             (unsigned int)event_id,
             (unsigned long)payload);

    Logger_UART_Transmit((const uint8_t *)row,
                         (uint16_t)strlen(row));
}


																					// A function that stores the logs in buffer and show properly to debug afterwards
void Logger_DumpUART(void)
{
    char row[UART_ROW_BUF_SIZE];

    uint32_t i;
    uint32_t idx;

    uint32_t snapshot_count;
    uint32_t snapshot_tail;

    if (xSemaphoreTake(s_log.mutex,
                       pdMS_TO_TICKS(LOG_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        return;
    }

    snapshot_count = s_log.count;
    snapshot_tail  = s_log.tail;

    xSemaphoreGive(s_log.mutex);

    for (i = 0; i < snapshot_count; i++)
    {
        idx = (snapshot_tail + i) & LOG_BUF_MASK;

        const LogEntry_t *e = &s_log.buffer[idx];

        snprintf(row,
                 sizeof(row),
                 "%lu,%u,%u,%lu\r\n",
                 (unsigned long)e->timestamp,
                 (unsigned int)e->level,
                 (unsigned int)e->event_id,
                 (unsigned long)e->payload);

        Logger_UART_Transmit((const uint8_t *)row,
                             (uint16_t)strlen(row));
    }
}


																			// Set the minimum level
void Logger_SetMinLevel(LogLevel_t level)
{
    s_log.min_level = level;
}

																			// Tell how many valid logs are currently stored
uint32_t Logger_GetCount(void)
{
    return s_log.count;
}

																			// Tell how many logs have been overwritten
uint32_t Logger_GetOverflowCount(void)
{
    return s_log.overflow_count;
}

																			// Tell how many logs were lost to mutex contention (never made it into the buffer)
uint32_t Logger_GetDroppedCount(void)
{
    return s_log.dropped_count;
}

																			// Restart the log buffer
void Logger_Clear(void)
{
    if (xSemaphoreTake(s_log.mutex,
                       pdMS_TO_TICKS(LOG_MUTEX_TIMEOUT_MS)) == pdTRUE)
    {
        s_log.head           = 0u;
        s_log.tail           = 0u;
        s_log.count          = 0u;
        s_log.overflow_count = 0u;
        s_log.dropped_count  = 0u;
        xSemaphoreGive(s_log.mutex);
    }
}
