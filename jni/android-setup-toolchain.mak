# DO NOT MODIFY THIS FILE
TARGET_ARCH = 

ifeq ($(APP_ABI),armeabi)
    EABI = arm-linux-androideabi-$(NDK_TOOLCHAIN_VERSION)
    TOOLCHAIN_PREFIX = arm-linux-androideabi-
    TARGET_ARCH=arm
    APP_ABI_DIR=$(APP_ABI)
else ifeq ($(APP_ABI),armeabi-v7a)
    EABI = arm-linux-androideabi-$(NDK_TOOLCHAIN_VERSION)
    TOOLCHAIN_PREFIX = arm-linux-androideabi-
    TARGET_ARCH=arm
    APP_ABI_DIR=$(APP_ABI)
else ifeq ($(APP_ABI),armeabi-v7a-hard)
    EABI = arm-linux-androideabi-$(NDK_TOOLCHAIN_VERSION)
    TOOLCHAIN_PREFIX = arm-linux-androideabi-
    TARGET_ARCH=arm
    APP_ABI_DIR=armeabi-v7a
else ifeq ($(APP_ABI),x86)
    EABI = x86-$(NDK_TOOLCHAIN_VERSION)
    TOOLCHAIN_PREFIX = i686-linux-android-
    TARGET_ARCH=x86
    APP_ABI_DIR=$(APP_ABI)
endif

TOOLCHAIN_DIR = $(NDK_ROOT)/toolchains/$(EABI)/prebuilt/$(NDK_ENV)
TOOLCHAIN_PREFIX := $(TOOLCHAIN_DIR)/bin/$(TOOLCHAIN_PREFIX)
NDK_PROJECT_PATH := $(LIMBO_JNI_ROOT)/../

#NDK Toolchain
CC = $(TOOLCHAIN_PREFIX)gcc
AR = $(TOOLCHAIN_PREFIX)ar
LNK = $(TOOLCHAIN_PREFIX)g++
STRIP = $(TOOLCHAIN_PREFIX)strip
AR_FLAGS = crs
SYS_ROOT = --sysroot=$(NDK_ROOT_INC)/$(NDK_PLATFORM)/arch-$(TARGET_ARCH)
NDK_INCLUDE = $(NDK_ROOT_INC)/$(NDK_PLATFORM)/arch-$(TARGET_ARCH)/usr/include

# INCLUDE_FIXED contains overrides for include files found under the toolchain's /usr/include.
# Hoping to get rid of those one day, when newer NDK versions are released.
INCLUDE_FIXED = $(LIMBO_JNI_ROOT_INC)/include-fixed

# The logutils header is injected into all compiled files in order to redirect
# output to the Android console, and provide debugging macros.
LOGUTILS = $(LIMBO_JNI_ROOT_INC)/compat/limbo_logutils.h

#Some fixes for Android compatibility
FIXUTILS_MEM = $(LIMBO_JNI_ROOT_INC)/compat/limbo_compat_memove.h
COMPATUTILS_FD = $(LIMBO_JNI_ROOT_INC)/compat/limbo_compat_fd.h
COMPATMACROS = $(LIMBO_JNI_ROOT_INC)/compat/limbo_compat_macros.h
COMPATANDROID = $(LIMBO_JNI_ROOT_INC)/compat/limbo_compat.h

# These compatibility functions should be forcebly included from the static compat library
$INCLUDE_FUNCS = -u set_jni

USR_LIB = \
-L$(TOOLCHAIN_DIR)//lib

ARCH_CFLAGS := $(ARCH_CFLAGS) -D__LIMBO__ -D__ANDROID__ -DANDROID -D__linux__ $(USE_NDK11) 

# logging macros
ARCH_CFLAGS := $(ARCH_CFLAGS)

# INCLUDE_FIXED
SYSTEM_INCLUDE = \
    -I$(INCLUDE_FIXED) \
    $(SYS_ROOT) \
    -I$(LIMBO_JNI_ROOT_INC)/qemu/linux-headers \
    -I$(TOOLCHAIN_DIR_INC)/$(EABI)/include \
    -I$(NDK_INCLUDE) \
    -include $(LOGUTILS) \
    -include $(FIXUTILS_MEM) \
    -include $(COMPATUTILS_FD) \
    -include $(COMPATMACROS) \
    -include $(COMPATANDROID)
