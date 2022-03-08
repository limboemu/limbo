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
#ifndef SDL_LIMBO_MOUSE_H
#define SDL_LIMBO_MOUSE_H

#include <jni.h>
#include "video/android/SDL_androidvideo.h"

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeMouseEvent(
        JNIEnv* env, jobject thiz, int button, int action, int relative, 
        int x, int y);

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeMouseBounds(
        JNIEnv* env, jobject thiz,  int xmin, int xmax, int ymin, int ymax);

#endif

