################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../nivel.c \
../tad_items.c 

OBJS += \
./nivel.o \
./tad_items.o 

C_DEPS += \
./nivel.d \
./tad_items.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/so-commons-library/src/commons" -I"/home/utnso/TPSO/tp-20131c-overflow/socketsCom" -I"/home/utnso/tp-20131c-overflow/nivel-gui" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


