################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CCS_Series_Drv.c \
../src/spxdrv.c \
../src/spxusb.c \
../src/thorspec.c 

OBJS += \
./src/CCS_Series_Drv.o \
./src/spxdrv.o \
./src/spxusb.o \
./src/thorspec.o 

C_DEPS += \
./src/CCS_Series_Drv.d \
./src/spxdrv.d \
./src/spxusb.d \
./src/thorspec.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


