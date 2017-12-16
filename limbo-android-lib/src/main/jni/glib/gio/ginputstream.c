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
#include <glib.h>
#include "glibintl.h"

#include "ginputstream.h"
#include "gseekable.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "gioerror.h"


/**
 * SECTION:ginputstream
 * @short_description: Base class for implementing streaming input
 * @include: gio/gio.h
 *
 * GInputStream has functions to read from a stream (g_input_stream_read()),
 * to close a stream (g_input_stream_close()) and to skip some content
 * (g_input_stream_skip()). 
 *
 * To copy the content of an input stream to an output stream without 
 * manually handling the reads and writes, use g_output_stream_splice(). 
 *
 * All of these functions have async variants too.
 **/

G_DEFINE_ABSTRACT_TYPE (GInputStream, g_input_stream, G_TYPE_OBJECT);

struct _GInputStreamPrivate {
  guint closed : 1;
  guint pending : 1;
  GAsyncReadyCallback outstanding_callback;
};

static gssize   g_input_stream_real_skip         (GInputStream         *stream,
						  gsize                 count,
						  GCancellable         *cancellable,
						  GError              **error);
static void     g_input_stream_real_read_async   (GInputStream         *stream,
						  void                 *buffer,
						  gsize                 count,
						  int                   io_priority,
						  GCancellable         *cancellable,
						  GAsyncReadyCallback   callback,
						  gpointer              user_data);
static gssize   g_input_stream_real_read_finish  (GInputStream         *stream,
						  GAsyncResult         *result,
						  GError              **error);
static void     g_input_stream_real_skip_async   (GInputStream         *stream,
						  gsize                 count,
						  int                   io_priority,
						  GCancellable         *cancellable,
						  GAsyncReadyCallback   callback,
						  gpointer              data);
static gssize   g_input_stream_real_skip_finish  (GInputStream         *stream,
						  GAsyncResult         *result,
						  GError              **error);
static void     g_input_stream_real_close_async  (GInputStream         *stream,
						  int                   io_priority,
						  GCancellable         *cancellable,
						  GAsyncReadyCallback   callback,
						  gpointer              data);
static gboolean g_input_stream_real_close_finish (GInputStream         *stream,
						  GAsyncResult         *result,
						  GError              **error);

static void
g_input_stream_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_input_stream_parent_class)->finalize (object);
}

static void
g_input_stream_dispose (GObject *object)
{
  GInputStream *stream;

  stream = G_INPUT_STREAM (object);
  
  if (!stream->priv->closed)
    g_input_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (g_input_stream_parent_class)->dispose (object);
}


static void
g_input_stream_class_init (GInputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (GInputStreamPrivate));
  
  gobject_class->finalize = g_input_stream_finalize;
  gobject_class->dispose = g_input_stream_dispose;
  
  klass->skip = g_input_stream_real_skip;
  klass->read_async = g_input_stream_real_read_async;
  klass->read_finish = g_input_stream_real_read_finish;
  klass->skip_async = g_input_stream_real_skip_async;
  klass->skip_finish = g_input_stream_real_skip_finish;
  klass->close_async = g_input_stream_real_close_async;
  klass->close_finish = g_input_stream_real_close_finish;
}

static void
g_input_stream_init (GInputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
					      G_TYPE_INPUT_STREAM,
					      GInputStreamPrivate);
}

/**
 * g_input_stream_read:
 * @stream: a #GInputStream.
 * @buffer: a buffer to read data into (which should be at least count bytes long).
 * @count: the number of bytes that will be read from the stream
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Tries to read @count bytes from the stream into the buffer starting at
 * @buffer. Will block during this read.
 * 
 * If count is zero returns zero and does nothing. A value of @count
 * larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes read into the buffer is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file. Zero is returned on end of file
 * (or if @count is zero),  but never otherwise.
 *
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 * 
 * Return value: Number of bytes read, or -1 on error
 **/
gssize
g_input_stream_read  (GInputStream  *stream,
		      void          *buffer,
		      gsize          count,
		      GCancellable  *cancellable,
		      GError       **error)
{
  GInputStreamClass *class;
  gssize res;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, 0);

  if (count == 0)
    return 0;
  
  if (((gssize) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (class->read_fn == NULL) 
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Input stream doesn't implement read"));
      return -1;
    }

  if (!g_input_stream_set_pending (stream, error))
    return -1;

  if (cancellable)
    g_cancellable_push_current (cancellable);
  
  res = class->read_fn (stream, buffer, count, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);
  
  g_input_stream_clear_pending (stream);

  return res;
}

