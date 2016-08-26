LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

include $(LIMBO_JNI_ROOT)/android-config.mak
 
LOCAL_SRC_FILES :=			\
	vm-executor-jni.c

#$(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

LOCAL_MODULE := limbo

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/.. \
	$(LIMBO_JNI_ROOT_INC)/qemu \
	$(LIMBO_JNI_ROOT_INC)/qemu/include \
	$(LIMBO_JNI_ROOT_INC)/compat \
	$(LIMBO_JNI_ROOT_INC)/glib/glib \
	$(LIMBO_JNI_ROOT_INC)/glib \
	$(LIMBO_JNI_ROOT_INC)/glib/android

LOCAL_STATIC_LIBRARIES := spice png glib-2.0

LOCAL_LDLIBS :=				\
	-ldl -llog

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS)
LOCAL_STATIC_LIBRARIES += liblimbocompat
LOCAL_ARM_MODE := $(ARM_MODE)

include $(BUILD_SHARED_LIBRARY)