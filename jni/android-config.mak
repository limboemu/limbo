# Base definitions for Android toolchain.
# This is the only part of the file you need to change before when compiling.

############## Project Config Start

# IMPORTANT: Speficy the root of the project
LIMBO_JNI_ROOT := C:/users/dev/limbo/limbo-android/jni
LIMBO_JNI_ROOT_INC := C:/users/dev/limbo/limbo-android/jni

# Enable KVM (NOT Tested)
# Note: KVM headers are available only for android-21 platform and above
USE_KVM ?= true

# If you want to use SDL (not working for Hard-float device config, see below)
USE_SDL ?= true

# If you want to use SDL Audio (currently not working)
USE_SDL_AUDIO ?= true

############## Project Config End

############## Environment Config Start

#PLATFORM CONFIG
# Ideally App platform used to compile should be equal or lower than the minSdkVersion in AndroidManifest.xml
# Note 1: Building for Android ARM requires ndk13 and android-14
# Note 2: Building for Android x86 requires ndk13b and android-17
# Note 3: Building for Android x86 w/ KVM support requires ndk13b and android-21 
APP_PLATFORM = android-21
NDK_PLATFORM = platforms/$(APP_PLATFORM)

# If you use platform-21 and above
USE_NDK_PLATFORM21 ?= true

# Faster Builds with multiple threads
BUILD_THREADS=8

####### Windows/Cygwin SECTION END
# If you use Windows to build

# ndk 13b for x86
NDK_ROOT = C:/tools/bin/android-ndk-r13b-windows-x86/android-ndk-r13b
NDK_ROOT_INC = C:/tools/bin/android-ndk-r13b-windows-x86/android-ndk-r13b
NDK_ENV = windows

# ndk 13 for 64bit
#NDK_ROOT = C:/tools/bin/android-ndk-r13-windows-x86_64/android-ndk-r13
#NDK_ROOT_INC = C:/tools/bin/android-ndk-r13-windows-x86_64/android-ndk-r13
#NDK_ENV = windows-x86_64

# ndk 12
#NDK_ROOT = C:/tools/bin/android-ndk-r12b-windows-x86/android-ndk-r12b
#NDK_ROOT_INC = C:/tools/bin/android-ndk-r12b-windows-x86/android-ndk-r12b
#NDK_ENV = windows

# ndk 12 for 64bit
#NDK_ROOT = C:/tools/bin/android-ndk-r12b-windows-x86_64/android-ndk-r12b
#NDK_ROOT_INC = C:/tools/bin/android-ndk-r12b-windows-x86_64/android-ndk-r12b
#NDK_ENV = windows-x86_64

####### Windows Config End 

############## Linux Config START
# If you use Linux for compiling

# Uncomment all lines below if you use Linux
#NDK_ROOT = /home/dev/tools/android/android-ndk-r12b
#NDK_ROOT_INC = $(NDK_ROOT)
#LIMBO_JNI_ROOT := $(CURDIR)
#LIMBO_JNI_ROOT_INC := $(LIMBO_JNI_ROOT)

# Uncomment if you use Ubuntu/Linux x86 to compile
#NDK_ENV = linux-x86

# Uncomment if you use Ubuntu/Linux 64bit to compile
#NDK_ENV = linux-x86_64


############## Linux Config END

############## MacOS X Config End
# If you use MacOS X to build

# ndk 12 for 64bit
#NDK_ROOT = C:/tools/bin/android-ndk-r12b-windows-x86_64/android-ndk-r12b
#NDK_ROOT_INC = C:/tools/bin/android-ndk-r12b-windows-x86_64/android-ndk-r12b
#NDK_ENV = darwin-x86

#NDK_ROOT = /home/dev/tools/android/android-ndk-r12b
#NDK_ROOT_INC = $(NDK_ROOT)
#LIMBO_JNI_ROOT := $(CURDIR)
#LIMBO_JNI_ROOT_INC := $(LIMBO_JNI_ROOT)


############## MacOS X Config End

############## ANDROID DEVICE CONFIGURATION
# Chooce ONLY ONE:

######### ARMv7 Soft Float  (Supports VNC and SDL)
# ARMv7 Generic soft float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-vfpv3d16.mak

# ARMv7 Generic soft float No Optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-vfpv3d16-noopt.mak

######### ARMv5 Soft Float  (Supports VNC and SDL for old devices)
# ARMv5 Generic soft float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv5te-softfp.mak

# ARMv5 Generic No optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv5te-softfp-noopt.mak

######### x86
# x86 Phones (ie Zenfone)
include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86.mak

# x86 Phones Debug No optimization (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86-noopt.mak

######### x86_64 (glib fails to compile)
# x86_64 Phones (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86_64.mak

# x86_64 Phones Debug No optimization (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86_64-noopt.mak

######### ARMv7 Hard Float (SDL interface not working, No longer supported from ndk 12 and above)
# ARMv7 Hard Float Generic Hard float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard.mak

# ARMv7 Hard Float Generic Hard float No optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard-noopt.mak

################ No modifications below this line are necessary #####################
include $(LIMBO_JNI_ROOT)/android-setup-toolchain.mak