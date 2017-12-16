LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

LOCAL_SRC_FILES :=			\
	gmodule.c    

LOCAL_MODULE := libgmodule-2.0

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/android-internal	\
	$(GLIB_TOP)/android-internal	\
	$(GLIB_C_INCLUDES)

LOCAL_CFLAGS :=				\
	-DHAVE_CONFIG_H			\
	-DG_LOG_DOMAIN=\"GModule\"	\
	-DG_DISABLE_DEPRECATED 

ifeq ($(GLIB_BUILD_STATIC),true)

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
#FIXME: Need to find out why this is failing
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS)
LOCAL_STATIC_LIBRARIES += liblimbocompat
LOCAL_ARM_MODE := $(ARM_MODE)

include $(BUILD_STATIC_LIBRARY)
else
LOCAL_SHARED_LIBRARIES := libglib-2.0

LOCAL_LDLIBS +=	-ldl -llog

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
#FIXME: Need to find out why this is failing
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS)
LOCAL_STATIC_LIBRARIES += limbocompat
LOCAL_ARM_MODE := $(ARM_MODE)
include $(BUILD_SHARED_LIBRARY)
endif
