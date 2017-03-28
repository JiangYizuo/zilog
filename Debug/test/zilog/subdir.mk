################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../test/zilog/zilog_test.c 

OBJS += \
./test/zilog/zilog_test.o 

C_DEPS += \
./test/zilog/zilog_test.d 


# Each subdirectory must supply rules for building sources it contributes
test/zilog/%.o: ../test/zilog/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g -Wall -c -fmessage-length=0 -fprofile-arcs -ftest-coverage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


