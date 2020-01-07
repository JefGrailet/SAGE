################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/graph/PeerDiscoveryTask.cpp \
../src/algo/graph/PeerScanner.cpp \
../src/algo/graph/TopologyInferrer.cpp

OBJS += \
./src/algo/graph/PeerDiscoveryTask.o \
./src/algo/graph/PeerScanner.o \
./src/algo/graph/TopologyInferrer.o

CPP_DEPS += \
./src/algo/graph/PeerDiscoveryTask.d \
./src/algo/graph/PeerScanner.d \
./src/algo/graph/TopologyInferrer.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/graph/%.o: ../src/algo/graph/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


