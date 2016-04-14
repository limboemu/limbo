#ifndef LIMBO_COMPAT_FD_H
#define LIMBO_COMPAT_FD_H

#include <jni.h>
#include "string.h"

int get_fd(char * filepath);

FILE *android_fopen(const char *path, const char *mode);
//int android_open(const char *path, int flags, mode_t mode);

#endif
