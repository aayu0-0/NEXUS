#ifndef FAULT_STACK_OVERFLOW_H
#define FAULT_STACK_OVERFLOW_H

#ifdef __cplusplus
extern "C" {
#endif

void TriggerFault_StackOverflow(void);
void Task_FaultStackOverflow(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
