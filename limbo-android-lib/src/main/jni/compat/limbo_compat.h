#ifndef LIMBO_COMPAT_H
#define LIMBO_COMPAT_H

#include <jni.h>
#include <pthread.h>

extern JavaVM *jvm;
extern jobject jobj;
extern jclass jcls;
extern pthread_mutex_t fd_lock;

extern void set_jni(JNIEnv* env, jobject obj1, jclass jclass1);

#endif
