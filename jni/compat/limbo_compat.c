#include <jni.h>
#include "limbo_logutils.h"
#include "limbo_compat.h"

JavaVM *jvm = NULL;
jobject jobj = NULL;

void set_jni(JNIEnv* env, jobject obj1) {
	jint rs = (*env)->GetJavaVM(env, &jvm);
	jobj = (*env)->NewGlobalRef(env, obj1);
}


