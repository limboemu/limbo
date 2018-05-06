#Needs platform = android-21
APP_PLATFORM = android-21

# Set to true if you use platform-21 and above
USE_NDK_PLATFORM21 ?= true

#NDK VERSION
NDK_TOOLCHAIN_VERSION=4.9

#ARCH SPECIFIC
APP_ABI := arm64-v8a
ARM_MODE:=arm
ARCH_CFLAGS := -march=armv8-a

# Tuning (Optional)
#ARCH_CFLAGS += -mtune=arm8

#ARM GENERIC
include $(LIMBO_JNI_ROOT)/android-device-config/android-generic.mak