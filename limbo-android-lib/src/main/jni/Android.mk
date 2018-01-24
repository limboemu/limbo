#include $(call all-subdir-makefiles)

include $(LIMBO_JNI_ROOT)/android-config.mak

#If you want to build individual modules
#include $(NDK_PROJECT_PATH)/jni/png/Android.mk
#include $(NDK_PROJECT_PATH)/jni/jpeg/Android.mk
include $(NDK_PROJECT_PATH)/jni/glib/Android.mk
#include $(NDK_PROJECT_PATH)/jni/iconv/Android.mk
include $(NDK_PROJECT_PATH)/jni/compat/Android.mk
#include $(NDK_PROJECT_PATH)/jni/openssl/Android.mk
#include $(NDK_PROJECT_PATH)/jni/spice/Android.mk

ifeq ($(USE_SDL),true)
	include $(NDK_PROJECT_PATH)/jni/SDL/Android.mk
	include $(NDK_PROJECT_PATH)/jni/SDL_image/Android.mk
	include $(NDK_PROJECT_PATH)/jni/limbo/sdl_main/Android.mk
endif

ifeq ($(USE_SDL_AUDIO),true)
	include $(NDK_PROJECT_PATH)/jni/SDL_mixer/Android.mk 
endif

include $(NDK_PROJECT_PATH)/jni/pixman/Android.mk
include $(NDK_PROJECT_PATH)/jni/limbo/Android.mk
