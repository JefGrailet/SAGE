################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/structure/RouteHop.cpp \
../src/algo/structure/Trail.cpp \
../src/algo/structure/AliasHints.cpp \
../src/algo/structure/AliasedInterface.cpp \
../src/algo/structure/Alias.cpp \
../src/algo/structure/AliasSet.cpp \
../src/algo/structure/SubnetInterface.cpp \
../src/algo/structure/Subnet.cpp \
../src/algo/structure/IPTableEntry.cpp \
../src/algo/structure/IPLookUpTable.cpp

OBJS += \
./src/algo/structure/RouteHop.o \
./src/algo/structure/Trail.o \
./src/algo/structure/AliasHints.o \
./src/algo/structure/AliasedInterface.o \
./src/algo/structure/Alias.o \
./src/algo/structure/AliasSet.o \
./src/algo/structure/SubnetInterface.o \
./src/algo/structure/Subnet.o \
./src/algo/structure/IPTableEntry.o \
./src/algo/structure/IPLookUpTable.o

CPP_DEPS += \
./src/algo/structure/RouteHop.d \
./src/algo/structure/Trail.d \
./src/algo/structure/AliasHints.d \
./src/algo/structure/AliasedInterface.d \
./src/algo/structure/Alias.d \
./src/algo/structure/AliasSet.d \
./src/algo/structure/SubnetInterface.d \
./src/algo/structure/Subnet.d \
./src/algo/structure/IPTableEntry.d \
./src/algo/structure/IPLookUpTable.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/structure/%.o: ../src/algo/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


