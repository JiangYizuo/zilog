################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/zilog/zilog.c \
../src/zilog/zilog_agent.c \
../src/zilog/zilog_time.c 

OBJS += \
./src/zilog/zilog.o \
./src/zilog/zilog_agent.o \
./src/zilog/zilog_time.o 

C_DEPS += \
./src/zilog/zilog.d \
./src/zilog/zilog_agent.d \
./src/zilog/zilog_time.d 


# Each subdirectory must supply rules for building sources it contributes
src/zilog/%.o: ../src/zilog/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


