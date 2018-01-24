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
#include "limbo_logutils.h"
#include "vm-executor-jni.h"
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
#include <glib.h>
#include <stdexcept>
#include "limbo_compat.h"
#include "limbo_compat_fd.h"

#define MSG_BUFSIZE 1024
#define MAX_STRING_LEN 1024

static int started = 0;
void * handle = 0;

void loadLib(const char * lib_path_str) {

	char res_msg[MAX_STRING_LEN];
	sprintf(res_msg, "Loading lib: %s", lib_path_str);
	LOGV(res_msg);
	handle = dlopen(lib_path_str, RTLD_LAZY);
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *pvt) {
	printf("* JNI_OnLoad called\n");
	return JNI_VERSION_1_2;
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_togglefullscreen(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	LOGV("Toggle Fullscreen\n");

	typedef void (*toggleFullScreen_t)();
	dlerror();
	toggleFullScreen_t toggleFullScreen = (toggleFullScreen_t) dlsym(handle,
			"toggleFullScreen");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'toggleFullScreen': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	toggleFullScreen();

	sprintf(res_msg, "Full Screen");
	LOGV(res_msg);

	return env->NewStringUTF(res_msg);
}

//FIXME: Snapshots Hanging, for now we use QMP Monitor console
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_save(
		JNIEnv* env, jobject thiz, jstring snapshot_name) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	const char *snapshot_name_str = NULL;
	if (snapshot_name != NULL)
		snapshot_name_str = env->GetStringUTFChars(snapshot_name, 0);

	typedef int (*limbo_savevm_t)(const char * limbo_snapshot_name);
	dlerror();
	limbo_savevm_t limbo_savevm = (limbo_savevm_t) dlsym(handle,
			"limbo_savevm");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'hmp_savevm': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	limbo_savevm(snapshot_name_str);
	sprintf(res_msg, "VM State Saved");
	LOGV(res_msg);

    env->ReleaseStringUTFChars(snapshot_name, snapshot_name_str);

	return env->NewStringUTF(res_msg);
}

//XXX: For saving snapshots
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getsavestate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	typedef int (*get_save_state_t)();
	dlerror();
	get_save_state_t get_save_state = (get_save_state_t) dlsym(handle,
			"get_save_state");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_save_state': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	int state = 0;
	state = get_save_state();

	if (state == 0)
		sprintf(res_msg, "NONE");
	else if (state == 1)
		sprintf(res_msg, "SAVING");
	else if (state == 2)
		sprintf(res_msg, "DONE");
	else if (state == -1)
		sprintf(res_msg, "ERROR");

	return env->NewStringUTF(res_msg);
}

//Save only VM no snapshot is faster
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_pausevm(
		JNIEnv* env, jobject thiz, jstring juri) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	char error[MSG_BUFSIZE + 1] = { 0 };
	const char * uri_str = NULL;
	if (juri != NULL)
		uri_str = env->GetStringUTFChars(juri, 0);

	typedef int (*limbo_migrate_t)(const char * uri, char * error);
	dlerror();
	limbo_migrate_t limbo_migrate = (limbo_migrate_t) dlsym(handle,
			"limbo_migrate");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		sprintf(res_msg, "Cannot load symbol 'limbo_migrate': %s\n",
				dlsym_error);
		LOGE(res_msg);
		return env->NewStringUTF(res_msg);
	}

	int res = limbo_migrate(uri_str, error);

	if (res) {
		LOGE(error);
		sprintf(res_msg, error);
	} else
		sprintf(res_msg, "VM State Saving Started");

	LOGV(res_msg);

    env->ReleaseStringUTFChars(juri, uri_str);

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_vncchangepassword(
		JNIEnv* env, jobject thiz, jstring jvnc_passwd) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	Error **err;

	const char *vnc_passwd_str = NULL;
	if (jvnc_passwd != NULL)
		vnc_passwd_str = env->GetStringUTFChars(jvnc_passwd, 0);
