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
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "gicon.h"
#include "gloadableicon.h"
#include "glibintl.h"


/**
 * SECTION:gloadableicon
 * @short_description: Loadable Icons
 * @include: gio/gio.h
 * @see_also: #GIcon, #GThemedIcon
 * 
 * Extends the #GIcon interface and adds the ability to 
 * load icons from streams.
 **/

static void          g_loadable_icon_real_load_async  (GLoadableIcon        *icon,
						       int                   size,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       gpointer              user_data);
static GInputStream *g_loadable_icon_real_load_finish (GLoadableIcon        *icon,
						       GAsyncResult         *res,
						       char                **type,
						       GError              **error);

typedef GLoadableIconIface GLoadableIconInterface;
G_DEFINE_INTERFACE(GLoadableIcon, g_loadable_icon, G_TYPE_ICON)

static void
g_loadable_icon_default_init (GLoadableIconIface *iface)
{
  iface->load_async = g_loadable_icon_real_load_async;
  iface->load_finish = g_loadable_icon_real_load_finish;
}

/**
 * g_loadable_icon_load:
 * @icon: a #GLoadableIcon.
 * @size: an integer.
 * @type:  a location to store the type of the loaded icon, %NULL to ignore.
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Loads a loadable icon. For the asynchronous version of this function, 
 * see g_loadable_icon_load_async().
 * 
 * Returns: a #GInputStream to read the icon from.
 **/
GInputStream *
g_loadable_icon_load (GLoadableIcon  *icon,
		      int             size,
		      char          **type,
		      GCancellable   *cancellable,
		      GError        **error)
{
  GLoadableIconIface *iface;

  g_return_val_if_fail (G_IS_LOADABLE_ICON (icon), NULL);

  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  return (* iface->load) (icon, size, type, cancellable, error);
}

/**
 * g_loadable_icon_load_async:
 * @icon: a #GLoadableIcon.
 * @size: an integer.
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 * 
 * Loads an icon asynchronously. To finish this function, see 
 * g_loadable_icon_load_finish(). For the synchronous, blocking 
 * version of this function, see g_loadable_icon_load().
 **/
void
g_loadable_icon_load_async (GLoadableIcon       *icon,
                            int                  size,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  GLoadableIconIface *iface;
  
  g_return_if_fail (G_IS_LOADABLE_ICON (icon));

  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  (* iface->load_async) (icon, size, cancellable, callback, user_data);
}

/**
 * g_loadable_icon_load_finish:
 * @icon: a #GLoadableIcon.
 * @res: a #GAsyncResult.
 * @type: a location to store the type of the loaded icon, %NULL to ignore.
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Finishes an asynchronous icon load started in g_loadable_icon_load_async().
 * 
 * Returns: a #GInputStream to read the icon from.
 **/
GInputStream *
g_loadable_icon_load_finish (GLoadableIcon  *icon,
			     GAsyncResult   *res,
			     char          **type,
			     GError        **error)
{
  GLoadableIconIface *iface;
  
  g_return_val_if_fail (G_IS_LOADABLE_ICON (icon), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  if (G_IS_SIMPLE_ASYNC_RESULT (res))
    {
      GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
      if (g_simple_async_result_propagate_error (simple, error))
	return NULL;
    }
  
  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  return (* iface->load_finish) (icon, res, type, error);
}

/********************************************
 *   Default implementation of async load   *
 ********************************************/

typedef struct {
  int size;
  char *type;
  GInputStream *stream;
} LoadData;

static void
load_data_free (LoadData *data)
{
  if (data->stream)
    g_object_unref (data->stream);
  g_free (data->type);
  g_free (data);
}

static void
load_async_thread (GSimpleAsyncResult *res,
		   GObject            *object,
		   GCancellable       *cancellable)
{
  GLoadableIconIface *iface;
  GInputStream *stream;
  LoadData *data;
  GError *error = NULL;
  char *type = NULL;

  data = g_simple_async_result_get_op_res_gpointer (res);
  
  iface = G_LOADABLE_ICON_GET_IFACE (object);
  stream = iface->load (G_LOADABLE_ICON (object), data->size, &type, cancellable, &error);

  if (stream == NULL)
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
  else
    {
      data->stream = stream;
      data->type = type;
    }
}



static void
g_loadable_icon_real_load_async (GLoadableIcon       *icon,
				 int                  size,
				 GCancellable        *cancellable,
				 GAsyncReadyCallback  callback,
				 gpointer             user_data)
{
  GSimpleAsyncResult *res;
  LoadData *data;
  
  res = g_simple_async_result_new (G_OBJECT (icon), callback, user_data, g_loadable_icon_real_load_async);
  data = g_new0 (LoadData, 1);
  g_simple_async_result_set_op_res_gpointer (res, data, (GDestroyNotify) load_data_free);
  g_simple_async_result_run_in_thread (res, load_async_thread, 0, cancellable);
  g_object_unref (res);
}

static GInputStream *
g_loadable_icon_real_load_finish (GLoadableIcon        *icon,
				  GAsyncResult         *res,
				  char                **type,
				  GError              **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  LoadData *data;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_loadable_icon_real_load_async);

  data = g_simple_async_result_get_op_res_gpointer (simple);

  if (type)
    {
      *type = data->type;
      data->type = NULL;
    }

  return g_object_ref (data->stream);
}
