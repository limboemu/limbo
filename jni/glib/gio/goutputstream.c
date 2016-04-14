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
#include "goutputstream.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "ginputstream.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:goutputstream
 * @short_description: Base class for implementing streaming output
 * @include: gio/gio.h
 *
 * GOutputStream has functions to write to a stream (g_output_stream_write()),
 * to close a stream (g_output_stream_close()) and to flush pending writes
 * (g_output_stream_flush()). 
 *
 * To copy the content of an input stream to an output stream without 
 * manually handling the reads and writes, use g_output_stream_splice(). 
 *
 * All of these functions have async variants too.
 **/

G_DEFINE_ABSTRACT_TYPE (GOutputStream, g_output_stream, G_TYPE_OBJECT);

struct _GOutputStreamPrivate {
  guint closed : 1;
  guint pending : 1;
  guint closing : 1;
  GAsyncReadyCallback outstanding_callback;
};

static gssize   g_output_stream_real_splice        (GOutputStream             *stream,
						    GInputStream              *source,
						    GOutputStreamSpliceFlags   flags,
						    GCancellable              *cancellable,
						    GError                   **error);
static void     g_output_stream_real_write_async   (GOutputStream             *stream,
						    const void                *buffer,
						    gsize                      count,
						    int                        io_priority,
						    GCancellable              *cancellable,
						    GAsyncReadyCallback        callback,
						    gpointer                   data);
static gssize   g_output_stream_real_write_finish  (GOutputStream             *stream,
						    GAsyncResult              *result,
						    GError                   **error);
static void     g_output_stream_real_splice_async  (GOutputStream             *stream,
						    GInputStream              *source,
						    GOutputStreamSpliceFlags   flags,
						    int                        io_priority,
						    GCancellable              *cancellable,
						    GAsyncReadyCallback        callback,
						    gpointer                   data);
static gssize   g_output_stream_real_splice_finish (GOutputStream             *stream,
						    GAsyncResult              *result,
						    GError                   **error);
static void     g_output_stream_real_flush_async   (GOutputStream             *stream,
						    int                        io_priority,
						    GCancellable              *cancellable,
						    GAsyncReadyCallback        callback,
						    gpointer                   data);
static gboolean g_output_stream_real_flush_finish  (GOutputStream             *stream,
						    GAsyncResult              *result,
						    GError                   **error);
static void     g_output_stream_real_close_async   (GOutputStream             *stream,
						    int                        io_priority,
						    GCancellable              *cancellable,
						    GAsyncReadyCallback        callback,
						    gpointer                   data);
static gboolean g_output_stream_real_close_finish  (GOutputStream             *stream,
						    GAsyncResult              *result,
						    GError                   **error);

static void
g_output_stream_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_output_stream_parent_class)->finalize (object);
}

static void
g_output_stream_dispose (GObject *object)
{
  GOutputStream *stream;

  stream = G_OUTPUT_STREAM (object);
  
  if (!stream->priv->closed)
    g_output_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (g_output_stream_parent_class)->dispose (object);
}

static void
g_output_stream_class_init (GOutputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (GOutputStreamPrivate));
  
  gobject_class->finalize = g_output_stream_finalize;
  gobject_class->dispose = g_output_stream_dispose;

  klass->splice = g_output_stream_real_splice;
  
  klass->write_async = g_output_stream_real_write_async;
  klass->write_finish = g_output_stream_real_write_finish;
  klass->splice_async = g_output_stream_real_splice_async;
  klass->splice_finish = g_output_stream_real_splice_finish;
  klass->flush_async = g_output_stream_real_flush_async;
  klass->flush_finish = g_output_stream_real_flush_finish;
  klass->close_async = g_output_stream_real_close_async;
  klass->close_finish = g_output_stream_real_close_finish;
}

static void
g_output_stream_init (GOutputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
					      G_TYPE_OUTPUT_STREAM,
					      GOutputStreamPrivate);
}

