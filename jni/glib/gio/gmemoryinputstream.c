/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Christian Kellner <gicmo@gnome.org> 
 */

#include "config.h"
#include "gmemoryinputstream.h"
#include "ginputstream.h"
#include "gseekable.h"
#include "string.h"
#include "gsimpleasyncresult.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gmemoryinputstream
 * @short_description: Streaming input operations on memory chunks
 * @include: gio/gio.h
 * @see_also: #GMemoryOutputStream
 *
 * #GMemoryInputStream is a class for using arbitrary
 * memory chunks as input for GIO streaming input operations.
 *
 */

typedef struct _Chunk Chunk;

struct _Chunk {
  guint8         *data;
  gsize           len;
  GDestroyNotify  destroy;
};

struct _GMemoryInputStreamPrivate {
  GSList *chunks;
  gsize   len;
  gsize   pos;
};

static gssize   g_memory_input_stream_read         (GInputStream         *stream,
						    void                 *buffer,
						    gsize                 count,
						    GCancellable         *cancellable,
						    GError              **error);
static gssize   g_memory_input_stream_skip         (GInputStream         *stream,
						    gsize                 count,
						    GCancellable         *cancellable,
						    GError              **error);
static gboolean g_memory_input_stream_close        (GInputStream         *stream,
						    GCancellable         *cancellable,
						    GError              **error);
static void     g_memory_input_stream_read_async   (GInputStream         *stream,
						    void                 *buffer,
						    gsize                 count,
						    int                   io_priority,
						    GCancellable         *cancellable,
						    GAsyncReadyCallback   callback,
						    gpointer              user_data);
static gssize   g_memory_input_stream_read_finish  (GInputStream         *stream,
						    GAsyncResult         *result,
						    GError              **error);
static void     g_memory_input_stream_skip_async   (GInputStream         *stream,
						    gsize                 count,
						    int                   io_priority,
						    GCancellable         *cancellabl,
						    GAsyncReadyCallback   callback,
						    gpointer              datae);
static gssize   g_memory_input_stream_skip_finish  (GInputStream         *stream,
						    GAsyncResult         *result,
						    GError              **error);
static void     g_memory_input_stream_close_async  (GInputStream         *stream,
						    int                   io_priority,
						    GCancellable         *cancellabl,
						    GAsyncReadyCallback   callback,
						    gpointer              data);
static gboolean g_memory_input_stream_close_finish (GInputStream         *stream,
						    GAsyncResult         *result,
						    GError              **error);

static void     g_memory_input_stream_seekable_iface_init (GSeekableIface  *iface);
static goffset  g_memory_input_stream_tell                (GSeekable       *seekable);
static gboolean g_memory_input_stream_can_seek            (GSeekable       *seekable);
static gboolean g_memory_input_stream_seek                (GSeekable       *seekable,
                                                           goffset          offset,
                                                           GSeekType        type,
                                                           GCancellable    *cancellable,
                                                           GError         **error);
static gboolean g_memory_input_stream_can_truncate        (GSeekable       *seekable);
static gboolean g_memory_input_stream_truncate            (GSeekable       *seekable,
                                                           goffset          offset,
                                                           GCancellable    *cancellable,
                                                           GError         **error);
static void     g_memory_input_stream_finalize            (GObject         *object);

G_DEFINE_TYPE_WITH_CODE (GMemoryInputStream, g_memory_input_stream, G_TYPE_INPUT_STREAM,
                         G_IMPLEMENT_INTERFACE (G_TYPE_SEEKABLE,
                                                g_memory_input_stream_seekable_iface_init))


static void
g_memory_input_stream_class_init (GMemoryInputStreamClass *klass)
{
  GObjectClass *object_class;
  GInputStreamClass *istream_class;

  g_type_class_add_private (klass, sizeof (GMemoryInputStreamPrivate));

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize     = g_memory_input_stream_finalize;
  
  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn  = g_memory_input_stream_read;
  istream_class->skip  = g_memory_input_stream_skip;
  istream_class->close_fn = g_memory_input_stream_close;

  istream_class->read_async  = g_memory_input_stream_read_async;
  istream_class->read_finish  = g_memory_input_stream_read_finish;
  istream_class->skip_async  = g_memory_input_stream_skip_async;
  istream_class->skip_finish  = g_memory_input_stream_skip_finish;
  istream_class->close_async = g_memory_input_stream_close_async;
  istream_class->close_finish = g_memory_input_stream_close_finish;
}

