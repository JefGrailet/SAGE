################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/prescanning/TargetPrescanningUnit.cpp \
../src/algo/prescanning/TargetPrescanner.cpp

OBJS += \
./src/algo/prescanning/TargetPrescanningUnit.o \
./src/algo/prescanning/TargetPrescanner.o

CPP_DEPS += \
./src/algo/prescanning/TargetPrescanningUnit.d \
./src/algo/prescanning/TargetPrescanner.d


# Each subdirectory must supply rules for building sources it contributes
src/algo/prescanning/%.o: ../src/algo/prescanning/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


