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
#include "gasyncinitable.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "glibintl.h"


/**
 * SECTION:gasyncinitable
 * @short_description: Asynchronously failable object initialization interface
 * @include: gio/gio.h
 * @see_also: #GInitable
 *
 * This is the asynchronous version of #GInitable; it behaves the same
 * in all ways except that initialization is asynchronous. For more details
 * see the descriptions on #GInitable.
 *
 * A class may implement both the #GInitable and #GAsyncInitable interfaces.
 *
 * Users of objects implementing this are not intended to use the interface
 * method directly; instead it will be used automatically in various ways.
 * For C applications you generally just call g_async_initable_new_async()
 * directly, or indirectly via a foo_thing_new_async() wrapper. This will call
 * g_async_initable_init_async() under the cover, calling back with %NULL and
 * a set %GError on failure.
 *
 * A typical implementation might look something like this:
 *
 * |[
 * enum {
 *    NOT_INITIALIZED,
 *    INITIALIZING,
 *    INITIALIZED
 * };
 *
 * static void
 * _foo_ready_cb (Foo *self)
 * {
 *   GList *l;
 *
 *   self->priv->state = INITIALIZED;
 *
 *   for (l = self->priv->init_results; l != NULL; l = l->next)
 *     {
 *       GSimpleAsyncResult *simple = l->data;
 *
 *       if (!self->priv->success)
 *         g_simple_async_result_set_error (simple, ...);
 *
 *       g_simple_async_result_complete (simple);
 *       g_object_unref (simple);
 *     }
 *
 *   g_list_free (self->priv->init_results);
 *   self->priv->init_results = NULL;
 * }
 *
 * static void
 * foo_init_async (GAsyncInitable       *initable,
 *                 int                   io_priority,
 *                 GCancellable         *cancellable,
 *                 GAsyncReadyCallback   callback,
 *                 gpointer              user_data)
 * {
 *   Foo *self = FOO (initable);
 *   GSimpleAsyncResult *simple;
 *
 *   simple = g_simple_async_result_new (G_OBJECT (initable)
 *                                       callback,
 *                                       user_data,
 *                                       foo_init_async);
 *
 *   switch (self->priv->state)
 *     {
 *       case NOT_INITIALIZED:
 *         _foo_get_ready (self);
 *         self->priv->init_results = g_list_append (self->priv->init_results,
 *                                                   simple);
 *         self->priv->state = INITIALIZING;
 *         break;
 *       case INITIALIZING:
 *         self->priv->init_results = g_list_append (self->priv->init_results,
 *                                                   simple);
 *         break;
 *       case INITIALIZED:
 *         if (!self->priv->success)
 *           g_simple_async_result_set_error (simple, ...);
 *
 *         g_simple_async_result_complete_in_idle (simple);
 *         g_object_unref (simple);
 *         break;
 *     }
 * }
 *
 * static gboolean
 * foo_init_finish (GAsyncInitable       *initable,
 *                  GAsyncResult         *result,
 *                  GError              **error)
 * {
 *   g_return_val_if_fail (g_simple_async_result_is_valid (result,
 *       G_OBJECT (initable), foo_init_async), FALSE);
 *
 *   if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
 *           error))
 *     return FALSE;
 *
 *   return TRUE;
 * }
 *
 * static void
 * foo_async_initable_iface_init (gpointer g_iface,
 *                                gpointer data)
 * {
 *   GAsyncInitableIface *iface = g_iface;
 *
 *   iface->init_async = foo_init_async;
 *   iface->init_finish = foo_init_finish;
 * }
 * ]|
 */

static void     g_async_initable_real_init_async  (GAsyncInitable       *initable,
						   int                   io_priority,
						   GCancellable         *cancellable,
						   GAsyncReadyCallback   callback,
						   gpointer              user_data);
static gboolean g_async_initable_real_init_finish (GAsyncInitable       *initable,
						   GAsyncResult         *res,
						   GError              **error);


