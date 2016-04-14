/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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
#include <glib.h>
#include "glibintl.h"

#include <stdio.h>
#include <string.h>

#include "gunixresolver.h"
#include "gnetworkingprivate.h"

#include "gcancellable.h"
#include "gsimpleasyncresult.h"
#include "gsocketaddress.h"


G_DEFINE_TYPE (GUnixResolver, g_unix_resolver, G_TYPE_THREADED_RESOLVER)

static gboolean g_unix_resolver_watch (GIOChannel   *iochannel,
                                       GIOCondition  condition,
                                       gpointer      user_data);
static void g_unix_resolver_reload (GResolver *resolver);

static void
g_unix_resolver_init (GUnixResolver *gur)
{
  g_unix_resolver_reload (G_RESOLVER (gur));
}

static void
g_unix_resolver_finalize (GObject *object)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (object);

  if (gur->watch)
    g_source_remove (gur->watch);
  _g_asyncns_free (gur->asyncns);

  G_OBJECT_CLASS (g_unix_resolver_parent_class)->finalize (object);
}

static void
g_unix_resolver_reload (GResolver *resolver)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  gint fd;
  GIOChannel *io;

  if (gur->asyncns)
    {
      if (_g_asyncns_getnqueries (gur->asyncns) == 0)
        {
          g_source_remove (gur->watch);
          _g_asyncns_free (gur->asyncns);
        }
      /* else, we will free it later from g_unix_resolver_watch */
    }

  /* FIXME: how many workers? */
  gur->asyncns = _g_asyncns_new (2);

  fd = _g_asyncns_fd (gur->asyncns);
  io = g_io_channel_unix_new (fd);
  gur->watch = g_io_add_watch (io, G_IO_IN | G_IO_HUP | G_IO_ERR,
                               g_unix_resolver_watch, gur->asyncns);
  g_io_channel_unref (io);
}

/* The various request possibilities:
 *
 * 1. Synchronous: handed off to the base class (GThreadedResolver);
 *    since it's never possible to cancel a synchronous request in a
 *    single-threaded program, the request is done in the calling
 *    thread.
 *
 * 2. Asynchronous: An appropriate _g_asyncns_query_t is created, and
 *    then a GUnixResolverRequest is created with that query and a
 *    GSimpleAsyncResult. Two sub-possibilities:
 *
 *      a. The resolution completes: g_unix_resolver_watch() sees that
 *         the request has completed, and calls
 *         g_unix_resolver_request_complete(), which detaches the
 *         "cancelled" signal handler (if it was present), queues the
 *         async_result to be completed, and then unrefs it.
 *
 *      b. The resolution is cancelled: request_cancelled() calls
 *         _g_asyncns_cancel() to cancel the resolution. Then it calls
 *         g_unix_resolver_request_complete(), which detaches the
 *         signal handler, and queues async_result to complete in an
 *         idle handler. Because the asyncns resolution was cancelled,
 *         g_unix_resolver_watch() will never be triggered for this
 *         req.
 *
 *    Since there's only a single thread, it's not possible for the
 *    request to both complete and be cancelled "at the same time",
 *    and each of the two possibilities takes steps to block the other
 *    from being able to happen later, so it's always safe to free req
 *    after the async_result completes.
 */

typedef struct _GUnixResolverRequest GUnixResolverRequest;
typedef void (*GUnixResolverFunc) (GUnixResolverRequest *);

struct _GUnixResolverRequest {
  GUnixResolver *gur;
  _g_asyncns_t *asyncns;

  _g_asyncns_query_t *qy;
  union {
    struct {
      gchar *hostname;
      GList *addresses;
    } name;

    struct {
      GInetAddress *address;
      gchar *hostname;
    } address;

    struct {
      gchar *service;
      GList *targets;
    } service;
  } u;
  GUnixResolverFunc process_func, free_func;

  GCancellable *cancellable;
  GSimpleAsyncResult *async_result;

};

static void g_unix_resolver_request_free (GUnixResolverRequest *req);
static void request_cancelled (GCancellable *cancellable,
                               gpointer      user_data);

static GUnixResolverRequest *
g_unix_resolver_request_new (GUnixResolver         *gur,
                             _g_asyncns_query_t    *qy,
                             GUnixResolverFunc      process_func,
                             GUnixResolverFunc      free_func,
                             GCancellable          *cancellable,
                             GSimpleAsyncResult    *async_result)
{
  GUnixResolverRequest *req;

  req = g_slice_new0 (GUnixResolverRequest);
  req->gur = g_object_ref (gur);
  req->asyncns = gur->asyncns;
  req->qy = qy;
  req->process_func = process_func;
  req->free_func = free_func;

  if (cancellable)
    {
      req->cancellable = g_object_ref (cancellable);
      g_signal_connect (cancellable, "cancelled",
                        G_CALLBACK (request_cancelled), req);
    }

  req->async_result = g_object_ref (async_result);

  g_simple_async_result_set_op_res_gpointer (req->async_result, req, (GDestroyNotify)g_unix_resolver_request_free);

  return req;
}

