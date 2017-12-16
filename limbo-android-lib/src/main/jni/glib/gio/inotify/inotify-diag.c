/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 8 -*- */

/* inotify-helper.c - Gnome VFS Monitor based on inotify.

   Copyright (C) 2005 John McCutchan

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: 
		 John McCutchan <john@johnmccutchan.com>
*/

#include "config.h"
#include <glib.h>
#include <sys/types.h>
#include <unistd.h>
#include "inotify-missing.h"
#include "inotify-path.h"
#include "inotify-diag.h"

#define DIAG_DUMP_TIME 20000 /* 20 seconds */

G_LOCK_EXTERN (inotify_lock);

static gboolean
id_dump (gpointer userdata)
{
  GIOChannel *ioc;
  pid_t pid;
  char *fname;
  G_LOCK (inotify_lock);
  ioc = NULL;
  pid = getpid ();

  fname = g_strdup_printf ("/tmp/gvfsid.%d", pid);
  ioc = g_io_channel_new_file (fname, "w", NULL);
  g_free (fname);
  
  if (!ioc)
    {
      G_UNLOCK (inotify_lock);
      return TRUE;
    }

  _im_diag_dump (ioc);
  
  g_io_channel_shutdown (ioc, TRUE, NULL);
  g_io_channel_unref (ioc);
  
  G_UNLOCK (inotify_lock);
  return TRUE;
}

void
_id_startup (void)
{
  if (!g_getenv ("GVFS_INOTIFY_DIAG"))
    return;
	
  g_timeout_add (DIAG_DUMP_TIME, id_dump, NULL);
}
