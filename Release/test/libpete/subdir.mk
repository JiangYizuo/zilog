################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../test/libpete/libpete.c 

OBJS += \
./test/libpete/libpete.o 

C_DEPS += \
./test/libpete/libpete.d 


# Each subdirectory must supply rules for building sources it contributes
test/libpete/%.o: ../test/libpete/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


