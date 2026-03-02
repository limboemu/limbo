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
#include <jni.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <unwind.h>
#include <dlfcn.h>
#include "vm-executor-jni.h"
#include "limbo_compat.h"

#define MSG_BUFSIZE 1024
#define MAX_STRING_LEN 1024

static int started = 0;
void * handle = 0;

void * loadLib(const char* lib_filename, const char * lib_path_str) {

	char res_msg[MAX_STRING_LEN];
	sprintf(res_msg, "Loading lib: %s", lib_path_str);
	LOGV("%s", res_msg);
	void *ldhandle = dlopen(lib_filename, RTLD_LAZY);
    if(ldhandle == NULL) {
        // try with the lib path
        printf("trying loading with full path: %s\n", lib_path_str);
        ldhandle = dlopen(lib_path_str, RTLD_LAZY);
    }
	return ldhandle;

}

void setup_jni(JNIEnv* env, jobject thiz, jstring storage_dir, jstring base_dir) {

    const char *base_dir_str = NULL;
    const char *storage_dir_str = NULL;

    if (base_dir != NULL)
		base_dir_str = (*env)->GetStringUTFChars(env, base_dir, 0);

    if (storage_dir != NULL)
		storage_dir_str = (*env)->GetStringUTFChars(env, storage_dir, 0);

	jclass c = (*env)->GetObjectClass(env, thiz);
	set_jni(env, thiz, c, storage_dir_str, base_dir_str);
}

int get_qemu_var(JNIEnv* env, jobject thiz, const char * var) {
    char res_msg[MSG_BUFSIZE + 1] = { 0 };
    
	dlerror();
    void * obj = dlsym (handle, var);
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        LOGE("Cannot load symbol %s: %s\n", var, dlsym_error);
    	return -1;
    }
    int * var_ptr = (int *) obj;
    return *var_ptr;
}

