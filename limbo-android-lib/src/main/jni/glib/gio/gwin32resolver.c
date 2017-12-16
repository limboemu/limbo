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

#include "gwin32resolver.h"
#include "gnetworkingprivate.h"

#include "gcancellable.h"
#include "gsimpleasyncresult.h"
#include "gsocketaddress.h"


G_DEFINE_TYPE (GWin32Resolver, g_win32_resolver, G_TYPE_THREADED_RESOLVER)

static void
g_win32_resolver_init (GWin32Resolver *gwr)
{
}

/* This is simpler than GThreadedResolver since we don't have to worry
 * about multiple application-level threads, but more complicated than
 * GUnixResolver, since we do have to deal with multiple threads of
 * our own.
 *
 * The various request possibilities:
 *
 * 1. Synchronous: handed off to the base class (GThreadedResolver);
 *    since it's never possible to cancel a synchronous request in a
 *    single-threaded program, the request is done in the calling
 *    thread.
 *
 * 2. Asynchronous: A GWin32ResolverRequest is created with
 *    appropriate query-specific information, a Windows event handle,
 *    and a GSimpleAsyncResult. This is then handed to the
 *    Windows-internal thread pool, which does the raw DNS query part
 *    of the operation (being careful to not call any glib methods
 *    that might fail when called from another thread when
 *    g_thread_init() has not been called). The main thread sets up a
 *    GSource to asynchronously poll the event handle. There are two
 *    sub-possibilities:
 *
 *      a. The resolution completes: the threadpool function calls
 *         SetEvent() on the event handle and then returns.
 *
 *      b. The resolution is cancelled: request_cancelled()
 *         disconnects the "cancelled" signal handler, and queues an
 *         idle handler to complete the async_result.
 *
 *    Since we can't free the request from the threadpool thread
 *    (because of glib locking issues), we *always* have to have it
 *    call SetEvent and trigger the callback to indicate that it is
 *    done. But this means that it's possible for the request to be
 *    cancelled (queuing an idle handler to return that result) and
 *    then have the resolution thread complete before the idle handler
 *    runs. So the event callback and the idle handler need to both
 *    watch out for this, making sure we don't complete the same
 *    result twice.
 */

typedef struct GWin32ResolverRequest GWin32ResolverRequest;
typedef void (*GWin32ResolverRequestFreeFunc) (GWin32ResolverRequest *);

struct GWin32ResolverRequest {
  GWin32ResolverRequestFreeFunc free_func;

  GCancellable *cancellable;
  GError *error;

  HANDLE *event;
  GSimpleAsyncResult *async_result;
  gboolean complete;
  GSource *cancelled_idle;

  union {
    struct {
      gchar *name;
      gint retval;
      struct addrinfo *res;
    } name;

    struct {
      GInetAddress *iaddr;
      struct sockaddr_storage addr;
      gsize addrlen;
      gint retval;
      gchar *namebuf;
    } address;

    struct {
      gchar *rrname;
      DNS_STATUS retval;
      DNS_RECORD *results;
    } service;
  } u;

};

static GSource *g_win32_handle_source_add (HANDLE      handle,
                                           GSourceFunc callback,
                                           gpointer    user_data);

static gboolean request_completed (gpointer      user_data);
static void     request_cancelled (GCancellable *cancellable,
                                   gpointer      user_data);

GWin32ResolverRequest *
g_win32_resolver_request_new (GResolver                     *resolver,
                              GWin32ResolverRequestFreeFunc  free_func,
                              GCancellable                  *cancellable,
                              GAsyncReadyCallback            callback,
                              gpointer                       user_data,
                              gpointer                       tag)
{
  GWin32ResolverRequest *req;

  req = g_slice_new0 (GWin32ResolverRequest);
  req->free_func = free_func;

  req->async_result = g_simple_async_result_new (G_OBJECT (resolver), callback,
                                                 user_data, tag);
  g_simple_async_result_set_op_res_gpointer (req->async_result, req, NULL);

  req->event = CreateEvent (NULL, FALSE, FALSE, NULL);
  g_win32_handle_source_add (req->event, request_completed, req);  

  if (cancellable)
    {
      req->cancellable = g_object_ref (cancellable);
      g_signal_connect (cancellable, "cancelled",
                        G_CALLBACK (request_cancelled), req);
    }

  return req;
}

