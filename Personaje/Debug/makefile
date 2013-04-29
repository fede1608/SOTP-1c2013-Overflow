################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: personaje

dependents:
	-cd /home/utnso/so-commons-library/src/commons/Debug && $(MAKE) clean all
	-cd /home/utnso/so-nivel-gui-library/nivel-gui/Debug && $(MAKE) clean all

# Tool invocations
personaje: $(OBJS) $(USER_OBJS) /home/utnso/so-commons-library/src/commons/Debug/libcommons-library.so /home/utnso/so-nivel-gui-library/nivel-gui/Debug/libnivel-gui.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "personaje" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) personaje
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/so-commons-library/src/commons/Debug/libcommons-library.so:
/home/utnso/so-nivel-gui-library/nivel-gui/Debug/libnivel-gui.so:

-include ../makefile.targets
