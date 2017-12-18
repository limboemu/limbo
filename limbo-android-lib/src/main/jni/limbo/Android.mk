LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

include $(LIMBO_JNI_ROOT)/android-config.mak
 
LOCAL_SRC_FILES :=			\
	vm-executor-jni.cpp \
	debugger.cpp

#$(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

LOCAL_MODULE := limbo

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/.. \
	$(LIMBO_JNI_ROOT)/qemu \
	$(LIMBO_JNI_ROOT)/qemu/include \
	$(LIMBO_JNI_ROOT)/compat \
	$(LIMBO_JNI_ROOT)/glib/glib \
	$(LIMBO_JNI_ROOT)/glib \
	$(LIMBO_JNI_ROOT)/glib/android

LOCAL_STATIC_LIBRARIES := spice png glib-2.0

LOCAL_LDLIBS :=				\
	-ldl -llog 

#LIMBO
#LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -std=c++11 -U__STRICT_ANSI__ -fexceptions
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS) 
LOCAL_STATIC_LIBRARIES += liblimbocompat
LOCAL_ARM_MODE := $(ARM_MODE)

APP_STL                 := stlport_static

include $(BUILD_SHARED_LIBRARY)