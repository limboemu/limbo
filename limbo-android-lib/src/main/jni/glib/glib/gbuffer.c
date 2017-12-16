/*
 * Copyright © 2009, 2010 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gbuffer.h"

#include <glib/gstrfuncs.h>
#include <glib/gatomic.h>
#include <glib/gmem.h>


typedef struct
{
  GBuffer buffer;

  GDestroyNotify user_destroy;
  gpointer user_data;
} GUserNotifyBuffer;

static void
g_buffer_free_gfree (GBuffer *buffer)
{
  g_free ((gpointer) buffer->data);
  g_slice_free (GBuffer, buffer);
}

/* < private >
 * g_buffer_new_from_data:
 * @data: the data to be used for the buffer
 * @size: the size of @data
 * @returns: a reference to a new #GBuffer
 *
 * Creates a new #GBuffer from @data.
 *
 * @data is copied.
 */

GBuffer *
g_buffer_new_from_data (gconstpointer data,
                        gsize         size)
{
  GBuffer *buffer;

  buffer = g_slice_new (GBuffer);
  buffer->data = g_memdup (data, size);
  buffer->size = size;
  buffer->free_func = g_buffer_free_gfree;
  buffer->ref_count = 1;

  return buffer;
}

/* < private >
 * g_buffer_new_take_data:
 * @data: the data to be used for the buffer
 * @size: the size of @data
 * returns: a reference to a new #GBuffer
 *
 * Creates a new #GBuffer from @data.
 *
 * @data must have been created by a call to g_malloc(), g_malloc0() or
 * g_realloc() or by one of the many functions that wrap these calls
 * (such as g_new(), g_strdup(), etc).
 *
 * After this call, @data belongs to the buffer and may no longer be
 * modified by the caller.  g_free() will be called on @data when the
 * buffer is no longer in use.
 */
GBuffer *
g_buffer_new_take_data (gpointer data,
                        gsize    size)
{
  GBuffer *buffer;

  buffer = g_slice_new (GBuffer);
  buffer->data = data;
  buffer->size = size;
  buffer->free_func = g_buffer_free_gfree;
  buffer->ref_count = 1;

  return buffer;
}

static void
g_buffer_free (GBuffer *buffer)
{
  g_slice_free (GBuffer, buffer);
}

/* < private >
 * g_buffer_new_from_static_data:
 * @data: the data to be used for the buffer
 * @size: the size of @data
 * @returns: a reference to a new #GBuffer
 *
 * Creates a new #GBuffer from static data.
 *
 * @data must be static (ie: never modified or freed).
 */
GBuffer *
g_buffer_new_from_static_data (gconstpointer data,
                               gsize         size)
{
  GBuffer *buffer;

  buffer = g_slice_new (GBuffer);
  buffer->data = data;
  buffer->size = size;
  buffer->free_func = g_buffer_free;
  buffer->ref_count = 1;

  return buffer;
}

static void
g_buffer_free_usernotify (GBuffer *buffer)
{
  GUserNotifyBuffer *ubuffer = (GUserNotifyBuffer *) buffer;

  ubuffer->user_destroy (ubuffer->user_data);
  g_slice_free (GUserNotifyBuffer, ubuffer);
}

/* < private >
 * g_buffer_new_from_pointer:
 * @data: the data to be used for the buffer
 * @size: the size of @data
 * @notify: the function to call to release the data
 * @user_data: the data to pass to @notify
 * @returns: a reference to a new #GBuffer
 *
 * Creates a #GBuffer from @data.
 *
 * When the last reference is dropped, @notify will be called on
 * @user_data.
 *
 * @data must not be modified after this call is made, until @notify has
 * been called to indicate that the buffer is no longer in use.
 */
GBuffer *
g_buffer_new_from_pointer (gconstpointer  data,
                           gsize          size,
                           GDestroyNotify notify,
                           gpointer       user_data)
{
  GUserNotifyBuffer *ubuffer;

  ubuffer = g_slice_new (GUserNotifyBuffer);
  ubuffer->buffer.data = data;
  ubuffer->buffer.size = size;
  ubuffer->buffer.free_func = g_buffer_free_usernotify;
  ubuffer->buffer.ref_count = 1;
  ubuffer->user_destroy = notify;
  ubuffer->user_data = user_data;

  return (GBuffer *) ubuffer;
}

/* < private >
 * g_buffer_ref:
 * @buffer: a #GBuffer
 * @returns: @buffer
 *
 * Increase the reference count on @buffer.
 */
GBuffer *
g_buffer_ref (GBuffer *buffer)
{
  g_atomic_int_inc (&buffer->ref_count);

  return buffer;
}

/* < private >
 * g_buffer_unref:
 * @buffer: a #GBuffer
 *
 * Releases a reference on @buffer.  This may result in the buffer being
 * freed.
 */
void
g_buffer_unref (GBuffer *buffer)
{
  if (g_atomic_int_dec_and_test (&buffer->ref_count))
    if (buffer->free_func != NULL)
      buffer->free_func (buffer);
}
