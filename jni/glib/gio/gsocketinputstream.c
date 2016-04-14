/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
 *           © 2009 codethink
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *          Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"
#include "gsocketinputstream.h"
#include "glibintl.h"

#include <gio/gsimpleasyncresult.h>
#include <gio/gcancellable.h>
#include <gio/gioerror.h>


#define g_socket_input_stream_get_type _g_socket_input_stream_get_type
G_DEFINE_TYPE (GSocketInputStream, g_socket_input_stream, G_TYPE_INPUT_STREAM);

enum
{
  PROP_0,
  PROP_SOCKET
};

struct _GSocketInputStreamPrivate
{
  GSocket *socket;

  /* pending operation metadata */
  GSimpleAsyncResult *result;
  GCancellable *cancellable;
  gpointer buffer;
  gsize count;
};

static void
g_socket_input_stream_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GSocketInputStream *stream = G_SOCKET_INPUT_STREAM (object);

  switch (prop_id)
    {
      case PROP_SOCKET:
        g_value_set_object (value, stream->priv->socket);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_socket_input_stream_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GSocketInputStream *stream = G_SOCKET_INPUT_STREAM (object);

  switch (prop_id)
    {
      case PROP_SOCKET:
        stream->priv->socket = g_value_dup_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_socket_input_stream_finalize (GObject *object)
{
  GSocketInputStream *stream = G_SOCKET_INPUT_STREAM (object);

  if (stream->priv->socket)
    g_object_unref (stream->priv->socket);

  if (G_OBJECT_CLASS (g_socket_input_stream_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_socket_input_stream_parent_class)->finalize) (object);
}

static gssize
g_socket_input_stream_read (GInputStream  *stream,
                            void          *buffer,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GSocketInputStream *input_stream = G_SOCKET_INPUT_STREAM (stream);

  return g_socket_receive_with_blocking (input_stream->priv->socket,
					 buffer, count, TRUE,
					 cancellable, error);
}

static gboolean
g_socket_input_stream_read_ready (GSocket *socket,
                                  GIOCondition condition,
				  GSocketInputStream *stream)
{
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  gssize result;

  result = g_socket_receive_with_blocking (stream->priv->socket,
					   stream->priv->buffer,
					   stream->priv->count,
					   FALSE,
					   stream->priv->cancellable,
					   &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    return TRUE;

  simple = stream->priv->result;
  stream->priv->result = NULL;

  if (result >= 0)
    g_simple_async_result_set_op_res_gssize (simple, result);

  if (error)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  if (stream->priv->cancellable)
    g_object_unref (stream->priv->cancellable);

  g_simple_async_result_complete (simple);
  g_object_unref (simple);

  return FALSE;
}

static void
g_socket_input_stream_read_async (GInputStream        *stream,
                                  void                *buffer,
                                  gsize                count,
                                  gint                 io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSocketInputStream *input_stream = G_SOCKET_INPUT_STREAM (stream);
  GSource *source;

  g_assert (input_stream->priv->result == NULL);

  input_stream->priv->result =
    g_simple_async_result_new (G_OBJECT (stream), callback, user_data,
                               g_socket_input_stream_read_async);
  if (cancellable)
    g_object_ref (cancellable);
  input_stream->priv->cancellable = cancellable;
  input_stream->priv->buffer = buffer;
  input_stream->priv->count = count;

  source = g_socket_create_source (input_stream->priv->socket,
				   G_IO_IN | G_IO_HUP | G_IO_ERR,
				   cancellable);
  g_source_set_callback (source,
			 (GSourceFunc) g_socket_input_stream_read_ready,
			 g_object_ref (input_stream), g_object_unref);
  g_source_attach (source, g_main_context_get_thread_default ());
  g_source_unref (source);
}

static gssize
g_socket_input_stream_read_finish (GInputStream  *stream,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  GSimpleAsyncResult *simple;
  gssize count;

  g_return_val_if_fail (G_IS_SOCKET_INPUT_STREAM (stream), -1);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_socket_input_stream_read_async);

  count = g_simple_async_result_get_op_res_gssize (simple);

  return count;
}

static void
g_socket_input_stream_class_init (GSocketInputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *ginputstream_class = G_INPUT_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GSocketInputStreamPrivate));

  gobject_class->finalize = g_socket_input_stream_finalize;
  gobject_class->get_property = g_socket_input_stream_get_property;
  gobject_class->set_property = g_socket_input_stream_set_property;

  ginputstream_class->read_fn = g_socket_input_stream_read;
  ginputstream_class->read_async = g_socket_input_stream_read_async;
  ginputstream_class->read_finish = g_socket_input_stream_read_finish;

  g_object_class_install_property (gobject_class, PROP_SOCKET,
				   g_param_spec_object ("socket",
							P_("socket"),
							P_("The socket that this stream wraps"),
							G_TYPE_SOCKET, G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
g_socket_input_stream_init (GSocketInputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, G_TYPE_SOCKET_INPUT_STREAM, GSocketInputStreamPrivate);
}

GSocketInputStream *
_g_socket_input_stream_new (GSocket *socket)
{
  return G_SOCKET_INPUT_STREAM (g_object_new (G_TYPE_SOCKET_INPUT_STREAM, "socket", socket, NULL));
}
