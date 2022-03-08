#ifndef LIMBO_COMPAT_H
#define LIMBO_COMPAT_H

#include <jni.h>
#include <pthread.h>

extern JavaVM *jvm;
extern jobject jobj;
extern jclass jcls;
extern pthread_mutex_t fd_lock;
extern const char * storage_base_dir;
extern const char * limbo_base_dir;

#ifdef __cplusplus
extern "C" {
#endif
void set_jni(JNIEnv* env, jobject obj1, jclass jclass1, const char * storage_dir, const char * limbo_dir);
void * valloc (size_t size);

#ifndef __ANDROID_HAVE_STRCHRNUL__
const char* strchrnul(const char* s, int c);
#endif

#ifdef __cplusplus
}
#endif



#endif