/**
 * g_output_stream_write:
 * @stream: a #GOutputStream.
 * @buffer: (array length=count) (element-type uint8): the buffer containing the data to write. 
 * @count: the number of bytes to write
 * @cancellable: optional cancellable object
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Tries to write @count bytes from @buffer into the stream. Will block
 * during the operation.
 * 
 * If count is 0, returns 0 and does nothing. A value of @count
 * larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes written to the stream is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. on a partial I/O error, or if there is not enough
 * storage in the stream. All writes block until at least one byte
 * is written or an error occurs; 0 is never returned (unless
 * @count is 0).
 * 
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 * 
 * Return value: Number of bytes written, or -1 on error
 **/
gssize
g_output_stream_write (GOutputStream  *stream,
		       const void     *buffer,
		       gsize           count,
		       GCancellable   *cancellable,
		       GError        **error)
{
  GOutputStreamClass *class;
  gssize res;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, 0);

  if (count == 0)
    return 0;
  
  if (((gssize) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (class->write_fn == NULL) 
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Output stream doesn't implement write"));
      return -1;
    }
  
  if (!g_output_stream_set_pending (stream, error))
    return -1;
  
  if (cancellable)
    g_cancellable_push_current (cancellable);
  
  res = class->write_fn (stream, buffer, count, cancellable, error);
  
  if (cancellable)
    g_cancellable_pop_current (cancellable);
  
  g_output_stream_clear_pending (stream);

  return res; 
}

/**
 * g_output_stream_write_all:
 * @stream: a #GOutputStream.
 * @buffer: (array length=count) (element-type uint8): the buffer containing the data to write. 
 * @count: the number of bytes to write
 * @bytes_written: location to store the number of bytes that was 
 *     written to the stream
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Tries to write @count bytes from @buffer into the stream. Will block
 * during the operation.
 * 
 * This function is similar to g_output_stream_write(), except it tries to
 * write as many bytes as requested, only stopping on an error.
 *
 * On a successful write of @count bytes, %TRUE is returned, and @bytes_written
 * is set to @count.
 * 
 * If there is an error during the operation FALSE is returned and @error
 * is set to indicate the error status, @bytes_written is updated to contain
 * the number of bytes written into the stream before the error occurred.
 *
 * Return value: %TRUE on success, %FALSE if there was an error
 **/
gboolean
g_output_stream_write_all (GOutputStream  *stream,
			   const void     *buffer,
			   gsize           count,
			   gsize          *bytes_written,
			   GCancellable   *cancellable,
			   GError        **error)
{
  gsize _bytes_written;
  gssize res;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);

  _bytes_written = 0;
  while (_bytes_written < count)
    {
      res = g_output_stream_write (stream, (char *)buffer + _bytes_written, count - _bytes_written,
				   cancellable, error);
      if (res == -1)
	{
	  if (bytes_written)
	    *bytes_written = _bytes_written;
	  return FALSE;
	}
      
      if (res == 0)
	g_warning ("Write returned zero without error");

      _bytes_written += res;
    }
  
  if (bytes_written)
    *bytes_written = _bytes_written;

  return TRUE;
}

/**
 * g_output_stream_flush:
 * @stream: a #GOutputStream.
 * @cancellable: optional cancellable object
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Flushed any outstanding buffers in the stream. Will block during 
 * the operation. Closing the stream will implicitly cause a flush.
 *
 * This function is optional for inherited classes.
 * 
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Return value: %TRUE on success, %FALSE on error
 **/
gboolean
g_output_stream_flush (GOutputStream  *stream,
                       GCancellable   *cancellable,
                       GError        **error)
{
  GOutputStreamClass *class;
  gboolean res;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);

  if (!g_output_stream_set_pending (stream, error))
    return FALSE;
  
  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  res = TRUE;
  if (class->flush)
    {
      if (cancellable)
	g_cancellable_push_current (cancellable);
      
      res = class->flush (stream, cancellable, error);
      
      if (cancellable)
	g_cancellable_pop_current (cancellable);
    }
  
  g_output_stream_clear_pending (stream);

  return res;
}