static void
g_unix_resolver_request_free (GUnixResolverRequest *req)
{
  req->free_func (req);

  /* We don't have to free req->cancellable and req->async_result,
   * since they must already have been freed if we're here.
   */

  /* Check if this was the last request remaining on an old asyncns. */
  if (req->asyncns != req->gur->asyncns &&
      _g_asyncns_getnqueries (req->asyncns) == 0)
    _g_asyncns_free (req->asyncns);

  g_object_unref (req->gur);
  g_slice_free (GUnixResolverRequest, req);
}

static void
g_unix_resolver_request_complete (GUnixResolverRequest *req)
{
  if (req->cancellable)
    {
      g_signal_handlers_disconnect_by_func (req->cancellable, request_cancelled, req);
      g_object_unref (req->cancellable);
      req->cancellable = NULL;
    }

  /* We always complete_in_idle, even if we were called from
   * g_unix_resolver_watch(), since we might have been started under a
   * non-default g_main_context_get_thread_default().
   */
  g_simple_async_result_complete_in_idle (req->async_result);
  g_object_unref (req->async_result);
}

static void
request_cancelled (GCancellable *cancellable,
                   gpointer      user_data)
{
  GUnixResolverRequest *req = user_data;
  GError *error = NULL;

  _g_asyncns_cancel (req->asyncns, req->qy);
  req->qy = NULL;

  g_cancellable_set_error_if_cancelled (cancellable, &error);
  g_simple_async_result_set_from_error (req->async_result, error);
  g_error_free (error);

  g_unix_resolver_request_complete (req);
}

static gboolean
g_unix_resolver_watch (GIOChannel   *iochannel,
                       GIOCondition  condition,
                       gpointer      user_data)
{
  _g_asyncns_t *asyncns = user_data;
  _g_asyncns_query_t *qy;
  GUnixResolverRequest *req;

  if (condition & (G_IO_HUP | G_IO_ERR))
    {
      /* Will happen if we reload, and then eventually kill the old
       * _g_asyncns_t when it's done processing requests.
       */
      return FALSE;
    }

  while (_g_asyncns_wait (asyncns, FALSE) == 0 &&
         (qy = _g_asyncns_getnext (asyncns)) != NULL)
    {
      req = _g_asyncns_getuserdata (asyncns, qy);
      req->process_func (req);
      g_unix_resolver_request_complete (req);
    }

  return TRUE;
}

static GUnixResolverRequest *
resolve_async (GUnixResolver         *gur,
               _g_asyncns_query_t    *qy,
               GUnixResolverFunc      process_func,
               GUnixResolverFunc      free_func,
               GCancellable          *cancellable,
               GAsyncReadyCallback    callback,
               gpointer               user_data,
               gpointer               tag)
{
  GSimpleAsyncResult *result;
  GUnixResolverRequest *req;

  result = g_simple_async_result_new (G_OBJECT (gur), callback, user_data, tag);
  req = g_unix_resolver_request_new (gur, qy, process_func, free_func,
                                     cancellable, result);
  g_object_unref (result);
  _g_asyncns_setuserdata (gur->asyncns, qy, req);

  return req;
}

static void
lookup_by_name_process (GUnixResolverRequest *req)
{
  struct addrinfo *res;
  gint retval;
  GError *error = NULL;

  retval = _g_asyncns_getaddrinfo_done (req->asyncns, req->qy, &res);
  req->u.name.addresses =
    _g_resolver_addresses_from_addrinfo (req->u.name.hostname,
                                         res, retval, &error);
  if (res)
    freeaddrinfo (res);

  if (error)
    {
      g_simple_async_result_set_from_error (req->async_result, error);
      g_error_free (error);
    }
}

static void
lookup_by_name_free (GUnixResolverRequest *req)
{
  g_free (req->u.name.hostname);
  if (req->u.name.addresses)
    g_resolver_free_addresses (req->u.name.addresses);
}

