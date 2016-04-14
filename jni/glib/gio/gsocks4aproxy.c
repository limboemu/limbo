 /* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
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
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"

#include "gsocks4aproxy.h"

#include <string.h>

#include "gasyncresult.h"
#include "giomodule.h"
#include "giomodule-priv.h"
#include "giostream.h"
#include "ginetaddress.h"
#include "ginputstream.h"
#include "glibintl.h"
#include "goutputstream.h"
#include "gproxy.h"
#include "gproxyaddress.h"
#include "gsimpleasyncresult.h"

#define SOCKS4_VERSION		  4

#define SOCKS4_CMD_CONNECT	  1
#define SOCKS4_CMD_BIND		  2

#define SOCKS4_MAX_LEN		  255

#define SOCKS4_REP_VERSION	  0
#define SOCKS4_REP_GRANTED	  90
#define SOCKS4_REP_REJECTED       91
#define SOCKS4_REP_NO_IDENT       92
#define SOCKS4_REP_BAD_IDENT      93

static void g_socks4a_proxy_iface_init (GProxyInterface *proxy_iface);

#define g_socks4a_proxy_get_type _g_socks4a_proxy_get_type
G_DEFINE_TYPE_WITH_CODE (GSocks4aProxy, g_socks4a_proxy, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_PROXY,
						g_socks4a_proxy_iface_init)
			 _g_io_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "socks4a",
							 0))

static void
g_socks4a_proxy_finalize (GObject *object)
{
  /* must chain up */
  G_OBJECT_CLASS (g_socks4a_proxy_parent_class)->finalize (object);
}

static void
g_socks4a_proxy_init (GSocks4aProxy *proxy)
{
  proxy->supports_hostname = TRUE;
}

/*                                                             |-> SOCKSv4a only
 * +----+----+----+----+----+----+----+----+----+----+....+----+------+....+------+
 * | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL| HOST |    | NULL |
 * +----+----+----+----+----+----+----+----+----+----+....+----+------+....+------+
 *    1    1      2              4           variable       1    variable
 */
#define SOCKS4_CONN_MSG_LEN	    (9 + SOCKS4_MAX_LEN * 2)
static gint
set_connect_msg (guint8      *msg,
		 const gchar *hostname,
		 guint16      port,
		 const char  *username,
		 GError     **error)
{
  GInetAddress *addr;
  guint len = 0;
  gsize addr_len;
  gboolean is_ip;
  const gchar *ip;

  msg[len++] = SOCKS4_VERSION;
  msg[len++] = SOCKS4_CMD_CONNECT;

    {
      guint16 hp = g_htons (port);
      memcpy (msg + len, &hp, 2);
      len += 2;
    }

  is_ip = g_hostname_is_ip_address (hostname);

  if (is_ip)
    ip = hostname;
  else
    ip = "0.0.0.1";
    
  addr = g_inet_address_new_from_string (ip);
  addr_len = g_inet_address_get_native_size (addr);

  if (addr_len != 4)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		  _("SOCKSv4 does not support IPv6 address '%s'"),
		  ip);
      g_object_unref (addr);
      return -1;
    }

  memcpy (msg + len, g_inet_address_to_bytes (addr), addr_len);
  len += addr_len;

  g_object_unref (addr);

  if (username)
    {
      gsize user_len = strlen (username);

      if (user_len > SOCKS4_MAX_LEN)
	{
	  g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		       _("SOCKSv4 implementation limits username to %i characters"),
		       SOCKS4_MAX_LEN);
	  return -1;
	}

      memcpy (msg + len, username, user_len);
      len += user_len;
    }

  msg[len++] = '\0';

  if (!is_ip)
    {
      gsize host_len = strlen (hostname);

      if (host_len > SOCKS4_MAX_LEN)
	{
	  g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		       _("SOCKSv4a implementation limits hostname to %i characters"),
		       SOCKS4_MAX_LEN);
	  return -1;
	}

      memcpy (msg + len, hostname, host_len);
      len += host_len;
      msg[len++] = '\0';
    }

  return len;
}

