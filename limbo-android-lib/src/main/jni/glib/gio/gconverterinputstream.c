/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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

#include <string.h>

#include "gconverterinputstream.h"
#include "gsimpleasyncresult.h"
#include "gcancellable.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gconverterinputstream
 * @short_description: Converter Input Stream
 * @include: gio/gio.h
 * @see_also: #GInputStream, #GConverter
 *
 * Converter input stream implements #GInputStream and allows
 * conversion of data of various types during reading.
 *
 **/

#define INITIAL_BUFFER_SIZE 4096

typedef struct {
  char *data;
  gsize start;
  gsize end;
  gsize size;
} Buffer;

struct _GConverterInputStreamPrivate {
  gboolean at_input_end;
  gboolean finished;
  GConverter *converter;
  Buffer input_buffer;
  Buffer converted_buffer;
};

enum {
  PROP_0,
  PROP_CONVERTER
};

static void   g_converter_input_stream_set_property (GObject       *object,
						     guint          prop_id,
						     const GValue  *value,
						     GParamSpec    *pspec);
static void   g_converter_input_stream_get_property (GObject       *object,
						     guint          prop_id,
						     GValue        *value,
						     GParamSpec    *pspec);
static void   g_converter_input_stream_finalize     (GObject       *object);
static gssize g_converter_input_stream_read         (GInputStream  *stream,
						     void          *buffer,
						     gsize          count,
						     GCancellable  *cancellable,
						     GError       **error);

G_DEFINE_TYPE (GConverterInputStream,
	       g_converter_input_stream,
	       G_TYPE_FILTER_INPUT_STREAM)

static void
g_converter_input_stream_class_init (GConverterInputStreamClass *klass)
{
  GObjectClass *object_class;
  GInputStreamClass *istream_class;

  g_type_class_add_private (klass, sizeof (GConverterInputStreamPrivate));

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_converter_input_stream_get_property;
  object_class->set_property = g_converter_input_stream_set_property;
  object_class->finalize     = g_converter_input_stream_finalize;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn = g_converter_input_stream_read;

  g_object_class_install_property (object_class,
				   PROP_CONVERTER,
				   g_param_spec_object ("converter",
							P_("Converter"),
							P_("The converter object"),
							G_TYPE_CONVERTER,
							G_PARAM_READWRITE|
							G_PARAM_CONSTRUCT_ONLY|
							G_PARAM_STATIC_STRINGS));

}

static void
g_converter_input_stream_finalize (GObject *object)
{
  GConverterInputStreamPrivate *priv;
  GConverterInputStream        *stream;

  stream = G_CONVERTER_INPUT_STREAM (object);
  priv = stream->priv;

  g_free (priv->input_buffer.data);
  g_free (priv->converted_buffer.data);
  if (priv->converter)
    g_object_unref (priv->converter);

  G_OBJECT_CLASS (g_converter_input_stream_parent_class)->finalize (object);
}

