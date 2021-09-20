############### Limbo Configuration ##############
# if the  makefile doesn't recognize the project path you can override it here:
#LIMBO_JNI_ROOT := /home/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni

# Last version with gcc support is 14b 
#NDK_ROOT = /home/dev/tools/ndk/android-ndk-r14b
#USE_GCC?=true
# Or use r23 with clang 
NDK_ROOT ?= /home/dev/tools/ndk/android-ndk-r23
USE_GCC?=false

### the ndk api should be the same as the minSdkVersion in your AndroidManifest.xml 
NDK_PLATFORM_API=26

# Set to true if you use platform-21 or above
USE_NDK_PLATFORM21 ?= true

# Set to true if you use platform-26 or above
USE_NDK_PLATFORM26 ?= true

# Optimization, generally it is better set to false when debugging
USE_OPTIMIZATION ?= true

# Hardening: it produces slower runtimes but helps preventing buffer overflow attacks
USE_SECURITY ?= true

# Uncomment to enable debugging
# If you enable debugging you should turn off optimization as well
#NDK_DEBUG=1

# Uncomment if you use Linux x86, Linux 64bit, or macosx PC to compile
# Compiling on Windows is no longer supported
#NDK_ENV ?= linux-x86
NDK_ENV ?= linux-x86_64
#NDK_ENV ?= darwin-x86

# Build threads (make -j ?) makes building faster
BUILD_THREADS ?= 3

############## QEMU Host and Guest

# Android device type (host arch)
# values: armeabi-v7a, arm64-v8a, x86, x86_64
BUILD_HOST?=arm64-v8a

# GUEST_ARCH is the Emulator type
# values: x86_64-softmmu,aarch64-softmmu,sparc64-softmmu,ppc64-softmmu
BUILD_GUEST?=x86_64-softmmu

# QEMU Version
# values: 2.9.1, 5.1.0
USE_QEMU_VERSION ?= 2.9.1

# If you want to use SDL interface
USE_SDL ?= true

# If you want to use SDL Audio with Android AudioTrack
USE_SDL_AUDIO ?= true

# if you want to use Android AAudio, it needs version platform API 26
USE_AAUDIO ?= true

# Enable KVM
# Note: KVM headers are available only for android-21 platform and above
USE_KVM ?= true