/*
 * +----+----+----+----+----+----+----+----+
 * | VN | CD | DSTPORT |      DSTIP        |
 * +----+----+----+----+----+----+----+----+
 *    1    1      2              4
 */
#define SOCKS4_CONN_REP_LEN	  8
static gboolean
parse_connect_reply (const guint8 *data, GError **error)
{
  if (data[0] != SOCKS4_REP_VERSION)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The server is not a SOCKSv4 proxy server."));
      return FALSE;
    }

  if (data[1] != SOCKS4_REP_GRANTED)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("Connection through SOCKSv4 server was rejected"));
      return FALSE;
    }

  return TRUE;
}

static GIOStream *
g_socks4a_proxy_connect (GProxy            *proxy,
			 GIOStream         *io_stream,
			 GProxyAddress     *proxy_address,
			 GCancellable      *cancellable,
			 GError           **error)
{
  GInputStream *in;
  GOutputStream *out;
  const gchar *hostname;
  guint16 port;
  const gchar *username;

  hostname = g_proxy_address_get_destination_hostname (proxy_address);
  port = g_proxy_address_get_destination_port (proxy_address);
  username = g_proxy_address_get_username (proxy_address);

  in = g_io_stream_get_input_stream (io_stream);
  out = g_io_stream_get_output_stream (io_stream);

  /* Send SOCKS4 connection request */
    {
      guint8 msg[SOCKS4_CONN_MSG_LEN];
      gint len;
      
      len = set_connect_msg (msg, hostname, port, username, error);

      if (len < 0)
	goto error;

      if (!g_output_stream_write_all (out, msg, len, NULL,
				      cancellable, error))
	goto error;
    }

  /* Read SOCKS4 response */
    {
      guint8 data[SOCKS4_CONN_REP_LEN];

      if (!g_input_stream_read_all (in, data, SOCKS4_CONN_REP_LEN, NULL,
				    cancellable, error))
	goto error;

      if (!parse_connect_reply (data, error))
	goto error;
    }

  return g_object_ref (io_stream);

error:
  return NULL;
}


typedef struct
{
  GSimpleAsyncResult *simple;
  GIOStream *io_stream;
  GProxyAddress *proxy_address;
  GCancellable *cancellable;

  /* For connecting */
  guint8 *buffer;
  gssize length;
  gssize offset;

} ConnectAsyncData;

static void connect_msg_write_cb      (GObject          *source,
				       GAsyncResult     *result,
				       gpointer          user_data);
static void connect_reply_read_cb     (GObject          *source,
				       GAsyncResult     *result,
				       gpointer          user_data);

static void
free_connect_data (ConnectAsyncData *data)
{
  if (data->io_stream)
    g_object_unref (data->io_stream);
  
  if (data->proxy_address)
    g_object_unref (data->proxy_address);

  if (data->cancellable)
    g_object_unref (data->cancellable);

  g_slice_free (ConnectAsyncData, data);
}