void set_qemu_var(JNIEnv* env, jobject thiz, const char * var, jint jvalue){
	int value_int = (jint) jvalue;

	dlerror();
    void * obj = dlsym (handle, var);
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        LOGE("Cannot load symbol %s: %s\n", var, dlsym_error);
    	return;
    }
    int * var_ptr = (int *) obj;
    *var_ptr = value_int;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *pvt) {
	return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeRefreshScreen(
                JNIEnv* env, jobject thiz, jint jvalue) {
    if(handle == NULL) {
    	return;
    }
    set_qemu_var(env, thiz, "limbo_vga_full_update", jvalue);
}

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setvncrefreshrate(
		JNIEnv* env, jobject thiz, jint jvalue) {
    set_qemu_var(env, thiz, "vnc_refresh_interval_inc", jvalue);
    set_qemu_var(env, thiz, "vnc_refresh_interval_base", jvalue);
}


JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setSDLRefreshRateDefault(
		JNIEnv* env, jobject thiz, jint jvalue) {
    set_qemu_var(env, thiz, "gui_refresh_interval_default", jvalue);
}

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_setSDLRefreshRateIdle(
		JNIEnv* env, jobject thiz, jint jvalue) {
            printf("setting sdl refresh rate idle: %d\n", jvalue);
    set_qemu_var(env, thiz, "gui_refresh_interval_idle", jvalue);
}


JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getSDLRefreshRateDefault(
		JNIEnv* env, jobject thiz) {
    
    int res = get_qemu_var(env, thiz, "gui_refresh_interval_default");
    return res;
}

JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getSDLRefreshRateIdle(
		JNIEnv* env, jobject thiz) {

    int res = get_qemu_var(env, thiz, "gui_refresh_interval_idle");
    printf("getting sdl refresh rate idle: %d\n", res);
    return res;
}



JNIEXPORT jint JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getvncrefreshrate(
		JNIEnv* env, jobject thiz) {

    int res = get_qemu_var(env, thiz, "vnc_refresh_interval_inc");
    return res;
}

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeIgnoreBreakpointInvalidate(
		JNIEnv* env, jobject thiz, jint jvalue) {
    if(handle == NULL) {
    	return;
    }
    set_qemu_var(env, thiz, "limbo_ignore_breakpoint_invalidate", jvalue);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_start(
        JNIEnv* env, jobject thiz,
		jstring storage_dir, jstring base_dir,
		jstring lib_filename, jstring lib_path,
		jint sdl_scale_hint,
		jobjectArray params) {
	int res;
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (started) {
		sprintf(res_msg, "VM Already started");
		LOGV("%s", res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}

	LOGV("Processing params");

	int MAX_PARAMS = 256;
	int argc = 0;
	char ** argv;

	argc = (*env)->GetArrayLength(env, params);

	argv = (char **) malloc((argc + 1) * sizeof(*argv));

	for (int i = 0; i < argc; i++) {
        jstring string = (jstring)((*env)->GetObjectArrayElement(env, params, i));
		const char *param_str = (*env)->GetStringUTFChars(env, string, 0);
		int length = strlen(param_str)+1;
        argv[i] = (char *) malloc(length * sizeof(char));
		strcpy(argv[i], param_str);
		(*env)->ReleaseStringUTFChars(env, string, param_str);
	}

	//XXX: Do not remove
	argv[argc] = NULL;

	printf("Starting VM");
    started = 1;

    //LOAD LIB
	const char *lib_filename_str = NULL;
	if (lib_filename!= NULL)
		lib_filename_str = (*env)->GetStringUTFChars(env, lib_filename, 0);
    const char *lib_path_str = NULL;
    if (lib_path != NULL)
        lib_path_str = (*env)->GetStringUTFChars(env, lib_path, 0);

	if (handle == NULL) {
		handle = loadLib(lib_filename_str, lib_path_str);
	}

	if (!handle) {
		sprintf(res_msg, "Error opening lib: %s :%s", lib_path_str, dlerror());
		LOGV("%s", res_msg);
		return (*env)->NewStringUTF(env, res_msg);
	}

	setup_jni(env, thiz, storage_dir, base_dir);
    set_qemu_var(env, thiz, "limbo_sdl_scale_hint", sdl_scale_hint);

	typedef void (*main_t)(int argc, char **argv, char **envp);
    typedef void (*qemu_main_loop_t)();
	typedef void (*qemu_cleanup_t)();

    qemu_main_loop_t qemu_main_loop = NULL;
    qemu_cleanup_t qemu_cleanup = NULL;

	dlerror();
	main_t qemu_main = (main_t) dlsym(handle, "qemu_init");
	const char *dlsym_error = dlerror();
	if (dlsym_error) { //older versions of qemu
		LOGE("Cannot find qemu symbol 'qemu_init' trying 'main': %s\n", dlsym_error);
	    qemu_main = (main_t) dlsym(handle, "main");
	    dlsym_error = dlerror();
	    if (dlsym_error) {
        	LOGE("Cannot find qemu symbol 'qemu_init' or 'main': %s\n", dlsym_error);
        	dlclose(handle);
        	handle = NULL;
        	return (*env)->NewStringUTF(env, dlsym_error);
        }
        qemu_main(argc, argv, NULL);
    } else { // new versions of qemu
        qemu_main_loop = (qemu_main_loop_t) dlsym(handle, "qemu_main_loop");
	    dlsym_error = dlerror();
	    if (dlsym_error) {
        	LOGE("Cannot find qemu symbol 'qemu_main_loop': %s\n", dlsym_error);
        	dlclose(handle);
        	handle = NULL;
        	return (*env)->NewStringUTF(env, dlsym_error);
        }

        qemu_cleanup = (qemu_cleanup_t) dlsym(handle, "qemu_cleanup");
	    dlsym_error = dlerror();
	    if (dlsym_error) {
        	LOGE("Cannot find qemu symbol 'qemu_cleanup': %s\n", dlsym_error);
        	dlclose(handle);
        	handle = NULL;
        	return (*env)->NewStringUTF(env, dlsym_error);
        }

        qemu_main(argc, argv, NULL);
        qemu_main_loop();
        qemu_cleanup();
	}

	sprintf(res_msg, "Closing lib: %s", lib_path_str);
	LOGV("%s", res_msg);
	dlclose(handle);
	handle = NULL;
	started = 0;

    (*env)->ReleaseStringUTFChars(env, lib_path, lib_path_str);

	sprintf(res_msg, "VM shutdown");
	LOGV("%s", res_msg);
    return (*env)->NewStringUTF(env, res_msg);
}


JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_stop(
		JNIEnv* env, jobject thiz, jint jint_restart) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	int restart_int = jint_restart;

    if(restart_int) {
        typedef void (*reset_vm_t)(int);
        dlerror();
        reset_vm_t qemu_system_reset_request = (reset_vm_t) dlsym(handle, "qemu_system_reset_request");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            LOGE("Cannot load symbol 'qemu_system_reset_request': %s\n", dlsym_error);
            return (*env)->NewStringUTF(env, res_msg);
        }
        qemu_system_reset_request(6); //SHUTDOWN_CAUSE_GUEST_RESET
        sprintf(res_msg, "VM Restart Request");
    } else {
        typedef void (*stop_vm_t)(int);
        dlerror();
        stop_vm_t qemu_system_shutdown_request = (stop_vm_t) dlsym(handle, "qemu_system_shutdown_request");
        const char *dlsym_error = dlerror();
        if (dlsym_error) {
            LOGE("Cannot load symbol 'qemu_system_shutdown_request': %s\n", dlsym_error);
            return (*env)->NewStringUTF(env, res_msg);
        }
        qemu_system_shutdown_request(3); //SHUTDOWN_CAUSE_HOST_SIGNAL
        sprintf(res_msg, "VM Stop Request");
	}

	LOGV("%s", res_msg);

	started = restart_int;

	return (*env)->NewStringUTF(env, res_msg);
}

// JNI End

