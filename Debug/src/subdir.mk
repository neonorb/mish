################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/mish.cpp 

OBJS += \
./src/mish.o 

CPP_DEPS += \
./src/mish.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	i686-elf-g++ -I"/home/chris/git/mish/include" -I"/home/chris/git/feta/include" -O0 -g3 -Wall -c -fmessage-length=0 -nostdlib -ffreestanding -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


