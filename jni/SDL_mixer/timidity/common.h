/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

#include <limits.h>

#ifndef PATH_MAX   /* GNU Hurd doesn't limit path size, thus no PATH_MAX... */
#define PATH_MAX 1024   /* ...so we'll just impose an arbitrary limit. */
#endif

extern char *program_name, current_filename[];

extern FILE *msgfp;

extern int num_ochannels;

#define MULTICHANNEL_OUT
#define MAX_OUT_CHANNELS 6

typedef struct {
  char *path;
  void *next;
} PathList;

/* Noise modes for open_file */
#define OF_SILENT	0
#define OF_NORMAL	1
#define OF_VERBOSE	2

extern FILE *open_file(const char *name, int decompress, int noise_mode);
extern void add_to_pathlist(const char *s);
extern void free_pathlist(void);
extern void close_file(FILE *fp);
extern void skip(FILE *fp, size_t len);
extern void *safe_malloc(size_t count);
