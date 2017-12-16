/*  GIO - GLib Input, Output and Streaming Library
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
#include "goutputstream.h"
#include "gsocketoutputstream.h"
#include "gsocket.h"

#include <gio/gsimpleasyncresult.h>
#include <gio/gcancellable.h>
#include <gio/gioerror.h>
#include "glibintl.h"


#define g_socket_output_stream_get_type _g_socket_output_stream_get_type
G_DEFINE_TYPE (GSocketOutputStream, g_socket_output_stream, G_TYPE_OUTPUT_STREAM);

enum
{
  PROP_0,
  PROP_SOCKET
};

struct _GSocketOutputStreamPrivate
{
  GSocket *socket;

  /* pending operation metadata */
  GSimpleAsyncResult *result;
  GCancellable *cancellable;
  gconstpointer buffer;
  gsize count;
};

static void
g_socket_output_stream_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GSocketOutputStream *stream = G_SOCKET_OUTPUT_STREAM (object);

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
g_socket_output_stream_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GSocketOutputStream *stream = G_SOCKET_OUTPUT_STREAM (object);

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
g_socket_output_stream_finalize (GObject *object)
{
  GSocketOutputStream *stream = G_SOCKET_OUTPUT_STREAM (object);

  if (stream->priv->socket)
    g_object_unref (stream->priv->socket);

  if (G_OBJECT_CLASS (g_socket_output_stream_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_socket_output_stream_parent_class)->finalize) (object);
}

static gssize
g_socket_output_stream_write (GOutputStream  *stream,
                              const void     *buffer,
                              gsize           count,
                              GCancellable   *cancellable,
                              GError        **error)
{
  GSocketOutputStream *onput_stream = G_SOCKET_OUTPUT_STREAM (stream);

  return g_socket_send_with_blocking (onput_stream->priv->socket,
				      buffer, count, TRUE,
				      cancellable, error);
}

static gboolean
g_socket_output_stream_write_ready (GSocket *socket,
                                    GIOCondition condition,
				    GSocketOutputStream *stream)
{
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  gssize result;

  result = g_socket_send_with_blocking (stream->priv->socket,
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
g_socket_output_stream_write_async (GOutputStream        *stream,
                                    const void           *buffer,
                                    gsize                 count,
                                    gint                  io_priority,
                                    GCancellable         *cancellable,
                                    GAsyncReadyCallback   callback,
                                    gpointer              user_data)
{
  GSocketOutputStream *output_stream = G_SOCKET_OUTPUT_STREAM (stream);
  GSource *source;

  g_assert (output_stream->priv->result == NULL);

  output_stream->priv->result =
    g_simple_async_result_new (G_OBJECT (stream), callback, user_data,
                               g_socket_output_stream_write_async);
  if (cancellable)
    g_object_ref (cancellable);
  output_stream->priv->cancellable = cancellable;
  output_stream->priv->buffer = buffer;
  output_stream->priv->count = count;

  source = g_socket_create_source (output_stream->priv->socket,
				   G_IO_OUT | G_IO_HUP | G_IO_ERR,
				   cancellable);
  g_source_set_callback (source,
			 (GSourceFunc) g_socket_output_stream_write_ready,
			 g_object_ref (output_stream), g_object_unref);
  g_source_attach (source, g_main_context_get_thread_default ());
  g_source_unref (source);
}

static gssize
g_socket_output_stream_write_finish (GOutputStream  *stream,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  GSimpleAsyncResult *simple;
  gssize count;

  g_return_val_if_fail (G_IS_SOCKET_OUTPUT_STREAM (stream), -1);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_socket_output_stream_write_async);

  count = g_simple_async_result_get_op_res_gssize (simple);

  return count;
}

static void
g_socket_output_stream_class_init (GSocketOutputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GOutputStreamClass *goutputstream_class = G_OUTPUT_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GSocketOutputStreamPrivate));

  gobject_class->finalize = g_socket_output_stream_finalize;
  gobject_class->get_property = g_socket_output_stream_get_property;
  gobject_class->set_property = g_socket_output_stream_set_property;

  goutputstream_class->write_fn = g_socket_output_stream_write;
  goutputstream_class->write_async = g_socket_output_stream_write_async;
  goutputstream_class->write_finish = g_socket_output_stream_write_finish;

  g_object_class_install_property (gobject_class, PROP_SOCKET,
				   g_param_spec_object ("socket",
							P_("socket"),
							P_("The socket that this stream wraps"),
							G_TYPE_SOCKET, G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
g_socket_output_stream_init (GSocketOutputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, G_TYPE_SOCKET_OUTPUT_STREAM, GSocketOutputStreamPrivate);
}

GSocketOutputStream *
_g_socket_output_stream_new (GSocket *socket)
{
  return G_SOCKET_OUTPUT_STREAM (g_object_new (G_TYPE_SOCKET_OUTPUT_STREAM, "socket", socket, NULL));
}
