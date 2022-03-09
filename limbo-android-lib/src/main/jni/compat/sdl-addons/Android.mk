LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# We override the target platform
LOCAL_CFLAGS += -target $(TARGET_PREFIX)26
LOCAL_LDFLAGS += -target $(TARGET_PREFIX)26

LOCAL_LDLIBS += -laaudio

LOCAL_MODULE := compat-SDL2-addons
LOCAL_SRC_FILES := SDL_limboaudio.c

include $(BUILD_SHARED_LIBRARY)
