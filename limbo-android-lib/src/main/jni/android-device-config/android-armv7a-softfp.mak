#Needs platform android-21
APP_PLATFORM = android-21

# Set to true if you use platform-21 and above
USE_NDK_PLATFORM21 ?= true

#NDK VERSION
NDK_TOOLCHAIN_VERSION=4.9

#LINKER SPECIFIC
ARCH_LD_FLAGS += -Wl,--fix-cortex-a8

#TARGET ARCH
APP_ABI := armeabi-v7a
ARM_MODE=arm
ARCH_CFLAGS += -march=armv7-a

#FLOAT
ARCH_CFLAGS += -mfloat-abi=softfp

# Use VFP (Optional)
ARCH_CFLAGS += -mfpu=vfpv3-d16
#ARCH_CFLAGS += -mfpu=vfpv3

# Tuning (Optional)
#ARCH_CFLAGS += -mtune=arm7


#ARM GENERIC
include $(LIMBO_JNI_ROOT)/android-device-config/android-generic.mak

