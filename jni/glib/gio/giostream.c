/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 codethink
 * Copyright © 2009 Red Hat, Inc.
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "giostream.h"
#include <gio/gsimpleasyncresult.h>
#include <gio/gasyncresult.h>


G_DEFINE_ABSTRACT_TYPE (GIOStream, g_io_stream, G_TYPE_OBJECT);

/**
 * SECTION:giostream
 * @short_description: Base class for implementing read/write streams
 * @include: gio/gio.h
 * @see_also: #GInputStream, #GOutputStream
 *
 * GIOStream represents an object that has both read and write streams.
 * Generally the two streams acts as separate input and output streams,
 * but they share some common resources and state. For instance, for
 * seekable streams they may use the same position in both streams.
 *
 * Examples of #GIOStream objects are #GSocketConnection which represents
 * a two-way network connection, and #GFileIOStream which represent a
 * file handle opened in read-write mode.
 *
 * To do the actual reading and writing you need to get the substreams
 * with g_io_stream_get_input_stream() and g_io_stream_get_output_stream().
 *
 * The #GIOStream object owns the input and the output streams, not the other
 * way around, so keeping the substreams alive will not keep the #GIOStream
 * object alive. If the #GIOStream object is freed it will be closed, thus
 * closing the substream, so even if the substreams stay alive they will
 * always just return a %G_IO_ERROR_CLOSED for all operations.
 *
 * To close a stream use g_io_stream_close() which will close the common
 * stream object and also the individual substreams. You can also close
 * the substreams themselves. In most cases this only marks the
 * substream as closed, so further I/O on it fails. However, some streams
 * may support "half-closed" states where one direction of the stream
 * is actually shut down.
 *
 * Since: 2.22
 */

enum
{
  PROP_0,
  PROP_INPUT_STREAM,
  PROP_OUTPUT_STREAM,
  PROP_CLOSED
};

struct _GIOStreamPrivate {
  guint closed : 1;
  guint pending : 1;
  GAsyncReadyCallback outstanding_callback;
};

static gboolean g_io_stream_real_close        (GIOStream            *stream,
					       GCancellable         *cancellable,
					       GError              **error);
static void     g_io_stream_real_close_async  (GIOStream            *stream,
					       int                   io_priority,
					       GCancellable         *cancellable,
					       GAsyncReadyCallback   callback,
					       gpointer              user_data);
static gboolean g_io_stream_real_close_finish (GIOStream            *stream,
					       GAsyncResult         *result,
					       GError              **error);

static void
g_io_stream_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_io_stream_parent_class)->finalize (object);
}

static void
g_io_stream_dispose (GObject *object)
{
  GIOStream *stream;

  stream = G_IO_STREAM (object);

  if (!stream->priv->closed)
    g_io_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (g_io_stream_parent_class)->dispose (object);
}

static void
g_io_stream_init (GIOStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
					      G_TYPE_IO_STREAM,
					      GIOStreamPrivate);
}

