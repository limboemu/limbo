/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <stdlib.h>

#include "gdbusutils.h"
#include "gdbusnameowning.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"
#include "gdbusconnection.h"
#include "gio-marshal.h"

#include "glibintl.h"

/**
 * SECTION:gdbusnameowning
 * @title: Owning Bus Names
 * @short_description: Simple API for owning bus names
 * @include: gio/gio.h
 *
 * Convenience API for owning bus names.
 *
 * <example id="gdbus-owning-names"><title>Simple application owning a name</title><programlisting><xi:include xmlns:xi="http://www.w3.org/2001/XInclude" parse="text" href="../../../../gio/tests/gdbus-example-own-name.c"><xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback></xi:include></programlisting></example>
 */

G_LOCK_DEFINE_STATIC (lock);

/* ---------------------------------------------------------------------------------------------------- */

typedef enum
{
  PREVIOUS_CALL_NONE = 0,
  PREVIOUS_CALL_ACQUIRED,
  PREVIOUS_CALL_LOST,
} PreviousCall;

typedef struct
{
  volatile gint             ref_count;
  guint                     id;
  GBusNameOwnerFlags        flags;
  gchar                    *name;
  GBusAcquiredCallback      bus_acquired_handler;
  GBusNameAcquiredCallback  name_acquired_handler;
  GBusNameLostCallback      name_lost_handler;
  gpointer                  user_data;
  GDestroyNotify            user_data_free_func;
  GMainContext             *main_context;

  PreviousCall              previous_call;

  GDBusConnection          *connection;
  gulong                    disconnected_signal_handler_id;
  guint                     name_acquired_subscription_id;
  guint                     name_lost_subscription_id;

  gboolean                  cancelled;

  gboolean                  needs_release;
} Client;

static guint next_global_id = 1;
static GHashTable *map_id_to_client = NULL;


static Client *
client_ref (Client *client)
{
  g_atomic_int_inc (&client->ref_count);
  return client;
}

static void
client_unref (Client *client)
{
  if (g_atomic_int_dec_and_test (&client->ref_count))
    {
      if (client->connection != NULL)
        {
          if (client->disconnected_signal_handler_id > 0)
            g_signal_handler_disconnect (client->connection, client->disconnected_signal_handler_id);
          if (client->name_acquired_subscription_id > 0)
            g_dbus_connection_signal_unsubscribe (client->connection, client->name_acquired_subscription_id);
          if (client->name_lost_subscription_id > 0)
            g_dbus_connection_signal_unsubscribe (client->connection, client->name_lost_subscription_id);
          g_object_unref (client->connection);
        }
      if (client->main_context != NULL)
        g_main_context_unref (client->main_context);
      g_free (client->name);
      if (client->user_data_free_func != NULL)
        client->user_data_free_func (client->user_data);
      g_free (client);
    }
}

/* ---------------------------------------------------------------------------------------------------- */


typedef enum
{
  CALL_TYPE_NAME_ACQUIRED,
  CALL_TYPE_NAME_LOST
} CallType;

typedef struct
{
  Client *client;

  /* keep this separate because client->connection may
   * be set to NULL after scheduling the call
   */
  GDBusConnection *connection;

  /* set to TRUE to call acquired */
  CallType call_type;
} CallHandlerData;

static void
call_handler_data_free (CallHandlerData *data)
{
  if (data->connection != NULL)
    g_object_unref (data->connection);
  client_unref (data->client);
  g_free (data);
}