/**
 * g_input_stream_read_all:
 * @stream: a #GInputStream.
 * @buffer: a buffer to read data into (which should be at least count bytes long).
 * @count: the number of bytes that will be read from the stream
 * @bytes_read: location to store the number of bytes that was read from the stream
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Tries to read @count bytes from the stream into the buffer starting at
 * @buffer. Will block during this read.
 *
 * This function is similar to g_input_stream_read(), except it tries to
 * read as many bytes as requested, only stopping on an error or end of stream.
 *
 * On a successful read of @count bytes, or if we reached the end of the
 * stream,  %TRUE is returned, and @bytes_read is set to the number of bytes
 * read into @buffer.
 * 
 * If there is an error during the operation %FALSE is returned and @error
 * is set to indicate the error status, @bytes_read is updated to contain
 * the number of bytes read into @buffer before the error occurred.
 *
 * Return value: %TRUE on success, %FALSE if there was an error
 **/
gboolean
g_input_stream_read_all (GInputStream  *stream,
			 void          *buffer,
			 gsize          count,
			 gsize         *bytes_read,
			 GCancellable  *cancellable,
			 GError       **error)
{
  gsize _bytes_read;
  gssize res;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);

  _bytes_read = 0;
  while (_bytes_read < count)
    {
      res = g_input_stream_read (stream, (char *)buffer + _bytes_read, count - _bytes_read,
				 cancellable, error);
      if (res == -1)
	{
	  if (bytes_read)
	    *bytes_read = _bytes_read;
	  return FALSE;
	}
      
      if (res == 0)
	break;

      _bytes_read += res;
    }

  if (bytes_read)
    *bytes_read = _bytes_read;
  return TRUE;
}

/**
 * g_input_stream_skip:
 * @stream: a #GInputStream.
 * @count: the number of bytes that will be skipped from the stream
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Tries to skip @count bytes from the stream. Will block during the operation.
 *
 * This is identical to g_input_stream_read(), from a behaviour standpoint,
 * but the bytes that are skipped are not returned to the user. Some
 * streams have an implementation that is more efficient than reading the data.
 *
 * This function is optional for inherited classes, as the default implementation
 * emulates it using read.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * Return value: Number of bytes skipped, or -1 on error
 **/
gssize
g_input_stream_skip (GInputStream  *stream,
		     gsize          count,
		     GCancellable  *cancellable,
		     GError       **error)
{
  GInputStreamClass *class;
  gssize res;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), -1);

  if (count == 0)
    return 0;

  if (((gssize) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }
  
  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (!g_input_stream_set_pending (stream, error))
    return -1;

  if (cancellable)
    g_cancellable_push_current (cancellable);
  
  res = class->skip (stream, count, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);
  
  g_input_stream_clear_pending (stream);

  return res;
}

static gssize
g_input_stream_real_skip (GInputStream  *stream,
			  gsize          count,
			  GCancellable  *cancellable,
			  GError       **error)
{
  GInputStreamClass *class;
  gssize ret, read_bytes;
  char buffer[8192];
  GError *my_error;

  if (G_IS_SEEKABLE (stream) && g_seekable_can_seek (G_SEEKABLE (stream)))
    {
      if (g_seekable_seek (G_SEEKABLE (stream),
			   count,
			   G_SEEK_CUR,
			   cancellable,
			   NULL))
	return count;
    }

  /* If not seekable, or seek failed, fall back to reading data: */

  class = G_INPUT_STREAM_GET_CLASS (stream);

  read_bytes = 0;
  while (1)
    {
      my_error = NULL;

      ret = class->read_fn (stream, buffer, MIN (sizeof (buffer), count),
                            cancellable, &my_error);
      if (ret == -1)
	{
	  if (read_bytes > 0 &&
	      my_error->domain == G_IO_ERROR &&
	      my_error->code == G_IO_ERROR_CANCELLED)
	    {
	      g_error_free (my_error);
	      return read_bytes;
	    }

	  g_propagate_error (error, my_error);
	  return -1;
	}

      count -= ret;
      read_bytes += ret;

      if (ret == 0 || count == 0)
        return read_bytes;
    }
}

