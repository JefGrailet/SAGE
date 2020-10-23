################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/graph/components/Vertex.cpp \
../src/algo/graph/components/Node.cpp \
../src/algo/graph/components/Cluster.cpp \
../src/algo/graph/components/SubnetVertexMapping.cpp \
../src/algo/graph/components/Edge.cpp \
../src/algo/graph/components/DirectLink.cpp \
../src/algo/graph/components/IndirectLink.cpp \
../src/algo/graph/components/RemoteLink.cpp \
../src/algo/graph/components/Graph.cpp

OBJS += \
./src/algo/graph/components/Vertex.o \
./src/algo/graph/components/Node.o \
./src/algo/graph/components/Cluster.o \
./src/algo/graph/components/SubnetVertexMapping.o \
./src/algo/graph/components/Edge.o \
./src/algo/graph/components/DirectLink.o \
./src/algo/graph/components/IndirectLink.o \
./src/algo/graph/components/RemoteLink.o \
./src/algo/graph/components/Graph.o

CPP_DEPS += \
./src/algo/graph/components/Vertex.d \
./src/algo/graph/components/Node.d \
./src/algo/graph/components/Cluster.d \
./src/algo/graph/components/SubnetVertexMapping.d \
./src/algo/graph/components/Edge.d \
./src/algo/graph/components/DirectLink.d \
./src/algo/graph/components/IndirectLink.d \
./src/algo/graph/components/RemoteLink.d \
./src/algo/graph/components/Graph.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/graph/components/%.o: ../src/algo/graph/components/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


