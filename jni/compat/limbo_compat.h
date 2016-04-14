#ifndef LIMBO_COMPAT_H
#define LIMBO_COMPAT_H

#include <jni.h>

extern JavaVM *jvm;
extern jobject jobj;

extern void set_jni(JNIEnv* env, jobject obj1);

#endif