/**
 * g_input_stream_close:
 * @stream: A #GInputStream.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it.
 *
 * Once the stream is closed, all other operations will return %G_IO_ERROR_CLOSED.
 * Closing a stream multiple times will not return an error.
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
 * is important to check and report the error to the user.
 *
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but some streams
 * can use a faster close that doesn't block to e.g. check errors. 
 *
 * Return value: %TRUE on success, %FALSE on failure
 **/
gboolean
g_input_stream_close (GInputStream  *stream,
		      GCancellable  *cancellable,
		      GError       **error)
{
  GInputStreamClass *class;
  gboolean res;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (stream->priv->closed)
    return TRUE;

  res = TRUE;

  if (!g_input_stream_set_pending (stream, error))
    return FALSE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  if (class->close_fn)
    res = class->close_fn (stream, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  g_input_stream_clear_pending (stream);
  
  stream->priv->closed = TRUE;
  
  return res;
}

static void
async_ready_callback_wrapper (GObject      *source_object,
			      GAsyncResult *res,
			      gpointer      user_data)
{
  GInputStream *stream = G_INPUT_STREAM (source_object);

  g_input_stream_clear_pending (stream);
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  g_object_unref (stream);
}

static void
async_ready_close_callback_wrapper (GObject      *source_object,
				    GAsyncResult *res,
				    gpointer      user_data)
{
  GInputStream *stream = G_INPUT_STREAM (source_object);

  g_input_stream_clear_pending (stream);
  stream->priv->closed = TRUE;
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  g_object_unref (stream);
}

/**
 * g_input_stream_read_async:
 * @stream: A #GInputStream.
 * @buffer: a buffer to read data into (which should be at least count bytes long).
 * @count: the number of bytes that will be read from the stream
 * @io_priority: the <link linkend="io-priority">I/O priority</link> 
 * of the request. 
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Request an asynchronous read of @count bytes from the stream into the buffer
 * starting at @buffer. When the operation is finished @callback will be called. 
 * You can then call g_input_stream_read_finish() to get the result of the 
 * operation.
 *
 * During an async request no other sync and async calls are allowed on @stream, and will
 * result in %G_IO_ERROR_PENDING errors. 
 *
 * A value of @count larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes read into the buffer will be passed to the
 * callback. It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file, but generally we try to read
 * as many bytes as requested. Zero is returned on end of file
 * (or if @count is zero),  but never otherwise.
 *
 * Any outstanding i/o request with higher priority (lower numerical value) will
 * be executed before an outstanding request with lower priority. Default
 * priority is %G_PRIORITY_DEFAULT.
 *
 * The asyncronous methods have a default fallback that uses threads to implement
 * asynchronicity, so they are optional for inheriting classes. However, if you
 * override one you must override all.
 **/
void
g_input_stream_read_async (GInputStream        *stream,
			   void                *buffer,
			   gsize                count,
			   int                  io_priority,
			   GCancellable        *cancellable,
			   GAsyncReadyCallback  callback,
			   gpointer             user_data)
{
  GInputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL);

  if (count == 0)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_input_stream_read_async);
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

  if (!g_input_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);
  class->read_async (stream, buffer, count, io_priority, cancellable,
		     async_ready_callback_wrapper, user_data);
}

/**
 * g_input_stream_read_finish:
 * @stream: a #GInputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes an asynchronous stream read operation. 
 * 
 * Returns: number of bytes read in, or -1 on error.
 **/
gssize
g_input_stream_read_finish (GInputStream  *stream,
			    GAsyncResult  *result,
			    GError       **error)
{
  GSimpleAsyncResult *simple;
  GInputStreamClass *class;
  
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), -1);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return -1;

      /* Special case read of 0 bytes */
      if (g_simple_async_result_get_source_tag (simple) == g_input_stream_read_async)
	return 0;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->read_finish (stream, result, error);
}

