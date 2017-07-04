#NDK VERSION
#NDK_TOOLCHAIN_VERSION := 4.4.3
#NDK_TOOLCHAIN_VERSION=4.6
#NDK_TOOLCHAIN_VERSION=4.7
#NDK_TOOLCHAIN_VERSION=4.8
NDK_TOOLCHAIN_VERSION=4.9

#TARGET ARCH
APP_ABI := arm64-v8a

#FLOAT
#FLOAT_FLAG := +fp16 

#LM
LM := -lm

LIMBO_LD_FLAGS +=  -Wl,--no-warn-mismatch

### CONFIGURATION
ARCH_CFLAGS = \
-std=gnu99 \
-march=armv8-a \
 $(FLOAT_FLAG)

# Tuning
-mtune=arm8

# Possible values: arm, thumb
ARM_MODE=arm

# Suppress some warnings
ARCH_CFLAGS += -Wno-psabi

# Optimization
#ANDROID_OPTIM_FLAGS = -O0
ANDROID_OPTIM_FLAGS = -O5

ARCH_CFLAGS += $(ANDROID_OPTIM_FLAGS)

ifeq ($(NDK_DEBUG),1)
	ARCH_CFLAGS += -g
	ARCH_CFLAGS += -UNDEBUG
	# for Debugging only
	ARCH_CFLAGS += -funwind-tables 
else
	ARCH_CFLAGS += -DNDEBUG
endif 

# Smaller code generation for shared libraries, usually faster
# if doesn't work use -fPIC
ARCH_CFLAGS += -fpic 

# Reduce executable size
ARCH_CFLAGS += -ffunction-sections

# Don't keep the frame pointer in a register for functions that don't need one
# Anyway enabled for -O2
ARCH_CFLAGS += -fomit-frame-pointer 

# prevent unwanted optimizations for Qemu
ARCH_CFLAGS += -fno-strict-aliasing

# Loop optimization might be safe
ARCH_CFLAGS += -fstrength-reduce 
ARCH_CFLAGS += -fforce-addr 

# Faster math might not be safe
#ARCH_CFLAGS += -ffast-math

# anyway enabled by -O2
ARCH_CFLAGS += -foptimize-sibling-calls

# Should not be limiting inline functions or this value should be very large
ARCH_CFLAGS += -finline-limit=99999

# Not supported
#ARCH_CFLAGS += -fforce-mem

# Fast optimizations but maybe crashing apps?
#ARCH_CFLAGS += -funsafe-math-optimizations 

# Useful for IEEE non-stop floating
#ARCH_CFLAGS += -fno-trapping-math

# To suppress looking in stadnard includes for the toolchain
#ARCH_CFLAGS += -nostdinc

 

# SLows down but needed for some devices
# where stack corruption is more probable
#ARCH_CFLAGS += -fstack-protector
ARCH_CFLAGS += -fno-stack-protector

# ORIGINAL CFLAGS FROM ANDROID NDK
#-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  \
#-march=armv5te -mtune=xscale -msoft-float -mthumb \
#-Os
#-fpic -ffunction-sections -funwind-tables -fstack-protector \
#-Wno-psabi 
#-fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 
