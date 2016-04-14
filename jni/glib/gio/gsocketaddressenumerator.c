/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#include "config.h"
#include "gsocketaddressenumerator.h"
#include "glibintl.h"

#include "gsimpleasyncresult.h"


G_DEFINE_ABSTRACT_TYPE (GSocketAddressEnumerator, g_socket_address_enumerator, G_TYPE_OBJECT);

static void            g_socket_address_enumerator_real_next_async  (GSocketAddressEnumerator  *enumerator,
								     GCancellable              *cancellable,
								     GAsyncReadyCallback        callback,
								     gpointer                   user_data);
static GSocketAddress *g_socket_address_enumerator_real_next_finish (GSocketAddressEnumerator  *enumerator,
								     GAsyncResult              *result,
								     GError                   **error);

static void
g_socket_address_enumerator_init (GSocketAddressEnumerator *enumerator)
{
}

static void
g_socket_address_enumerator_class_init (GSocketAddressEnumeratorClass *enumerator_class)
{
  enumerator_class->next_async = g_socket_address_enumerator_real_next_async;
  enumerator_class->next_finish = g_socket_address_enumerator_real_next_finish;
}

/**
 * g_socket_address_enumerator_next:
 * @enumerator: a #GSocketAddressEnumerator
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: a #GError.
 *
 * Retrieves the next #GSocketAddress from @enumerator. Note that this
 * may block for some amount of time. (Eg, a #GNetworkAddress may need
 * to do a DNS lookup before it can return an address.) Use
 * g_socket_address_enumerator_next_async() if you need to avoid
 * blocking.
 *
 * If @enumerator is expected to yield addresses, but for some reason
 * is unable to (eg, because of a DNS error), then the first call to
 * g_socket_address_enumerator_next() will return an appropriate error
 * in *@error. However, if the first call to
 * g_socket_address_enumerator_next() succeeds, then any further
 * internal errors (other than @cancellable being triggered) will be
 * ignored.
 *
 * Return value: a #GSocketAddress (owned by the caller), or %NULL on
 *     error (in which case *@error will be set) or if there are no
 *     more addresses.
 */
GSocketAddress *
g_socket_address_enumerator_next (GSocketAddressEnumerator  *enumerator,
				  GCancellable              *cancellable,
				  GError                   **error)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_val_if_fail (G_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator), NULL);

  klass = G_SOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  return (* klass->next) (enumerator, cancellable, error);
}

/* Default implementation just calls the synchronous method; this can
 * be used if the implementation already knows all of its addresses,
 * and so the synchronous method will never block.
 */
static void
g_socket_address_enumerator_real_next_async (GSocketAddressEnumerator *enumerator,
					     GCancellable             *cancellable,
					     GAsyncReadyCallback       callback,
					     gpointer                  user_data)
{
  GSimpleAsyncResult *result;
  GSocketAddress *address;
  GError *error = NULL;

  result = g_simple_async_result_new (G_OBJECT (enumerator),
				      callback, user_data,
				      g_socket_address_enumerator_real_next_async);
  address = g_socket_address_enumerator_next (enumerator, cancellable, &error);
  if (address)
    g_simple_async_result_set_op_res_gpointer (result, address, NULL);
  else if (error)
    {
      g_simple_async_result_set_from_error (result, error);
      g_error_free (error);
    }
  g_simple_async_result_complete_in_idle (result);
  g_object_unref (result);
}

/**
 * g_socket_address_enumerator_next_async:
 * @enumerator: a #GSocketAddressEnumerator
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously retrieves the next #GSocketAddress from @enumerator
 * and then calls @callback, which must call
 * g_socket_address_enumerator_next_finish() to get the result.
 */
void
g_socket_address_enumerator_next_async (GSocketAddressEnumerator *enumerator,
					GCancellable             *cancellable,
					GAsyncReadyCallback       callback,
					gpointer                  user_data)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_if_fail (G_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator));

  klass = G_SOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  (* klass->next_async) (enumerator, cancellable, callback, user_data);
}

static GSocketAddress *
g_socket_address_enumerator_real_next_finish (GSocketAddressEnumerator  *enumerator,
					      GAsyncResult              *result,
					      GError                   **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == g_socket_address_enumerator_real_next_async, NULL);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else
    return g_simple_async_result_get_op_res_gpointer (simple);
}

/**
 * g_socket_address_enumerator_next_finish:
 * @enumerator: a #GSocketAddressEnumerator
 * @result: a #GAsyncResult
 * @error: a #GError
 *
 * Retrieves the result of a completed call to
 * g_socket_address_enumerator_next_async(). See
 * g_socket_address_enumerator_next() for more information about
 * error handling.
 *
 * Return value: a #GSocketAddress (owned by the caller), or %NULL on
 *     error (in which case *@error will be set) or if there are no
 *     more addresses.
 */
GSocketAddress *
g_socket_address_enumerator_next_finish (GSocketAddressEnumerator  *enumerator,
					 GAsyncResult              *result,
					 GError                   **error)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_val_if_fail (G_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator), NULL);

  klass = G_SOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  return (* klass->next_finish) (enumerator, result, error);
}
