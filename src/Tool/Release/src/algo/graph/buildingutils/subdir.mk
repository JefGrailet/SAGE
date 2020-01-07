################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/graph/buildingutils/Aggregate.cpp \
../src/algo/graph/buildingutils/IPClusterer.cpp \
../src/algo/graph/buildingutils/Peer.cpp \
../src/algo/graph/buildingutils/GraphPeers.cpp

OBJS += \
./src/algo/graph/buildingutils/Aggregate.o \
./src/algo/graph/buildingutils/IPClusterer.o \
./src/algo/graph/buildingutils/Peer.o \
./src/algo/graph/buildingutils/GraphPeers.o

CPP_DEPS += \
./src/algo/graph/buildingutils/Aggregate.d \
./src/algo/graph/buildingutils/IPClusterer.d \
./src/algo/graph/buildingutils/Peer.d \
./src/algo/graph/buildingutils/GraphPeers.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/graph/buildingutils/%.o: ../src/algo/graph/buildingutils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