static void
complete_async_from_error (ConnectAsyncData *data, GError *error)
{
  GSimpleAsyncResult *simple = data->simple;
  g_simple_async_result_set_from_error (data->simple,
					error);
  g_error_free (error);
  g_simple_async_result_set_op_res_gpointer (simple, NULL, NULL);
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

static void
do_read (GAsyncReadyCallback callback, ConnectAsyncData *data)
{
   GInputStream *in;
   in = g_io_stream_get_input_stream (data->io_stream);
   g_input_stream_read_async (in,
			      data->buffer + data->offset,
			      data->length - data->offset,
			      G_PRIORITY_DEFAULT, data->cancellable,
			      callback, data);
}

static void
do_write (GAsyncReadyCallback callback, ConnectAsyncData *data)
{
  GOutputStream *out;
  out = g_io_stream_get_output_stream (data->io_stream);
  g_output_stream_write_async (out,
			       data->buffer + data->offset,
			       data->length - data->offset,
			       G_PRIORITY_DEFAULT, data->cancellable,
			       callback, data);
}



static void
g_socks4a_proxy_connect_async (GProxy               *proxy,
			       GIOStream            *io_stream,
			       GProxyAddress        *proxy_address,
			       GCancellable         *cancellable,
			       GAsyncReadyCallback   callback,
			       gpointer              user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *simple;
  ConnectAsyncData *data;
  const gchar *hostname;
  guint16 port;
  const gchar *username;

  simple = g_simple_async_result_new (G_OBJECT (proxy),
				      callback, user_data,
				      g_socks4a_proxy_connect_async);

  data = g_slice_new0 (ConnectAsyncData);

  data->simple = simple;
  data->io_stream = g_object_ref (io_stream);

  if (cancellable)
    data->cancellable = g_object_ref (cancellable);

  g_simple_async_result_set_op_res_gpointer (simple, data, 
					     (GDestroyNotify) free_connect_data);

  hostname = g_proxy_address_get_destination_hostname (proxy_address);
  port = g_proxy_address_get_destination_port (proxy_address);
  username = g_proxy_address_get_username (proxy_address); 

  data->buffer = g_malloc0 (SOCKS4_CONN_MSG_LEN);
  data->length = set_connect_msg (data->buffer,
				  hostname, port, username,
				  &error);
  data->offset = 0;

  if (data->length < 0)
    {
      g_simple_async_result_set_from_error (data->simple,
					    error);
      g_error_free (error);
      g_simple_async_result_set_op_res_gpointer (simple, NULL, NULL);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
    }
  else
    {
      do_write (connect_msg_write_cb, data);
    }
}

static void
connect_msg_write_cb (GObject      *source,
		      GAsyncResult *result,
		      gpointer      user_data)
{
  GError *error = NULL;
  ConnectAsyncData *data = user_data;
  gssize written;

  written = g_output_stream_write_finish (G_OUTPUT_STREAM (source),
					  result, &error);
  
  if (written < 0)
    {
      complete_async_from_error (data, error);
      return;
    }

  data->offset += written;

  if (data->offset == data->length)
    {
      g_free (data->buffer);

      data->buffer = g_malloc0 (SOCKS4_CONN_REP_LEN);
      data->length = SOCKS4_CONN_REP_LEN;
      data->offset = 0;

      do_read (connect_reply_read_cb, data);
    }
  else
    {
      do_write (connect_msg_write_cb, data);
    }
}

static void
connect_reply_read_cb (GObject       *source,
		       GAsyncResult  *result,
		       gpointer       user_data)
{
  GError *error = NULL;
  ConnectAsyncData *data = user_data;
  gssize read;

  read = g_input_stream_read_finish (G_INPUT_STREAM (source),
				     result, &error);

  if (read < 0)
    {
      complete_async_from_error (data, error);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      if (!parse_connect_reply (data->buffer, &error))
	{
	  complete_async_from_error (data, error);
	}
      else
	{
	  GSimpleAsyncResult *simple = data->simple;
	  g_simple_async_result_complete (simple);
	  g_object_unref (simple);
	}
    }
  else
    {
      do_read (connect_reply_read_cb, data);
    }
}

static GIOStream *g_socks4a_proxy_connect_finish (GProxy       *proxy,
						  GAsyncResult *result,
						  GError      **error);

static GIOStream *
g_socks4a_proxy_connect_finish (GProxy       *proxy,
			        GAsyncResult *result,
			        GError      **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  ConnectAsyncData *data = g_simple_async_result_get_op_res_gpointer (simple);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  return g_object_ref (data->io_stream);
}

static gboolean
g_socks4a_proxy_supports_hostname (GProxy *proxy)
{
  return G_SOCKS4A_PROXY (proxy)->supports_hostname;
}

static void
g_socks4a_proxy_class_init (GSocks4aProxyClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) class;
  object_class->finalize = g_socks4a_proxy_finalize;
}

static void
g_socks4a_proxy_iface_init (GProxyInterface *proxy_iface)
{
  proxy_iface->connect  = g_socks4a_proxy_connect;
  proxy_iface->connect_async = g_socks4a_proxy_connect_async;
  proxy_iface->connect_finish = g_socks4a_proxy_connect_finish;
  proxy_iface->supports_hostname = g_socks4a_proxy_supports_hostname;
}