static gboolean
request_completed (gpointer user_data)
{
  GWin32ResolverRequest *req = user_data;

  /* Clean up cancellation-related stuff first */
  if (req->cancelled_idle)
    {
      g_source_destroy (req->cancelled_idle);
      g_source_unref (req->cancelled_idle);
      req->cancelled_idle = NULL;
    }
  if (req->cancellable)
    {
      g_signal_handlers_disconnect_by_func (req->cancellable, request_cancelled, req);
      g_object_unref (req->cancellable);
    }

  /* Now complete the result (assuming it wasn't already completed) */
  if (req->async_result)
    {
      g_simple_async_result_complete (req->async_result);
      g_object_unref (req->async_result);
    }

  /* And free req */
  CloseHandle (req->event);
  req->free_func (req);
  g_slice_free (GWin32ResolverRequest, req);

  return FALSE;
}

static gboolean
request_cancelled_idle (gpointer user_data)
{
  GWin32ResolverRequest *req = user_data;
  GError *error = NULL;

  g_source_unref (req->cancelled_idle);
  req->cancelled_idle = NULL;

  g_cancellable_set_error_if_cancelled (req->cancellable, &error);
  g_simple_async_result_set_from_error (req->async_result, error);
  g_simple_async_result_complete (req->async_result);

  g_object_unref (req->async_result);
  req->async_result = NULL;

  /* request_completed will eventually be called to free req */

  return FALSE;
}

static void
request_cancelled (GCancellable *cancellable,
                   gpointer      user_data)
{
  GWin32ResolverRequest *req = user_data;

  if (req->cancellable)
    {
      g_signal_handlers_disconnect_by_func (req->cancellable, request_cancelled, req);
      g_object_unref (req->cancellable);
      req->cancellable = NULL;
    }

  /* We need to wait until main-loop-time to actually complete the
   * result; we don't use _complete_in_idle() here because we need to
   * keep track of the source so we can potentially cancel it before
   * it runs.
   */
  req->cancelled_idle = g_idle_source_new ();
  g_source_set_callback (req->cancelled_idle,
                         (GSourceFunc)request_cancelled_idle, req, NULL);
  g_source_attach (req->cancelled_idle, g_main_context_get_thread_default ());
}

static DWORD WINAPI
lookup_by_name_in_thread (LPVOID data)
{
  GWin32ResolverRequest *req = data;

  req->u.name.retval = getaddrinfo (req->u.name.name, NULL,
                                    &_g_resolver_addrinfo_hints,
                                    &req->u.name.res);
  SetEvent (req->event);
  return 0;
}

static void
free_lookup_by_name (GWin32ResolverRequest *req)
{
  g_free (req->u.name.name);
  if (req->u.name.res)
    freeaddrinfo (req->u.name.res);
}

static void
lookup_by_name_async (GResolver           *resolver,
                      const gchar         *hostname,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  GWin32ResolverRequest *req;

  req = g_win32_resolver_request_new (resolver, free_lookup_by_name,
                                      cancellable, callback, user_data,
                                      lookup_by_name_async);
  req->u.name.name = g_strdup (hostname);

  QueueUserWorkItem (lookup_by_name_in_thread, req, 0);
}

static GList *
lookup_by_name_finish (GResolver     *resolver,
                       GAsyncResult  *result,
                       GError       **error)
{
  GSimpleAsyncResult *simple;
  GWin32ResolverRequest *req;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_by_name_async), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);

  req = g_simple_async_result_get_op_res_gpointer (simple);
  return _g_resolver_addresses_from_addrinfo (req->u.name.name, req->u.name.res,
                                              req->u.name.retval, error);
}


static DWORD WINAPI
lookup_by_addresses_in_thread (LPVOID data)
{
  GWin32ResolverRequest *req = data;

  req->u.address.retval =
    getnameinfo ((struct sockaddr *)&req->u.address.addr,
                 req->u.address.addrlen,
                 req->u.address.namebuf, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
  SetEvent (req->event);
  return 0;
}

static void
free_lookup_by_address (GWin32ResolverRequest *req)
{
  g_object_unref (req->u.address.iaddr);
  g_free (req->u.address.namebuf);
}

static void
lookup_by_address_async (GResolver           *resolver,
                         GInetAddress        *address,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  GWin32ResolverRequest *req;

  req = g_win32_resolver_request_new (resolver, free_lookup_by_address,
                                      cancellable, callback, user_data,
                                      lookup_by_address_async);

  req->u.address.iaddr = g_object_ref (address);
  _g_resolver_address_to_sockaddr (address, &req->u.address.addr,
                                   &req->u.address.addrlen);
  req->u.address.namebuf = g_malloc (NI_MAXHOST);

  QueueUserWorkItem (lookup_by_addresses_in_thread, req, 0);
}

static char *
lookup_by_address_finish (GResolver     *resolver,
                          GAsyncResult  *result,
                          GError       **error)
{
  GSimpleAsyncResult *simple;
  GWin32ResolverRequest *req;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_by_address_async), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);

  req = g_simple_async_result_get_op_res_gpointer (simple);
  return _g_resolver_name_from_nameinfo (req->u.address.iaddr,
                                         req->u.address.namebuf,
                                         req->u.address.retval, error);
}


