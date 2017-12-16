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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>
#include "gioerror.h"
#include "gunixoutputstream.h"
#include "gcancellable.h"
#include "gsimpleasyncresult.h"
#include "gasynchelper.h"
#include "glibintl.h"


/**
 * SECTION:gunixoutputstream
 * @short_description: Streaming output operations for UNIX file descriptors
 * @include: gio/gunixoutputstream.h
 * @see_also: #GOutputStream
 *
 * #GUnixOutputStream implements #GOutputStream for writing to a
 * UNIX file descriptor, including asynchronous operations. The file
 * descriptor must be selectable, so it doesn't work with opened files.
 *
 * Note that <filename>&lt;gio/gunixoutputstream.h&gt;</filename> belongs
 * to the UNIX-specific GIO interfaces, thus you have to use the
 * <filename>gio-unix-2.0.pc</filename> pkg-config file when using it.
 */

enum {
  PROP_0,
  PROP_FD,
  PROP_CLOSE_FD
};

G_DEFINE_TYPE (GUnixOutputStream, g_unix_output_stream, G_TYPE_OUTPUT_STREAM);


struct _GUnixOutputStreamPrivate {
  int fd;
  gboolean close_fd;
};

static void     g_unix_output_stream_set_property (GObject              *object,
						   guint                 prop_id,
						   const GValue         *value,
						   GParamSpec           *pspec);
static void     g_unix_output_stream_get_property (GObject              *object,
						   guint                 prop_id,
						   GValue               *value,
						   GParamSpec           *pspec);
static gssize   g_unix_output_stream_write        (GOutputStream        *stream,
						   const void           *buffer,
						   gsize                 count,
						   GCancellable         *cancellable,
						   GError              **error);
static gboolean g_unix_output_stream_close        (GOutputStream        *stream,
						   GCancellable         *cancellable,
						   GError              **error);
static void     g_unix_output_stream_write_async  (GOutputStream        *stream,
						   const void           *buffer,
						   gsize                 count,
						   int                   io_priority,
						   GCancellable         *cancellable,
						   GAsyncReadyCallback   callback,
						   gpointer              data);
static gssize   g_unix_output_stream_write_finish (GOutputStream        *stream,
						   GAsyncResult         *result,
						   GError              **error);
static void     g_unix_output_stream_close_async  (GOutputStream        *stream,
						   int                   io_priority,
						   GCancellable         *cancellable,
						   GAsyncReadyCallback   callback,
						   gpointer              data);
static gboolean g_unix_output_stream_close_finish (GOutputStream        *stream,
						   GAsyncResult         *result,
						   GError              **error);


static void
g_unix_output_stream_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_unix_output_stream_parent_class)->finalize (object);
}