/**
 * g_input_stream_skip_async:
 * @stream: A #GInputStream.
 * @count: the number of bytes that will be skipped from the stream
 * @io_priority: the <link linkend="io-priority">I/O priority</link>
 * of the request.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Request an asynchronous skip of @count bytes from the stream.
 * When the operation is finished @callback will be called.
 * You can then call g_input_stream_skip_finish() to get the result
 * of the operation.
 *
 * During an async request no other sync and async calls are allowed,
 * and will result in %G_IO_ERROR_PENDING errors.
 *
 * A value of @count larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes skipped will be passed to the callback.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file, but generally we try to skip
 * as many bytes as requested. Zero is returned on end of file
 * (or if @count is zero), but never otherwise.
 *
 * Any outstanding i/o request with higher priority (lower numerical value)
 * will be executed before an outstanding request with lower priority.
 * Default priority is %G_PRIORITY_DEFAULT.
 *
 * The asynchronous methods have a default fallback that uses threads to
 * implement asynchronicity, so they are optional for inheriting classes.
 * However, if you override one, you must override all.
 **/
void
g_input_stream_skip_async (GInputStream        *stream,
			   gsize                count,
			   int                  io_priority,
			   GCancellable        *cancellable,
			   GAsyncReadyCallback  callback,
			   gpointer             user_data)
{
  GInputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_INPUT_STREAM (stream));

  if (count == 0)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_input_stream_skip_async);

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

  if (!g_input_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);
  class->skip_async (stream, count, io_priority, cancellable,
		     async_ready_callback_wrapper, user_data);
}

/**
 * g_input_stream_skip_finish:
 * @stream: a #GInputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes a stream skip operation.
 * 
 * Returns: the size of the bytes skipped, or %-1 on error.
 **/
gssize
g_input_stream_skip_finish (GInputStream  *stream,
			    GAsyncResult  *result,
			    GError       **error)
{
  GSimpleAsyncResult *simple;
  GInputStreamClass *class;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), -1);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return -1;

      /* Special case skip of 0 bytes */
      if (g_simple_async_result_get_source_tag (simple) == g_input_stream_skip_async)
	return 0;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->skip_finish (stream, result, error);
}

/**
 * g_input_stream_close_async:
 * @stream: A #GInputStream.
 * @io_priority: the <link linkend="io-priority">I/O priority</link> 
 * of the request. 
 * @cancellable: optional cancellable object
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Requests an asynchronous closes of the stream, releasing resources related to it.
 * When the operation is finished @callback will be called. 
 * You can then call g_input_stream_close_finish() to get the result of the 
 * operation.
 *
 * For behaviour details see g_input_stream_close().
 *
 * The asyncronous methods have a default fallback that uses threads to implement
 * asynchronicity, so they are optional for inheriting classes. However, if you
 * override one you must override all.
 **/
void
g_input_stream_close_async (GInputStream        *stream,
			    int                  io_priority,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
  GInputStreamClass *class;
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  g_return_if_fail (G_IS_INPUT_STREAM (stream));

  if (stream->priv->closed)
    {
      simple = g_simple_async_result_new (G_OBJECT (stream),
					  callback,
					  user_data,
					  g_input_stream_close_async);

      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  if (!g_input_stream_set_pending (stream, &error))
    {
      g_simple_async_report_gerror_in_idle (G_OBJECT (stream),
					    callback,
					    user_data,
					    error);
      g_error_free (error);
      return;
    }
  
  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  g_object_ref (stream);
  class->close_async (stream, io_priority, cancellable,
		      async_ready_close_callback_wrapper, user_data);
}

/**
 * g_input_stream_close_finish:
 * @stream: a #GInputStream.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes closing a stream asynchronously, started from g_input_stream_close_async().
 * 
 * Returns: %TRUE if the stream was closed successfully.
 **/
gboolean
g_input_stream_close_finish (GInputStream  *stream,
			     GAsyncResult  *result,
			     GError       **error)
{
  GSimpleAsyncResult *simple;
  GInputStreamClass *class;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      simple = G_SIMPLE_ASYNC_RESULT (result);
      if (g_simple_async_result_propagate_error (simple, error))
	return FALSE;

      /* Special case already closed */
      if (g_simple_async_result_get_source_tag (simple) == g_input_stream_close_async)
	return TRUE;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->close_finish (stream, result, error);
}

/**
 * g_input_stream_is_closed:
 * @stream: input stream.
 * 
 * Checks if an input stream is closed.
 * 
 * Returns: %TRUE if the stream is closed.
 **/
gboolean
g_input_stream_is_closed (GInputStream *stream)
{
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), TRUE);
  
  return stream->priv->closed;
}
 
