#include <jni.h>
#include "limbo_logutils.h"
#include "limbo_compat.h"

JavaVM *jvm = NULL;
jobject jobj = NULL;
jclass jcls = NULL;
pthread_mutex_t fd_lock;

void set_jni(JNIEnv* env, jobject obj1, jclass jclass1) {
	if (pthread_mutex_init(&fd_lock, NULL) != 0) {
		LOGE("JNI Mutex init failed");
		return;
	}
	jint rs = (*env)->GetJavaVM(env, &jvm);
	jobj = (*env)->NewGlobalRef(env, obj1);
	jcls = (jclass) (*env)->NewGlobalRef(env, jclass1);
}