static DWORD WINAPI
lookup_service_in_thread (LPVOID data)
{
  GWin32ResolverRequest *req = data;

  req->u.service.retval =
    DnsQuery_A (req->u.service.rrname, DNS_TYPE_SRV, DNS_QUERY_STANDARD,
                NULL, &req->u.service.results, NULL);
  SetEvent (req->event);
  return 0;
}

static void
free_lookup_service (GWin32ResolverRequest *req)
{
  g_free (req->u.service.rrname);
  if (req->u.service.results)
    DnsRecordListFree (req->u.service.results, DnsFreeRecordList);
}

static void
lookup_service_async (GResolver           *resolver,
                      const char          *rrname,
		      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
		      gpointer             user_data)
{
  GWin32ResolverRequest *req;

  req = g_win32_resolver_request_new (resolver, free_lookup_service,
                                      cancellable, callback, user_data,
                                      lookup_service_async);
  req->u.service.rrname = g_strdup (rrname);

  QueueUserWorkItem (lookup_service_in_thread, req, 0);
}

static GList *
lookup_service_finish (GResolver     *resolver,
                       GAsyncResult  *result,
		       GError       **error)
{
  GSimpleAsyncResult *simple;
  GWin32ResolverRequest *req;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (resolver), lookup_service_async), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);

  req = g_simple_async_result_get_op_res_gpointer (simple);
  return _g_resolver_targets_from_DnsQuery (req->u.service.rrname,
                                            req->u.service.retval,
                                            req->u.service.results, error);
}


static void
g_win32_resolver_class_init (GWin32ResolverClass *win32_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (win32_class);

  resolver_class->lookup_by_name_async     = lookup_by_name_async;
  resolver_class->lookup_by_name_finish    = lookup_by_name_finish;
  resolver_class->lookup_by_address_async  = lookup_by_address_async;
  resolver_class->lookup_by_address_finish = lookup_by_address_finish;
  resolver_class->lookup_service_async     = lookup_service_async;
  resolver_class->lookup_service_finish    = lookup_service_finish;
}


/* Windows HANDLE GSource */

typedef struct {
  GSource source;
  GPollFD pollfd;
} GWin32HandleSource;

static gboolean
g_win32_handle_source_prepare (GSource *source,
                               gint    *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean
g_win32_handle_source_check (GSource *source)
{
  GWin32HandleSource *hsource = (GWin32HandleSource *)source;

  return hsource->pollfd.revents;
}

static gboolean
g_win32_handle_source_dispatch (GSource     *source,
                                GSourceFunc  callback,
                                gpointer     user_data)
{
  return (*callback) (user_data);
}

static void
g_win32_handle_source_finalize (GSource *source)
{
  ;
}

GSourceFuncs g_win32_handle_source_funcs = {
  g_win32_handle_source_prepare,
  g_win32_handle_source_check,
  g_win32_handle_source_dispatch,
  g_win32_handle_source_finalize
};

static GSource *
g_win32_handle_source_add (HANDLE      handle,
                           GSourceFunc callback,
                           gpointer    user_data)
{
  GWin32HandleSource *hsource;
  GSource *source;

  source = g_source_new (&g_win32_handle_source_funcs, sizeof (GWin32HandleSource));
  hsource = (GWin32HandleSource *)source;
  hsource->pollfd.fd = (gint)handle;
  hsource->pollfd.events = G_IO_IN;
  hsource->pollfd.revents = 0;
  g_source_add_poll (source, &hsource->pollfd);

  g_source_set_callback (source, callback, user_data, NULL);
  g_source_attach (source, g_main_context_get_thread_default ());
  return source;
}