typedef GAsyncInitableIface GAsyncInitableInterface;
G_DEFINE_INTERFACE (GAsyncInitable, g_async_initable, G_TYPE_OBJECT)


static void
g_async_initable_default_init (GAsyncInitableInterface *iface)
{
  iface->init_async = g_async_initable_real_init_async;
  iface->init_finish = g_async_initable_real_init_finish;
}

/**
 * g_async_initable_init_async:
 * @initable: a #GAsyncInitable.
 * @io_priority: the <link linkend="io-priority">I/O priority</link>
 *     of the operation.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts asynchronous initialization of the object implementing the
 * interface. This must be done before any real use of the object after
 * initial construction. If the object also implements #GInitable you can
 * optionally call g_initable_init() instead.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call g_async_initable_init_finish() to get the result of the
 * initialization.
 *
 * Implementations may also support cancellation. If @cancellable is not
 * %NULL, then initialization can be cancelled by triggering the cancellable
 * object from another thread. If the operation was cancelled, the error
 * %G_IO_ERROR_CANCELLED will be returned. If @cancellable is not %NULL, and
 * the object doesn't support cancellable initialization, the error
 * %G_IO_ERROR_NOT_SUPPORTED will be returned.
 *
 * If this function is not called, or returns with an error, then all
 * operations on the object should fail, generally returning the
 * error %G_IO_ERROR_NOT_INITIALIZED.
 *
 * Implementations of this method must be idempotent: i.e. multiple calls
 * to this function with the same argument should return the same results.
 * Only the first call initializes the object; further calls return the result
 * of the first call. This is so that it's safe to implement the singleton
 * pattern in the GObject constructor function.
 *
 * For classes that also support the #GInitable interface, the default
 * implementation of this method will run the g_initable_init() function
 * in a thread, so if you want to support asynchronous initialization via
 * threads, just implement the #GAsyncInitable interface without overriding
 * any interface methods.
 *
 * Since: 2.22
 */
void
g_async_initable_init_async (GAsyncInitable      *initable,
			     int                  io_priority,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
  GAsyncInitableIface *iface;

  g_return_if_fail (G_IS_ASYNC_INITABLE (initable));

  iface = G_ASYNC_INITABLE_GET_IFACE (initable);

  (* iface->init_async) (initable, io_priority, cancellable, callback, user_data);
}

/**
 * g_async_initable_init_finish:
 * @initable: a #GAsyncInitable.
 * @res: a #GAsyncResult.
 * @error: a #GError location to store the error occuring, or %NULL to
 * ignore.
 *
 * Finishes asynchronous initialization and returns the result.
 * See g_async_initable_init_async().
 *
 * Returns: %TRUE if successful. If an error has occurred, this function
 * will return %FALSE and set @error appropriately if present.
 *
 * Since: 2.22
 */
