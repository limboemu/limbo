LIMBO_JNI_ROOT := $(CURDIR)/jni

include $(LIMBO_JNI_ROOT)/android-limbo-build.mak

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
APP_CFLAGS += $(ARCH_EXTRA_CFLAGS)
APP_LDFLAGS += $(ARCH_LD_FLAGS)

#FIXME: we should use memmove for the utils also
#APP_CFLAGS += -include $(FIXUTILS)

APP_ARM_MODE=$(ARM_MODE)

$(info NDK_TOOLCHAIN_VERSION = $(NDK_TOOLCHAIN_VERSION))
$(info NDK_DEBUG = $(NDK_DEBUG))
$(info APP_ARM_MODE = $(APP_ARM_MODE))
$(info APP_ARM_NEON = $(APP_ARM_NEON))
$(info APP_OPTIM = $(APP_OPTIM))
$(info APP_ABI = $(APP_ABI))
$(info APP_PLATFORM = $(APP_PLATFORM))
$(info NDK_PROJECT_PATH = $(NDK_PROJECT_PATH))
$(info ARCH_CFLAGS = $(ARCH_CFLAGS))
$(info APP_CFLAGS = $(APP_CFLAGS))