static void
g_unix_output_stream_class_init (GUnixOutputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (GUnixOutputStreamPrivate));

  gobject_class->get_property = g_unix_output_stream_get_property;
  gobject_class->set_property = g_unix_output_stream_set_property;
  gobject_class->finalize = g_unix_output_stream_finalize;

  stream_class->write_fn = g_unix_output_stream_write;
  stream_class->close_fn = g_unix_output_stream_close;
  stream_class->write_async = g_unix_output_stream_write_async;
  stream_class->write_finish = g_unix_output_stream_write_finish;
  stream_class->close_async = g_unix_output_stream_close_async;
  stream_class->close_finish = g_unix_output_stream_close_finish;

   /**
   * GUnixOutputStream:fd:
   *
   * The file descriptor that the stream writes to.
   *
   * Since: 2.20
   */
  g_object_class_install_property (gobject_class,
				   PROP_FD,
				   g_param_spec_int ("fd",
						     P_("File descriptor"),
						     P_("The file descriptor to write to"),
						     G_MININT, G_MAXINT, -1,
						     G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

  /**
   * GUnixOutputStream:close-fd:
   *
   * Whether to close the file descriptor when the stream is closed.
   *
   * Since: 2.20
   */
  g_object_class_install_property (gobject_class,
				   PROP_CLOSE_FD,
				   g_param_spec_boolean ("close-fd",
							 P_("Close file descriptor"),
							 P_("Whether to close the file descriptor when the stream is closed"),
							 TRUE,
							 G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void
g_unix_output_stream_set_property (GObject         *object,
				   guint            prop_id,
				   const GValue    *value,
				   GParamSpec      *pspec)
{
  GUnixOutputStream *unix_stream;

  unix_stream = G_UNIX_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_FD:
      unix_stream->priv->fd = g_value_get_int (value);
      break;
    case PROP_CLOSE_FD:
      unix_stream->priv->close_fd = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_unix_output_stream_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  GUnixOutputStream *unix_stream;

  unix_stream = G_UNIX_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_FD:
      g_value_set_int (value, unix_stream->priv->fd);
      break;
    case PROP_CLOSE_FD:
      g_value_set_boolean (value, unix_stream->priv->close_fd);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_unix_output_stream_init (GUnixOutputStream *unix_stream)
{
  unix_stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (unix_stream,
						   G_TYPE_UNIX_OUTPUT_STREAM,
						   GUnixOutputStreamPrivate);

  unix_stream->priv->fd = -1;
  unix_stream->priv->close_fd = TRUE;
}

/**
 * g_unix_output_stream_new:
 * @fd: a UNIX file descriptor
 * @close_fd: %TRUE to close the file descriptor when done
 * 
 * Creates a new #GUnixOutputStream for the given @fd. 
 * 
 * If @close_fd, is %TRUE, the file descriptor will be closed when 
 * the output stream is destroyed.
 * 
 * Returns: a new #GOutputStream
 **/
GOutputStream *
g_unix_output_stream_new (gint     fd,
			  gboolean close_fd)
{
  GUnixOutputStream *stream;

  g_return_val_if_fail (fd != -1, NULL);

  stream = g_object_new (G_TYPE_UNIX_OUTPUT_STREAM,
			 "fd", fd,
			 "close-fd", close_fd,
			 NULL);
  
  return G_OUTPUT_STREAM (stream);
}

/**
 * g_unix_output_stream_set_close_fd:
 * @stream: a #GUnixOutputStream
 * @close_fd: %TRUE to close the file descriptor when done
 *
 * Sets whether the file descriptor of @stream shall be closed
 * when the stream is closed.
 *
 * Since: 2.20
 */
void
g_unix_output_stream_set_close_fd (GUnixOutputStream *stream,
                                   gboolean           close_fd)
{
  g_return_if_fail (G_IS_UNIX_OUTPUT_STREAM (stream));

  close_fd = close_fd != FALSE;
  if (stream->priv->close_fd != close_fd)
    {
      stream->priv->close_fd = close_fd;
      g_object_notify (G_OBJECT (stream), "close-fd");
    }
}

/**
 * g_unix_output_stream_get_close_fd:
 * @stream: a #GUnixOutputStream
 *
 * Returns whether the file descriptor of @stream will be
 * closed when the stream is closed.
 *
 * Return value: %TRUE if the file descriptor is closed when done
 *
 * Since: 2.20
 */
gboolean
g_unix_output_stream_get_close_fd (GUnixOutputStream *stream)
{
  g_return_val_if_fail (G_IS_UNIX_OUTPUT_STREAM (stream), FALSE);

  return stream->priv->close_fd;
}

/**
 * g_unix_output_stream_get_fd:
 * @stream: a #GUnixOutputStream
 *
 * Return the UNIX file descriptor that the stream writes to.
 *
 * Return value: The file descriptor of @stream
 *
 * Since: 2.20
 */
gint
g_unix_output_stream_get_fd (GUnixOutputStream *stream)
{
  g_return_val_if_fail (G_IS_UNIX_OUTPUT_STREAM (stream), -1);

  return stream->priv->fd;
}

static gssize
g_unix_output_stream_write (GOutputStream  *stream,
			    const void     *buffer,
			    gsize           count,
			    GCancellable   *cancellable,
			    GError        **error)
{
  GUnixOutputStream *unix_stream;
  gssize res;
  GPollFD poll_fds[2];
  int poll_ret;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

  if (g_cancellable_make_pollfd (cancellable, &poll_fds[1]))
    {
      poll_fds[0].fd = unix_stream->priv->fd;
      poll_fds[0].events = G_IO_OUT;
      do
	poll_ret = g_poll (poll_fds, 2, -1);
      while (poll_ret == -1 && errno == EINTR);
      g_cancellable_release_fd (cancellable);
      
      if (poll_ret == -1)
	{
          int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to unix: %s"),
		       g_strerror (errsv));
	  return -1;
	}
    }
      
  while (1)
    {
      if (g_cancellable_set_error_if_cancelled (cancellable, error))
	return -1;

      res = write (unix_stream->priv->fd, buffer, count);
      if (res == -1)
	{
          int errsv = errno;

	  if (errsv == EINTR)
	    continue;
	  
	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to unix: %s"),
		       g_strerror (errsv));
	}
      
      break;
    }
  
  return res;
}

static gboolean
g_unix_output_stream_close (GOutputStream  *stream,
			    GCancellable   *cancellable,
			    GError        **error)
{
  GUnixOutputStream *unix_stream;
  int res;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

  if (!unix_stream->priv->close_fd)
    return TRUE;
  
  while (1)
    {
      /* This might block during the close. Doesn't seem to be a way to avoid it though. */
      res = close (unix_stream->priv->fd);
      if (res == -1)
	{
          int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error closing unix: %s"),
		       g_strerror (errsv));
	}
      break;
    }

  return res != -1;
}

typedef struct {
  gsize count;
  const void *buffer;
  GAsyncReadyCallback callback;
  gpointer user_data;
  GCancellable *cancellable;
  GUnixOutputStream *stream;
} WriteAsyncData;

static gboolean
write_async_cb (WriteAsyncData *data,
		GIOCondition    condition,
		int             fd)
{
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  gssize count_written;

  while (1)
    {
      if (g_cancellable_set_error_if_cancelled (data->cancellable, &error))
	{
	  count_written = -1;
	  break;
	}
      
      count_written = write (data->stream->priv->fd, data->buffer, data->count);
      if (count_written == -1)
	{
          int errsv = errno;

	  if (errsv == EINTR)
	    continue;
	  
	  g_set_error (&error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error reading from unix: %s"),
		       g_strerror (errsv));
	}
      break;
    }

  simple = g_simple_async_result_new (G_OBJECT (data->stream),
				      data->callback,
				      data->user_data,
				      g_unix_output_stream_write_async);
  
  g_simple_async_result_set_op_res_gssize (simple, count_written);

  if (count_written == -1)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  /* Complete immediately, not in idle, since we're already in a mainloop callout */
  g_simple_async_result_complete (simple);
  g_object_unref (simple);

  return FALSE;
}

static void
g_unix_output_stream_write_async (GOutputStream       *stream,
				  const void          *buffer,
				  gsize                count,
				  int                  io_priority,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
  GSource *source;
  GUnixOutputStream *unix_stream;
  WriteAsyncData *data;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

  data = g_new0 (WriteAsyncData, 1);
  data->count = count;
  data->buffer = buffer;
  data->callback = callback;
  data->user_data = user_data;
  data->cancellable = cancellable;
  data->stream = unix_stream;

  source = _g_fd_source_new (unix_stream->priv->fd,
			     G_IO_OUT,
			     cancellable);
  g_source_set_name (source, "GUnixOutputStream");
  
  g_source_set_callback (source, (GSourceFunc)write_async_cb, data, g_free);
  g_source_attach (source, g_main_context_get_thread_default ());
  
  g_source_unref (source);
}

static gssize
g_unix_output_stream_write_finish (GOutputStream  *stream,
				   GAsyncResult   *result,
				   GError        **error)
{
  GSimpleAsyncResult *simple;
  gssize nwritten;

  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_unix_output_stream_write_async);
  
  nwritten = g_simple_async_result_get_op_res_gssize (simple);
  return nwritten;
}

typedef struct {
  GOutputStream *stream;
  GAsyncReadyCallback callback;
  gpointer user_data;
} CloseAsyncData;

static gboolean
close_async_cb (CloseAsyncData *data)
{
  GUnixOutputStream *unix_stream;
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  gboolean result;
  int res;

  unix_stream = G_UNIX_OUTPUT_STREAM (data->stream);

  if (!unix_stream->priv->close_fd)
    {
      result = TRUE;
      goto out;
    }
  
  while (1)
    {
      res = close (unix_stream->priv->fd);
      if (res == -1)
	{
          int errsv = errno;

	  g_set_error (&error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error closing unix: %s"),
		       g_strerror (errsv));
	}
      break;
    }
  
  result = res != -1;
  
 out:
  simple = g_simple_async_result_new (G_OBJECT (data->stream),
				      data->callback,
				      data->user_data,
				      g_unix_output_stream_close_async);

  if (!result)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  /* Complete immediately, not in idle, since we're already in a mainloop callout */
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
  
  return FALSE;
}

static void
g_unix_output_stream_close_async (GOutputStream        *stream,
				  int                  io_priority,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
  GSource *idle;
  CloseAsyncData *data;

  data = g_new0 (CloseAsyncData, 1);

  data->stream = stream;
  data->callback = callback;
  data->user_data = user_data;
  
  idle = g_idle_source_new ();
  g_source_set_callback (idle, (GSourceFunc)close_async_cb, data, g_free);
  g_source_attach (idle, g_main_context_get_thread_default ());
  g_source_unref (idle);
}

static gboolean
g_unix_output_stream_close_finish (GOutputStream  *stream,
				   GAsyncResult   *result,
				   GError        **error)
{
  /* Failures handled in generic close_finish code */
  return TRUE;
}
