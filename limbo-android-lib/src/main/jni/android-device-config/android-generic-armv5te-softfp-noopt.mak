#NDK VERSION
#NDK_TOOLCHAIN_VERSION := 4.4.3
#NDK_TOOLCHAIN_VERSION=4.6
#NDK_TOOLCHAIN_VERSION=4.7
#NDK_TOOLCHAIN_VERSION=4.8
NDK_TOOLCHAIN_VERSION=4.9

#TARGET ARCH
APP_ABI := armeabi

LIMBO_LD_FLAGS +=  -Wl,--no-warn-mismatch

### CONFIGURATION
ARCH_CFLAGS = \
-std=gnu99 \
-D__ARM_ARCH_5__ \
-D__ARM_ARCH_5T__ \
-D__ARM_ARCH_5E__ \
-D__ARM_ARCH_5TE__  \
-march=armv5te -mtune=xscale -msoft-float

# No specific tuning
#-mtune=arm7

# Possible values: arm, thumb
ARM_MODE=arm

# Suppress some warnings
ARCH_CFLAGS += -Wno-psabi

# Suppress some warnings
ARCH_CFLAGS += -Wno-format-security

# Optimization
ANDROID_OPTIM_FLAGS = -O0
 
ifeq ($(NDK_DEBUG),1)
	ARCH_CFLAGS += -O0
	ARCH_CFLAGS += -g 
else
	ARCH_CFLAGS += $(ANDROID_OPTIM_FLAGS)
endif 

#DEBUG
ARCH_CFLAGS += -UNDEBUG

# Smaller code generation for shared libraries, usually faster
# if doesn't work use -fPIC
ARCH_CFLAGS += -fpic 
#ARCH_CFLAGS += -fPIC

#Will be also executable
#ARCH_CFLAGS += -fPIE

#Keeping - No stack pointers might be faster
ARCH_CFLAGS += -fno-omit-frame-pointer 

# prevent unwanted optimizations for Qemu
ARCH_CFLAGS += -fno-strict-aliasing

ARCH_CFLAGS += -ffunction-sections 	

ARCH_CFLAGS += -no-canonical-prefixes

# Should not be limiting inline functions or this value should be very large
ARCH_CFLAGS += -finline-limit=99999

# for Debugging only
ARCH_CFLAGS += -funwind-tables 

#Unswitch loops
ARCH_CFLAGS += -funswitch-loops

# SLows down but needed for some devices
# where stack corruption is more probable
ARCH_CFLAGS += -fstack-protector
#ARCH_CFLAGS += -fno-stack-protector


# ORIGINAL CFLAGS FROM ANDROID NDK
#-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  \
#-march=armv5te -mtune=xscale -msoft-float -mthumb \
#-Os
#-fpic -ffunction-sections -funwind-tables -fstack-protector \
#-Wno-psabi 
#-fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 
