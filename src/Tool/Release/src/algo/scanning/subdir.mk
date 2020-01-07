################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/scanning/LocationTask.cpp \
../src/algo/scanning/TrailCorrectionTask.cpp \
../src/algo/scanning/TargetScanner.cpp

OBJS += \
./src/algo/scanning/LocationTask.o \
./src/algo/scanning/TrailCorrectionTask.o \
./src/algo/scanning/TargetScanner.o

CPP_DEPS += \
./src/algo/scanning/LocationTask.d \
./src/algo/scanning/TrailCorrectionTask.d \
./src/algo/scanning/TargetScanner.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/scanning/%.o: ../src/algo/scanning/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


