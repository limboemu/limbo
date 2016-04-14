/* GLib testing framework examples and tests
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

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

#include "gdbus-tests.h"

/* all tests rely on a global connection */
static GDBusConnection *c = NULL;

/* all tests rely on a shared mainloop */
static GMainLoop *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
/* Ensure that signal and method replies are delivered in the right thread */
/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  GThread *thread;
  GMainLoop *thread_loop;
  guint signal_count;
} DeliveryData;

static void
msg_cb_expect_success (GDBusConnection *connection,
                       GAsyncResult    *res,
                       gpointer         user_data)
{
  DeliveryData *data = user_data;
  GError *error;
  GVariant *result;

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_variant_unref (result);

  g_assert (g_thread_self () == data->thread);

  g_main_loop_quit (data->thread_loop);
}

static void
msg_cb_expect_error_cancelled (GDBusConnection *connection,
                               GAsyncResult    *res,
                               gpointer         user_data)
{
  DeliveryData *data = user_data;
  GError *error;
  GVariant *result;

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert (!g_dbus_error_is_remote_error (error));
  g_error_free (error);
  g_assert (result == NULL);

  g_assert (g_thread_self () == data->thread);

  g_main_loop_quit (data->thread_loop);
}

static void
signal_handler (GDBusConnection *connection,
                const gchar      *sender_name,
                const gchar      *object_path,
                const gchar      *interface_name,
                const gchar      *signal_name,
                GVariant         *parameters,
                gpointer         user_data)
{
  DeliveryData *data = user_data;

  g_assert (g_thread_self () == data->thread);

  data->signal_count++;

  g_main_loop_quit (data->thread_loop);
}

static gpointer
test_delivery_in_thread_func (gpointer _data)
{
  GMainLoop *thread_loop;
  GMainContext *thread_context;
  DeliveryData data;
  GCancellable *ca;
  guint subscription_id;
  GDBusConnection *priv_c;
  GError *error;

  error = NULL;

  thread_context = g_main_context_new ();
  thread_loop = g_main_loop_new (thread_context, FALSE);
  g_main_context_push_thread_default (thread_context);

  data.thread = g_thread_self ();
  data.thread_loop = thread_loop;
  data.signal_count = 0;

  /* ---------------------------------------------------------------------------------------------------- */

  /*
   * Check that we get a reply to the GetId() method call.
   */
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) msg_cb_expect_success,
                          &data);
  g_main_loop_run (thread_loop);

  /*
   * Check that we never actually send a message if the GCancellable
   * is already cancelled - i.e.  we should get #G_IO_ERROR_CANCELLED
   * when the actual connection is not up.
   */
  ca = g_cancellable_new ();
  g_cancellable_cancel (ca);
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (GAsyncReadyCallback) msg_cb_expect_error_cancelled,
                          &data);
  g_main_loop_run (thread_loop);
  g_object_unref (ca);

  /*
   * Check that cancellation works when the message is already in flight.
   */
  ca = g_cancellable_new ();
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (GAsyncReadyCallback) msg_cb_expect_error_cancelled,
                          &data);
  g_cancellable_cancel (ca);
  g_main_loop_run (thread_loop);
  g_object_unref (ca);

  /*
   * Check that signals are delivered to the correct thread.
   *
   * First we subscribe to the signal, then we create a a private
   * connection. This should cause a NameOwnerChanged message from
   * the message bus.
   */
  subscription_id = g_dbus_connection_signal_subscribe (c,
                                                        "org.freedesktop.DBus",  /* sender */
                                                        "org.freedesktop.DBus",  /* interface */
                                                        "NameOwnerChanged",      /* member */
                                                        "/org/freedesktop/DBus", /* path */
                                                        NULL,
                                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                                        signal_handler,
                                                        &data,
                                                        NULL);
  g_assert (subscription_id != 0);
  g_assert (data.signal_count == 0);

  priv_c = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (priv_c != NULL);

  g_main_loop_run (thread_loop);
  g_assert (data.signal_count == 1);

  g_object_unref (priv_c);

  g_dbus_connection_signal_unsubscribe (c, subscription_id);

  /* ---------------------------------------------------------------------------------------------------- */

  g_main_context_pop_thread_default (thread_context);
  g_main_loop_unref (thread_loop);
  g_main_context_unref (thread_context);

  g_main_loop_quit (loop);

  return NULL;
}