//
	typedef void (*vnc_change_pwd_t)(const char *password, Error **errp);
	dlerror();
	vnc_change_pwd_t qmp_change_vnc_password = (vnc_change_pwd_t) dlsym(handle,
			"qmp_change_vnc_password");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qmp_change_vnc_password': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	qmp_change_vnc_password(vnc_passwd_str, err);

	sprintf(res_msg, "VNC Password Changed");
	LOGV(res_msg);

    env->ReleaseStringUTFChars(jvnc_passwd, vnc_passwd_str);

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_changedev(
		JNIEnv* env, jobject thiz, jstring jdev, jstring jdev_value) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };
	Error **err;

	const char *dev = NULL;
	if (jdev != NULL)
		dev = env->GetStringUTFChars(jdev, 0);

	const char *dev_value = NULL;
	if (jdev_value != NULL)
		dev_value = env->GetStringUTFChars(jdev_value, 0);

	LOGI("command: change_dev: %s %s\n", dev, dev_value);

	typedef void (*qmp_change_t)(const char *device, const char *target,
	    bool has_arg, const char *arg, Error **errp);

	typedef void (*qemu_mutex_lock_iothread_t)();
	typedef void (*qemu_mutex_unlock_iothread_t)();

	dlerror();

	qmp_change_t qmp_change = (qmp_change_t) dlsym(handle, "qmp_change");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qmp_change': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

    qemu_mutex_lock_iothread_t qemu_mutex_lock_iothread
        = (qemu_mutex_lock_iothread_t) dlsym(handle, "qemu_mutex_lock_iothread");
	dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qemu_mutex_lock_iothread': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	qemu_mutex_unlock_iothread_t qemu_mutex_unlock_iothread
	    = (qemu_mutex_unlock_iothread_t) dlsym(handle, "qemu_mutex_unlock_iothread");
    dlsym_error = dlerror();
    if (dlsym_error) {
        LOGE("Cannot load symbol 'qemu_mutex_unlock_iothread': %s\n", dlsym_error);
    	return env->NewStringUTF(res_msg);
    }

    printf("Changing Device: %s to %s", dev, dev_value);
    qemu_mutex_lock_iothread();
	qmp_change(dev, dev_value, 0, NULL, err);
	qemu_mutex_unlock_iothread();

    sprintf(res_msg, "Changed Device: %s to %s", dev, dev_value);

    env->ReleaseStringUTFChars(jdev, dev);
    env->ReleaseStringUTFChars(jdev_value, dev_value);

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_ejectdev(
		JNIEnv* env, jobject thiz, jstring jdev) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	Error **err;
	const char *dev = NULL;
	if (jdev != NULL)
		dev = env->GetStringUTFChars(jdev, 0);

	typedef void (*qmp_eject_t)(bool has_device, const char *device,
	    bool has_id, const char *id, bool has_force, bool force, Error **errp);
	typedef void (*qemu_mutex_lock_iothread_t)();
    typedef void (*qemu_mutex_unlock_iothread_t)();

	dlerror();

	qmp_eject_t qmp_eject = (qmp_eject_t) dlsym(handle, "qmp_eject");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qmp_eject': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

    qemu_mutex_lock_iothread_t qemu_mutex_lock_iothread =
        (qemu_mutex_lock_iothread_t) dlsym(handle, "qemu_mutex_lock_iothread");
	dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'qemu_mutex_lock_iothread': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	qemu_mutex_unlock_iothread_t qemu_mutex_unlock_iothread
	    = (qemu_mutex_unlock_iothread_t) dlsym(handle, "qemu_mutex_unlock_iothread");
    dlsym_error = dlerror();
    if (dlsym_error) {
        LOGE("Cannot load symbol 'qemu_mutex_unlock_iothread': %s\n", dlsym_error);
    	return env->NewStringUTF(res_msg);
    }

    printf("Ejecting: %s", dev);
    qemu_mutex_lock_iothread();
	qmp_eject(1, dev, 0, NULL, 1, 1, err);
	qemu_mutex_unlock_iothread();

    sprintf(res_msg, "Ejected Device: %s", dev);

    env->ReleaseStringUTFChars(jdev, dev);

    return env->NewStringUTF(res_msg);

}

