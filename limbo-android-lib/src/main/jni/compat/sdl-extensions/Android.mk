LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SDL_limbomouse.c \
	SDL_limboscreen.c

LOCAL_MODULE := compat-SDL2-ext

LOCAL_C_INCLUDES :=			\
	$(LOCAL_PATH)/../../SDL2 \
	$(LOCAL_PATH)/../../SDL2/src \
	$(LOCAL_PATH)/../../SDL2/include

LOCAL_SHARED_LIBRARIES += SDL2

include $(BUILD_SHARED_LIBRARY)


