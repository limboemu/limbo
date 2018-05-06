#ifndef LIMBO_COMPAT_QEMU_H
#define LIMBO_COMPAT_QEMU_H

#include <jni.h>

typedef struct sdl_res_t {
	int width;
	int height;
} sdl_res_t;

#ifdef __cplusplus
extern "C" {
#endif
void Android_JNI_SetVMResolution(int width, int height);
#ifdef __cplusplus
}
#endif

#endif
