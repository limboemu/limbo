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
#include <string.h>
#include "gdataoutputstream.h"
#include "gioenumtypes.h"
#include "glibintl.h"


/**
 * SECTION:gdataoutputstream
 * @short_description: Data Output Stream
 * @include: gio/gio.h
 * @see_also: #GOutputStream
 * 
 * Data output stream implements #GOutputStream and includes functions for 
 * writing data directly to an output stream.
 *
 **/



struct _GDataOutputStreamPrivate {
  GDataStreamByteOrder byte_order;
};

enum {
  PROP_0,
  PROP_BYTE_ORDER
};

static void g_data_output_stream_set_property (GObject      *object,
					       guint         prop_id,
					       const GValue *value,
					       GParamSpec   *pspec);
static void g_data_output_stream_get_property (GObject      *object,
					       guint         prop_id,
					       GValue       *value,
					       GParamSpec   *pspec);

G_DEFINE_TYPE (GDataOutputStream,
               g_data_output_stream,
               G_TYPE_FILTER_OUTPUT_STREAM)


static void
g_data_output_stream_class_init (GDataOutputStreamClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private (klass, sizeof (GDataOutputStreamPrivate));

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_data_output_stream_get_property;
  object_class->set_property = g_data_output_stream_set_property;

  /**
   * GDataOutputStream:byte-order:
   *
   * Determines the byte ordering that is used when writing 
   * multi-byte entities (such as integers) to the stream.
   */
  g_object_class_install_property (object_class,
                                   PROP_BYTE_ORDER,
                                   g_param_spec_enum ("byte-order",
                                                      P_("Byte order"),
                                                      P_("The byte order"),
                                                      G_TYPE_DATA_STREAM_BYTE_ORDER,
                                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN,
                                                      G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_BLURB));

}

static void
g_data_output_stream_set_property (GObject     *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  GDataOutputStream *dstream;

  dstream = G_DATA_OUTPUT_STREAM (object);

  switch (prop_id) 
    {
    case PROP_BYTE_ORDER:
      g_data_output_stream_set_byte_order (dstream, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_data_output_stream_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  GDataOutputStreamPrivate *priv;
  GDataOutputStream        *dstream;

  dstream = G_DATA_OUTPUT_STREAM (object);
  priv = dstream->priv;

  switch (prop_id)
    {
    case PROP_BYTE_ORDER:
      g_value_set_enum (value, priv->byte_order);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_data_output_stream_init (GDataOutputStream *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
                                              G_TYPE_DATA_OUTPUT_STREAM,
                                              GDataOutputStreamPrivate);

  stream->priv->byte_order = G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
}

/**
 * g_data_output_stream_new:
 * @base_stream: a #GOutputStream.
 * 
 * Creates a new data output stream for @base_stream.
 * 
 * Returns: #GDataOutputStream.
 **/
GDataOutputStream *
g_data_output_stream_new (GOutputStream *base_stream)
{
  GDataOutputStream *stream;

  g_return_val_if_fail (G_IS_OUTPUT_STREAM (base_stream), NULL);

  stream = g_object_new (G_TYPE_DATA_OUTPUT_STREAM,
                         "base-stream", base_stream,
                         NULL);

  return stream;
}

/**
 * g_data_output_stream_set_byte_order:
 * @stream: a #GDataOutputStream.
 * @order: a %GDataStreamByteOrder.
 * 
 * Sets the byte order of the data output stream to @order.
 **/
void
g_data_output_stream_set_byte_order (GDataOutputStream    *stream,
                                     GDataStreamByteOrder  order)
{
  GDataOutputStreamPrivate *priv;
  g_return_if_fail (G_IS_DATA_OUTPUT_STREAM (stream));
  priv = stream->priv;
  if (priv->byte_order != order)
    {
      priv->byte_order = order;
      g_object_notify (G_OBJECT (stream), "byte-order");
    }
}

/**
 * g_data_output_stream_get_byte_order:
 * @stream: a #GDataOutputStream.
 * 
 * Gets the byte order for the stream.
 * 
 * Returns: the #GDataStreamByteOrder for the @stream.
 **/
GDataStreamByteOrder
g_data_output_stream_get_byte_order (GDataOutputStream *stream)
{
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN);

  return stream->priv->byte_order;
}

/**
 * g_data_output_stream_put_byte:
 * @stream: a #GDataOutputStream.
 * @data: a #guchar.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts a byte into the output stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_byte (GDataOutputStream  *stream,
			       guchar              data,
			       GCancellable       *cancellable,
			       GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 1,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_int16:
 * @stream: a #GDataOutputStream.
 * @data: a #gint16.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts a signed 16-bit integer into the output stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_int16 (GDataOutputStream  *stream,
				gint16              data,
				GCancellable       *cancellable,
				GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT16_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT16_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 2,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_uint16:
 * @stream: a #GDataOutputStream.
 * @data: a #guint16.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts an unsigned 16-bit integer into the output stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_uint16 (GDataOutputStream  *stream,
				 guint16             data,
				 GCancellable       *cancellable,
				 GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT16_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT16_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 2,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_int32:
 * @stream: a #GDataOutputStream.
 * @data: a #gint32.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts a signed 32-bit integer into the output stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_int32 (GDataOutputStream  *stream,
				gint32              data,
				GCancellable       *cancellable,
				GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT32_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT32_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 4,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_uint32:
 * @stream: a #GDataOutputStream.
 * @data: a #guint32.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts an unsigned 32-bit integer into the stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_uint32 (GDataOutputStream  *stream,
				 guint32             data,
				 GCancellable       *cancellable,
				 GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT32_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT32_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 4,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_int64:
 * @stream: a #GDataOutputStream.
 * @data: a #gint64.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts a signed 64-bit integer into the stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_int64 (GDataOutputStream  *stream,
				gint64              data,
				GCancellable       *cancellable,
				GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT64_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT64_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 8,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_uint64:
 * @stream: a #GDataOutputStream.
 * @data: a #guint64.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts an unsigned 64-bit integer into the stream.
 * 
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_uint64 (GDataOutputStream  *stream,
				 guint64             data,
				 GCancellable       *cancellable,
				 GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT64_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT64_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }
  
  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 8,
				    &bytes_written,
				    cancellable, error);
}

/**
 * g_data_output_stream_put_string:
 * @stream: a #GDataOutputStream.
 * @str: a string.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError, %NULL to ignore.
 * 
 * Puts a string into the output stream. 
 * 
 * Returns: %TRUE if @string was successfully added to the @stream.
 **/
gboolean
g_data_output_stream_put_string (GDataOutputStream  *stream,
				 const char         *str,
				 GCancellable       *cancellable,
				 GError            **error)
{
  gsize bytes_written;
  
  g_return_val_if_fail (G_IS_DATA_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  return g_output_stream_write_all (G_OUTPUT_STREAM (stream),
				    str, strlen (str),
				    &bytes_written,
				    cancellable, error);
}
