################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/bytecode.cpp \
../src/code.cpp \
../src/function.cpp \
../src/mish.cpp \
../src/scope.cpp 

OBJS += \
./src/bytecode.o \
./src/code.o \
./src/function.o \
./src/mish.o \
./src/scope.o 

CPP_DEPS += \
./src/bytecode.d \
./src/code.d \
./src/function.d \
./src/mish.d \
./src/scope.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	i686-elf-g++ -I"/home/chris/git/mish/include" -I"/home/chris/git/feta/include" -O0 -g3 -Wall -c -fmessage-length=0 -nostdlib -ffreestanding -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


