
#include <jni.h>
#include <unistd.h>
#include "limbo_logutils.h"
#include "limbo_compat.h"

JavaVM *jvm = NULL;
jobject jobj = NULL;
jclass jcls = NULL;
pthread_mutex_t fd_lock;
const char * storage_base_dir;
const char * limbo_base_dir;

void set_jni(JNIEnv* env, jobject obj1, jclass jclass1,
    const char * storage_dir, const char * base_dir) {
	if (pthread_mutex_init(&fd_lock, NULL) != 0) {
		LOGE("JNI Mutex init failed");
		return;
	}
	jint rs = (*env)->GetJavaVM(env, &jvm);
	jobj = (*env)->NewGlobalRef(env, obj1);
	jcls = (jclass) (*env)->NewGlobalRef(env, jclass1);
	limbo_base_dir = base_dir;
	storage_base_dir = storage_dir;
}

void *
valloc (size_t size)
{
  return memalign (getpagesize (), size);
}

const char* strchrnul(const char* s, int c) {
    char *str = strchr(s, c);
    if(str == NULL) {
        int length = strlen(s);
        int endofs = s + length;
        return endofs;
    }
    return str;
}



