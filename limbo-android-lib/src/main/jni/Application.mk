LIMBO_JNI_ROOT := $(CURDIR)/jni

#STL port
#APP_STL := stlport_shared
#APP_STL := c++_shared

include $(LIMBO_JNI_ROOT)/android-config.mak

#Suppress Format errors from logutils.h macros
APP_CFLAGS += -Wno-format-security

#Debug/Release
ifeq ($(NDK_DEBUG),1)
    APP_OPTIM := debug
else
    APP_OPTIM := release
endif

#Don't remove this 
APP_CFLAGS += $(ARCH_CFLAGS)

#FIXME: we should use memmove for the utils also
#APP_CFLAGS += -include $(FIXUTILS)

#FIXME: we should override logging for the utils also
#APP_CFLAGS += -include $(LOGUTILS)

APP_ARM_MODE=$(ARM_MODE)

$(warning NDK_TOOLCHAIN_VERSION = $(NDK_TOOLCHAIN_VERSION))
$(warning NDK_DEBUG = $(NDK_DEBUG))
$(warning APP_ARM_MODE = $(APP_ARM_MODE))
$(warning APP_ARM_NEON = $(APP_ARM_NEON))
$(warning APP_OPTIM = $(APP_OPTIM))
$(warning APP_ABI = $(APP_ABI))
$(warning APP_PLATFORM = $(APP_PLATFORM))
$(warning NDK_PROJECT_PATH = $(NDK_PROJECT_PATH))
$(warning ARCH_CFLAGS = $(ARCH_CFLAGS))
$(warning APP_CFLAGS = $(APP_CFLAGS))