gboolean
g_async_initable_init_finish (GAsyncInitable  *initable,
			      GAsyncResult    *res,
			      GError         **error)
{
  GAsyncInitableIface *iface;

  g_return_val_if_fail (G_IS_ASYNC_INITABLE (initable), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  if (G_IS_SIMPLE_ASYNC_RESULT (res))
    {
      GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
      if (g_simple_async_result_propagate_error (simple, error))
	return FALSE;
    }

  iface = G_ASYNC_INITABLE_GET_IFACE (initable);

  return (* iface->init_finish) (initable, res, error);
}

static void
async_init_thread (GSimpleAsyncResult *res,
		   GObject            *object,
		   GCancellable       *cancellable)
{
  GError *error = NULL;

  if (!g_initable_init (G_INITABLE (object), cancellable, &error))
    {
      g_simple_async_result_set_from_error (res, error);
      g_error_free (error);
    }
}

static void
g_async_initable_real_init_async (GAsyncInitable      *initable,
				  int                  io_priority,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
  GSimpleAsyncResult *res;

  g_return_if_fail (G_IS_INITABLE (initable));

  res = g_simple_async_result_new (G_OBJECT (initable), callback, user_data,
				   g_async_initable_real_init_async);
  g_simple_async_result_run_in_thread (res, async_init_thread,
				       io_priority, cancellable);
  g_object_unref (res);
}

static gboolean
g_async_initable_real_init_finish (GAsyncInitable  *initable,
				   GAsyncResult    *res,
				   GError         **error)
{
  return TRUE; /* Errors handled by base impl */
}

/**
 * g_async_initable_new_async:
 * @object_type: a #GType supporting #GAsyncInitable.
 * @io_priority: the <link linkend="io-priority">I/O priority</link>
 *     of the operation.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 * @first_property_name: the name of the first property, or %NULL if no
 *     properties
 * @...:  the value of the first property, followed by other property
 *    value pairs, and ended by %NULL.
 *
 * Helper function for constructing #GAsyncInitiable object. This is
 * similar to g_object_new() but also initializes the object asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call g_async_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 */
void
g_async_initable_new_async (GType                object_type,
			    int                  io_priority,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data,
			    const gchar         *first_property_name,
			    ...)
{
  va_list var_args;

  va_start (var_args, first_property_name);
  g_async_initable_new_valist_async (object_type,
				     first_property_name, var_args,
				     io_priority, cancellable,
				     callback, user_data);
  va_end (var_args);
}

/**
 * g_async_initable_newv_async:
 * @object_type: a #GType supporting #GAsyncInitable.
 * @n_parameters: the number of parameters in @parameters
 * @parameters: the parameters to use to construct the object
 * @io_priority: the <link linkend="io-priority">I/O priority</link>
 *     of the operation.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 *
 * Helper function for constructing #GAsyncInitiable object. This is
 * similar to g_object_newv() but also initializes the object asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call g_async_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 */
void
g_async_initable_newv_async (GType                object_type,
			     guint                n_parameters,
			     GParameter          *parameters,
			     int                  io_priority,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
  GObject *obj;

  g_return_if_fail (G_TYPE_IS_ASYNC_INITABLE (object_type));

  obj = g_object_newv (object_type, n_parameters, parameters);

  g_async_initable_init_async (G_ASYNC_INITABLE (obj),
			       io_priority, cancellable,
			       callback, user_data);
}

/**
 * g_async_initable_new_valist_async:
 * @object_type: a #GType supporting #GAsyncInitable.
 * @first_property_name: the name of the first property, followed by
 * the value, and other property value pairs, and ended by %NULL.
 * @var_args: The var args list generated from @first_property_name.
 * @io_priority: the <link linkend="io-priority">I/O priority</link>
 *     of the operation.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 *
 * Helper function for constructing #GAsyncInitiable object. This is
 * similar to g_object_new_valist() but also initializes the object
 * asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call g_async_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 */
void
g_async_initable_new_valist_async (GType                object_type,
				   const gchar         *first_property_name,
				   va_list              var_args,
				   int                  io_priority,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
  GObject *obj;

  g_return_if_fail (G_TYPE_IS_ASYNC_INITABLE (object_type));

  obj = g_object_new_valist (object_type,
			     first_property_name,
			     var_args);

  g_async_initable_init_async (G_ASYNC_INITABLE (obj),
			       io_priority, cancellable,
			       callback, user_data);
  g_object_unref (obj); /* Passed ownership to async call */
}

/**
 * g_async_initable_new_finish:
 * @initable: the #GAsyncInitable from the callback
 * @res: the #GAsyncResult.from the callback
 * @error: a #GError location to store the error occuring, or %NULL to
 *     ignore.
 *
 * Finishes the async construction for the various g_async_initable_new calls,
 * returning the created object or %NULL on error.
 *
 * Returns: a newly created #GObject, or %NULL on error. Free with
 *     g_object_unref().
 *
 * Since: 2.22
 */
GObject *
g_async_initable_new_finish (GAsyncInitable  *initable,
			     GAsyncResult    *res,
			     GError         **error)
{
  if (g_async_initable_init_finish (initable, res, error))
    return g_object_ref (initable);
  else
    return NULL;
}