static void
test_delivery_in_thread (void)
{
  GError *error;
  GThread *thread;

  error = NULL;
  thread = g_thread_create (test_delivery_in_thread_func,
                            NULL,
                            TRUE,
                            &error);
  g_assert_no_error (error);
  g_assert (thread != NULL);

  /* run the event loop - it is needed to dispatch D-Bus messages */
  g_main_loop_run (loop);

  g_thread_join (thread);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  GDBusProxy *proxy;
  gint msec;
  guint num;
  gboolean async;

  GMainLoop *thread_loop;
  GThread *thread;

  gboolean done;
} SyncThreadData;

static void
sleep_cb (GDBusProxy   *proxy,
          GAsyncResult *res,
          gpointer      user_data)
{
  SyncThreadData *data = user_data;
  GError *error;
  GVariant *result;

  error = NULL;
  result = g_dbus_proxy_call_finish (proxy,
                                     res,
                                     &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_assert_cmpstr (g_variant_get_type_string (result), ==, "()");
  g_variant_unref (result);

  g_assert (data->thread == g_thread_self ());

  g_main_loop_quit (data->thread_loop);

  //g_debug ("async cb (%p)", g_thread_self ());
}

static gpointer
test_sleep_in_thread_func (gpointer _data)
{
  SyncThreadData *data = _data;
  GMainContext *thread_context;
  guint n;

  thread_context = g_main_context_new ();
  data->thread_loop = g_main_loop_new (thread_context, FALSE);
  g_main_context_push_thread_default (thread_context);

  data->thread = g_thread_self ();

  for (n = 0; n < data->num; n++)
    {
      if (data->async)
        {
          //g_debug ("invoking async (%p)", g_thread_self ());
          g_dbus_proxy_call (data->proxy,
                             "Sleep",
                             g_variant_new ("(i)", data->msec),
                             G_DBUS_CALL_FLAGS_NONE,
                             -1,
                             NULL,
                             (GAsyncReadyCallback) sleep_cb,
                             data);
          g_main_loop_run (data->thread_loop);
          g_print ("A");
          //g_debug ("done invoking async (%p)", g_thread_self ());
        }
      else
        {
          GError *error;
          GVariant *result;

          error = NULL;
          //g_debug ("invoking sync (%p)", g_thread_self ());
          result = g_dbus_proxy_call_sync (data->proxy,
                                           "Sleep",
                                           g_variant_new ("(i)", data->msec),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
          g_print ("S");
          //g_debug ("done invoking sync (%p)", g_thread_self ());
          g_assert_no_error (error);
          g_assert (result != NULL);
          g_assert_cmpstr (g_variant_get_type_string (result), ==, "()");
          g_variant_unref (result);
        }
    }

  g_main_context_pop_thread_default (thread_context);
  g_main_loop_unref (data->thread_loop);
  g_main_context_unref (thread_context);

  data->done = TRUE;
  g_main_loop_quit (loop);

  return NULL;
}

static void
test_method_calls_on_proxy (GDBusProxy *proxy)
{
  guint n;

  /*
   * Check that multiple threads can do calls without interferring with
   * each other. We do this by creating three threads that call the
   * Sleep() method on the server (which handles it asynchronously, e.g.
   * it won't block other requests) with different sleep durations and
   * a number of times. We do this so each set of calls add up to 4000
   * milliseconds.
   *
   * The dbus test server that this code calls into uses glib timeouts
   * to do the sleeping which have only a granularity of 1ms.  It is
   * therefore possible to lose as much as 40ms; the test could finish
   * in slightly less than 4 seconds.
   *
   * We run this test twice - first with async calls in each thread, then
   * again with sync calls
   */

  for (n = 0; n < 2; n++)
    {
      gboolean do_async;
      GThread *thread1;
      GThread *thread2;
      GThread *thread3;
      SyncThreadData data1;
      SyncThreadData data2;
      SyncThreadData data3;
      GError *error;
      GTimeVal start_time;
      GTimeVal end_time;
      guint elapsed_msec;

      error = NULL;
      do_async = (n == 0);

      g_get_current_time (&start_time);

      data1.proxy = proxy;
      data1.msec = 40;
      data1.num = 100;
      data1.async = do_async;
      data1.done = FALSE;
      thread1 = g_thread_create (test_sleep_in_thread_func,
                                 &data1,
                                 TRUE,
                                 &error);
      g_assert_no_error (error);
      g_assert (thread1 != NULL);

      data2.proxy = proxy;
      data2.msec = 20;
      data2.num = 200;
      data2.async = do_async;
      data2.done = FALSE;
      thread2 = g_thread_create (test_sleep_in_thread_func,
                                 &data2,
                                 TRUE,
                                 &error);
      g_assert_no_error (error);
      g_assert (thread2 != NULL);

      data3.proxy = proxy;
      data3.msec = 100;
      data3.num = 40;
      data3.async = do_async;
      data3.done = FALSE;
      thread3 = g_thread_create (test_sleep_in_thread_func,
                                 &data3,
                                 TRUE,
                                 &error);
      g_assert_no_error (error);
      g_assert (thread3 != NULL);

      /* we handle messages in the main loop - threads will quit it when they are done */
      while (!(data1.done && data2.done && data3.done))
        g_main_loop_run (loop);

      g_thread_join (thread1);
      g_thread_join (thread2);
      g_thread_join (thread3);

      g_get_current_time (&end_time);

      elapsed_msec = ((end_time.tv_sec * G_USEC_PER_SEC + end_time.tv_usec) -
                      (start_time.tv_sec * G_USEC_PER_SEC + start_time.tv_usec)) / 1000;

      //g_debug ("Elapsed time for %s = %d msec", n == 0 ? "async" : "sync", elapsed_msec);

      /* elapsed_msec should be 4000 msec +/- change for overhead/inaccuracy */
      g_assert_cmpint (elapsed_msec, >=, 3950);
      g_assert_cmpint (elapsed_msec,  <, 6000);

      g_print (" ");
    }

  g_main_loop_quit (loop);
}

static void
test_method_calls_in_thread (void)
{
  GDBusProxy *proxy;
  GDBusConnection *connection;
  GError *error;
  gchar *name_owner;

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                               NULL,
                               &error);
  g_assert_no_error (error);
  error = NULL;
  proxy = g_dbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,                      /* GDBusInterfaceInfo */
                                 "com.example.TestService", /* name */
                                 "/com/example/TestObject", /* object path */
                                 "com.example.Frob",        /* interface */
                                 NULL, /* GCancellable */
                                 &error);
  g_assert_no_error (error);

  name_owner = g_dbus_proxy_get_name_owner (proxy);
  g_assert_cmpstr (name_owner, !=, NULL);
  g_free (name_owner);

  test_method_calls_on_proxy (proxy);

  g_object_unref (proxy);
  g_object_unref (connection);
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  GError *error;
  gint ret;

  g_type_init ();
  g_thread_init (NULL);
  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* all the tests use a session bus with a well-known address that we can bring up and down
   * using session_bus_up() and session_bus_down().
   */
  g_unsetenv ("DISPLAY");
  g_setenv ("DBUS_SESSION_BUS_ADDRESS", session_bus_get_temporary_address (), TRUE);

  session_bus_up ();

  /* TODO: wait a bit for the bus to come up.. ideally session_bus_up() won't return
   * until one can connect to the bus but that's not how things work right now
   */
  usleep (500 * 1000);

  /* this is safe; testserver will exit once the bus goes away */
  g_assert (g_spawn_command_line_async (SRCDIR "/gdbus-testserver.py", NULL));

  /* wait for the service to come up */
  usleep (500 * 1000);

  /* Create the connection in the main thread */
  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  g_test_add_func ("/gdbus/delivery-in-thread", test_delivery_in_thread);
  g_test_add_func ("/gdbus/method-calls-in-thread", test_method_calls_in_thread);

  ret = g_test_run();

  g_object_unref (c);

  /* tear down bus */
  session_bus_down ();

  return ret;
}
