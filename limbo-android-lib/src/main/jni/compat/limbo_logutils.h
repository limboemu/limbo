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

//Reroute print* logging for Android
// Define this to get stdout stderr messages on the adb logcat
#define DEBUG_OUTPUT 1

//Truncate long source filepaths
#define DEBUG_SHOW_BASENAME 1

//Optional: Define ENABLE_OVERRIDE_QEMU_LOG to provide extra information on stdout stderr like the file and line number
// that the errors are coming from otherwise you're only getting the messages/logs in logcat.
// To enable this you need to comment out the macros and implementations in
//  qemu/error-report.h, util/qemu-error.c, qemu/audio/sdlaudio.c, stubs/error-printf.c
//#define ENABLE_OVERRIDE_QEMU_LOG 1

#ifdef DEBUG_SHOW_BASENAME
#define __FILENAME1__ strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#else
#define __FILENAME1__ __FILE__
#endif

#define STR1(x) #x
#define STR(x) STR1(x)
#define TAG __FILENAME1__ ":" STR(__LINE__)

#ifdef DEBUG_OUTPUT

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG ,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)

//fprintf and vfprintf should be rerouted for stdout and stderr only
static inline int limbo_fprintf(FILE *stream, const char *format, ...){
    int res = 0;
    va_list ap;
    va_start(ap, format);
    if(stream == stderr) res = __android_log_vprint(ANDROID_LOG_ERROR, TAG, format, ap);
    else if(stream == stdout) res = __android_log_vprint(ANDROID_LOG_DEBUG, TAG,format, ap);
    else res = vfprintf(stream, format, ap);
    va_end(ap);
    return res;
}
#define fprintf(...) limbo_fprintf(__VA_ARGS__)

static inline int limbo_vfprintf(FILE *stream, const char *format, va_list ap){
    int res = 0;
    if(stream == stderr) res = __android_log_vprint(ANDROID_LOG_ERROR, TAG, format, ap);
    else if(stream == stdout) res = __android_log_vprint(ANDROID_LOG_DEBUG, TAG,format, ap);
    else res = vfprintf(stream, format, ap);
    return res;
}
#define vfprintf(...) limbo_vfprintf(__VA_ARGS__)


//Generic
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define vprintf(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define perror(x) __android_log_print(ANDROID_LOG_ERROR, TAG, x)

//QEMU Logging rerouting
#ifdef ENABLE_OVERRIDE_QEMU_LOG
#define sdl_logerr(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_report(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_printf(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_vprintf(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define error_vreport(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define warn_vreport(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define info_vreport(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define warn_report(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define info_report(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#endif

#else

// Suppress all logging activity
#define LOGV(...)  ((void)0)
#define LOGD(...) ((void)0)
#define LOGE(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGI(...) ((void)0)

#ifdef ENABLE_OVERRIDE_QEMU_LOG
#define sdl_logerr(...) ((void)0)
#define error_report(...) ((void)0)
#define error_printf(...) ((void)0)
#define error_vprintf(...) ((void)0)
#define error_vreport(...) ((void)0)
#define warn_vreport(...) ((void)0)
#define info_vreport(...) ((void)0)
#define warn_report(...) ((void)0)
#define info_report(...) ((void)0)
#endif

#endif //end DEBUG_OUTPUT

#ifdef	__cplusplus
	}
#endif

#endif	/* _LIMBO_LOGUTILS_H */

