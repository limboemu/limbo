/*
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 2004 Tobias Lorenz
 *
 *  string handling functions
 *  based on linux/include/linux/ctype.h
 *       and linux/include/linux/string.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

FILE_LICENCE ( GPL2_ONLY );

#ifndef ETHERBOOT_STRING_H
#define ETHERBOOT_STRING_H

#include <stddef.h>
#include <bits/string.h>

int __pure strnicmp(const char *s1, const char *s2, size_t len) __nonnull;
char * strcpy(char * dest,const char *src) __nonnull;
char * strncpy(char * dest,const char *src,size_t count) __nonnull;
char * strcat(char * dest, const char * src) __nonnull;
char * strncat(char *dest, const char *src, size_t count) __nonnull;
int __pure strcmp(const char * cs,const char * ct) __nonnull;
int __pure strncmp(const char * cs,const char * ct,
				     size_t count) __nonnull;
char * __pure strchr(const char * s, int c) __nonnull;
char * __pure strrchr(const char * s, int c) __nonnull;
size_t __pure strlen(const char * s) __nonnull;
size_t __pure strnlen(const char * s, size_t count) __nonnull;
size_t __pure strspn(const char *s, const char *accept) __nonnull;
size_t __pure strcspn(const char *s, const char *reject) __nonnull;
char * __pure strpbrk(const char * cs,const char * ct) __nonnull;
char * strtok(char * s,const char * ct) __nonnull;
char * strsep(char **s, const char *ct) __nonnull;
void * memset(void * s,int c,size_t count) __nonnull;
void * memcpy ( void *dest, const void *src, size_t len ) __nonnull;
void * memmove(void * dest,const void *src,size_t count) __nonnull;
int __pure memcmp(const void * cs,const void * ct,
				    size_t count) __nonnull;
void * __pure memscan(const void * addr, int c, size_t size) __nonnull;
char * __pure strstr(const char * s1,const char * s2) __nonnull;
void * __pure memchr(const void *s, int c, size_t n) __nonnull;
char * __malloc strdup(const char *s) __nonnull;
char * __malloc strndup(const char *s, size_t n) __nonnull;

extern const char * __pure strerror ( int errno );

#endif /* ETHERBOOT_STRING */
