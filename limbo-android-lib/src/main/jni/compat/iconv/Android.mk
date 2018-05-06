LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

LOCAL_SRC_FILES := \
    $(NDK_ROOT)/sources/android/support/src/musl-locale/iconv.c

LOCAL_MODULE := compat-iconv

LOCAL_C_INCLUDES :=			\
	$(NDK_ROOT)/sources/android/support/include

LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -include $(LOGUTILS)
LOCAL_ARM_MODE := $(ARM_MODE)

LOCAL_LDFLAGS += $(ARCH_LD_FLAGS)
LOCAL_LDFLAGS += -u iconv_open

LOCAL_CFLAGS +=

LOCAL_SHARED_LIBRARIES :=


LOCAL_LDLIBS :=				\
	-ldl -llog

include $(BUILD_SHARED_LIBRARY)

