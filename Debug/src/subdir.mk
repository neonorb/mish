################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/bytecode.cpp \
../src/code.cpp \
../src/expression.cpp \
../src/function.cpp \
../src/functioncallreturn.cpp \
../src/functioncallvoid.cpp \
../src/mish.cpp \
../src/scope.cpp \
../src/syscall.cpp \
../src/value.cpp 

OBJS += \
./src/bytecode.o \
./src/code.o \
./src/expression.o \
./src/function.o \
./src/functioncallreturn.o \
./src/functioncallvoid.o \
./src/mish.o \
./src/scope.o \
./src/syscall.o \
./src/value.o 

CPP_DEPS += \
./src/bytecode.d \
./src/code.d \
./src/expression.d \
./src/function.d \
./src/functioncallreturn.d \
./src/functioncallvoid.d \
./src/mish.d \
./src/scope.d \
./src/syscall.d \
./src/value.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	i686-elf-g++ -I"/home/chris/git/mish/include" -I"/home/chris/git/feta/include" -O0 -g3 -Wall -c -fmessage-length=0 -nostdlib -ffreestanding -fno-exceptions -fno-rtti -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


