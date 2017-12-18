# Base definitions for Android toolchain.
# This is the only part of the file you need to change before when compiling.


############## Project Config

# Enable KVM (NOT Tested)
# Note: KVM headers are available only for android-21 platform and above
USE_KVM ?= false

# If you want to use SDL (not working for Hard-float device config, see below)
USE_SDL ?= true

# If you want to use SDL Audio (currently not working)
USE_SDL_AUDIO ?= true



############## Environment Config

#PLATFORM CONFIG
# Ideally App platform used to compile should be equal or lower than the minSdkVersion in AndroidManifest.xml
# Note 1: Building for Android ARM host requires ndk13 and android-17
# Note 2: Building for Android x86 host requires ndk13 and android-17
# Note 3: Building for Android x86 host w/ KVM support requires ndk13 and android-21
# Note 4: Building for Android x86_64 host requires ndk13 and android-21
# Note 5: Building for Android ARM64 host requires ndk13 and android-21
 
APP_PLATFORM = android-17
NDK_PLATFORM = platforms/$(APP_PLATFORM)

# Set to true if you use platform-21 and above
USE_NDK_PLATFORM21 ?= false

# Faster Builds with multiple threads
BUILD_THREADS=6



############## Windows Config
# Uncomment all lines below if you use Windows to compile
##### ndk 13b for Windows x86
LIMBO_JNI_ROOT := C:/users/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni
NDK_ROOT = C:/tools/bin/android-ndk-r13b-windows-x86/android-ndk-r13b
NDK_ENV = windows
# or Windows 64 bit to compile
#NDK_ENV = windows-x86_64



############## Linux Config
# Uncomment all lines below if you use Linux to compile
#LIMBO_JNI_ROOT := /home/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni
#NDK_ROOT = /home/dev/tools/android/android-ndk-r13b
#NDK_ENV = linux-x86
# or Linux 64bit to compile
#NDK_ENV = linux-x86_64



############## MacOS X Config
# Uncomment all lines below if you use MacOS to compile
#LIMBO_JNI_ROOT := /home/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni
#NDK_ROOT = /home/dev/tools/android/android-ndk-r13b
#NDK_ENV = darwin-x86



############## ANDROID DEVICE CONFIGURATION
# Choose ONLY ONE:

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

######### x86_64 (glib fails to compile, glibconfig.h needs update)
# x86_64 Phones (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86_64.mak

# x86_64 Phones Debug No optimization (ie Zenfone)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-ndkr8-x86_64-noopt.mak

######### ARMv7 Hard Float (SDL interface not working, No longer supported from ndk 12 and above)
# ARMv7 Hard Float Generic Hard float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard.mak

# ARMv7 Hard Float Generic Hard float No optimization
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-hard-noopt.mak

######### Armv8 64 bit (glib fails to compile, glibconfig.h needs update)

# ARMv8 64 bit - Generic soft float
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv8-vfpv3d16.mak

# ARMv8 64 bit - Generic soft float No optimazation
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv8-vfpv3d16-noopt.mak


######## Guest Config
# Uncomment to build for the guest architecture
# Supported
#QEMU_TARGET_LIST = i386-softmmu
#QEMU_TARGET_LIST = x86_64-softmmu
#QEMU_TARGET_LIST = arm-softmmu
#QEMU_TARGET_LIST = ppc-softmmu
#QEMU_TARGET_LIST = ppc64-softmmu
#QEMU_TARGET_LIST = sparc-softmmu

# Or create multiple archs
QEMU_TARGET_LIST = x86_64-softmmu,arm-softmmu,ppc-softmmu,sparc-softmmu


################ No modifications below this line are necessary #####################

include $(LIMBO_JNI_ROOT)/android-setup-toolchain.mak