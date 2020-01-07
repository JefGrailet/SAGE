################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/subnetinference/SubnetInferenceRules.cpp \
../src/algo/subnetinference/SubnetInferrer.cpp \
../src/algo/subnetinference/MergingCandidate.cpp \
../src/algo/subnetinference/SubnetPostProcessor.cpp

OBJS += \
./src/algo/subnetinference/SubnetInferenceRules.o \
./src/algo/subnetinference/SubnetInferrer.o \
./src/algo/subnetinference/MergingCandidate.o \
./src/algo/subnetinference/SubnetPostProcessor.o

CPP_DEPS += \
./src/algo/subnetinference/SubnetInferenceRules.d \
./src/algo/subnetinference/SubnetInferrer.d \
./src/algo/subnetinference/MergingCandidate.d \
./src/algo/subnetinference/SubnetPostProcessor.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/subnetinference/%.o: ../src/algo/subnetinference/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


