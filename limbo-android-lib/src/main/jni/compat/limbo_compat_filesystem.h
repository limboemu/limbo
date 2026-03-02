
#ifndef LIMBO_COMPAT_FD_H
#define LIMBO_COMPAT_FD_H

#include <jni.h>
#include <sys/stat.h>
#include "string.h"

#define ENABLE_ASF 1

#define F_TLOCK 2

typedef struct fd_t {
	int fd;
	const char * filepath;
	int res;
} fd_t;

#ifdef ENABLE_ASF

#include <pthread.h>
int get_fd(const char * filepath);
int close_fd(int fd);
void *close_fd_thread(void *t);
int create_thread_close_fd(int fd);
void *get_fd_thread(void *t);
int create_thread_get_fd(const char * filepath);
#endif

FILE* android_fopen(const char *path, const char * mode);
int android_open(const char *path, int flags, ...);
//int android_close(int fd);
int android_stat(const char*, struct stat*);
int android_mkstemp(char * path);
int lockf(int fd, int cmd, off_t len);

#endif
