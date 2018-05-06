#Needs platform = android-21
APP_PLATFORM = android-21

# Set to true if you use platform-21 and above
USE_NDK_PLATFORM21 ?= true

#NDK VERSION
NDK_TOOLCHAIN_VERSION=4.9

#TARGET ARCH
APP_ABI := x86

#X86 GENERIC
include $(LIMBO_JNI_ROOT)/android-device-config/android-generic.mak