//Check state for pausing vm
JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getpausestate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	typedef int (*get_save_state_t)();
	dlerror();
	get_save_state_t get_migration_status = (get_save_state_t) dlsym(handle,
			"get_migration_status");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_migration_status': %s\n", dlsym_error);
		sprintf(res_msg, "NONE");
		return env->NewStringUTF(res_msg);
	}

	int state = 0;
	state = get_migration_status();

	if (state == 0)
		sprintf(res_msg, "NONE");
	else if (state == 1)
		sprintf(res_msg, "SAVING");
	else if (state == 2)
		sprintf(res_msg, "DONE");
	else if (state == -1)
		sprintf(res_msg, "ERROR");

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_getstate(
		JNIEnv* env, jobject thiz) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	typedef int (*get_state_t)();
	dlerror();
	get_state_t get_state = (get_state_t) dlsym(handle, "get_state");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'get_state': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	int state = 0;
	state = get_state();

	if (state)
		sprintf(res_msg, "RUNNING");
	else
		sprintf(res_msg, "READY");

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_stop(
		JNIEnv* env, jobject thiz, jint jint_restart) {
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	int restart_int = jint_restart;

	typedef void (*stop_vm_t)(int restart);
	dlerror();
	stop_vm_t stop_vm = (stop_vm_t) dlsym(handle, "stop_vm");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOGE("Cannot load symbol 'stop_vm': %s\n", dlsym_error);
		return env->NewStringUTF(res_msg);
	}

	stop_vm(!restart_int);

	if (restart_int)
		sprintf(res_msg, "VM Restart Request");
	else
		sprintf(res_msg, "VM Stop Request");
	LOGV(res_msg);

	started = restart_int;

	return env->NewStringUTF(res_msg);
}