static void
lookup_by_name_async (GResolver           *resolver,
                      const gchar         *hostname,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  GUnixResolverRequest *req;
  _g_asyncns_query_t *qy;

  qy = _g_asyncns_getaddrinfo (gur->asyncns, hostname, NULL,
                               &_g_resolver_addrinfo_hints);
  req = resolve_async (gur, qy, lookup_by_name_process, lookup_by_name_free,
                       cancellable, callback, user_data, lookup_by_name_async);
  req->u.name.hostname = g_strdup (hostname);
}

static GList *
lookup_by_name_finish (GResolver     *resolver,
                       GAsyncResult  *result,
                       GError       **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  GList *addresses;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_by_name_async), NULL);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  addresses = req->u.name.addresses;
  req->u.name.addresses = NULL;

  return addresses;
}


static void
lookup_by_address_process (GUnixResolverRequest *req)
{
  gchar host[NI_MAXHOST];
  gint retval;
  GError *error = NULL;

  retval = _g_asyncns_getnameinfo_done (req->asyncns, req->qy,
                                        host, sizeof (host), NULL, 0);
  req->u.address.hostname =
    _g_resolver_name_from_nameinfo (req->u.address.address,
                                    host, retval, &error);

  if (error)
    {
      g_simple_async_result_set_from_error (req->async_result, error);
      g_error_free (error);
    }
}

static void
lookup_by_address_free (GUnixResolverRequest *req)
{
  g_object_unref (req->u.address.address);
  if (req->u.address.hostname)
    g_free (req->u.address.hostname);
}

static void
lookup_by_address_async (GResolver           *resolver,
                         GInetAddress        *address,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  GUnixResolverRequest *req;
  _g_asyncns_query_t *qy;
  struct sockaddr_storage sockaddr;
  gsize sockaddr_size;

  _g_resolver_address_to_sockaddr (address, &sockaddr, &sockaddr_size);
  qy = _g_asyncns_getnameinfo (gur->asyncns,
                               (struct sockaddr *)&sockaddr, sockaddr_size,
                               NI_NAMEREQD, TRUE, FALSE);
  req = resolve_async (gur, qy, lookup_by_address_process,
                       lookup_by_address_free, cancellable,
                       callback, user_data, lookup_by_address_async);
  req->u.address.address = g_object_ref (address);
}

static gchar *
lookup_by_address_finish (GResolver     *resolver,
                          GAsyncResult  *result,
                          GError       **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  gchar *name;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_by_address_async), NULL);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  name = req->u.address.hostname;
  req->u.address.hostname = NULL;

  return name;
}


static void
lookup_service_process (GUnixResolverRequest *req)
{
  guchar *answer;
  gint len, herr;
  GError *error = NULL;

  len = _g_asyncns_res_done (req->asyncns, req->qy, &answer);
  if (len < 0)
    herr = h_errno;
  else
    herr = 0;

  req->u.service.targets =
    _g_resolver_targets_from_res_query (req->u.service.service,
                                        answer, len, herr, &error);
  _g_asyncns_freeanswer (answer);

  if (error)
    {
      g_simple_async_result_set_from_error (req->async_result, error);
      g_error_free (error);
    }
}

static void
lookup_service_free (GUnixResolverRequest *req)
{
  g_free (req->u.service.service);
  if (req->u.service.targets)
    g_resolver_free_targets (req->u.service.targets);
}

static void
lookup_service_async (GResolver           *resolver,
                      const char          *rrname,
		      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
		      gpointer             user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  GUnixResolverRequest *req;
  _g_asyncns_query_t *qy;

  qy = _g_asyncns_res_query (gur->asyncns, rrname, C_IN, T_SRV);
  req = resolve_async (gur, qy, lookup_service_process, lookup_service_free,
                       cancellable, callback, user_data, lookup_service_async);
  req->u.service.service = g_strdup (rrname);
}

static GList *
lookup_service_finish (GResolver     *resolver,
                       GAsyncResult  *result,
		       GError       **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  GList *targets;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_service_async), NULL);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  targets = req->u.service.targets;
  req->u.service.targets = NULL;

  return targets;
}


static void
g_unix_resolver_class_init (GUnixResolverClass *unix_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (unix_class);
  GObjectClass *object_class = G_OBJECT_CLASS (unix_class);

  resolver_class->reload                   = g_unix_resolver_reload;
  resolver_class->lookup_by_name_async     = lookup_by_name_async;
  resolver_class->lookup_by_name_finish    = lookup_by_name_finish;
  resolver_class->lookup_by_address_async  = lookup_by_address_async;
  resolver_class->lookup_by_address_finish = lookup_by_address_finish;
  resolver_class->lookup_service_async     = lookup_service_async;
  resolver_class->lookup_service_finish    = lookup_service_finish;

  object_class->finalize = g_unix_resolver_finalize;
}
