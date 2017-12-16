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

#include "gfileicon.h"
#include "gfile.h"
#include "gicon.h"
#include "glibintl.h"
#include "gloadableicon.h"
#include "ginputstream.h"
#include "gsimpleasyncresult.h"
#include "gioerror.h"


/**
 * SECTION:gfileicon
 * @short_description: Icons pointing to an image file
 * @include: gio/gio.h
 * @see_also: #GIcon, #GLoadableIcon
 * 
 * #GFileIcon specifies an icon by pointing to an image file
 * to be used as icon.
 * 
 **/

static void g_file_icon_icon_iface_init          (GIconIface          *iface);
static void g_file_icon_loadable_icon_iface_init (GLoadableIconIface  *iface);
static void g_file_icon_load_async               (GLoadableIcon       *icon,
						  int                  size,
						  GCancellable        *cancellable,
						  GAsyncReadyCallback  callback,
						  gpointer             user_data);

struct _GFileIcon
{
  GObject parent_instance;

  GFile *file;
};

struct _GFileIconClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_0,
  PROP_FILE
};

G_DEFINE_TYPE_WITH_CODE (GFileIcon, g_file_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ICON,
                                                g_file_icon_icon_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LOADABLE_ICON,
                                                g_file_icon_loadable_icon_iface_init))

static void
g_file_icon_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GFileIcon *icon = G_FILE_ICON (object);

  switch (prop_id)
    {
      case PROP_FILE:
        g_value_set_object (value, icon->file);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_file_icon_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GFileIcon *icon = G_FILE_ICON (object);

  switch (prop_id)
    {
      case PROP_FILE:
        icon->file = G_FILE (g_value_dup_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_file_icon_finalize (GObject *object)
{
  GFileIcon *icon;

  icon = G_FILE_ICON (object);

  g_object_unref (icon->file);

  G_OBJECT_CLASS (g_file_icon_parent_class)->finalize (object);
}

static void
g_file_icon_class_init (GFileIconClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->get_property = g_file_icon_get_property;
  gobject_class->set_property = g_file_icon_set_property;
  gobject_class->finalize = g_file_icon_finalize;

  /**
   * GFileIcon:file:
   *
   * The file containing the icon.
   */
  g_object_class_install_property (gobject_class, PROP_FILE,
                                   g_param_spec_object ("file",
                                                        P_("file"),
                                                        P_("The file containing the icon"),
                                                        G_TYPE_FILE,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK));
}

static void
g_file_icon_init (GFileIcon *file)
{
}

/**
 * g_file_icon_new:
 * @file: a #GFile.
 * 
 * Creates a new icon for a file.
 * 
 * Returns: a #GIcon for the given @file, or %NULL on error.
 **/
GIcon *
g_file_icon_new (GFile *file)
{
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return G_ICON (g_object_new (G_TYPE_FILE_ICON, "file", file, NULL));
}

/**
 * g_file_icon_get_file:
 * @icon: a #GIcon.
 * 
 * Gets the #GFile associated with the given @icon.
 * 
 * Returns: a #GFile, or %NULL.
 **/
GFile *
g_file_icon_get_file (GFileIcon *icon)
{
  g_return_val_if_fail (G_IS_FILE_ICON (icon), NULL);

  return icon->file;
}

static guint
g_file_icon_hash (GIcon *icon)
{
  GFileIcon *file_icon = G_FILE_ICON (icon);

  return g_file_hash (file_icon->file);
}

static gboolean
g_file_icon_equal (GIcon *icon1,
		   GIcon *icon2)
{
  GFileIcon *file1 = G_FILE_ICON (icon1);
  GFileIcon *file2 = G_FILE_ICON (icon2);
  
  return g_file_equal (file1->file, file2->file);
}

static gboolean
g_file_icon_to_tokens (GIcon *icon,
		       GPtrArray *tokens,
                       gint  *out_version)
{
  GFileIcon *file_icon = G_FILE_ICON (icon);

  g_return_val_if_fail (out_version != NULL, FALSE);

  *out_version = 0;

  g_ptr_array_add (tokens, g_file_get_uri (file_icon->file));
  return TRUE;
}

static GIcon *
g_file_icon_from_tokens (gchar  **tokens,
                         gint     num_tokens,
                         gint     version,
                         GError **error)
{
  GIcon *icon;
  GFile *file;

  icon = NULL;

  if (version != 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Can't handle version %d of GFileIcon encoding"),
                   version);
      goto out;
    }

  if (num_tokens != 1)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Malformed input data for GFileIcon"));
      goto out;
    }

  file = g_file_new_for_uri (tokens[0]);
  icon = g_file_icon_new (file);
  g_object_unref (file);

 out:
  return icon;
}

static void
g_file_icon_icon_iface_init (GIconIface *iface)
{
  iface->hash = g_file_icon_hash;
  iface->equal = g_file_icon_equal;
  iface->to_tokens = g_file_icon_to_tokens;
  iface->from_tokens = g_file_icon_from_tokens;
}


static GInputStream *
g_file_icon_load (GLoadableIcon  *icon,
		  int            size,
		  char          **type,
		  GCancellable   *cancellable,
		  GError        **error)
{
  GFileInputStream *stream;
  GFileIcon *file_icon = G_FILE_ICON (icon);

  stream = g_file_read (file_icon->file,
			cancellable,
			error);
  
  return G_INPUT_STREAM (stream);
}

typedef struct {
  GLoadableIcon *icon;
  GAsyncReadyCallback callback;
  gpointer user_data;
} LoadData;

static void
load_data_free (LoadData *data)
{
  g_object_unref (data->icon);
  g_free (data);
}

static void
load_async_callback (GObject      *source_object,
		     GAsyncResult *res,
		     gpointer      user_data)
{
  GFileInputStream *stream;
  GError *error = NULL;
  GSimpleAsyncResult *simple;
  LoadData *data = user_data;

  stream = g_file_read_finish (G_FILE (source_object), res, &error);
  
  if (stream == NULL)
    {
      simple = g_simple_async_result_new_from_error (G_OBJECT (data->icon),
						     data->callback,
						     data->user_data,
						     error);
      g_error_free (error);
    }
  else
    {
      simple = g_simple_async_result_new (G_OBJECT (data->icon),
					  data->callback,
					  data->user_data,
					  g_file_icon_load_async);
      
      g_simple_async_result_set_op_res_gpointer (simple,
						 stream,
						 g_object_unref);
  }


  g_simple_async_result_complete (simple);
  
  load_data_free (data);
}

static void
g_file_icon_load_async (GLoadableIcon       *icon,
                        int                  size,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
  GFileIcon *file_icon = G_FILE_ICON (icon);
  LoadData *data;

  data = g_new0 (LoadData, 1);
  data->icon = g_object_ref (icon);
  data->callback = callback;
  data->user_data = user_data;
  
  g_file_read_async (file_icon->file, 0,
		     cancellable,
		     load_async_callback, data);
  
}

static GInputStream *
g_file_icon_load_finish (GLoadableIcon  *icon,
			 GAsyncResult   *res,
			 char          **type,
			 GError        **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  gpointer op;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_file_icon_load_async);

  if (type)
    *type = NULL;
  
  op = g_simple_async_result_get_op_res_gpointer (simple);
  if (op)
    return g_object_ref (op);
  
  return NULL;
}

static void
g_file_icon_loadable_icon_iface_init (GLoadableIconIface *iface)
{
  iface->load = g_file_icon_load;
  iface->load_async = g_file_icon_load_async;
  iface->load_finish = g_file_icon_load_finish;
}