static void
g_io_stream_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  GIOStream *stream = G_IO_STREAM (object);

  switch (prop_id)
    {
      case PROP_CLOSED:
        g_value_set_boolean (value, stream->priv->closed);
        break;

      case PROP_INPUT_STREAM:
        g_value_set_object (value, g_io_stream_get_input_stream (stream));
        break;

      case PROP_OUTPUT_STREAM:
        g_value_set_object (value, g_io_stream_get_output_stream (stream));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_io_stream_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_io_stream_class_init (GIOStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GIOStreamPrivate));

  gobject_class->finalize = g_io_stream_finalize;
  gobject_class->dispose = g_io_stream_dispose;
  gobject_class->set_property = g_io_stream_set_property;
  gobject_class->get_property = g_io_stream_get_property;

  klass->close_fn = g_io_stream_real_close;
  klass->close_async = g_io_stream_real_close_async;
  klass->close_finish = g_io_stream_real_close_finish;

  g_object_class_install_property (gobject_class, PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         P_("Closed"),
                                                         P_("Is the stream closed"),
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INPUT_STREAM,
				   g_param_spec_object ("input-stream",
							P_("Input stream"),
							P_("The GInputStream to read from"),
							G_TYPE_INPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OUTPUT_STREAM,
				   g_param_spec_object ("output-stream",
							P_("Output stream"),
							P_("The GOutputStream to write to"),
							G_TYPE_OUTPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

/**
 * g_io_stream_is_closed:
 * @stream: a #GIOStream
 *
 * Checks if a stream is closed.
 *
 * Returns: %TRUE if the stream is closed.
 *
 * Since: 2.22
 */
gboolean
g_io_stream_is_closed (GIOStream *stream)
{
  g_return_val_if_fail (G_IS_IO_STREAM (stream), TRUE);

  return stream->priv->closed;
}

/**
 * g_io_stream_get_input_stream:
 * @stream: a #GIOStream
 *
 * Gets the input stream for this object. This is used
 * for reading.
 *
 * Returns: (transfer none): a #GInputStream, owned by the #GIOStream.
 * Do not free.
 *
 * Since: 2.22
 */
GInputStream *
g_io_stream_get_input_stream (GIOStream *stream)
{
  GIOStreamClass *klass;

  klass = G_IO_STREAM_GET_CLASS (stream);

  g_assert (klass->get_input_stream != NULL);

  return klass->get_input_stream (stream);
}

/**
 * g_io_stream_get_output_stream:
 * @stream: a #GIOStream
 *
 * Gets the output stream for this object. This is used for
 * writing.
 *
 * Returns: (transfer none): a #GOutputStream, owned by the #GIOStream.
 * Do not free.
 *
 * Since: 2.22
 */
GOutputStream *
g_io_stream_get_output_stream (GIOStream *stream)
{
  GIOStreamClass *klass;

  klass = G_IO_STREAM_GET_CLASS (stream);

  g_assert (klass->get_output_stream != NULL);
  return klass->get_output_stream (stream);
}

/**
 * g_io_stream_has_pending:
 * @stream: a #GIOStream
 *
 * Checks if a stream has pending actions.
 *
 * Returns: %TRUE if @stream has pending actions.
 *
 * Since: 2.22
 **/
gboolean
g_io_stream_has_pending (GIOStream *stream)
{
  g_return_val_if_fail (G_IS_IO_STREAM (stream), FALSE);

  return stream->priv->pending;
}

/**
 * g_io_stream_set_pending:
 * @stream: a #GIOStream
 * @error: a #GError location to store the error occuring, or %NULL to
 *     ignore
 *
 * Sets @stream to have actions pending. If the pending flag is
 * already set or @stream is closed, it will return %FALSE and set
 * @error.
 *
 * Return value: %TRUE if pending was previously unset and is now set.
 *
 * Since: 2.22
 */
gboolean
g_io_stream_set_pending (GIOStream  *stream,
			 GError    **error)
{
  g_return_val_if_fail (G_IS_IO_STREAM (stream), FALSE);

  if (stream->priv->closed)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Stream is already closed"));
      return FALSE;
    }

  if (stream->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
                           /* Translators: This is an error you get if there is
                            * already an operation running against this stream when
                            * you try to start one */
                           _("Stream has outstanding operation"));
      return FALSE;
    }

  stream->priv->pending = TRUE;
  return TRUE;
}

/**
 * g_io_stream_clear_pending:
 * @stream: a #GIOStream
 *
 * Clears the pending flag on @stream.
 *
 * Since: 2.22
 */
void
g_io_stream_clear_pending (GIOStream *stream)
{
  g_return_if_fail (G_IS_IO_STREAM (stream));

  stream->priv->pending = FALSE;
}

static gboolean
g_io_stream_real_close (GIOStream     *stream,
			GCancellable  *cancellable,
			GError       **error)
{
  gboolean res;

  res = g_output_stream_close (g_io_stream_get_output_stream (stream),
			       cancellable, error);

  /* If this errored out, unset error so that we don't report
     further errors, but still do the following ops */
  if (error != NULL && *error != NULL)
    error = NULL;

  res &= g_input_stream_close (g_io_stream_get_input_stream (stream),
			       cancellable, error);

  return res;
}

/**
 * g_io_stream_close:
 * @stream: a #GIOStream
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it. This will also
 * closes the individual input and output streams, if they are not already
 * closed.
 *
 * Once the stream is closed, all other operations will return
 * %G_IO_ERROR_CLOSED. Closing a stream multiple times will not
 * return an error.
 *
 * Closing a stream will automatically flush any outstanding buffers
 * in the stream.
 *
 * Streams will be automatically closed when the last reference
 * is dropped, but you might want to call this function to make sure
 * resources are released as early as possible.
 *
 * Some streams might keep the backing store of the stream (e.g. a file
 * descriptor) open after the stream is closed. See the documentation for
 * the individual stream for details.
 *
 * On failure the first error that happened will be reported, but the
 * close operation will finish as much as possible. A stream that failed
 * to close will still return %G_IO_ERROR_CLOSED for all operations.
 * Still, it is important to check and report the error to the user,
 * otherwise there might be a loss of data as all data might not be written.
 *
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but some streams
 * can use a faster close that doesn't block to e.g. check errors.
 *
 * The default implementation of this method just calls close on the
 * individual input/output streams.
 *
 * Return value: %TRUE on success, %FALSE on failure
 *
 * Since: 2.22
 */
gboolean
g_io_stream_close (GIOStream     *stream,
		   GCancellable  *cancellable,
		   GError       **error)
{
  GIOStreamClass *class;
  gboolean res;

  g_return_val_if_fail (G_IS_IO_STREAM (stream), FALSE);

  class = G_IO_STREAM_GET_CLASS (stream);

  if (stream->priv->closed)
    return TRUE;

  if (!g_io_stream_set_pending (stream, error))
    return FALSE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = TRUE;
  if (class->close_fn)
    res = class->close_fn (stream, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  stream->priv->closed = TRUE;
  g_io_stream_clear_pending (stream);

  return res;
}

static void
async_ready_close_callback_wrapper (GObject      *source_object,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
  GIOStream *stream = G_IO_STREAM (source_object);

  stream->priv->closed = TRUE;
  g_io_stream_clear_pending (stream);
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  g_object_unref (stream);
}

/**
 * g_io_stream_close_async:
 * @stream: a #GIOStream
 * @io_priority: the io priority of the request
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 * @cancellable: optional cancellable object
 *
 * Requests an asynchronous close of the stream, releasing resources
 * related to it. When the operation is finished @callback will be
 * called. You can then call g_io_stream_close_finish() to get
 * the result of the operation.
 *
 * For behaviour details see g_io_stream_close().
 *
 * The asynchronous methods have a default fallback that uses threads
 * to implement asynchronicity, so they are optional for inheriting
 * classes. However, if you override one you must override all.
 *
 * Since: 2.22
 */
void
g_io_stream_close_async (GIOStream           *stream,
			 int                  io_priority,
			 GCancellable        *cancellable,
			 GAsyncReadyCallback  callback,
			 gpointer             user_data)
{
  GIOStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_IO_STREAM (stream));

  if (stream->priv->closed)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_io_stream_close_async);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  if (!g_io_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }

  class = G_IO_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);
  class->close_async (stream, io_priority, cancellable,
		      async_ready_close_callback_wrapper, user_data);
}

