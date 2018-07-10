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

############### Make Changes below this line ##############

#The section above this will figure out the current directory on its own
# though if it doesn't you can specify it here:
#LIMBO_JNI_ROOT := /home/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni

# If you want to use SDL
USE_SDL ?= true

# If you want to use SDL Audio
USE_SDL_AUDIO ?= true

# Optimization (set to false when debugging)
USE_OPTIMIZATION ?= true

# Enable KVM (NOT Tested)
# Note: KVM headers are available only for android-21 platform and above
USE_KVM ?= true

# Build threads
BUILD_THREADS = 3

############## Build Environment (toolchain)

# Uncomment all lines below if you use Linux to compile
#NDK_ROOT = /home/dev/tools/ndk/android-ndk-r13b
#NDK_ROOT = /home/dev/tools/ndk/android-ndk-r14b
#NDK_ROOT = /home/dev/tools/ndk/android-ndk-r15c
NDK_ROOT = /home/dev/tools/ndk/android-ndk-r17b

# Uncomment if you use Linux x86 to compile
#NDK_ENV = linux-x86
# or Linux 64bit
NDK_ENV = linux-x86_64

# Uncomment all lines below if you use MacOS to compile, not TESTED
#NDK_ROOT = /home/dev/tools/android/android-ndk-r17b
#NDK_ENV = darwin-x86

################ No modifications below this line are necessary #####################

include $(LIMBO_JNI_ROOT)/android-setup-toolchain.mak
