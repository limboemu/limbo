
#include "src/SDL_internal.h"
#include "SDL_limbomouse.h"
#include "SDL_events.h"
#include "events/SDL_mouse_c.h"
#include "core/android/SDL_android.h"

#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define ACTION_HOVER_MOVE 7
#define ACTION_SCROLL 8
#define BUTTON_PRIMARY 1
#define BUTTON_SECONDARY 2
#define BUTTON_TERTIARY 4
#define BUTTON_BACK 8
#define BUTTON_FORWARD 16

int relativeMouseMode = 1;

JNIEXPORT int JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setrelativemousemode(
        JNIEnv* env, jobject thiz,
		int relative) {


	if (!Android_Window) {
            return -1;
    }

    printf("Set relative mouse mode: %d", relative);
    relativeMouseMode = relative;

    return 0;
}

JNIEXPORT int JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_onmouse(
        JNIEnv* env, jobject thiz,
		int button, int action, int relative, float x, float y) {

    if (!Android_Window) {
        return -1;
    }

    if(relativeMouseMode)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);

    switch(action) {
        case ACTION_DOWN:
            if(!relative){
                SDL_SendMouseMotion(Android_Window, 0, 0, x, y);
            }
            SDL_SendMouseButton(Android_Window, SDL_TOUCH_MOUSEID, SDL_PRESSED, button);
            break;

        case ACTION_UP:
            if(!relative){
                SDL_SendMouseMotion(Android_Window, 0, 0, x, y);
            }
            SDL_SendMouseButton(Android_Window, SDL_TOUCH_MOUSEID, SDL_RELEASED, button);
            break;

        case ACTION_MOVE:
        case ACTION_HOVER_MOVE:
            SDL_SendMouseMotion(Android_Window, 0, relative, (int) x, (int) y);
            break;

        case ACTION_SCROLL:
            SDL_SendMouseWheel(Android_Window, SDL_TOUCH_MOUSEID, x, y, SDL_MOUSEWHEEL_NORMAL);
            break;

        default:
            break;
    }
    return 0;
}