/**
 * g_output_stream_splice:
 * @stream: a #GOutputStream.
 * @source: a #GInputStream.
 * @flags: a set of #GOutputStreamSpliceFlags.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError location to store the error occuring, or %NULL to
 * ignore.
 *
 * Splices an input stream into an output stream.
 *
 * Returns: a #gssize containing the size of the data spliced, or
 *     -1 if an error occurred.
 **/
gssize
g_output_stream_splice (GOutputStream             *stream,
			GInputStream              *source,
			GOutputStreamSpliceFlags   flags,
			GCancellable              *cancellable,
			GError                   **error)
{
  GOutputStreamClass *class;
  gssize bytes_copied;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (G_IS_INPUT_STREAM (source), -1);

  if (g_input_stream_is_closed (source))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Source stream is already closed"));
      return -1;
    }

  if (!g_output_stream_set_pending (stream, error))
    return -1;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (cancellable)
    g_cancellable_push_current (cancellable);

  bytes_copied = class->splice (stream, source, flags, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  g_output_stream_clear_pending (stream);

  return bytes_copied;
}

static gssize
g_output_stream_real_splice (GOutputStream             *stream,
                             GInputStream              *source,
                             GOutputStreamSpliceFlags   flags,
                             GCancellable              *cancellable,
                             GError                   **error)
{
  GOutputStreamClass *class = G_OUTPUT_STREAM_GET_CLASS (stream);
  gssize n_read, n_written;
  gssize bytes_copied;
  char buffer[8192], *p;
  gboolean res;

  bytes_copied = 0;
  if (class->write_fn == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Output stream doesn't implement write"));
      res = FALSE;
      goto notsupported;
    }

  res = TRUE;
  do
    {
      n_read = g_input_stream_read (source, buffer, sizeof (buffer), cancellable, error);
      if (n_read == -1)
	{
	  res = FALSE;
	  break;
	}

      if (n_read == 0)
	break;

      p = buffer;
      while (n_read > 0)
	{
	  n_written = class->write_fn (stream, p, n_read, cancellable, error);
	  if (n_written == -1)
	    {
	      res = FALSE;
	      break;
	    }

	  p += n_written;
	  n_read -= n_written;
	  bytes_copied += n_written;
	}
    }
  while (res);

 notsupported:
  if (!res)
    error = NULL; /* Ignore further errors */

  if (flags & G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE)
    {
      /* Don't care about errors in source here */
      g_input_stream_close (source, cancellable, NULL);
    }

  if (flags & G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET)
    {
      /* But write errors on close are bad! */
      if (class->close_fn &&
	  !class->close_fn (stream, cancellable, error))
	res = FALSE;
    }

  if (res)
    return bytes_copied;

  return -1;
}


/**
 * g_output_stream_close:
 * @stream: A #GOutputStream.
 * @cancellable: optional cancellable object
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it.
 *
 * Once the stream is closed, all other operations will return %G_IO_ERROR_CLOSED.
 * Closing a stream multiple times will not return an error.
 *
 * Closing a stream will automatically flush any outstanding buffers in the
 * stream.
 *
 * Streams will be automatically closed when the last reference
 * is dropped, but you might want to call this function to make sure 
 * resources are released as early as possible.
 *
 * Some streams might keep the backing store of the stream (e.g. a file descriptor)
 * open after the stream is closed. See the documentation for the individual
 * stream for details.
 *
 * On failure the first error that happened will be reported, but the close
 * operation will finish as much as possible. A stream that failed to
 * close will still return %G_IO_ERROR_CLOSED for all operations. Still, it
 * is important to check and report the error to the user, otherwise
 * there might be a loss of data as all data might not be written.
 * 
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but there some streams
 * can use a faster close that doesn't block to e.g. check errors. On
 * cancellation (as with any error) there is no guarantee that all written
 * data will reach the target. 
 *
 * Return value: %TRUE on success, %FALSE on failure
 **/
