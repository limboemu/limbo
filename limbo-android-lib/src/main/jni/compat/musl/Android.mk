LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

LOCAL_SRC_FILES := \
    $(LOCAL_PATH)/../musl/musl-locale/iconv.c \
    $(LOCAL_PATH)/../musl/musl-locale/intl.c

LOCAL_MODULE := compat-musl

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/../musl/include

LOCAL_ARM_MODE := $(ARM_MODE)
LOCAL_LDFLAGS += -u iconv_open
include $(BUILD_SHARED_LIBRARY)

