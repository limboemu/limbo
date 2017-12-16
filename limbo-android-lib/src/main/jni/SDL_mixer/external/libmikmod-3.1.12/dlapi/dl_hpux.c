/*==============================================================================

  $Id: dl_hpux.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  simple dlopen()-like implementation above HP-UX shl_xxx() API

  Written by Miodrag Vallat <miod@mikmod.org>, released in the public domain

==============================================================================*/

/*
 * This implementation currently only works on hppa systems.
 * It could be made working on m68k (hp9000s300) series, but my HP-UX 5.5
 * disk is dead and I don't have the system tapes...
 */

#include <dl.h>
#include <malloc.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"	/* const */
#endif

#include "dlfcn.h"

void *dlopen(const char *name, int flags)
{
	shl_t handle;
	char *library;

	if (name == NULL || *name == '\0')
		return NULL;

	/* By convention, libmikmod will look for "foo.so" while on HP-UX the
	   name would be "foo.sl". Change the last letter here. */
	library = strdup(name);
	library[strlen(library) - 1] = 'l';

	handle = shl_load(library,
		(flags & RTLD_LAZY ? BIND_DEFERRED : BIND_IMMEDIATE) | DYNAMIC_PATH,
	    0L);

	free(library);

	return (void *)handle;
}

int dlclose(void *handle)
{
	return shl_unload((shl_t)handle);
}

void *dlsym(void *handle, const char *sym)
{
	shl_t h = (shl_t)handle;
	void *address;

	if (shl_findsym(&h, sym, TYPE_UNDEFINED, &address) == 0)
			return address;
	return NULL;
}

/* ex:set ts=4: */