static void
free_chunk (gpointer data, 
            gpointer user_data)
{
  Chunk *chunk = data;

  if (chunk->destroy)
    chunk->destroy (chunk->data);

  g_slice_free (Chunk, chunk);
}

static void
g_memory_input_stream_finalize (GObject *object)
{
  GMemoryInputStream        *stream;
  GMemoryInputStreamPrivate *priv;

  stream = G_MEMORY_INPUT_STREAM (object);
  priv = stream->priv;

  g_slist_foreach (priv->chunks, free_chunk, NULL);
  g_slist_free (priv->chunks);

  G_OBJECT_CLASS (g_memory_input_stream_parent_class)->finalize (object);
}

static void
g_memory_input_stream_seekable_iface_init (GSeekableIface *iface)
{
  iface->tell         = g_memory_input_stream_tell;
  iface->can_seek     = g_memory_input_stream_can_seek;
  iface->seek         = g_memory_input_stream_seek;
  iface->can_truncate = g_memory_input_stream_can_truncate;
  iface->truncate_fn  = g_memory_input_stream_truncate;
}

static void
g_memory_input_stream_init (GMemoryInputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
                                              G_TYPE_MEMORY_INPUT_STREAM,
                                              GMemoryInputStreamPrivate);
}

/**
 * g_memory_input_stream_new:
 *
 * Creates a new empty #GMemoryInputStream. 
 *
 * Returns: a new #GInputStream
 */
GInputStream *
g_memory_input_stream_new (void)
{
  GInputStream *stream;

  stream = g_object_new (G_TYPE_MEMORY_INPUT_STREAM, NULL);

  return stream;
}

/**
 * g_memory_input_stream_new_from_data:
 * @data: input data
 * @len: length of the data, may be -1 if @data is a nul-terminated string
 * @destroy: function that is called to free @data, or %NULL
 *
 * Creates a new #GMemoryInputStream with data in memory of a given size.
 * 
 * Returns: new #GInputStream read from @data of @len bytes.
 **/
GInputStream *
g_memory_input_stream_new_from_data (const void     *data, 
                                     gssize          len,
                                     GDestroyNotify  destroy)
{
  GInputStream *stream;

  stream = g_memory_input_stream_new ();

  g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (stream),
                                  data, len, destroy);

  return stream;
}

/**
 * g_memory_input_stream_add_data:
 * @stream: a #GMemoryInputStream
 * @data: input data
 * @len: length of the data, may be -1 if @data is a nul-terminated string
 * @destroy: function that is called to free @data, or %NULL
 *
 * Appends @data to data that can be read from the input stream
 */
void
g_memory_input_stream_add_data (GMemoryInputStream *stream,
                                const void         *data,
                                gssize              len,
                                GDestroyNotify      destroy)
{
  GMemoryInputStreamPrivate *priv;
  Chunk *chunk;
 
  g_return_if_fail (G_IS_MEMORY_INPUT_STREAM (stream));
  g_return_if_fail (data != NULL);

  priv = stream->priv;

  if (len == -1)
    len = strlen (data);
  
  chunk = g_slice_new (Chunk);
  chunk->data = (guint8 *)data;
  chunk->len = len;
  chunk->destroy = destroy;

  priv->chunks = g_slist_append (priv->chunks, chunk);
  priv->len += chunk->len;
}

static gssize
g_memory_input_stream_read (GInputStream  *stream,
                            void          *buffer,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GMemoryInputStream *memory_stream;
  GMemoryInputStreamPrivate *priv;
  GSList *l;
  Chunk *chunk;
  gsize offset, start, rest, size;

  memory_stream = G_MEMORY_INPUT_STREAM (stream);
  priv = memory_stream->priv;

  count = MIN (count, priv->len - priv->pos);

  offset = 0;
  for (l = priv->chunks; l; l = l->next) 
    {
      chunk = (Chunk *)l->data;

      if (offset + chunk->len > priv->pos)
        break;

      offset += chunk->len;
    }
  
  start = priv->pos - offset;
  rest = count;

  for (; l && rest > 0; l = l->next)
    {
      chunk = (Chunk *)l->data;
      size = MIN (rest, chunk->len - start);

      memcpy ((guint8 *)buffer + (count - rest), chunk->data + start, size);
      rest -= size;

      start = 0;
    }

  priv->pos += count;

  return count;
}

