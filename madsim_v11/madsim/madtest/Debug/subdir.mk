################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../madtest.cpp 

OBJS += \
./madtest.o 

CPP_DEPS += \
./madtest.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DBIO -I/home/ubuntu/Documents/Link+Workspace/include -include/home/ubuntu/MadWkSpc/madtest/madtest.h -include/home/ubuntu/Documents/Link+Workspace/include/madapplib.h -include/home/ubuntu/Documents/Link+Workspace/include/madioctls.h -include/home/ubuntu/Documents/Link+Workspace/include/madkonsts.h -include/home/ubuntu/Documents/Link+Workspace/include/maddefs.h -O0 -g3 -Wall -c -fmessage-length=0 -v -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


