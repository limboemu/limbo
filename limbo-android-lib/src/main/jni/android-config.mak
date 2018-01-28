############### Base definitions for Android toolchain.
# DO NOT MODIFY THIS SECTION
LIMBO_JNI_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# If Windows/CYGWIN
ifeq ($(findstring CYGWIN,$(shell uname -s)),CYGWIN)
    # Already a windows path
    ifeq ($(findstring :,$(LIMBO_JNI_ROOT)),:)
        LIMBO_JNI_ROOT:=$(shell dirname $(lastword $(MAKEFILE_LIST)))
    # Otherwise we convert to a Windows path
    else
        LIMBO_JNI_ROOT:=$(shell cygpath -m $(LIMBO_JNI_ROOT))
    endif
endif

############### This is the only part of the file you need to change before when compiling.
############## Project Config

#The script will figure out the current directory on its own
# though if it doesn't you can specify it here
#LIMBO_JNI_ROOT := C:/users/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni

# If you want to use SDL
USE_SDL ?= true

# If you want to use SDL Audio
USE_SDL_AUDIO ?= true

# Optimization
USE_OPTIMIZATION ?= true

# Enable KVM (Currently Disabled, NOT Tested)
# Note: KVM headers are available only for android-21 platform and above
#USE_KVM ?= true



############## Build Environment (toolchain)

# Uncomment all lines below if you use Windows to compile
NDK_ROOT = C:/tools/bin/android-ndk-r13b-windows-x86/android-ndk-r13b
NDK_ENV = windows
# or Windows 64 bit to compile
#NDK_ROOT = C:/tools/bin/android-ndk-r13-windows-x86_64/android-ndk-r13
#NDK_ENV = windows-x86_64

# Uncomment all lines below if you use Linux to compile
#NDK_ROOT = /home/dev/tools/android/android-ndk-r13b
#NDK_ENV = linux-x86
# or Linux 64bit to compile
#NDK_ENV = linux-x86_64

# Uncomment all lines below if you use MacOS to compile
#NDK_ROOT = /home/dev/tools/android/android-ndk-r13b
#NDK_ENV = darwin-x86



############## ANDROID DEVICE CONFIGURATION

#PLATFORM CONFIG
# Ideally App platform used to compile should be equal or lower than the minSdkVersion in AndroidManifest.xml
# Note 1: Building for Android ARM host requires ndk13 and android-17
# Note 2: Building for Android x86 host requires ndk13 and android-17
# Note 3: Building for Android x86 host w/ KVM support requires ndk13 and android-21
# Note 4: Building for Android x86_64 host requires ndk13 and android-21
# Note 5: Building for Android ARM64 host requires ndk13 and android-21


######### Armv8 64 bit (Newest ARM phones only, Supports VNC, Needs android-21)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv8.mak

######### ARMv7 Soft Float  (Most ARM phones, Supports VNC and SDL, Needs android-17)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-armv7a-softfp.mak

######### x86 (x86 Phones only, Supports VNC and SDL, Needs android-17)
#include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-x86.mak

######### x86_64 (x86 64bit Phones only, Supports VNC, Needs android-21)
include $(LIMBO_JNI_ROOT)/android-device-config/android-generic-x86_64.mak



######## Guest Config
# Uncomment to build for the guest architecture
# Supported
#QEMU_TARGET_LIST = i386-softmmu
#QEMU_TARGET_LIST = x86_64-softmmu
#QEMU_TARGET_LIST = arm-softmmu
#QEMU_TARGET_LIST = ppc-softmmu
#QEMU_TARGET_LIST = ppc64-softmmu
#QEMU_TARGET_LIST = sparc-softmmu
#QEMU_TARGET_LIST = alpha-softmmu

# Or create multiple archs
QEMU_TARGET_LIST = x86_64-softmmu,arm-softmmu,ppc-softmmu,sparc-softmmu


################ No modifications below this line are necessary #####################

include $(LIMBO_JNI_ROOT)/android-setup-toolchain.mak