/**
 * g_io_stream_close_finish:
 * @stream: a #GIOStream
 * @result: a #GAsyncResult
 * @error: a #GError location to store the error occuring, or %NULL to
 *    ignore
 *
 * Closes a stream.
 *
 * Returns: %TRUE if stream was successfully closed, %FALSE otherwise.
 *
 * Since: 2.22
 */
gboolean
g_io_stream_close_finish (GIOStream     *stream,
			  GAsyncResult  *result,
			  GError       **error)
{
  GSimpleAsyncResult *simple;
  GIOStreamClass *class;

  g_return_val_if_fail (G_IS_IO_STREAM (stream), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return FALSE;

      /* Special case already closed */
      if (g_simple_async_result_get_source_tag (simple) == g_io_stream_close_async)
	return TRUE;
    }

  class = G_IO_STREAM_GET_CLASS (stream);
  return class->close_finish (stream, result, error);
}


static void
close_async_thread (GSimpleAsyncResult *res,
		    GObject            *object,
		    GCancellable       *cancellable)
{
  GIOStreamClass *class;
  GError *error = NULL;
  gboolean result;

  /* Auto handling of cancelation disabled, and ignore cancellation,
   * since we want to close things anyway, although possibly in a
   * quick-n-dirty way. At least we never want to leak open handles
   */
  class = G_IO_STREAM_GET_CLASS (object);
  if (class->close_fn)
    {
      result = class->close_fn (G_IO_STREAM (object), cancellable, &error);
      if (!result)
	{
	  g_simple_async_result_set_from_error (res, error);
	  g_error_free (error);
	}
    }
}

static void
g_io_stream_real_close_async (GIOStream           *stream,
			      int                  io_priority,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
  GSimpleAsyncResult *res;

  res = g_simple_async_result_new (G_OBJECT (stream),
				   callback,
				   user_data,
				   g_io_stream_real_close_async);

  g_simple_async_result_set_handle_cancellation (res, FALSE);

  g_simple_async_result_run_in_thread (res,
				       close_async_thread,
				       io_priority,
				       cancellable);
  g_object_unref (res);
}

static gboolean
g_io_stream_real_close_finish (GIOStream     *stream,
			       GAsyncResult  *result,
			       GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) ==
		  g_io_stream_real_close_async);
  return TRUE;
}
