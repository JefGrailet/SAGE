################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/aliasresolution/IPIDTuple.cpp \
../src/algo/aliasresolution/IPIDUnit.cpp \
../src/algo/aliasresolution/UDPUnreachablePortUnit.cpp \
../src/algo/aliasresolution/TimestampCheckUnit.cpp \
../src/algo/aliasresolution/ReverseDNSUnit.cpp \
../src/algo/aliasresolution/AliasHintsCollector.cpp \
../src/algo/aliasresolution/AliasResolver.cpp

OBJS += \
./src/algo/aliasresolution/IPIDTuple.o \
./src/algo/aliasresolution/IPIDUnit.o \
./src/algo/aliasresolution/UDPUnreachablePortUnit.o \
./src/algo/aliasresolution/TimestampCheckUnit.o \
./src/algo/aliasresolution/ReverseDNSUnit.o \
./src/algo/aliasresolution/AliasHintsCollector.o \
./src/algo/aliasresolution/AliasResolver.o

CPP_DEPS += \
./src/algo/aliasresolution/IPIDTuple.d \
./src/algo/aliasresolution/IPIDUnit.d \
./src/algo/aliasresolution/UDPUnreachablePortUnit.d \
./src/algo/aliasresolution/TimestampCheckUnit.d \
./src/algo/aliasresolution/ReverseDNSUnit.d \
./src/algo/aliasresolution/AliasHintsCollector.d \
./src/algo/aliasresolution/AliasResolver.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/aliasresolution/%.o: ../src/algo/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


