LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

$(warning NDK_TOOLCHAIN_VERSION = $(NDK_TOOLCHAIN_VERSION))

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

LOCAL_SRC_FILES := \
	limbo_compat_filesystem.c \
	limbo_compat.c \
	limbo_compat_qemu.c \
	limbo_compat_signals.cpp \
	$(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

LOCAL_MODULE := compat-limbo

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/signals \
	$(LOCAL_PATH)/../SDL/src \
	$(LOCAL_PATH)/../SDL/include

LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -include $(LOGUTILS)
LOCAL_ARM_MODE := $(ARM_MODE)
LOCAL_LDFLAGS += $(ARCH_LD_FLAGS)

LOCAL_CFLAGS +=
LOCAL_STATIC_LIBRARIES +=

LOCAL_CFLAGS +=

LOCAL_LDLIBS := -ldl -llog

include $(BUILD_SHARED_LIBRARY)

