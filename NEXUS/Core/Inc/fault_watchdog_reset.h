/**
 * @file  fault_watchdog_reset.h
 * @brief NEXUS — Hardware watchdog (IWDG) reset detection + fault injection
 *
 * Two halves to this file:
 *
 *   1. Boot-time detection (always relevant, not just during fault testing):
 *      WDT_CheckResetCause() reads RCC_CSR to find out if the MCU's last
 *      reset was caused by the Independent Watchdog. Call this once at
 *      startup, immediately after Logger_Init(), before anything else
 *      touches RCC_CSR.
 *
 *   2. Fault injection task:
 *      Task_FaultWatchdogReset() enables the real IWDG peripheral, kicks
 *      it for a while during normal operation, then deliberately stops.
 *      The hardware resets the chip. On the *next* boot,
 *      WDT_CheckResetCause() is what reports it.
 */

#ifndef FAULT_WATCHDOG_RESET_H
#define FAULT_WATCHDOG_RESET_H

#ifdef __cplusplus
extern "C" {
#endif

/* Call once at startup, right after Logger_Init().
 * Reads and clears RCC_CSR reset flags, logs EVT_WDT_HW_RESET if the
 * previous reset was caused by the IWDG. Safe to call even when the
 * reset was a normal power-on / pin reset — it simply won't log anything
 * in that case. */
void WDT_CheckResetCause(void);

void Task_FaultWatchdogReset(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_WATCHDOG_RESET_H */
