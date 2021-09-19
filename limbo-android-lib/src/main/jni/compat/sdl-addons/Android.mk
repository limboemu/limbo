LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := compat-SDL2-addons

LOCAL_SRC_FILES := SDL_limboaudio.c
LOCAL_C_INCLUDES :=

include $(BUILD_SHARED_LIBRARY)