gboolean
g_output_stream_close (GOutputStream  *stream,
		       GCancellable   *cancellable,
		       GError        **error)
{
  GOutputStreamClass *class;
  gboolean res;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (stream->priv->closed)
    return TRUE;

  if (!g_output_stream_set_pending (stream, error))
    return FALSE;

  stream->priv->closing = TRUE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  if (class->flush)
    res = class->flush (stream, cancellable, error);
  else
    res = TRUE;
  
  if (!res)
    {
      /* flushing caused the error that we want to return,
       * but we still want to close the underlying stream if possible
       */
      if (class->close_fn)
	class->close_fn (stream, cancellable, NULL);
    }
  else
    {
      res = TRUE;
      if (class->close_fn)
	res = class->close_fn (stream, cancellable, error);
    }
  
  if (cancellable)
    g_cancellable_pop_current (cancellable);

  stream->priv->closing = FALSE;
  stream->priv->closed = TRUE;
  g_output_stream_clear_pending (stream);
  
  return res;
}

static void
async_ready_callback_wrapper (GObject      *source_object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source_object);

  g_output_stream_clear_pending (stream);
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  g_object_unref (stream);
}

typedef struct {
  gint io_priority;
  GCancellable *cancellable;
  GError *flush_error;
  gpointer user_data;
} CloseUserData;

static void
async_ready_close_callback_wrapper (GObject      *source_object,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source_object);
  CloseUserData *data = user_data;

  stream->priv->closing = FALSE;
  stream->priv->closed = TRUE;

  g_output_stream_clear_pending (stream);

  if (stream->priv->outstanding_callback)
    {
      if (data->flush_error != NULL)
        {
          GSimpleAsyncResult *err;

          err = g_simple_async_result_new_from_error (source_object,
                                                      stream->priv->outstanding_callback,
                                                      data->user_data,
                                                      data->flush_error);

          (*stream->priv->outstanding_callback) (source_object,
                                                 G_ASYNC_RESULT (err),
                                                 data->user_data);
          g_object_unref (err);
        }
      else
        {
          (*stream->priv->outstanding_callback) (source_object,
                                                 res,
                                                 data->user_data);
        }
    }

  g_object_unref (stream);

  if (data->cancellable)
    g_object_unref (data->cancellable);

  if (data->flush_error)
    g_error_free (data->flush_error);

  g_slice_free (CloseUserData, data);
}

static void
async_ready_close_flushed_callback_wrapper (GObject      *source_object,
                                            GAsyncResult *res,
                                            gpointer      user_data)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source_object);
  GOutputStreamClass *class;
  CloseUserData *data = user_data;
  GSimpleAsyncResult *simple;

  /* propagate the possible error */
  if (G_IS_SIMPLE_ASYNC_RESULT (res))
    {
      simple = G_SIMPLE_ASYNC_RESULT (res);
      g_simple_async_result_propagate_error (simple, &data->flush_error);
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  /* we still close, even if there was a flush error */
  class->close_async (stream, data->io_priority, data->cancellable,
		      async_ready_close_callback_wrapper, user_data);
}

/**
 * g_output_stream_write_async:
 * @stream: A #GOutputStream.
 * @buffer: (array length=count) (element-type uint8): the buffer containing the data to write. 
 * @count: the number of bytes to write
 * @io_priority: the io priority of the request.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Request an asynchronous write of @count bytes from @buffer into 
 * the stream. When the operation is finished @callback will be called.
 * You can then call g_output_stream_write_finish() to get the result of the 
 * operation.
 *
 * During an async request no other sync and async calls are allowed, 
 * and will result in %G_IO_ERROR_PENDING errors. 
 *
 * A value of @count larger than %G_MAXSSIZE will cause a 
 * %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes written will be passed to the
 * @callback. It is not an error if this is not the same as the 
 * requested size, as it can happen e.g. on a partial I/O error, 
 * but generally we try to write as many bytes as requested. 
 *
 * You are guaranteed that this method will never fail with
 * %G_IO_ERROR_WOULD_BLOCK - if @stream can't accept more data, the
 * method will just wait until this changes.
 *
 * Any outstanding I/O request with higher priority (lower numerical 
 * value) will be executed before an outstanding request with lower 
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * The asyncronous methods have a default fallback that uses threads 
 * to implement asynchronicity, so they are optional for inheriting 
 * classes. However, if you override one you must override all.
 *
 * For the synchronous, blocking version of this function, see 
 * g_output_stream_write().
 **/
