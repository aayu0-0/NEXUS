/**
 * @file  fault_chain.h
 * @brief NEXUS — Cascading fault chain injection
 *
 * Demonstrates a realistic multi-fault escalation sequence:
 *
 *   Stage 1 — Communication degradation
 *             Mock bus stops responding; retries accumulate;
 *             EVT_COMM_TIMEOUT warnings logged on each failed retry.
 *             EVT_COMM_ERROR logged when consecutive failure threshold hit.
 *
 *   Stage 2 — Queue backup
 *             A producer task keeps sending while the comm fault makes
 *             the consumer stall; queue fills and drops begin.
 *             EVT_QUEUE_OVERFLOW logged on each dropped message.
 *
 *   Stage 3 — Memory pressure from failed recovery
 *             Recovery code allocates diagnostic buffers; they are never
 *             freed because recovery never completes.
 *             EVT_LOW_HEAP logged when free heap crosses the warning
 *             threshold; EVT_MEM_ALLOC_FAIL logged when pvPortMalloc()
 *             returns NULL.
 *
 *   Stage 4 — Watchdog kills the hung recovery task
 *             A software watchdog timer is armed before Stage 1 begins.
 *             The recovery task is supposed to kick it; once the task
 *             hangs waiting for memory, kicking stops.
 *             EVT_WDT_TIMEOUT logged by the timer callback; system halts.
 *
 * No headers from other fault_* modules are included — every behaviour
 * is re-implemented inline at minimal scale to avoid link-order
 * dependencies and keep the trace self-contained.
 */

#ifndef FAULT_CHAIN_H
#define FAULT_CHAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Entry task for the fault chain scenario.
 *
 * Spawn this as the sole NEXUS task for a fault-chain build.
 * It arms the software watchdog, runs normal-operation cycles,
 * then triggers the four-stage cascade described above.
 *
 * Recommended stack: 512 words (the task spawns two sub-tasks
 * internally and uses modest local storage).
 */
void Task_FaultChain(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_CHAIN_H */
