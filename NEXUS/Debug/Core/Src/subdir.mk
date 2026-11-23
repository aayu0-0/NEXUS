################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Logger.c \
../Core/Src/fault_chain.c \
../Core/Src/fault_comm_timeout.c \
../Core/Src/fault_hard_fault.c \
../Core/Src/fault_memory_exhaustion.c \
../Core/Src/fault_priority_inversion.c \
../Core/Src/fault_queue_overflow.c \
../Core/Src/fault_stack_overflow.c \
../Core/Src/fault_task_create_fail.c \
../Core/Src/fault_watchdog_reset.c \
../Core/Src/fault_watchdog_timeout.c \
../Core/Src/freertos.c \
../Core/Src/gpio.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/Logger.o \
./Core/Src/fault_chain.o \
./Core/Src/fault_comm_timeout.o \
./Core/Src/fault_hard_fault.o \
./Core/Src/fault_memory_exhaustion.o \
./Core/Src/fault_priority_inversion.o \
./Core/Src/fault_queue_overflow.o \
./Core/Src/fault_stack_overflow.o \
./Core/Src/fault_task_create_fail.o \
./Core/Src/fault_watchdog_reset.o \
./Core/Src/fault_watchdog_timeout.o \
./Core/Src/freertos.o \
./Core/Src/gpio.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/Logger.d \
./Core/Src/fault_chain.d \
./Core/Src/fault_comm_timeout.d \
./Core/Src/fault_hard_fault.d \
./Core/Src/fault_memory_exhaustion.d \
./Core/Src/fault_priority_inversion.d \
./Core/Src/fault_queue_overflow.d \
./Core/Src/fault_stack_overflow.d \
./Core/Src/fault_task_create_fail.d \
./Core/Src/fault_watchdog_reset.d \
./Core/Src/fault_watchdog_timeout.d \
./Core/Src/freertos.d \
./Core/Src/gpio.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xC -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Logger.cyclo ./Core/Src/Logger.d ./Core/Src/Logger.o ./Core/Src/Logger.su ./Core/Src/fault_chain.cyclo ./Core/Src/fault_chain.d ./Core/Src/fault_chain.o ./Core/Src/fault_chain.su ./Core/Src/fault_comm_timeout.cyclo ./Core/Src/fault_comm_timeout.d ./Core/Src/fault_comm_timeout.o ./Core/Src/fault_comm_timeout.su ./Core/Src/fault_hard_fault.cyclo ./Core/Src/fault_hard_fault.d ./Core/Src/fault_hard_fault.o ./Core/Src/fault_hard_fault.su ./Core/Src/fault_memory_exhaustion.cyclo ./Core/Src/fault_memory_exhaustion.d ./Core/Src/fault_memory_exhaustion.o ./Core/Src/fault_memory_exhaustion.su ./Core/Src/fault_priority_inversion.cyclo ./Core/Src/fault_priority_inversion.d ./Core/Src/fault_priority_inversion.o ./Core/Src/fault_priority_inversion.su ./Core/Src/fault_queue_overflow.cyclo ./Core/Src/fault_queue_overflow.d ./Core/Src/fault_queue_overflow.o ./Core/Src/fault_queue_overflow.su ./Core/Src/fault_stack_overflow.cyclo ./Core/Src/fault_stack_overflow.d ./Core/Src/fault_stack_overflow.o ./Core/Src/fault_stack_overflow.su ./Core/Src/fault_task_create_fail.cyclo ./Core/Src/fault_task_create_fail.d ./Core/Src/fault_task_create_fail.o ./Core/Src/fault_task_create_fail.su ./Core/Src/fault_watchdog_reset.cyclo ./Core/Src/fault_watchdog_reset.d ./Core/Src/fault_watchdog_reset.o ./Core/Src/fault_watchdog_reset.su ./Core/Src/fault_watchdog_timeout.cyclo ./Core/Src/fault_watchdog_timeout.d ./Core/Src/fault_watchdog_timeout.o ./Core/Src/fault_watchdog_timeout.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