void
g_output_stream_write_async (GOutputStream       *stream,
			     const void          *buffer,
			     gsize                count,
			     int                  io_priority,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
  GOutputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL);

  if (count == 0)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_output_stream_write_async);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  if (((gssize) count) < 0)
    {
      g_simple_async_report_error_in_idle (G_OBJECT (stream),
					   callback,
					   user_data,
					   G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
					   _("Too large count value passed to %s"),
					   G_STRFUNC);
      return;
    }

  if (!g_output_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }
  
  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);
  class->write_async (stream, buffer, count, io_priority, cancellable,
		      async_ready_callback_wrapper, user_data);
}

/**
 * g_output_stream_write_finish:
 * @stream: a #GOutputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes a stream write operation.
 * 
 * Returns: a #gssize containing the number of bytes written to the stream.
 **/
gssize
g_output_stream_write_finish (GOutputStream  *stream,
                              GAsyncResult   *result,
                              GError        **error)
{
  GSimpleAsyncResult *simple;
  GOutputStreamClass *class;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), -1);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return -1;

      /* Special case writes of 0 bytes */
      if (g_simple_async_result_get_source_tag (simple) == g_output_stream_write_async)
	return 0;
    }
  
  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  return class->write_finish (stream, result, error);
}

typedef struct {
  GInputStream *source;
  gpointer user_data;
  GAsyncReadyCallback callback;
} SpliceUserData;

static void
async_ready_splice_callback_wrapper (GObject      *source_object,
                                     GAsyncResult *res,
                                     gpointer     _data)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source_object);
  SpliceUserData *data = _data;
  
  g_output_stream_clear_pending (stream);
  
  if (data->callback)
    (*data->callback) (source_object, res, data->user_data);
  
  g_object_unref (stream);
  g_object_unref (data->source);
  g_free (data);
}

/**
 * g_output_stream_splice_async:
 * @stream: a #GOutputStream.
 * @source: a #GInputStream. 
 * @flags: a set of #GOutputStreamSpliceFlags.
 * @io_priority: the io priority of the request.
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @callback: a #GAsyncReadyCallback. 
 * @user_data: user data passed to @callback.
 * 
 * Splices a stream asynchronously.
 * When the operation is finished @callback will be called.
 * You can then call g_output_stream_splice_finish() to get the 
 * result of the operation.
 *
 * For the synchronous, blocking version of this function, see 
 * g_output_stream_splice().
 **/
void
g_output_stream_splice_async (GOutputStream            *stream,
			      GInputStream             *source,
			      GOutputStreamSpliceFlags  flags,
			      int                       io_priority,
			      GCancellable             *cancellable,
			      GAsyncReadyCallback       callback,
			      gpointer                  user_data)
{
  GOutputStreamClass *class;
  SpliceUserData *data;
  GError *error = NULL;

  g_return_if_fail (G_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (G_IS_INPUT_STREAM (source));

  if (g_input_stream_is_closed (source))
    {
      g_simple_async_report_error_in_idle (G_OBJECT (stream),
					   callback,
					   user_data,
					   G_IO_ERROR, G_IO_ERROR_CLOSED,
					   _("Source stream is already closed"));
      return;
    }
  
  if (!g_output_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  data = g_new0 (SpliceUserData, 1);
  data->callback = callback;
  data->user_data = user_data;
  data->source = g_object_ref (source);
  
  g_object_ref (stream);
  class->splice_async (stream, source, flags, io_priority, cancellable,
		      async_ready_splice_callback_wrapper, data);
}

/**
 * g_output_stream_splice_finish:
 * @stream: a #GOutputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 *
 * Finishes an asynchronous stream splice operation.
 * 
 * Returns: a #gssize of the number of bytes spliced.
 **/
gssize
g_output_stream_splice_finish (GOutputStream  *stream,
			       GAsyncResult   *result,
			       GError        **error)
{
  GSimpleAsyncResult *simple;
  GOutputStreamClass *class;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), -1);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return -1;
    }
  
  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  return class->splice_finish (stream, result, error);
}

