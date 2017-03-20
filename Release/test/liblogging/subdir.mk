################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../test/liblogging/libloggingTest.c 

OBJS += \
./test/liblogging/libloggingTest.o 

C_DEPS += \
./test/liblogging/libloggingTest.d 


# Each subdirectory must supply rules for building sources it contributes
test/liblogging/%.o: ../test/liblogging/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


