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
 * Author: Christian Kellner <gicmo@gnome.org> 
 */

#include "config.h"
#include "gfilterinputstream.h"
#include "ginputstream.h"
#include "gsimpleasyncresult.h"
#include "glibintl.h"


/**
 * SECTION:gfilterinputstream
 * @short_description: Filter Input Stream
 * @include: gio/gio.h
 *
 * Base class for input stream implementations that perform some
 * kind of filtering operation on a base stream. Typical examples
 * of filtering operations are character set conversion, compression
 * and byte order flipping.
 **/

enum {
  PROP_0,
  PROP_BASE_STREAM,
  PROP_CLOSE_BASE
};

static void     g_filter_input_stream_set_property (GObject      *object,
                                                    guint         prop_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);

static void     g_filter_input_stream_get_property (GObject      *object,
                                                    guint         prop_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void     g_filter_input_stream_finalize     (GObject *object);


static gssize   g_filter_input_stream_read         (GInputStream         *stream,
                                                    void                 *buffer,
                                                    gsize                 count,
                                                    GCancellable         *cancellable,
                                                    GError              **error);
static gssize   g_filter_input_stream_skip         (GInputStream         *stream,
                                                    gsize                 count,
                                                    GCancellable         *cancellable,
                                                    GError              **error);
static gboolean g_filter_input_stream_close        (GInputStream         *stream,
                                                    GCancellable         *cancellable,
                                                    GError              **error);

G_DEFINE_ABSTRACT_TYPE (GFilterInputStream, g_filter_input_stream, G_TYPE_INPUT_STREAM)

#define GET_PRIVATE(inst) G_TYPE_INSTANCE_GET_PRIVATE (inst, \
  G_TYPE_FILTER_INPUT_STREAM, GFilterInputStreamPrivate)

typedef struct
{
  gboolean close_base;
  GAsyncReadyCallback outstanding_callback;
  gpointer outstanding_user_data;
} GFilterInputStreamPrivate;

static void
g_filter_input_stream_class_init (GFilterInputStreamClass *klass)
{
  GObjectClass *object_class;
  GInputStreamClass *istream_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_filter_input_stream_get_property;
  object_class->set_property = g_filter_input_stream_set_property;
  object_class->finalize     = g_filter_input_stream_finalize;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn  = g_filter_input_stream_read;
  istream_class->skip  = g_filter_input_stream_skip;
  istream_class->close_fn = g_filter_input_stream_close;

  g_type_class_add_private (klass, sizeof (GFilterInputStreamPrivate));

  g_object_class_install_property (object_class,
                                   PROP_BASE_STREAM,
                                   g_param_spec_object ("base-stream",
                                                         P_("The Filter Base Stream"),
                                                         P_("The underlying base stream on which the io ops will be done."),
                                                         G_TYPE_INPUT_STREAM,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | 
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_CLOSE_BASE,
                                   g_param_spec_boolean ("close-base-stream",
                                                         P_("Close Base Stream"),
                                                         P_("If the base stream should be closed when the filter stream is closed."),
                                                         TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | 
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

static void
g_filter_input_stream_set_property (GObject         *object,
                                    guint            prop_id,
                                    const GValue    *value,
                                    GParamSpec      *pspec)
{
  GFilterInputStream *filter_stream;
  GObject *obj;

  filter_stream = G_FILTER_INPUT_STREAM (object);

  switch (prop_id) 
    {
    case PROP_BASE_STREAM:
      obj = g_value_dup_object (value);
      filter_stream->base_stream = G_INPUT_STREAM (obj); 
      break;

    case PROP_CLOSE_BASE:
      g_filter_input_stream_set_close_base_stream (filter_stream,
                                                   g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_input_stream_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GFilterInputStream *filter_stream;

  filter_stream = G_FILTER_INPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BASE_STREAM:
      g_value_set_object (value, filter_stream->base_stream);
      break;

    case PROP_CLOSE_BASE:
      g_value_set_boolean (value, GET_PRIVATE (filter_stream)->close_base);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_input_stream_finalize (GObject *object)
{
  GFilterInputStream *stream;

  stream = G_FILTER_INPUT_STREAM (object);

  g_object_unref (stream->base_stream);

  G_OBJECT_CLASS (g_filter_input_stream_parent_class)->finalize (object);
}

static void
g_filter_input_stream_init (GFilterInputStream *stream)
{

}

/**
 * g_filter_input_stream_get_base_stream:
 * @stream: a #GFilterInputStream.
 * 
 * Gets the base stream for the filter stream.
 *
 * Returns: (transfer none): a #GInputStream.
 **/
GInputStream *
g_filter_input_stream_get_base_stream (GFilterInputStream *stream)
{
  g_return_val_if_fail (G_IS_FILTER_INPUT_STREAM (stream), NULL);

  return stream->base_stream;
}

/**
 * g_filter_input_stream_get_close_base_stream:
 * @stream: a #GFilterInputStream.
 *
 * Returns whether the base stream will be closed when @stream is
 * closed.
 *
 * Return value: %TRUE if the base stream will be closed.
 **/
gboolean
g_filter_input_stream_get_close_base_stream (GFilterInputStream *stream)
{
  g_return_val_if_fail (G_IS_FILTER_INPUT_STREAM (stream), FALSE);

  return GET_PRIVATE (stream)->close_base;
}

/**
 * g_filter_input_stream_set_close_base_stream:
 * @stream: a #GFilterInputStream.
 * @close_base: %TRUE to close the base stream.
 *
 * Sets whether the base stream will be closed when @stream is closed.
 **/
void
g_filter_input_stream_set_close_base_stream (GFilterInputStream *stream,
                                             gboolean            close_base)
{
  GFilterInputStreamPrivate *priv;

  g_return_if_fail (G_IS_FILTER_INPUT_STREAM (stream));

  close_base = !!close_base;
 
  priv = GET_PRIVATE (stream);

  if (priv->close_base != close_base)
    {
      priv->close_base = close_base;
      g_object_notify (G_OBJECT (stream), "close-base-stream");
    }
}

static gssize
g_filter_input_stream_read (GInputStream  *stream,
                            void          *buffer,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GFilterInputStream *filter_stream;
  GInputStream       *base_stream;
  gssize              nread;

  filter_stream = G_FILTER_INPUT_STREAM (stream);
  base_stream = filter_stream->base_stream;

  nread = g_input_stream_read (base_stream,
                               buffer,
                               count,
                               cancellable,
                               error);

  return nread;
}

static gssize
g_filter_input_stream_skip (GInputStream  *stream,
                            gsize          count,
                            GCancellable  *cancellable,
                            GError       **error)
{
  GFilterInputStream *filter_stream;
  GInputStream       *base_stream;
  gssize              nskipped;

  filter_stream = G_FILTER_INPUT_STREAM (stream);
  base_stream = filter_stream->base_stream;

  nskipped = g_input_stream_skip (base_stream,
                                  count,
                                  cancellable,
                                  error);
  return nskipped;
}

static gboolean
g_filter_input_stream_close (GInputStream  *stream,
                             GCancellable  *cancellable,
                             GError       **error)
{
  gboolean res = TRUE;

  if (GET_PRIVATE (stream)->close_base)
    {
      GFilterInputStream *filter_stream;
      GInputStream       *base_stream;

      filter_stream = G_FILTER_INPUT_STREAM (stream);
      base_stream = filter_stream->base_stream;

      res = g_input_stream_close (base_stream,
                                  cancellable,
                                  error);
    }

  return res;
}
