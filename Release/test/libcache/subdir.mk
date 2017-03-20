################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../test/libcache/libhashmap.c 

OBJS += \
./test/libcache/libhashmap.o 

C_DEPS += \
./test/libcache/libhashmap.d 


# Each subdirectory must supply rules for building sources it contributes
test/libcache/%.o: ../test/libcache/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


