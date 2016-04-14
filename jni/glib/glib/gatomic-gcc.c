/* GLIB - Library of useful routines for C programming
 * gatomic-gcc.c: atomic operations using GCC builtins.
 * Copyright (C) 2009 Hiroyuki Ikezoe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gatomic.h"

gint
g_atomic_int_exchange_and_add (volatile gint G_GNUC_MAY_ALIAS *atomic,
			       gint           val)
{
  return __sync_fetch_and_add (atomic, val);
}

void
g_atomic_int_add (volatile gint G_GNUC_MAY_ALIAS *atomic,
		  gint val)
{
  __sync_fetch_and_add (atomic, val);
}

gboolean
g_atomic_int_compare_and_exchange (volatile gint G_GNUC_MAY_ALIAS *atomic,
				   gint           oldval,
				   gint           newval)
{
  return __sync_bool_compare_and_swap (atomic, oldval, newval);
}

gboolean
g_atomic_pointer_compare_and_exchange (volatile gpointer G_GNUC_MAY_ALIAS *atomic,
				       gpointer           oldval,
				       gpointer           newval)
{
  return __sync_bool_compare_and_swap (atomic, oldval, newval);
}

void
_g_atomic_thread_init (void)
{
}

gint
(g_atomic_int_get) (volatile gint G_GNUC_MAY_ALIAS *atomic)
{
  __sync_synchronize ();
  return *atomic;
}

void
(g_atomic_int_set) (volatile gint G_GNUC_MAY_ALIAS *atomic,
		    gint           newval)
{
  *atomic = newval;
  __sync_synchronize ();
}

gpointer
(g_atomic_pointer_get) (volatile gpointer G_GNUC_MAY_ALIAS *atomic)
{
  __sync_synchronize ();
  return *atomic;
}

void
(g_atomic_pointer_set) (volatile gpointer G_GNUC_MAY_ALIAS *atomic,
			gpointer           newval)
{
  *atomic = newval;
  __sync_synchronize ();
}
