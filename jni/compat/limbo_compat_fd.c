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
#include "limbo_compat_fd.h"
#include "limbo_compat.h"


int close_fd(int fd) {

	pthread_mutex_lock(&fd_lock);

	JNIEnv *env;
	jmethodID methodID;
	jint jfd, jres;
	int res = -1;

	if (jvm == NULL) {
		LOGE("Jvm not initialized");
		return -1;
	}
	jint rs = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (rs != JNI_OK) {
		LOGE("Could not get env");
		return -1;
	}

	jclass cls = (*env)->GetObjectClass(env, jobj);
	methodID = (*env)->GetMethodID(env, cls, "close_fd", "(I)I");
	jres = (jint)(*env)->CallIntMethod(env, jobj, methodID, fd);

	res = (int) jres;

	(*jvm)->DetachCurrentThread(jvm);
	pthread_mutex_unlock(&fd_lock);
	return res;

}
int get_fd(const char * filepath) {
	pthread_mutex_lock(&fd_lock);
	int fd = -1;

	JNIEnv *env;
	jmethodID methodID;
	jint jfd;

	LOGI("get_fd: %s", filepath);

	if (jvm == NULL) {
		LOGE("Jvm not initialized");
		return -1;
	}
	jint rs = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (rs != JNI_OK) {
		LOGE("Could not get env");
		return -1;
	}
	jstring jfilepath = (*env)->NewStringUTF(env, filepath);

	jclass cls = (*env)->GetObjectClass(env, jobj);
	methodID = (*env)->GetMethodID(env, cls, "get_fd", "(Ljava/lang/String;)I");
	jfd = (jint)(*env)->CallIntMethod(env, jobj, methodID, jfilepath);

	fd = (int) jfd;

	(*env)->DeleteLocalRef(env, jfilepath);

	(*jvm)->DetachCurrentThread(jvm);
	pthread_mutex_unlock(&fd_lock);
	LOGI("Done get_fd: %d, %s", fd, filepath);
	return fd;
}

int android_open(const char *path, int flags) {
	int fd;
	fd = create_thread_get_fd(path);
	//fd = get_fd(path);
	return fd;
}
int android_openm(const char *path, int flags, mode_t mode) {
	return android_open(path, flags);
}
//

int android_close(int fd) {
	int res = -1;
	//First we try via android to close
	//XXX: spawn a thread and wait till finishes so the ART JNI will be happy
	res = create_thread_close_fd(fd);

	//And we try Natively also
	res = close(fd);
	return res;
}

void *close_fd_thread(void *t) {
	int i;
	int fd;
	fd_t * fd_data;
	fd_data = (fd_t *) t;

	fd_data->res = close_fd(fd_data->fd);

	pthread_exit(NULL);
}

int create_thread_close_fd(int fd) {
	int res = 0;
	int rc;
	int i;
	pthread_t thread;
	pthread_attr_t attr;
	void *status;

	// Initialize and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//void * param = (void *) fd;
	fd_t * fd_data = (struct fd_t*) malloc(sizeof(struct fd_t));
	fd_data->fd = fd;
	void * param = (void *) fd_data;
	rc = pthread_create(&thread, NULL, close_fd_thread, param);
	if (rc) {
		LOGE("Error:unable to create thread for close fd: %d, %d", fd, rc);
		exit(-1);
	}

	// free attribute and wait for the other threads
	pthread_attr_destroy(&attr);
	rc = pthread_join(thread, &status);
	if (rc) {
		LOGE("Error:unable to join: %d, %d", fd, rc);
		exit(-1);
	}

	res = fd_data->res;
	free(fd_data);
	return res;

}

void *get_fd_thread(void *t) {
	int i;
	char * filepath;
	fd_t * fd_data;
	fd_data = (fd_t *) t;

	fd_data->fd = get_fd(fd_data->filepath);
	pthread_exit(NULL);
}

int create_thread_get_fd(const char * filepath) {
	int fd = 0;
	int rc;
	int i;
	pthread_t thread;
	pthread_attr_t attr;
	void *status;

	LOGI("get fd Filepath: %s", filepath);
	// Initialize and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//void * param = (void *) fd;
	fd_t * fd_data = (struct fd_t*) malloc(sizeof(struct fd_t));
	fd_data->filepath = filepath;
	void * param = (void *) fd_data;
	rc = pthread_create(&thread, NULL, get_fd_thread, param);
	if (rc) {
		LOGE("Error:unable to create thread for close fd: %d, %d", fd, rc);
		exit(-1);
	}

	// free attribute and wait for the other threads
	pthread_attr_destroy(&attr);
	rc = pthread_join(thread, &status);
	if (rc) {
		LOGE("Error:unable to join: %s, %d, %d", filepath, fd, rc);
		exit(-1);
	}

	fd = fd_data->fd;
	free(fd_data);
	return fd;
}