/**
 * g_output_stream_flush_async:
 * @stream: a #GOutputStream.
 * @io_priority: the io priority of the request.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 * 
 * Flushes a stream asynchronously.
 * For behaviour details see g_output_stream_flush().
 *
 * When the operation is finished @callback will be 
 * called. You can then call g_output_stream_flush_finish() to get the 
 * result of the operation.
 **/
void
g_output_stream_flush_async (GOutputStream       *stream,
                             int                  io_priority,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  GOutputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_OUTPUT_STREAM (stream));

  if (!g_output_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }

  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  
  if (class->flush_async == NULL)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  async_ready_callback_wrapper,
					  user_data,
					  g_output_stream_flush_async);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }
      
  class->flush_async (stream, io_priority, cancellable,
		      async_ready_callback_wrapper, user_data);
}

/**
 * g_output_stream_flush_finish:
 * @stream: a #GOutputStream.
 * @result: a GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes flushing an output stream.
 * 
 * Returns: %TRUE if flush operation suceeded, %FALSE otherwise.
 **/
gboolean
g_output_stream_flush_finish (GOutputStream  *stream,
                              GAsyncResult   *result,
                              GError        **error)
{
  GSimpleAsyncResult *simple;
  GOutputStreamClass *klass;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return FALSE;

      /* Special case default implementation */
      if (g_simple_async_result_get_source_tag (simple) == g_output_stream_flush_async)
	return TRUE;
    }

  klass = G_OUTPUT_STREAM_GET_CLASS (stream);
  return klass->flush_finish (stream, result, error);
}


/**
 * g_output_stream_close_async:
 * @stream: A #GOutputStream.
 * @io_priority: the io priority of the request.
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 * @cancellable: optional cancellable object
 *
 * Requests an asynchronous close of the stream, releasing resources 
 * related to it. When the operation is finished @callback will be 
 * called. You can then call g_output_stream_close_finish() to get 
 * the result of the operation.
 *
 * For behaviour details see g_output_stream_close().
 *
 * The asyncronous methods have a default fallback that uses threads 
 * to implement asynchronicity, so they are optional for inheriting 
 * classes. However, if you override one you must override all.
 **/
void
g_output_stream_close_async (GOutputStream       *stream,
                             int                  io_priority,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  GOutputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  CloseUserData *data;

  g_return_if_fail (G_IS_OUTPUT_STREAM (stream));
  
  if (stream->priv->closed)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_output_stream_close_async);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  if (!g_output_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }
  
  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  stream->priv->closing = TRUE;
  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);

  data = g_slice_new0 (CloseUserData);

  if (cancellable != NULL)
    data->cancellable = g_object_ref (cancellable);

  data->io_priority = io_priority;
  data->user_data = user_data;

  /* Call close_async directly if there is no need to flush, or if the flush
     can be done sync (in the output stream async close thread) */
  if (class->flush_async == NULL ||
      (class->flush_async == g_output_stream_real_flush_async &&
       (class->flush == NULL || class->close_async == g_output_stream_real_close_async)))
    {
      class->close_async (stream, io_priority, cancellable,
                          async_ready_close_callback_wrapper, data);
    }
  else
    {
      /* First do an async flush, then do the async close in the callback
         wrapper (see async_ready_close_flushed_callback_wrapper) */
      class->flush_async (stream, io_priority, cancellable,
                          async_ready_close_flushed_callback_wrapper, data);
    }
}

/**
 * g_output_stream_close_finish:
 * @stream: a #GOutputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Closes an output stream.
 * 
 * Returns: %TRUE if stream was successfully closed, %FALSE otherwise.
 **/
gboolean
g_output_stream_close_finish (GOutputStream  *stream,
                              GAsyncResult   *result,
                              GError        **error)
{
  GSimpleAsyncResult *simple;
  GOutputStreamClass *class;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return FALSE;

      /* Special case already closed */
      if (g_simple_async_result_get_source_tag (simple) == g_output_stream_close_async)
	return TRUE;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  return class->close_finish (stream, result, error);
}

