LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE:= $(APP_ARM_MODE)

#include $(LIMBO_JNI_ROOT)/android-config.mak

  
LOCAL_SRC_FILES := \
	limbo_compat_memmove.c \
	limbo_compat_fd.c \
	limbo_compat.c

# For backwards compatibility with 1.2	
#LOCAL_SRC_FILES := \
	SDL_compat.c \
	SDL_compat.h

#$(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

LOCAL_MODULE := limbocompat

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../SDL/src \
	$(LOCAL_PATH)/../SDL/include

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -include $(LOGUTILS)
LOCAL_ARM_MODE := $(ARM_MODE)

#No optimization for memmove
LOCAL_CFLAGS += -O0

LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES := 

LOCAL_LDLIBS :=				\
	-ldl -llog

include $(BUILD_STATIC_LIBRARY)
