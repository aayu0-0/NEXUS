/**
 * @file  fault_hard_fault.h
 * @brief NEXUS — Cortex-M HardFault fault injection
 *
 * Triggers a genuine Cortex-M HardFault exception (NULL pointer
 * dereference) during normal operation. The HardFault_Handler
 * captures CPU fault status registers and logs EVT_HARD_FAULT
 * with diagnostic payload before halting.
 */

#ifndef FAULT_HARD_FAULT_H
#define FAULT_HARD_FAULT_H

#ifdef __cplusplus
extern "C" {
#endif

void TriggerFault_HardFault(void);
void Task_FaultHardFault(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_HARD_FAULT_H */