/**
 * g_output_stream_is_closed:
 * @stream: a #GOutputStream.
 * 
 * Checks if an output stream has already been closed.
 * 
 * Returns: %TRUE if @stream is closed. %FALSE otherwise. 
 **/
gboolean
g_output_stream_is_closed (GOutputStream *stream)
{
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), TRUE);
  
  return stream->priv->closed;
}

/**
 * g_output_stream_is_closing:
 * @stream: a #GOutputStream.
 *
 * Checks if an output stream is being closed. This can be
 * used inside e.g. a flush implementation to see if the
 * flush (or other i/o operation) is called from within
 * the closing operation.
 *
 * Returns: %TRUE if @stream is being closed. %FALSE otherwise.
 *
 * Since: 2.24
 **/
gboolean
g_output_stream_is_closing (GOutputStream *stream)
{
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), TRUE);

  return stream->priv->closing;
}

/**
 * g_output_stream_has_pending:
 * @stream: a #GOutputStream.
 * 
 * Checks if an ouput stream has pending actions.
 * 
 * Returns: %TRUE if @stream has pending actions. 
 **/
gboolean
g_output_stream_has_pending (GOutputStream *stream)
{
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  
  return stream->priv->pending;
}

/**
 * g_output_stream_set_pending:
 * @stream: a #GOutputStream.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Sets @stream to have actions pending. If the pending flag is
 * already set or @stream is closed, it will return %FALSE and set
 * @error.
 *
 * Return value: %TRUE if pending was previously unset and is now set.
 **/
gboolean
g_output_stream_set_pending (GOutputStream *stream,
			     GError **error)
{
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  
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
 * g_output_stream_clear_pending:
 * @stream: output stream
 * 
 * Clears the pending flag on @stream.
 **/
void
g_output_stream_clear_pending (GOutputStream *stream)
{
  g_return_if_fail (G_IS_OUTPUT_STREAM (stream));
  
  stream->priv->pending = FALSE;
}


/********************************************
 *   Default implementation of async ops    *
 ********************************************/

typedef struct {
  const void         *buffer;
  gsize               count_requested;
  gssize              count_written;
} WriteData;

static void
write_async_thread (GSimpleAsyncResult *res,
                    GObject            *object,
                    GCancellable       *cancellable)
{
  WriteData *op;
  GOutputStreamClass *class;
  GError *error = NULL;

  class = G_OUTPUT_STREAM_GET_CLASS (object);
  op = g_simple_async_result_get_op_res_gpointer (res);
  op->count_written = class->write_fn (G_OUTPUT_STREAM (object), op->buffer, op->count_requested,
				       cancellable, &error);
  if (op->count_written == -1)
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
}

static void
g_output_stream_real_write_async (GOutputStream       *stream,
                                  const void          *buffer,
                                  gsize                count,
                                  int                  io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSimpleAsyncResult *res;
  WriteData *op;

  op = g_new0 (WriteData, 1);
  res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, g_output_stream_real_write_async);
  g_simple_async_result_set_op_res_gpointer (res, op, g_free);
  op->buffer = buffer;
  op->count_requested = count;
  
  g_simple_async_result_run_in_thread (res, write_async_thread, io_priority, cancellable);
  g_object_unref (res);
}

static gssize
g_output_stream_real_write_finish (GOutputStream  *stream,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  WriteData *op;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_output_stream_real_write_async);
  op = g_simple_async_result_get_op_res_gpointer (simple);
  return op->count_written;
}

typedef struct {
  GInputStream *source;
  GOutputStreamSpliceFlags flags;
  gssize bytes_copied;
} SpliceData;

static void
splice_async_thread (GSimpleAsyncResult *result,
                     GObject            *object,
                     GCancellable       *cancellable)
{
  SpliceData *op;
  GOutputStreamClass *class;
  GError *error = NULL;
  GOutputStream *stream;

  stream = G_OUTPUT_STREAM (object);
  class = G_OUTPUT_STREAM_GET_CLASS (object);
  op = g_simple_async_result_get_op_res_gpointer (result);
  
  op->bytes_copied = class->splice (stream,
				    op->source,
				    op->flags,
				    cancellable,
				    &error);
  if (op->bytes_copied == -1)
    {
      g_simple_async_result_set_from_error (result, error);
      g_error_free (error);
    }
}

