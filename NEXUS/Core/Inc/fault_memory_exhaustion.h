/**
 * @file  fault_memory_exhaustion.h
 * @brief NEXUS — Memory fault injection: low-heap warning + full exhaustion
 *
 * Covers two related dataset classes from one module:
 *   - Low heap warning   : sustained operation below the warning threshold
 *                          without ever fully exhausting the heap
 *   - Heap exhaustion     : heap drained until pvPortMalloc() fails
 */

#ifndef FAULT_MEMORY_EXHAUSTION_H
#define FAULT_MEMORY_EXHAUSTION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Sustains the heap in a low-but-not-exhausted state for a fixed
 * duration, logging EVT_LOW_HEAP periodically. Does not fail allocation. */
void TriggerFault_LowHeap(void);

/* Drains the heap until pvPortMalloc() returns NULL. */
void TriggerFault_MemExhaustion(void);

void Task_FaultMemExhaustion(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_MEMORY_EXHAUSTION_H */
