#include <jni.h>
#include <stdio.h>
#include "limbo_logutils.h"
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
#include <sys/stat.h>
#include "limbo_compat_qemu.h"
#include "limbo_compat.h"
#include <errno.h>

void set_sdl_res(int width, int height) {
	pthread_mutex_lock(&fd_lock);

	JNIEnv *env;
	jmethodID methodID;

	//LOGI("sending sdl res: %d, %d", width, height);

	if (jvm == NULL) {
		LOGE("Jvm not initialized");
		return;
	}
	jint rs = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (rs != JNI_OK) {
		LOGE("Could not get env");
		return;
	}

	jclass cls = (*env)->GetObjectClass(env, jobj);
	methodID = (*env)->GetStaticMethodID(env, cls, "onVMResolutionChanged", "(II)V");
	(*env)->CallStaticVoidMethod(env, cls, methodID, width, height);

	(*jvm)->DetachCurrentThread(jvm);
	pthread_mutex_unlock(&fd_lock);

	//LOGI("sent sdl res: %d, %d", width, height);

}


void *set_sdl_res_thread(void *t) {
	int i;
	char * filepath;
	sdl_res_t * sdl_res_data;
	sdl_res_data = (sdl_res_t *) t;

	set_sdl_res(sdl_res_data->width, sdl_res_data->height);
	pthread_exit(NULL);
}

void create_thread_set_sdl_res(int width, int height) {
	int fd = 0;
	int rc;
	int i;
	pthread_t thread;
	pthread_attr_t attr;
	void *status;

	// Initialize and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//void * param = (void *) fd;
	sdl_res_t * sdl_res_data = (struct sdl_res_t*) malloc(sizeof(struct sdl_res_t));
	sdl_res_data->width = width;
	sdl_res_data->height = height;
	void * param = (void *) sdl_res_data;
	rc = pthread_create(&thread, NULL, set_sdl_res_thread, param);
	if (rc) {
		LOGE("Could not create thread for set_sdl_res: %d", rc);
		exit(-1);
	}

	// free attribute and wait for the other threads
	pthread_attr_destroy(&attr);
	rc = pthread_join(thread, &status);
	if (rc) {
		LOGE("Could not join set_sdl_res: %d", rc);
		exit(-1);
	}

	free(sdl_res_data);
}

void Android_JNI_SetVMResolution(int width, int height)
{
	create_thread_set_sdl_res(width, height);
}
