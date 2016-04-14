/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Collabora Ltd.
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
 * Authors: Nicolas Dufresne <nicolas.dufresne@colllabora.co.uk>
 */

#include "config.h"

#include "gproxyconnection.h"

#include "gtcpconnection.h"
#include "glibintl.h"

#define g_proxy_connection_get_type _g_proxy_connection_get_type
G_DEFINE_TYPE (GProxyConnection,
	       g_proxy_connection, G_TYPE_TCP_CONNECTION);

enum
{
  PROP_NONE,
  PROP_IO_STREAM
};

struct _GProxyConnectionPrivate
{
  GIOStream *io_stream;
};

static GInputStream *
g_proxy_connection_get_input_stream (GIOStream *io_stream)
{
  GProxyConnection *connection;
  connection = G_PROXY_PROXY_CONNECTION (io_stream);
  return g_io_stream_get_input_stream (connection->priv->io_stream);
}

static GOutputStream *
g_proxy_connection_get_output_stream (GIOStream *io_stream)
{
  GProxyConnection *connection;
  connection = G_PROXY_PROXY_CONNECTION (io_stream);
  return g_io_stream_get_output_stream (connection->priv->io_stream);
}

static void
g_proxy_connection_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GProxyConnection *connection = G_PROXY_PROXY_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_IO_STREAM:
      g_value_set_object (value, connection->priv->io_stream);
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_proxy_connection_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GProxyConnection *connection = G_PROXY_PROXY_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_IO_STREAM:
      connection->priv->io_stream = G_IO_STREAM (g_value_dup_object (value));
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_proxy_connection_finalize (GObject *object)
{
  GProxyConnection *connection = G_PROXY_PROXY_CONNECTION (object);

  g_object_unref (connection->priv->io_stream);

  G_OBJECT_CLASS (g_proxy_connection_parent_class)->finalize (object);
}

static void
g_proxy_connection_class_init (GProxyConnectionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GIOStreamClass *stream_class = G_IO_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GProxyConnectionPrivate));

  gobject_class->set_property = g_proxy_connection_set_property;
  gobject_class->get_property = g_proxy_connection_get_property;
  gobject_class->finalize = g_proxy_connection_finalize;

  stream_class->get_input_stream = g_proxy_connection_get_input_stream;
  stream_class->get_output_stream = g_proxy_connection_get_output_stream;

  g_object_class_install_property (gobject_class,
                                   PROP_IO_STREAM,
                                   g_param_spec_object ("io-stream",
			                                P_("IO Stream"),
			                                P_("The altered streams"),
                                                        G_TYPE_SOCKET_CONNECTION,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
g_proxy_connection_init (GProxyConnection *connection)
{
  connection->priv = G_TYPE_INSTANCE_GET_PRIVATE (connection,
                                                  G_TYPE_PROXY_CONNECTION,
                                                  GProxyConnectionPrivate);
}

GSocketConnection *
_g_proxy_connection_new (GTcpConnection *connection,
                         GIOStream      *io_stream)
{
  GSocket *socket;

  socket = g_socket_connection_get_socket (G_SOCKET_CONNECTION (connection));

  return G_SOCKET_CONNECTION (g_object_new (G_TYPE_PROXY_CONNECTION,
                                            "io-stream", io_stream,
                                            "socket", socket,
                                            NULL));
}