JNIEXPORT jstring JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_start(
		JNIEnv* env, jobject thiz, jstring lib_path, jobjectArray params,
		jstring extra_params, jint paused, jstring save_state) {
	int res;
	char res_msg[MSG_BUFSIZE + 1] = { 0 };

	if (started) {
		sprintf(res_msg, "VM Already started");
		LOGV(res_msg);
		return env->NewStringUTF(res_msg);
	}

	LOGV("***** INIT LIMBO VARS *****");

	LOGV("Setting Params");
	int MAX_PARAMS = 256;
	int argc_params = 0;
	int argc = 0;
	gint argc_extra_params = 0;
	gchar **argv_extra_params;
	char ** argv;

	// XXX: install our debug handler
//	signal(SIGSEGV, print_stack_trace);
//	signal(SIGABRT, print_stack_trace);

	//extra params
	const char * extra_params_str = NULL;
	if (extra_params != NULL)
		extra_params_str = env->GetStringUTFChars(extra_params, 0);

	if (extra_params_str != NULL && strcmp(extra_params_str, "") != 0) {
		GError *error;
		gboolean extra_params_res = g_shell_parse_argv(extra_params_str,
				&argc_extra_params, &argv_extra_params, &error);
//		LOGD("Parsed args for extra_params: %d : %d", extra_params_res,
//				argc_extra_params);
	}

	//params
	argc = env->GetArrayLength(params);

	//save state
	int migration_params=0;
	const char * save_state_name_str = NULL;
	int fd_save_state=0;
	char fd[5];
	if (save_state != NULL)
		save_state_name_str = env->GetStringUTFChars(save_state, 0);
	if(paused == 1 && save_state_name_str!= NULL && strcmp(save_state_name_str, "") != 0) {
		migration_params = 2;
	}

	int argcf = argc + argc_extra_params + migration_params;
	argv = (char **) malloc((argcf + 1) * sizeof(*argv));

	for (int i = 0; i < argcf; i++) {
		argv[i] = (char *) malloc(MAX_STRING_LEN * sizeof(char));
		if (i < argc) {
			jstring string = (jstring)(env->GetObjectArrayElement(params, i));
			const char *param_str = env->GetStringUTFChars(string, 0);
//			LOGD("Copying param: %d: %s", i, param_str);
			strcpy(argv[i], param_str);
			env->ReleaseStringUTFChars(string, param_str);
		} else if (i<argc+argc_extra_params){
//			LOGD("Copying extra param: %d: %s", i, argv_extra_params[i - argc]);
			strcpy(argv[i], argv_extra_params[i - argc]);
		}

//        env->ReleaseStringUTFChars(string, param_str);
	}

	if (paused == 1 && save_state_name_str != NULL && strcmp(save_state_name_str, "") != 0) {
//		LOGV("Loading VM State: %s", save_state_name_str);
		int fd_tmp = open(save_state_name_str, O_RDWR | O_CLOEXEC);
		if (fd_tmp < 0) {
			LOGE("Error while getting fd for: %s", save_state_name_str);
		} else {
			LOGI("Got new fd %d for: %s", fd_tmp, save_state_name_str);
			fd_save_state = fd_tmp;
		}
		argv[argc+argc_extra_params] = (char *) malloc(MAX_STRING_LEN * sizeof(char));
		strcpy(argv[argc+argc_extra_params], "-incoming");
		sprintf(fd, "%d", fd_save_state);
		argv[argc+argc_extra_params+1] = (char *) malloc(MAX_STRING_LEN * sizeof(char));
		strcpy(argv[argc+argc_extra_params+1], "fd:");
		strcat(argv[argc+argc_extra_params+1], fd);

	}

	//XXX: Do not remove
	argv[argcf] = NULL;

	for (int k = 0; k < argcf; k++) {
		LOGV("QEMU param[%d]=%s", k, argv[k]);
	}

	LOGV("***** INIT QEMU *****");
	started = 1;
	LOGV("Starting VM...");

//LOAD LIB
	const char *lib_path_str = NULL;
	if (lib_path != NULL)
		lib_path_str = env->GetStringUTFChars(lib_path, 0);

#ifndef __LP64__
	if (handle == NULL) {
		loadLib(lib_path_str);
	}

	if (!handle) {
		sprintf(res_msg, "Error opening lib: %s :%s", lib_path_str, dlerror());
		LOGV(res_msg);
		return env->NewStringUTF(res_msg);
	}

#endif

	setup_jni(env, thiz);

	LOGV("Loading symbol qemu_start...\n");
	typedef void (*qemu_start_t)(int argc, char **argv, char **envp);

	// reset errors
	dlerror();
	qemu_start_t qemu_start = (qemu_start_t) dlsym(handle, "qemu_start");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
	    sprintf(res_msg, dlsym_error);
		LOGE("Cannot load symbol 'qemu_start': %s\n", dlsym_error);
		dlclose(handle);
		handle = NULL;
		return env->NewStringUTF(res_msg);
	}

	try {
		qemu_start(argcf, argv, NULL);
	} catch (std::exception& e) {
		LOGV("Exception: %s", e.what());
		jclass c = env->FindClass("java/lang/Exception");
		env->ThrowNew(c, e.what());
	}

	//UNLOAD LIB
	sprintf(res_msg, "Closing lib: %s", lib_path_str);
	LOGV(res_msg);
	dlclose(handle);
	handle = NULL;

    env->ReleaseStringUTFChars(lib_path, lib_path_str);
    if(save_state!=NULL && save_state_name_str != NULL)
        env->ReleaseStringUTFChars(save_state, save_state_name_str);
    if(extra_params!=NULL && extra_params_str != NULL)
        env->ReleaseStringUTFChars(extra_params, extra_params_str);


	sprintf(res_msg, "VM shutdown");
	LOGV(res_msg);
    return env->NewStringUTF(res_msg);
	//exit(1);
}
}

void setup_jni(JNIEnv* env, jobject thiz) {
	typedef void (*set_jni_t)(JNIEnv* env, jobject obj1, jclass jclass1);
	dlerror();
	set_jni_t set_jni = (set_jni_t) dlsym(handle, "set_jni");
	const char *dlsym_error3 = dlerror();
	if (dlsym_error3) {
		LOGE("Cannot load symbol 'set_jni': %s\n", dlsym_error3);
		exit(-1);
	}
	jclass c = env->GetObjectClass(thiz);
	set_jni(env, thiz, c);
}

