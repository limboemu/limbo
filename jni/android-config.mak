# Base definitions for Android toolchain.
# This is the only part of the file you need to change before when compiling.

####### GLOBAL CONFIGURATION
# Faster Builds with multiple threads
BUILD_THREADS=4

#PLATFORM CONFIG
APP_PLATFORM = android-21
NDK_PLATFORM = platforms/$(APP_PLATFORM)

# Uncomment if you use NDK11 and above
#USE_NDK11 = -D__NDK11_FUNC_MISSING__

# If you want to use SDL
USE_SDL ?= true

# If you want to use SDL Audio (currently not working)
USE_SDL_AUDIO ?= false

################## ENVIRONMENT CONFIG
####### Windows/Cygwin
NDK_ROOT = /cygdrive/d/tools/bin/android-ndk-r10e
NDK_ROOT_INC = C:/tools/bin/android-ndk-r10e
LIMBO_JNI_ROOT := /cygdrive/c/users/dev/limbo/limbo-android/jni
LIMBO_JNI_ROOT_INC := C:/users/dev/limbo/limbo-android/jni

####### Linux/Macos X
#NDK_ROOT = /home/dev/tools/android/android-ndk-r10e
#NDK_ROOT_INC = $(NDK_ROOT)
#LIMBO_JNI_ROOT := $(CURDIR)
#LIMBO_JNI_ROOT_INC := $(LIMBO_JNI_ROOT)

####### SPECIFIC OS
# Uncomment For Windows/Cygwin
NDK_ENV = windows-x86_64
# Uncomment For Ubuntu/Linux 64bit
#NDK_ENV = linux-x86_64
# Uncomment For Ubuntu/Linux x86
#NDK_ENV = linux-x86
# Uncomment For MacOSX
#NDK_ENV = darwin-x86

############## ANDROID DEVICE CONFIGURATION
# Chooce ONLY ONE:


######### ARMv7 Hard Float (SDL interface not working)
# ARMv7 Hard Float Generic Hard float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard.mak

# ARMv7 Hard Float Generic Hard float No optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard-noopt.mak


######### ARMv7 Soft Float  (Supports VNC and SDL)
# ARMv7 Generic soft float
include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-vfpv3d16.mak

# ARMv7 Generic soft float No Optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-vfpv3d16-noopt.mak

######### ARMv5 Soft Float  (Supports VNC and SDL for old devices)
# ARMv5 Generic soft float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv5te-softfp.mak

# ARMv5 Generic No optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv5te-softfp-noopt.mak

######### x86
# x86 Phones (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86.mak

# x86 Phones Debug No optimization (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86-noopt.mak

################ No modifications below this line are necessary #####################
include $(LIMBO_JNI_ROOT)/android-setup-toolchain.mak