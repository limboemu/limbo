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

#ifndef __G_BUFFER_H__
#define __G_BUFFER_H__

#include <glib/gtypes.h>

/* < private >
 * GBuffer:
 * @data: a pointer to the data held in the buffer
 * @size: the size of @data
 *
 * A simple refcounted data type representing a byte sequence from an
 * unspecified origin.
 *
 * The purpose of a #GBuffer is to keep the memory region that it holds
 * alive for as long as anyone holds a reference to the buffer.  When
 * the last reference count is dropped, the memory is released.
 *
 * A #GBuffer can come from many different origins that may have
 * different procedures for freeing the memory region.  Examples are
 * memory from g_malloc(), from memory slices, from a #GMappedFile or
 * memory from other allocators.
 */
typedef struct _GBuffer GBuffer;

/* < private >
 * GBufferFreeFunc:
 * @buffer: the #GBuffer to be freed
 *
 * This function is provided by creators of a #GBuffer.  It is the
 * function to be called when the reference count of @buffer drops to
 * zero.  It should free any memory associated with the buffer and free
 * @buffer itself.
 */
typedef void (* GBufferFreeFunc)                (GBuffer        *buffer);

struct _GBuffer
{
  gconstpointer data;
  gsize size;

  /*< protected >*/
  GBufferFreeFunc free_func;

  /*< private >*/
  gint ref_count;
};

G_GNUC_INTERNAL
GBuffer *       g_buffer_new_from_data          (gconstpointer   data,
                                                 gsize           size);
G_GNUC_INTERNAL
GBuffer *       g_buffer_new_take_data          (gpointer        data,
                                                 gsize           size);
G_GNUC_INTERNAL
GBuffer *       g_buffer_new_from_static_data   (gconstpointer   data,
                                                 gsize           size);
G_GNUC_INTERNAL
GBuffer *       g_buffer_new_from_pointer       (gconstpointer   data,
                                                 gsize           size,
                                                 GDestroyNotify  notify,
                                                 gpointer        user_data);
G_GNUC_INTERNAL
GBuffer *     g_buffer_ref                      (GBuffer        *buffer);
G_GNUC_INTERNAL
void          g_buffer_unref                    (GBuffer        *buffer);

#endif /* __G_BUFFER_H__ */
