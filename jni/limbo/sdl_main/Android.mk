LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(SDL_PATH) \
	$(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/../SDL2_image \
	$(LOCAL_PATH)/../SDL2_mixer

# Add any compilation flags for your project here...
LOCAL_CFLAGS := \
	-DPLAY_MOD

#TEST_SRC := $(LOCAL_PATH)/tests/limbo-android-sdl-tests.c

SDLSRC := limbo-android-sdl.c $(TEST_SRC)

# Add your application source files here...
LOCAL_SRC_FILES := $(LOCAL_PATH)/$(SDLSRC)


LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL_ttf png jpeg

LOCAL_LDLIBS := -lGLESv1_CM -llog

#LIMBO
LOCAL_CFLAGS += $(ARCH_CFLAGS)
LOCAL_CFLAGS += -include $(FIXUTILS_MEM) -include $(LOGUTILS)
LOCAL_STATIC_LIBRARIES += liblimbocompat
LOCAL_ARM_MODE := $(ARM_MODE)

include $(BUILD_SHARED_LIBRARY)
