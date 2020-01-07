################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/common/exception/NTmapException.cpp

OBJS += \
./src/common/exception/NTmapException.o

CPP_DEPS += \
./src/common/exception/NTmapException.d


# Each subdirectory must supply rules for building sources it contributes
src/common/exception/%.o: ../src/common/exception/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


