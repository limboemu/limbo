############### Limbo Configuration ##############
# if the  makefile doesn't recognize the project path you can override it here:
#LIMBO_JNI_ROOT := /home/dev/limbo/workspace_limbo/limbo-android-lib/src/main/jni

# If you want to use SDL interface
USE_SDL ?= true

# If you want to use Audio (only SDL supported)
USE_SDL_AUDIO ?= true

# Optimization (set to false when debugging)
USE_OPTIMIZATION ?= true

# Hardening, it produces slower runtimes but helps preventing buffer overflow attacks
USE_SECURITY ?= true

# Enable KVM
# Note: KVM headers are available only for android-21 platform and above
USE_KVM ?= true

# Build threads, if use multiple and fails, try building again
BUILD_THREADS = 3


############## Build Environment (toolchain)
# Last version with gcc support is 14b 
#USE_GCC?=true
#NDK_ROOT = /home/dev/tools/ndk/android-ndk-r14b

# Or use r23 with clang 
USE_GCC?=false
NDK_ROOT = /home/dev/tools/ndk/android-ndk-r23

# Uncomment if you use Linux x86, Linux 64bit, or macosx PC to compile
# Compiling on Windows is no longer supported
#NDK_ENV = linux-x86
NDK_ENV = linux-x86_64
#NDK_ENV = darwin-x86