/**
 * g_input_stream_has_pending:
 * @stream: input stream.
 * 
 * Checks if an input stream has pending actions.
 * 
 * Returns: %TRUE if @stream has pending actions.
 **/  
gboolean
g_input_stream_has_pending (GInputStream *stream)
{
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), TRUE);
  
  return stream->priv->pending;
}

/**
 * g_input_stream_set_pending:
 * @stream: input stream
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
g_input_stream_set_pending (GInputStream *stream, GError **error)
{
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
  
  if (stream->priv->closed)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Stream is already closed"));
      return FALSE;
    }
  
  if (stream->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
		/* Translators: This is an error you get if there is already an
		 * operation running against this stream when you try to start
		 * one */
		 _("Stream has outstanding operation"));
      return FALSE;
    }
  
  stream->priv->pending = TRUE;
  return TRUE;
}

/**
 * g_input_stream_clear_pending:
 * @stream: input stream
 * 
 * Clears the pending flag on @stream.
 **/
void
g_input_stream_clear_pending (GInputStream *stream)
{
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  
  stream->priv->pending = FALSE;
}

/********************************************
 *   Default implementation of async ops    *
 ********************************************/

typedef struct {
  void              *buffer;
  gsize              count_requested;
  gssize             count_read;
} ReadData;

static void
read_async_thread (GSimpleAsyncResult *res,
		   GObject            *object,
		   GCancellable       *cancellable)
{
  ReadData *op;
  GInputStreamClass *class;
  GError *error = NULL;
 
  op = g_simple_async_result_get_op_res_gpointer (res);

  class = G_INPUT_STREAM_GET_CLASS (object);

  op->count_read = class->read_fn (G_INPUT_STREAM (object),
				   op->buffer, op->count_requested,
				   cancellable, &error);
  if (op->count_read == -1)
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
}

static void
g_input_stream_real_read_async (GInputStream        *stream,
				void                *buffer,
				gsize                count,
				int                  io_priority,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
  GSimpleAsyncResult *res;
  ReadData *op;
  
  op = g_new (ReadData, 1);
  res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, g_input_stream_real_read_async);
  g_simple_async_result_set_op_res_gpointer (res, op, g_free);
  op->buffer = buffer;
  op->count_requested = count;
  
  g_simple_async_result_run_in_thread (res, read_async_thread, io_priority, cancellable);
  g_object_unref (res);
}

static gssize
g_input_stream_real_read_finish (GInputStream  *stream,
				 GAsyncResult  *result,
				 GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  ReadData *op;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == 
	    g_input_stream_real_read_async);

  op = g_simple_async_result_get_op_res_gpointer (simple);

  return op->count_read;
}

typedef struct {
  gsize count_requested;
  gssize count_skipped;
} SkipData;


static void
skip_async_thread (GSimpleAsyncResult *res,
		   GObject            *object,
		   GCancellable       *cancellable)
{
  SkipData *op;
  GInputStreamClass *class;
  GError *error = NULL;
  
  class = G_INPUT_STREAM_GET_CLASS (object);
  op = g_simple_async_result_get_op_res_gpointer (res);
  op->count_skipped = class->skip (G_INPUT_STREAM (object),
				   op->count_requested,
				   cancellable, &error);
  if (op->count_skipped == -1)
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
}

typedef struct {
  char buffer[8192];
  gsize count;
  gsize count_skipped;
  int io_prio;
  GCancellable *cancellable;
  gpointer user_data;
  GAsyncReadyCallback callback;
} SkipFallbackAsyncData;

