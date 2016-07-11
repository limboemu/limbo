#ifndef LIMBO_COMPAT_FD_H
#define LIMBO_COMPAT_FD_H

#include <jni.h>
#include "string.h"

int get_fd(const char * filepath);
int android_openm(const char *path, int flags, mode_t mode);
int android_open(const char *path, int flags);
int android_close(int fd);

#endif