static void
g_converter_input_stream_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  GConverterInputStream *cstream;

  cstream = G_CONVERTER_INPUT_STREAM (object);

   switch (prop_id)
    {
    case PROP_CONVERTER:
      cstream->priv->converter = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_converter_input_stream_get_property (GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec)
{
  GConverterInputStreamPrivate *priv;
  GConverterInputStream        *cstream;

  cstream = G_CONVERTER_INPUT_STREAM (object);
  priv = cstream->priv;

  switch (prop_id)
    {
    case PROP_CONVERTER:
      g_value_set_object (value, priv->converter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}
static void
g_converter_input_stream_init (GConverterInputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
					      G_TYPE_CONVERTER_INPUT_STREAM,
					      GConverterInputStreamPrivate);
}

/**
 * g_converter_input_stream_new:
 * @base_stream: a #GInputStream
 * @converter: a #GConverter
 *
 * Creates a new converter input stream for the @base_stream.
 *
 * Returns: a new #GInputStream.
 **/
GInputStream *
g_converter_input_stream_new (GInputStream *base_stream,
                              GConverter   *converter)
{
  GInputStream *stream;

  g_return_val_if_fail (G_IS_INPUT_STREAM (base_stream), NULL);

  stream = g_object_new (G_TYPE_CONVERTER_INPUT_STREAM,
                         "base-stream", base_stream,
			 "converter", converter,
			 NULL);

  return stream;
}

static gsize
buffer_data_size (Buffer *buffer)
{
  return buffer->end - buffer->start;
}

static gsize
buffer_tailspace (Buffer *buffer)
{
  return buffer->size - buffer->end;
}

static char *
buffer_data (Buffer *buffer)
{
  return buffer->data + buffer->start;
}

static void
buffer_consumed (Buffer *buffer,
		 gsize count)
{
  buffer->start += count;
  if (buffer->start == buffer->end)
    buffer->start = buffer->end = 0;
}

static void
buffer_read (Buffer *buffer,
	     char *dest,
	     gsize count)
{
  memcpy (dest, buffer->data + buffer->start, count);
  buffer_consumed (buffer, count);
}

static void
compact_buffer (Buffer *buffer)
{
  gsize in_buffer;

  in_buffer = buffer_data_size (buffer);
  memmove (buffer->data,
	   buffer->data + buffer->start,
	   in_buffer);
  buffer->end -= buffer->start;
  buffer->start = 0;
}

static void
grow_buffer (Buffer *buffer)
{
  char *data;
  gsize size, in_buffer;

  if (buffer->size == 0)
    size = INITIAL_BUFFER_SIZE;
  else
    size = buffer->size * 2;

  data = g_malloc (size);
  in_buffer = buffer_data_size (buffer);

  memcpy (data,
	  buffer->data + buffer->start,
	  in_buffer);
  g_free (buffer->data);
  buffer->data = data;
  buffer->end -= buffer->start;
  buffer->start = 0;
  buffer->size = size;
}

/* Ensures that the buffer can fit at_least_size bytes,
 * *including* the current in-buffer data */
static void
buffer_ensure_space (Buffer *buffer,
		     gsize at_least_size)
{
  gsize in_buffer, left_to_fill;

  in_buffer = buffer_data_size (buffer);

  if (in_buffer >= at_least_size)
    return;

  left_to_fill = buffer_tailspace (buffer);

  if (in_buffer + left_to_fill >= at_least_size)
    {
      /* We fit in remaining space at end */
      /* If the copy is small, compact now anyway so we can fill more */
      if (in_buffer < 256)
	compact_buffer (buffer);
    }
  else if (buffer->size >= at_least_size)
    {
      /* We fit, but only if we compact */
      compact_buffer (buffer);
    }
  else
    {
      /* Need to grow buffer */
      while (buffer->size < at_least_size)
	grow_buffer (buffer);
    }
}

static gssize
fill_input_buffer (GConverterInputStream  *stream,
		   gsize                   at_least_size,
		   GCancellable           *cancellable,
		   GError                **error)
{
  GConverterInputStreamPrivate *priv;
  GInputStream *base_stream;
  gssize nread;

  priv = stream->priv;

  buffer_ensure_space (&priv->input_buffer, at_least_size);

  base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  nread = g_input_stream_read (base_stream,
			       priv->input_buffer.data + priv->input_buffer.end,
			       buffer_tailspace (&priv->input_buffer),
			       cancellable,
			       error);

  if (nread > 0)
    priv->input_buffer.end += nread;

  return nread;
}


static gssize
g_converter_input_stream_read (GInputStream *stream,
			       void         *buffer,
			       gsize         count,
			       GCancellable *cancellable,
			       GError      **error)
{
  GConverterInputStream *cstream;
  GConverterInputStreamPrivate *priv;
  gsize available, total_bytes_read;
  gssize nread;
  GConverterResult res;
  gsize bytes_read;
  gsize bytes_written;
  GError *my_error;
  GError *my_error2;

  cstream = G_CONVERTER_INPUT_STREAM (stream);
  priv = cstream->priv;

  available = buffer_data_size (&priv->converted_buffer);

  if (available > 0 &&
      count <= available)
    {
      /* Converted data available, return that */
      buffer_read (&priv->converted_buffer, buffer, count);
      return count;
    }

  /* Full request not available, read all currently available and request
     refill/conversion for more */

  buffer_read (&priv->converted_buffer, buffer, available);

  total_bytes_read = available;
  count -= available;

  /* If there is no data to convert, and no pre-converted data,
     do some i/o for more input */
  if (buffer_data_size (&priv->input_buffer) == 0 &&
      total_bytes_read == 0 &&
      !priv->at_input_end)
    {
      nread = fill_input_buffer (cstream, count, cancellable, error);
      if (nread < 0)
	return -1;
      if (nread == 0)
	priv->at_input_end = TRUE;
    }

  /* First try to convert any available data (or state) directly to the user buffer: */
  if (!priv->finished)
    {
      my_error = NULL;
      res = g_converter_convert (priv->converter,
				 buffer_data (&priv->input_buffer),
				 buffer_data_size (&priv->input_buffer),
				 buffer, count,
				 priv->at_input_end ? G_CONVERTER_INPUT_AT_END : 0,
				 &bytes_read,
				 &bytes_written,
				 &my_error);
      if (res != G_CONVERTER_ERROR)
	{
	  total_bytes_read += bytes_written;
	  buffer_consumed (&priv->input_buffer, bytes_read);
	  if (res == G_CONVERTER_FINISHED)
	    priv->finished = TRUE; /* We're done converting */
	}
      else if (total_bytes_read == 0 &&
	       !g_error_matches (my_error,
				 G_IO_ERROR,
				 G_IO_ERROR_PARTIAL_INPUT) &&
	       !g_error_matches (my_error,
				 G_IO_ERROR,
				 G_IO_ERROR_NO_SPACE))
	{
	  /* No previously read data and no "special" error, return error */
	  g_propagate_error (error, my_error);
	  return -1;
	}
      else
	g_error_free (my_error);
    }

  /* We had some pre-converted data and/or we converted directly to the
     user buffer */
  if (total_bytes_read > 0)
    return total_bytes_read;

  /* If there is no more to convert, return EOF */
  if (priv->finished)
    {
      g_assert (buffer_data_size (&priv->converted_buffer) == 0);
      return 0;
    }

  /* There was "complexity" in the straight-to-buffer conversion,
   * convert to our own buffer and write from that.
   * At this point we didn't produce any data into @buffer.
   */

  /* Ensure we have *some* initial target space */
  buffer_ensure_space (&priv->converted_buffer, count);

  while (TRUE)
    {
      g_assert (!priv->finished);

      /* Try to convert to our buffer */
      my_error = NULL;
      res = g_converter_convert (priv->converter,
				 buffer_data (&priv->input_buffer),
				 buffer_data_size (&priv->input_buffer),
				 buffer_data (&priv->converted_buffer),
				 buffer_tailspace (&priv->converted_buffer),
				 priv->at_input_end ? G_CONVERTER_INPUT_AT_END : 0,
				 &bytes_read,
				 &bytes_written,
				 &my_error);
      if (res != G_CONVERTER_ERROR)
	{
	  priv->converted_buffer.end += bytes_written;
	  buffer_consumed (&priv->input_buffer, bytes_read);

	  /* Maybe we consumed without producing any output */
	  if (buffer_data_size (&priv->converted_buffer) == 0 && res != G_CONVERTER_FINISHED)
	    continue; /* Convert more */

	  if (res == G_CONVERTER_FINISHED)
	    priv->finished = TRUE;

	  total_bytes_read = MIN (count, buffer_data_size (&priv->converted_buffer));
	  buffer_read (&priv->converted_buffer, buffer, total_bytes_read);

	  g_assert (priv->finished || total_bytes_read > 0);

	  return total_bytes_read;
	}

      /* There was some kind of error filling our buffer */

      if (g_error_matches (my_error,
			   G_IO_ERROR,
			   G_IO_ERROR_PARTIAL_INPUT) &&
	  !priv->at_input_end)
	{
	  /* Need more data */
	  my_error2 = NULL;
	  res = fill_input_buffer (cstream,
				   buffer_data_size (&priv->input_buffer) + 4096,
				   cancellable,
				   &my_error2);
	  if (res < 0)
	    {
	      /* Can't read any more data, return that error */
	      g_error_free (my_error);
	      g_propagate_error (error, my_error2);
	      return -1;
	    }
	  else if (res == 0)
	    {
	      /* End of file, try INPUT_AT_END */
	      priv->at_input_end = TRUE;
	    }
	  g_error_free (my_error);
	  continue;
	}

      if (g_error_matches (my_error,
			   G_IO_ERROR,
			   G_IO_ERROR_NO_SPACE))
	{
	  /* Need more destination space, grow it
	   * Note: if we actually grow the buffer (as opposed to compacting it),
	   * this will double the size, not just add one byte. */
	  buffer_ensure_space (&priv->converted_buffer,
			       priv->converted_buffer.size + 1);
	  g_error_free (my_error);
	  continue;
	}

      /* Any other random error, return it */
      g_propagate_error (error, my_error);
      return -1;
    }

  g_assert_not_reached ();
}

/**
 * g_converter_input_stream_get_converter:
 * @converter_stream: a #GConverterInputStream
 *
 * Gets the #GConverter that is used by @converter_stream.
 *
 * Returns: the converter of the converter input stream
 *
 * Since: 2.24
 */
GConverter *
g_converter_input_stream_get_converter (GConverterInputStream *converter_stream)
{
  return converter_stream->priv->converter;
}
