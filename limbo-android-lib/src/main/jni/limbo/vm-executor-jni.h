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

#ifndef VM_EXECUTOR_JNI_H
#define VM_EXECUTOR_JNI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "qemu/typedefs.h"

typedef struct Error Error;


void * loadLib(const char * lib_path_str);

void setup_jni(JNIEnv* env, jobject thiz, jstring storage_dir, jstring base_dir);

int get_qemu_var(JNIEnv* env, jobject thiz, const char * var);

void set_qemu_var(JNIEnv* env, jobject thiz, const char * var, jint jvalue);

extern "C" {
    
JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeRefreshScreen(
                JNIEnv* env, jobject thiz, jint jvalue);
                
JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setvncrefreshrate(
		JNIEnv* env, jobject thiz, jint jvalue);

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setSDLRefreshRateDefault(
		JNIEnv* env, jobject thiz, jint jvalue);
        
JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setSDLRefreshRateIdle(
		JNIEnv* env, jobject thiz, jint jvalue);
        
JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getSDLRefreshRateDefault(
		JNIEnv* env, jobject thiz);
        
JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getSDLRefreshRateIdle(
		JNIEnv* env, jobject thiz);

JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getvncrefreshrate(
		JNIEnv* env, jobject thiz);

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_start(
        JNIEnv* env, jobject thiz,
		jstring storage_dir, jstring base_dir,jstring lib_path, 
        jint sdl_scale_hint, jobjectArray params);
        
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_stop(
		JNIEnv* env, jobject thiz, jint jint_restart);

}

#endif

