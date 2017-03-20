################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/liblogging/liblogging.c \
../src/liblogging/liblogging_agent.c 

OBJS += \
./src/liblogging/liblogging.o \
./src/liblogging/liblogging_agent.o 

C_DEPS += \
./src/liblogging/liblogging.d \
./src/liblogging/liblogging_agent.d 


# Each subdirectory must supply rules for building sources it contributes
src/liblogging/%.o: ../src/liblogging/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


