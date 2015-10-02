################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
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
all: MSE_TempController.axf

# Tool invocations
MSE_TempController.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="MSE_TempController.map" -Xlinker --gc-sections -mcpu=cortex-m0 -mthumb -T "LPC810_CodeBase_Release.ld" -o "MSE_TempController.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) MSE_TempController.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "MSE_TempController.axf"; arm-none-eabi-objcopy -O binary "MSE_TempController.axf" "MSE_TempController.bin" ; arm-none-eabi-objcopy -O ihex "MSE_TempController.axf" "MSE_TempController.hex" ; checksum -p LPC810 -d "MSE_TempController.bin";
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