static void
g_output_stream_real_splice_async (GOutputStream             *stream,
                                   GInputStream              *source,
                                   GOutputStreamSpliceFlags   flags,
                                   int                        io_priority,
                                   GCancellable              *cancellable,
                                   GAsyncReadyCallback        callback,
                                   gpointer                   user_data)
{
  GSimpleAsyncResult *res;
  SpliceData *op;

  op = g_new0 (SpliceData, 1);
  res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, g_output_stream_real_splice_async);
  g_simple_async_result_set_op_res_gpointer (res, op, g_free);
  op->flags = flags;
  op->source = source;

  /* TODO: In the case where both source and destintion have
     non-threadbased async calls we can use a true async copy here */
  
  g_simple_async_result_run_in_thread (res, splice_async_thread, io_priority, cancellable);
  g_object_unref (res);
}

static gssize
g_output_stream_real_splice_finish (GOutputStream  *stream,
                                    GAsyncResult   *result,
                                    GError        **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  SpliceData *op;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_output_stream_real_splice_async);
  op = g_simple_async_result_get_op_res_gpointer (simple);
  return op->bytes_copied;
}


static void
flush_async_thread (GSimpleAsyncResult *res,
                    GObject            *object,
                    GCancellable       *cancellable)
{
  GOutputStreamClass *class;
  gboolean result;
  GError *error = NULL;

  class = G_OUTPUT_STREAM_GET_CLASS (object);
  result = TRUE;
  if (class->flush)
    result = class->flush (G_OUTPUT_STREAM (object), cancellable, &error);

  if (!result)
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
}

static void
g_output_stream_real_flush_async (GOutputStream       *stream,
                                  int                  io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSimpleAsyncResult *res;

  res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, g_output_stream_real_write_async);
  
  g_simple_async_result_run_in_thread (res, flush_async_thread, io_priority, cancellable);
  g_object_unref (res);
}

static gboolean
g_output_stream_real_flush_finish (GOutputStream  *stream,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  return TRUE;
}

static void
close_async_thread (GSimpleAsyncResult *res,
                    GObject            *object,
                    GCancellable       *cancellable)
{
  GOutputStreamClass *class;
  GError *error = NULL;
  gboolean result = TRUE;

  class = G_OUTPUT_STREAM_GET_CLASS (object);

  /* Do a flush here if there is a flush function, and we did not have to do
     an async flush before (see g_output_stream_close_async) */
  if (class->flush != NULL &&
      (class->flush_async == NULL ||
       class->flush_async == g_output_stream_real_flush_async))
    {
      result = class->flush (G_OUTPUT_STREAM (object), cancellable, &error);
    }

  /* Auto handling of cancelation disabled, and ignore
     cancellation, since we want to close things anyway, although
     possibly in a quick-n-dirty way. At least we never want to leak
     open handles */
  
  if (class->close_fn)
    {
      /* Make sure to close, even if the flush failed (see sync close) */
      if (!result)
        class->close_fn (G_OUTPUT_STREAM (object), cancellable, NULL);
      else
        result = class->close_fn (G_OUTPUT_STREAM (object), cancellable, &error);

      if (!result)
	{
	  g_simple_async_result_set_from_error (res, error);
	  g_error_free (error);
	}
    }
}

static void
g_output_stream_real_close_async (GOutputStream       *stream,
                                  int                  io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GSimpleAsyncResult *res;
  
  res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, g_output_stream_real_close_async);

  g_simple_async_result_set_handle_cancellation (res, FALSE);
  
  g_simple_async_result_run_in_thread (res, close_async_thread, io_priority, cancellable);
  g_object_unref (res);
}

static gboolean
g_output_stream_real_close_finish (GOutputStream  *stream,
                                   GAsyncResult   *result,
                                   GError        **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_output_stream_real_close_async);
  return TRUE;
}
