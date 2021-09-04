/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
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

JNIEXPORT int JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_onmouse(
        JNIEnv* env, jobject thiz,
		int button, int action, int relative, float x, float y) {

    if (!Android_Window) {
        return -1;
    }
    
    //XXX: If the guest input device is not usb-tablet (ps2/usb) QEMU overrides to relative mode
    // in order to provide a workaround we force the mode. The user will still need to disable 
    // mouse acceleration within the guest and calibrate the mouse in limbo.
    SDL_bool relativeMouseMode = relative?SDL_TRUE:SDL_FALSE; 
    if(SDL_GetRelativeMouseMode() != relativeMouseMode ) {
    	printf("force relative mouse mode: %d", relativeMouseMode);
    	SDL_SetRelativeMouseMode(relativeMouseMode);
    }

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