static void
actually_do_call (Client *client, GDBusConnection *connection, CallType call_type)
{
  switch (call_type)
    {
    case CALL_TYPE_NAME_ACQUIRED:
      if (client->name_acquired_handler != NULL)
        {
          client->name_acquired_handler (connection,
                                         client->name,
                                         client->user_data);
        }
      break;

    case CALL_TYPE_NAME_LOST:
      if (client->name_lost_handler != NULL)
        {
          client->name_lost_handler (connection,
                                     client->name,
                                     client->user_data);
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static gboolean
call_in_idle_cb (gpointer _data)
{
  CallHandlerData *data = _data;
  actually_do_call (data->client, data->connection, data->call_type);
  return FALSE;
}

static void
schedule_call_in_idle (Client *client, CallType  call_type)
{
  CallHandlerData *data;
  GSource *idle_source;

  data = g_new0 (CallHandlerData, 1);
  data->client = client_ref (client);
  data->connection = client->connection != NULL ? g_object_ref (client->connection) : NULL;
  data->call_type = call_type;

  idle_source = g_idle_source_new ();
  g_source_set_priority (idle_source, G_PRIORITY_HIGH);
  g_source_set_callback (idle_source,
                         call_in_idle_cb,
                         data,
                         (GDestroyNotify) call_handler_data_free);
  g_source_attach (idle_source, client->main_context);
  g_source_unref (idle_source);
}

static void
do_call (Client *client, CallType call_type)
{
  /* only schedule in idle if we're not in the right thread */
  if (g_main_context_get_thread_default () != client->main_context)
    schedule_call_in_idle (client, call_type);
  else
    actually_do_call (client, client->connection, call_type);
}

static void
call_acquired_handler (Client *client)
{
  if (client->previous_call != PREVIOUS_CALL_ACQUIRED)
    {
      client->previous_call = PREVIOUS_CALL_ACQUIRED;
      if (!client->cancelled)
        {
          do_call (client, CALL_TYPE_NAME_ACQUIRED);
        }
    }
}

static void
call_lost_handler (Client  *client)
{
  if (client->previous_call != PREVIOUS_CALL_LOST)
    {
      client->previous_call = PREVIOUS_CALL_LOST;
      if (!client->cancelled)
        {
          do_call (client, CALL_TYPE_NAME_LOST);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_name_lost_or_acquired (GDBusConnection  *connection,
                          const gchar      *sender_name,
                          const gchar      *object_path,
                          const gchar      *interface_name,
                          const gchar      *signal_name,
                          GVariant         *parameters,
                          gpointer          user_data)
{
  Client *client = user_data;
  const gchar *name;

  if (g_strcmp0 (object_path, "/org/freedesktop/DBus") != 0 ||
      g_strcmp0 (interface_name, "org.freedesktop.DBus") != 0 ||
      g_strcmp0 (sender_name, "org.freedesktop.DBus") != 0)
    goto out;

  if (g_strcmp0 (signal_name, "NameLost") == 0)
    {
      g_variant_get (parameters, "(&s)", &name);
      if (g_strcmp0 (name, client->name) == 0)
        {
          call_lost_handler (client);
        }
    }
  else if (g_strcmp0 (signal_name, "NameAcquired") == 0)
    {
      g_variant_get (parameters, "(&s)", &name);
      if (g_strcmp0 (name, client->name) == 0)
        {
          call_acquired_handler (client);
        }
    }
 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
request_name_cb (GObject      *source_object,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  Client *client = user_data;
  GVariant *result;
  guint32 request_name_reply;
  gboolean subscribe;

  request_name_reply = 0;
  result = NULL;

  result = g_dbus_connection_call_finish (client->connection,
                                          res,
                                          NULL);
  if (result != NULL)
    {
      g_variant_get (result, "(u)", &request_name_reply);
      g_variant_unref (result);
    }

  subscribe = FALSE;

  switch (request_name_reply)
    {
    case 1: /* DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER */
      /* We got the name - now listen for NameLost and NameAcquired */
      call_acquired_handler (client);
      subscribe = TRUE;
      client->needs_release = TRUE;
      break;

    case 2: /* DBUS_REQUEST_NAME_REPLY_IN_QUEUE */
      /* Waiting in line - listen for NameLost and NameAcquired */
      call_lost_handler (client);
      subscribe = TRUE;
      client->needs_release = TRUE;
      break;

    default:
      /* assume we couldn't get the name - explicit fallthrough */
    case 3: /* DBUS_REQUEST_NAME_REPLY_EXISTS */
    case 4: /* DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER */
      /* Some other part of the process is already owning the name */
      call_lost_handler (client);
      break;
    }

  if (subscribe)
    {
      /* start listening to NameLost and NameAcquired messages */
      client->name_lost_subscription_id =
        g_dbus_connection_signal_subscribe (client->connection,
                                            "org.freedesktop.DBus",
                                            "org.freedesktop.DBus",
                                            "NameLost",
                                            "/org/freedesktop/DBus",
                                            client->name,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            on_name_lost_or_acquired,
                                            client,
                                            NULL);
      client->name_acquired_subscription_id =
        g_dbus_connection_signal_subscribe (client->connection,
                                            "org.freedesktop.DBus",
                                            "org.freedesktop.DBus",
                                            "NameAcquired",
                                            "/org/freedesktop/DBus",
                                            client->name,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            on_name_lost_or_acquired,
                                            client,
                                            NULL);
    }

  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_connection_disconnected (GDBusConnection *connection,
                            gboolean         remote_peer_vanished,
                            GError          *error,
                            gpointer         user_data)
{
  Client *client = user_data;

  if (client->disconnected_signal_handler_id > 0)
    g_signal_handler_disconnect (client->connection, client->disconnected_signal_handler_id);
  if (client->name_acquired_subscription_id > 0)
    g_dbus_connection_signal_unsubscribe (client->connection, client->name_acquired_subscription_id);
  if (client->name_lost_subscription_id > 0)
    g_dbus_connection_signal_unsubscribe (client->connection, client->name_lost_subscription_id);
  g_object_unref (client->connection);
  client->disconnected_signal_handler_id = 0;
  client->name_acquired_subscription_id = 0;
  client->name_lost_subscription_id = 0;
  client->connection = NULL;

  call_lost_handler (client);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
has_connection (Client *client)
{
  /* listen for disconnection */
  client->disconnected_signal_handler_id = g_signal_connect (client->connection,
                                                             "closed",
                                                             G_CALLBACK (on_connection_disconnected),
                                                             client);

  /* attempt to acquire the name */
  g_dbus_connection_call (client->connection,
                          "org.freedesktop.DBus",  /* bus name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "RequestName",           /* method name */
                          g_variant_new ("(su)",
                                         client->name,
                                         client->flags),
                          G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) request_name_cb,
                          client_ref (client));
}


static void
connection_get_cb (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
  Client *client = user_data;

  client->connection = g_bus_get_finish (res, NULL);
  if (client->connection == NULL)
    {
      call_lost_handler (client);
      goto out;
    }

  /* No need to schedule this in idle as we're already in the thread
   * that the user called g_bus_own_name() from. This is because
   * g_bus_get() guarantees that.
   *
   * Also, we need to ensure that the handler is invoked *before*
   * we call RequestName(). Otherwise there is a race.
   */
  if (client->bus_acquired_handler != NULL)
    {
      client->bus_acquired_handler (client->connection,
                                    client->name,
                                    client->user_data);
    }

  has_connection (client);

 out:
  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_bus_own_name_on_connection:
 * @connection: A #GDBusConnection.
 * @name: The well-known name to own.
 * @flags: A set of flags from the #GBusNameOwnerFlags enumeration.
 * @name_acquired_handler: Handler to invoke when @name is acquired or %NULL.
 * @name_lost_handler: Handler to invoke when @name is lost or %NULL.
 * @user_data: User data to pass to handlers.
 * @user_data_free_func: Function for freeing @user_data or %NULL.
 *
 * Like g_bus_own_name() but takes a #GDBusConnection instead of a
 * #GBusType.
 *
 * Returns: An identifier (never 0) that an be used with
 * g_bus_unown_name() to stop owning the name.
 *
 * Since: 2.26
 */
guint
g_bus_own_name_on_connection (GDBusConnection          *connection,
                              const gchar              *name,
                              GBusNameOwnerFlags        flags,
                              GBusNameAcquiredCallback  name_acquired_handler,
                              GBusNameLostCallback      name_lost_handler,
                              gpointer                  user_data,
                              GDestroyNotify            user_data_free_func)
{
  Client *client;

  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (g_dbus_is_name (name) && !g_dbus_is_unique_name (name), 0);

  G_LOCK (lock);

  client = g_new0 (Client, 1);
  client->ref_count = 1;
  client->id = next_global_id++; /* TODO: uh oh, handle overflow */
  client->name = g_strdup (name);
  client->flags = flags;
  client->name_acquired_handler = name_acquired_handler;
  client->name_lost_handler = name_lost_handler;
  client->user_data = user_data;
  client->user_data_free_func = user_data_free_func;
  client->main_context = g_main_context_get_thread_default ();
  if (client->main_context != NULL)
    g_main_context_ref (client->main_context);

  client->connection = g_object_ref (connection);

  if (map_id_to_client == NULL)
    {
      map_id_to_client = g_hash_table_new (g_direct_hash, g_direct_equal);
    }
  g_hash_table_insert (map_id_to_client,
                       GUINT_TO_POINTER (client->id),
                       client);

  G_UNLOCK (lock);

  has_connection (client);

  return client->id;
}

/**
 * g_bus_own_name:
 * @bus_type: The type of bus to own a name on.
 * @name: The well-known name to own.
 * @flags: A set of flags from the #GBusNameOwnerFlags enumeration.
 * @bus_acquired_handler: Handler to invoke when connected to the bus of type @bus_type or %NULL.
 * @name_acquired_handler: Handler to invoke when @name is acquired or %NULL.
 * @name_lost_handler: Handler to invoke when @name is lost or %NULL.
 * @user_data: User data to pass to handlers.
 * @user_data_free_func: Function for freeing @user_data or %NULL.
 *
 * Starts acquiring @name on the bus specified by @bus_type and calls
 * @name_acquired_handler and @name_lost_handler when the name is
 * acquired respectively lost. Callbacks will be invoked in the <link
 * linkend="g-main-context-push-thread-default">thread-default main
 * loop</link> of the thread you are calling this function from.
 *
 * You are guaranteed that one of the @name_acquired_handler and @name_lost_handler
 * callbacks will be invoked after calling this function - there are three
 * possible cases:
 * <itemizedlist>
 *   <listitem><para>
 *     @name_lost_handler with a %NULL connection (if a connection to the bus can't be made).
 *   </para></listitem>
 *   <listitem><para>
 *     @bus_acquired_handler then @name_lost_handler (if the name can't be obtained)
 *   </para></listitem>
 *   <listitem><para>
 *     @bus_acquired_handler then @name_acquired_handler (if the name was obtained).
 *   </para></listitem>
 * </itemizedlist>
 * When you are done owning the name, just call g_bus_unown_name()
 * with the owner id this function returns.
 *
 * If the name is acquired or lost (for example another application
 * could acquire the name if you allow replacement or the application
 * currently owning the name exits), the handlers are also invoked. If the
 * #GDBusConnection that is used for attempting to own the name
 * closes, then @name_lost_handler is invoked since it is no
 * longer possible for other processes to access the process.
 *
 * You cannot use g_bus_own_name() several times for the same name (unless
 * interleaved with calls to g_bus_unown_name()) - only the first call
 * will work.
 *
 * Another guarantee is that invocations of @name_acquired_handler
 * and @name_lost_handler are guaranteed to alternate; that
 * is, if @name_acquired_handler is invoked then you are
 * guaranteed that the next time one of the handlers is invoked, it
 * will be @name_lost_handler. The reverse is also true.
 *
 * If you plan on exporting objects (using e.g.
 * g_dbus_connection_register_object()), note that it is generally too late
 * to export the objects in @name_acquired_handler. Instead, you can do this
 * in @bus_acquired_handler since you are guaranteed that this will run
 * before @name is requested from the bus.
 *
 * This behavior makes it very simple to write applications that wants
 * to own names and export objects, see <xref linkend="gdbus-owning-names"/>.
 * Simply register objects to be exported in @bus_acquired_handler and
 * unregister the objects (if any) in @name_lost_handler.
 *
 * Returns: An identifier (never 0) that an be used with
 * g_bus_unown_name() to stop owning the name.
 *
 * Since: 2.26
 */
guint
g_bus_own_name (GBusType                  bus_type,
                const gchar              *name,
                GBusNameOwnerFlags        flags,
                GBusAcquiredCallback      bus_acquired_handler,
                GBusNameAcquiredCallback  name_acquired_handler,
                GBusNameLostCallback      name_lost_handler,
                gpointer                  user_data,
                GDestroyNotify            user_data_free_func)
{
  Client *client;

  g_return_val_if_fail (g_dbus_is_name (name) && !g_dbus_is_unique_name (name), 0);

  G_LOCK (lock);

  client = g_new0 (Client, 1);
  client->ref_count = 1;
  client->id = next_global_id++; /* TODO: uh oh, handle overflow */
  client->name = g_strdup (name);
  client->flags = flags;
  client->bus_acquired_handler = bus_acquired_handler;
  client->name_acquired_handler = name_acquired_handler;
  client->name_lost_handler = name_lost_handler;
  client->user_data = user_data;
  client->user_data_free_func = user_data_free_func;
  client->main_context = g_main_context_get_thread_default ();
  if (client->main_context != NULL)
    g_main_context_ref (client->main_context);

  if (map_id_to_client == NULL)
    {
      map_id_to_client = g_hash_table_new (g_direct_hash, g_direct_equal);
    }
  g_hash_table_insert (map_id_to_client,
                       GUINT_TO_POINTER (client->id),
                       client);

  g_bus_get (bus_type,
             NULL,
             connection_get_cb,
             client_ref (client));

  G_UNLOCK (lock);

  return client->id;
}

typedef struct {
  GClosure *bus_acquired_closure;
  GClosure *name_acquired_closure;
  GClosure *name_lost_closure;
} OwnNameData;

static OwnNameData *
own_name_data_new (GClosure *bus_acquired_closure,
                   GClosure *name_acquired_closure,
                   GClosure *name_lost_closure)
{
  OwnNameData *data;

  data = g_new0 (OwnNameData, 1);

  if (bus_acquired_closure != NULL)
    {
      data->bus_acquired_closure = g_closure_ref (bus_acquired_closure);
      g_closure_sink (bus_acquired_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (bus_acquired_closure))
        g_closure_set_marshal (bus_acquired_closure, _gio_marshal_VOID__STRING);
    }

  if (name_acquired_closure != NULL)
    {
      data->name_acquired_closure = g_closure_ref (name_acquired_closure);
      g_closure_sink (name_acquired_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (name_acquired_closure))
        g_closure_set_marshal (name_acquired_closure, _gio_marshal_VOID__STRING);
    }

  if (name_lost_closure != NULL)
    {
      data->name_lost_closure = g_closure_ref (name_lost_closure);
      g_closure_sink (name_lost_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (name_lost_closure))
        g_closure_set_marshal (name_lost_closure, _gio_marshal_VOID__STRING);
    }

  return data;
}

static void
own_with_closures_on_bus_acquired (GDBusConnection *connection,
                                   const gchar     *name,
                                   gpointer         user_data)
{
  OwnNameData *data = user_data;
  GValue params[2] = { { 0, }, { 0, } };

  g_value_init (&params[0], G_TYPE_DBUS_CONNECTION);
  g_value_set_object (&params[0], connection);

  g_value_init (&params[1], G_TYPE_STRING);
  g_value_set_string (&params[1], name);

  g_closure_invoke (data->bus_acquired_closure, NULL, 2, params, NULL);

  g_value_unset (params + 0);
  g_value_unset (params + 1);
}

static void
own_with_closures_on_name_acquired (GDBusConnection *connection,
                                    const gchar     *name,
                                    gpointer         user_data)
{
  OwnNameData *data = user_data;
  GValue params[2] = { { 0, }, { 0, } };

  g_value_init (&params[0], G_TYPE_DBUS_CONNECTION);
  g_value_set_object (&params[0], connection);

  g_value_init (&params[1], G_TYPE_STRING);
  g_value_set_string (&params[1], name);

  g_closure_invoke (data->name_acquired_closure, NULL, 2, params, NULL);

  g_value_unset (params + 0);
  g_value_unset (params + 1);
}

static void
own_with_closures_on_name_lost (GDBusConnection *connection,
                                const gchar     *name,
                                gpointer         user_data)
{
  OwnNameData *data = user_data;
  GValue params[2] = { { 0, }, { 0, } };

  g_value_init (&params[0], G_TYPE_DBUS_CONNECTION);
  g_value_set_object (&params[0], connection);

  g_value_init (&params[1], G_TYPE_STRING);
  g_value_set_string (&params[1], name);

  g_closure_invoke (data->name_lost_closure, NULL, 2, params, NULL);

  g_value_unset (params + 0);
  g_value_unset (params + 1);
}

static void
bus_own_name_free_func (gpointer user_data)
{
  OwnNameData *data = user_data;

  if (data->bus_acquired_closure != NULL)
    g_closure_unref (data->bus_acquired_closure);

  if (data->name_acquired_closure != NULL)
    g_closure_unref (data->name_acquired_closure);

  if (data->name_lost_closure != NULL)
    g_closure_unref (data->name_lost_closure);

  g_free (data);
}

/**
 * g_bus_own_name_with_closures:
 * @bus_type: The type of bus to own a name on.
 * @name: The well-known name to own.
 * @flags: A set of flags from the #GBusNameOwnerFlags enumeration.
 * @bus_acquired_closure: (allow-none): #GClosure to invoke when connected to
 * the bus of type @bus_type or %NULL.
 * @name_acquired_closure: (allow-none): #GClosure to invoke when @name is
 * acquired or %NULL.
 * @name_lost_closure: (allow-none): #GClosure to invoke when @name is lost or
 * %NULL.
 *
 * Version of g_bus_own_name() using closures instead of callbacks for
 * easier binding in other languages.
 *
 * Returns: An identifier (never 0) that an be used with
 * g_bus_unown_name() to stop owning the name.
 *
 * Rename to: g_bus_own_name
 *
 * Since: 2.26
 */
guint
g_bus_own_name_with_closures (GBusType                  bus_type,
                              const gchar              *name,
                              GBusNameOwnerFlags        flags,
                              GClosure                 *bus_acquired_closure,
                              GClosure                 *name_acquired_closure,
                              GClosure                 *name_lost_closure)
{
  return g_bus_own_name (bus_type,
          name,
          flags,
          bus_acquired_closure != NULL ? own_with_closures_on_bus_acquired : NULL,
          name_acquired_closure != NULL ? own_with_closures_on_name_acquired : NULL,
          name_lost_closure != NULL ? own_with_closures_on_name_lost : NULL,
          own_name_data_new (bus_acquired_closure,
                             name_acquired_closure,
                             name_lost_closure),
          bus_own_name_free_func);
}

/**
 * g_bus_own_name_on_connection_with_closures:
 * @connection: A #GDBusConnection.
 * @name: The well-known name to own.
 * @flags: A set of flags from the #GBusNameOwnerFlags enumeration.
 * @name_acquired_closure: (allow-none): #GClosure to invoke when @name is
 * acquired or %NULL.
 * @name_lost_closure: (allow-none): #GClosure to invoke when @name is lost or
 * %NULL.
 *
 * Version of g_bus_own_name_on_connection() using closures instead of callbacks for
 * easier binding in other languages.
 *
 * Returns: An identifier (never 0) that an be used with
 * g_bus_unown_name() to stop owning the name.
 *
 * Rename to: g_bus_own_name_on_connection
 *
 * Since: 2.26
 */
guint
g_bus_own_name_on_connection_with_closures (GDBusConnection          *connection,
                                            const gchar              *name,
                                            GBusNameOwnerFlags        flags,
                                            GClosure                 *name_acquired_closure,
                                            GClosure                 *name_lost_closure)
{
  return g_bus_own_name_on_connection (connection,
          name,
          flags,
          name_acquired_closure != NULL ? own_with_closures_on_name_acquired : NULL,
          name_lost_closure != NULL ? own_with_closures_on_name_lost : NULL,
          own_name_data_new (NULL,
                             name_acquired_closure,
                             name_lost_closure),
          bus_own_name_free_func);
}

/**
 * g_bus_unown_name:
 * @owner_id: An identifier obtained from g_bus_own_name()
 *
 * Stops owning a name.
 *
 * Since: 2.26
 */
void
g_bus_unown_name (guint owner_id)
{
  Client *client;

  g_return_if_fail (owner_id > 0);

  client = NULL;

  G_LOCK (lock);
  if (owner_id == 0 || map_id_to_client == NULL ||
      (client = g_hash_table_lookup (map_id_to_client, GUINT_TO_POINTER (owner_id))) == NULL)
    {
      g_warning ("Invalid id %d passed to g_bus_unown_name()", owner_id);
      goto out;
    }

  client->cancelled = TRUE;
  g_warn_if_fail (g_hash_table_remove (map_id_to_client, GUINT_TO_POINTER (owner_id)));

 out:
  G_UNLOCK (lock);

  /* do callback without holding lock */
  if (client != NULL)
    {
      /* Release the name if needed */
      if (client->needs_release && client->connection != NULL)
        {
          GVariant *result;
          GError *error;
          guint32 release_name_reply;

          /* TODO: it kinda sucks having to do a sync call to release the name - but if
           * we don't, then a subsequent grab of the name will make the bus daemon return
           * IN_QUEUE which will trigger name_lost().
           *
           * I believe this is a bug in the bus daemon.
           */
          error = NULL;
          result = g_dbus_connection_call_sync (client->connection,
                                                "org.freedesktop.DBus",  /* bus name */
                                                "/org/freedesktop/DBus", /* object path */
                                                "org.freedesktop.DBus",  /* interface name */
                                                "ReleaseName",           /* method name */
                                                g_variant_new ("(s)", client->name),
                                                G_VARIANT_TYPE ("(u)"),
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &error);
          if (result == NULL)
            {
              g_warning ("Error releasing name %s: %s", client->name, error->message);
              g_error_free (error);
            }
          else
            {
              g_variant_get (result, "(u)", &release_name_reply);
              if (release_name_reply != 1 /* DBUS_RELEASE_NAME_REPLY_RELEASED */)
                {
                  g_warning ("Unexpected reply %d when releasing name %s", release_name_reply, client->name);
                }
              g_variant_unref (result);
            }
        }

      if (client->disconnected_signal_handler_id > 0)
        g_signal_handler_disconnect (client->connection, client->disconnected_signal_handler_id);
      if (client->name_acquired_subscription_id > 0)
        g_dbus_connection_signal_unsubscribe (client->connection, client->name_acquired_subscription_id);
      if (client->name_lost_subscription_id > 0)
        g_dbus_connection_signal_unsubscribe (client->connection, client->name_lost_subscription_id);
      client->disconnected_signal_handler_id = 0;
      client->name_acquired_subscription_id = 0;
      client->name_lost_subscription_id = 0;
      if (client->connection != NULL)
        {
          g_object_unref (client->connection);
          client->connection = NULL;
        }

      client_unref (client);
    }
}