static gssize
g_memory_input_stream_skip (GInputStream  *stream,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GMemoryInputStream *memory_stream;
  GMemoryInputStreamPrivate *priv;

  memory_stream = G_MEMORY_INPUT_STREAM (stream);
  priv = memory_stream->priv;

  count = MIN (count, priv->len - priv->pos);
  priv->pos += count;

  return count;
}

static gboolean
g_memory_input_stream_close (GInputStream  *stream,
                             GCancellable  *cancellable,
                             GError       **error)
{
  return TRUE;
}

static void
g_memory_input_stream_read_async (GInputStream        *stream,
                                  void                *buffer,
                                  gsize                count,
                                  int                  io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSimpleAsyncResult *simple;
  gssize nread;

  nread = g_memory_input_stream_read (stream, buffer, count, cancellable, NULL);
  simple = g_simple_async_result_new (G_OBJECT (stream),
				      callback,
				      user_data,
				      g_memory_input_stream_read_async);
  g_simple_async_result_set_op_res_gssize (simple, nread);
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static gssize
g_memory_input_stream_read_finish (GInputStream  *stream,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  GSimpleAsyncResult *simple;
  gssize nread;

  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_memory_input_stream_read_async);
  
  nread = g_simple_async_result_get_op_res_gssize (simple);
  return nread;
}

static void
g_memory_input_stream_skip_async (GInputStream        *stream,
                                  gsize                count,
                                  int                  io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSimpleAsyncResult *simple;
  gssize nskipped;

  nskipped = g_memory_input_stream_skip (stream, count, cancellable, NULL);
  simple = g_simple_async_result_new (G_OBJECT (stream),
                                      callback,
                                      user_data,
                                      g_memory_input_stream_skip_async);
  g_simple_async_result_set_op_res_gssize (simple, nskipped);
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static gssize
g_memory_input_stream_skip_finish (GInputStream  *stream,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  GSimpleAsyncResult *simple;
  gssize nskipped;

  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_memory_input_stream_skip_async);
  
  nskipped = g_simple_async_result_get_op_res_gssize (simple);
  return nskipped;
}

static void
g_memory_input_stream_close_async (GInputStream        *stream,
                                   int                  io_priority,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  GSimpleAsyncResult *simple;
  
  simple = g_simple_async_result_new (G_OBJECT (stream),
				      callback,
				      user_data,
				      g_memory_input_stream_close_async);
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static gboolean
g_memory_input_stream_close_finish (GInputStream  *stream,
                                    GAsyncResult  *result,
                                    GError       **error)
{
  return TRUE;
}

static goffset
g_memory_input_stream_tell (GSeekable *seekable)
{
  GMemoryInputStream *memory_stream;
  GMemoryInputStreamPrivate *priv;

  memory_stream = G_MEMORY_INPUT_STREAM (seekable);
  priv = memory_stream->priv;

  return priv->pos;
}

static
gboolean g_memory_input_stream_can_seek (GSeekable *seekable)
{
  return TRUE;
}

static gboolean
g_memory_input_stream_seek (GSeekable     *seekable,
                            goffset        offset,
                            GSeekType      type,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GMemoryInputStream *memory_stream;
  GMemoryInputStreamPrivate *priv;
  goffset absolute;

  memory_stream = G_MEMORY_INPUT_STREAM (seekable);
  priv = memory_stream->priv;

  switch (type) 
    {
    case G_SEEK_CUR:
      absolute = priv->pos + offset;
      break;

    case G_SEEK_SET:
      absolute = offset;
      break;

    case G_SEEK_END:
      absolute = priv->len + offset;
      break;
  
    default:
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Invalid GSeekType supplied"));

      return FALSE;
    }

  if (absolute < 0 || absolute > priv->len)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Invalid seek request"));
      return FALSE;
    }

  priv->pos = absolute;

  return TRUE;
}

static gboolean
g_memory_input_stream_can_truncate (GSeekable *seekable)
{
  return FALSE;
}

static gboolean
g_memory_input_stream_truncate (GSeekable     *seekable,
                                goffset        offset,
                                GCancellable  *cancellable,
                                GError       **error)
{
  g_set_error_literal (error,
                       G_IO_ERROR,
                       G_IO_ERROR_NOT_SUPPORTED,
                       _("Cannot truncate GMemoryInputStream"));
  return FALSE;
}
