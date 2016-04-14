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
#ifndef _LIMBO_LOGUTILS_H
#define	_LIMBO_LOGUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <android/log.h>

//XXX: Keep in mind this needs to have the following prop
// on your device/emulator otherwise it might crash app
//$ adb shell stop
//$ adb shell setprop log.redirect-stdio true
//$ adb shell start
#define DEBUG_OUTPUT 1
//#define DEBUG_OUTPUT_EXTRA 1

#define STR1(x) #x
#define STR(x) STR1(x)
#define TAG __FILE__ ":" STR(__LINE__)

//We declare this as static to prevent from getting undefined behaviour during link time
//FIXME: Shouldn't be returning a local addr we should fix this in the future
//static inline char * limbo_get_tag(const char * file, const char * func,
//		int line) {
//
//	int MAX_LENGTH = 24;
//	char tag[1024];
//	char buf[1024];
//	if(strlen(file) > MAX_LENGTH)
//		strcpy(tag, file+strlen(file) - MAX_LENGTH);
//	else
//		strcpy(tag, file);
//	strcat(tag, " : ");
//	strcat(tag, func);
//	strcat(tag, "() : ");
//	sprintf(buf, "%d", line);
//	strcat(tag, buf);
//	return tag;
//
//}

#ifdef DEBUG_OUTPUT

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG ,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARNING, TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)
#define printf(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define fprintf(stdout, ...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define perror(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
//FIXME: logging for SDL
//#define dolog(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define sdl_logerr(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_report(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_printf(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_vprintf(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_printf_unless_qmp(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#else // Suppress all logging activity
//#error Suppressing
#define LOGV(...)  ((void)0)
#define LOGD(...) ((void)0)
#define LOGE(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGI(...) ((void)0)
#define printf(...) ((void)0)
#define fprintf(stdout, ...) ((void)0)
#define perror(...) ((void)0)
#define sdl_logerr(...) ((void)0)
#define dolog(...) ((void)0)
#define error_report(...) ((void)0)
#define error_printf(...) ((void)0)
#define error_vprintf(...) ((void)0)
#define error_printf_unless_qmp(...) ((void)0)

#endif //end DEBUG_OUTPUT

//Extra Debug (lots of data)
#ifdef DEBUG_OUTPUT_EXTRA
#error DEBUG_OUTPUT_EXTRA is turned ON!
#define LOGD_AIO(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGD_TRD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGD_CPUS(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGD_VL(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGD_TMR(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGD_MLP(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#else // Suppress all logging
#define LOGD_AIO(...) ((void)0)
#define LOGD_TRD(...) ((void)0)
#define LOGD_CPUS(...) ((void)0)
#define LOGD_VL(...) ((void)0)
#define LOGD_TMR(...) ((void)0)
#define LOGD_MLP(...) ((void)0)

#endif  //end DEBUG_OUTPUT_EXTRA

#ifdef	__cplusplus
	}
#endif

#endif	/* _LIMBO_LOGUTILS_H */