static void
skip_callback_wrapper (GObject      *source_object,
		       GAsyncResult *res,
		       gpointer      user_data)
{
  GInputStreamClass *class;
  SkipFallbackAsyncData *data = user_data;
  SkipData *op;
  GSimpleAsyncResult *simple;
  GError *error = NULL;
  gssize ret;

  ret = g_input_stream_read_finish (G_INPUT_STREAM (source_object), res, &error);

  if (ret > 0)
    {
      data->count -= ret;
      data->count_skipped += ret;

      if (data->count > 0)
	{
	  class = G_INPUT_STREAM_GET_CLASS (source_object);
	  class->read_async (G_INPUT_STREAM (source_object), data->buffer, MIN (8192, data->count), data->io_prio, data->cancellable,
			     skip_callback_wrapper, data);
	  return;
	}
    }

  op = g_new0 (SkipData, 1);
  op->count_skipped = data->count_skipped;
  simple = g_simple_async_result_new (source_object,
				      data->callback, data->user_data,
				      g_input_stream_real_skip_async);

  g_simple_async_result_set_op_res_gpointer (simple, op, g_free);

  if (ret == -1)
    {
      if (data->count_skipped && 
	  error->domain == G_IO_ERROR &&
	  error->code == G_IO_ERROR_CANCELLED)
	{ /* No error, return partial read */ }
      else
	g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  /* Complete immediately, not in idle, since we're already in a mainloop callout */
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
  
  g_free (data);
 }

static void
g_input_stream_real_skip_async (GInputStream        *stream,
				gsize                count,
				int                  io_priority,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
  GInputStreamClass *class;
  SkipData *op;
  SkipFallbackAsyncData *data;
  GSimpleAsyncResult *res;

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (class->read_async == g_input_stream_real_read_async)
    {
      /* Read is thread-using async fallback.
       * Make skip use threads too, so that we can use a possible sync skip
       * implementation. */
      op = g_new0 (SkipData, 1);
      
      res = g_simple_async_result_new (G_OBJECT (stream), callback, user_data,
				       g_input_stream_real_skip_async);

      g_simple_async_result_set_op_res_gpointer (res, op, g_free);

      op->count_requested = count;

      g_simple_async_result_run_in_thread (res, skip_async_thread, io_priority, cancellable);
      g_object_unref (res);
    }
  else
    {
      /* TODO: Skip fallback uses too much memory, should do multiple read calls */
      
      /* There is a custom async read function, lets use that. */
      data = g_new (SkipFallbackAsyncData, 1);
      data->count = count;
      data->count_skipped = 0;
      data->io_prio = io_priority;
      data->cancellable = cancellable;
      data->callback = callback;
      data->user_data = user_data;
      class->read_async (stream, data->buffer, MIN (8192, count), io_priority, cancellable,
			 skip_callback_wrapper, data);
    }

}

static gssize
g_input_stream_real_skip_finish (GInputStream  *stream,
				 GAsyncResult  *result,
				 GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  SkipData *op;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_input_stream_real_skip_async);
  op = g_simple_async_result_get_op_res_gpointer (simple);
  return op->count_skipped;
}

static void
close_async_thread (GSimpleAsyncResult *res,
		    GObject            *object,
		    GCancellable       *cancellable)
{
  GInputStreamClass *class;
  GError *error = NULL;
  gboolean result;

  /* Auto handling of cancelation disabled, and ignore
     cancellation, since we want to close things anyway, although
     possibly in a quick-n-dirty way. At least we never want to leak
     open handles */

  class = G_INPUT_STREAM_GET_CLASS (object);
  if (class->close_fn)
    {
      result = class->close_fn (G_INPUT_STREAM (object), cancellable, &error);
      if (!result)
	{
	  g_simple_async_result_set_from_error (res, error);
	  g_error_free (error);
	}
    }
}

static void
g_input_stream_real_close_async (GInputStream        *stream,
				 int                  io_priority,
				 GCancellable        *cancellable,
				 GAsyncReadyCallback  callback,
				 gpointer             user_data)
{
  GSimpleAsyncResult *res;
  
  res = g_simple_async_result_new (G_OBJECT (stream),
				   callback,
				   user_data,
				   g_input_stream_real_close_async);

  g_simple_async_result_set_handle_cancellation (res, FALSE);
  
  g_simple_async_result_run_in_thread (res,
				       close_async_thread,
				       io_priority,
				       cancellable);
  g_object_unref (res);
}

static gboolean
g_input_stream_real_close_finish (GInputStream  *stream,
				  GAsyncResult  *result,
				  GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_input_stream_real_close_async);
  return TRUE;
}
