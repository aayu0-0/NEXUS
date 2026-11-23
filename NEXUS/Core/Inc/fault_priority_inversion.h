/**
 * @file  fault_priority_inversion.h
 * @brief NEXUS — Priority inversion fault injection
 *
 * Classic three-task scenario: a low-priority task holds a mutex
 * needed by a high-priority task, while a medium-priority task
 * (which needs no mutex) preempts the low-priority holder —
 * indefinitely starving the high-priority task.
 *
 * Demonstrated WITHOUT priority inheritance (using a binary
 * semaphore instead of a FreeRTOS mutex) to actually produce the
 * fault, since FreeRTOS mutexes have priority inheritance built in
 * by default and would silently fix this.
 */

#ifndef FAULT_PRIORITY_INVERSION_H
#define FAULT_PRIORITY_INVERSION_H

#ifdef __cplusplus
extern "C" {
#endif

void Task_FaultPriorityInversion(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_PRIORITY_INVERSION_H */
