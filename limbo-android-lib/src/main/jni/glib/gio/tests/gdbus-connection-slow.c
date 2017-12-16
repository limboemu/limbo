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

#include <sys/types.h>
#include <sys/wait.h>

#include "gdbus-tests.h"

/* all tests rely on a shared mainloop */
static GMainLoop *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_flush_signal_handler (GDBusConnection  *connection,
                                      const gchar      *sender_name,
                                      const gchar      *object_path,
                                      const gchar      *interface_name,
                                      const gchar      *signal_name,
                                      GVariant         *parameters,
                                      gpointer         user_data)
{
  g_main_loop_quit (loop);
}

static gboolean
test_connection_flush_on_timeout (gpointer user_data)
{
  guint iteration = GPOINTER_TO_UINT (user_data);
  g_printerr ("Timeout waiting 1000 msec on iteration %d\n", iteration);
  g_assert_not_reached ();
  return FALSE;
}

static void
test_connection_flush (void)
{
  GDBusConnection *connection;
  GError *error;
  guint n;
  guint signal_handler_id;

  session_bus_up ();

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (connection != NULL);

  signal_handler_id = g_dbus_connection_signal_subscribe (connection,
                                                          NULL, /* sender */
                                                          "org.gtk.GDBus.FlushInterface",
                                                          "SomeSignal",
                                                          "/org/gtk/GDBus/FlushObject",
                                                          NULL,
                                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                                          test_connection_flush_signal_handler,
                                                          NULL,
                                                          NULL);
  g_assert_cmpint (signal_handler_id, !=, 0);

  for (n = 0; n < 50; n++)
    {
      gboolean ret;
      gint exit_status;
      guint timeout_mainloop_id;

      error = NULL;
      ret = g_spawn_command_line_sync ("./gdbus-connection-flush-helper",
                                       NULL, /* stdout */
                                       NULL, /* stderr */
                                       &exit_status,
                                       &error);
      g_assert_no_error (error);
      g_assert (WIFEXITED (exit_status));
      g_assert_cmpint (WEXITSTATUS (exit_status), ==, 0);
      g_assert (ret);

      timeout_mainloop_id = g_timeout_add (1000, test_connection_flush_on_timeout, GUINT_TO_POINTER (n));
      g_main_loop_run (loop);
      g_source_remove (timeout_mainloop_id);
    }

  g_dbus_connection_signal_unsubscribe (connection, signal_handler_id);
  _g_object_wait_for_single_ref (connection);
  g_object_unref (connection);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

/* Message size > 20MiB ... should be enough to make sure the message
 * is fragmented when shoved across any transport
 */
#define LARGE_MESSAGE_STRING_LENGTH (20*1024*1024)

static void
large_message_on_name_appeared (GDBusConnection *connection,
                                const gchar *name,
                                const gchar *name_owner,
                                gpointer user_data)
{
  GError *error;
  gchar *request;
  const gchar *reply;
  GVariant *result;
  guint n;

  request = g_new (gchar, LARGE_MESSAGE_STRING_LENGTH + 1);
  for (n = 0; n < LARGE_MESSAGE_STRING_LENGTH; n++)
    request[n] = '0' + (n%10);
  request[n] = '\0';

  error = NULL;
  result = g_dbus_connection_call_sync (connection,
                                        "com.example.TestService",      /* bus name */
                                        "/com/example/TestObject",      /* object path */
                                        "com.example.Frob",             /* interface name */
                                        "HelloWorld",                   /* method name */
                                        g_variant_new ("(s)", request), /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_variant_get (result, "(&s)", &reply);
  g_assert_cmpint (strlen (reply), >, LARGE_MESSAGE_STRING_LENGTH);
  g_assert (g_str_has_prefix (reply, "You greeted me with '01234567890123456789012"));
  g_assert (g_str_has_suffix (reply, "6789'. Thanks!"));
  g_variant_unref (result);

  g_free (request);

  g_main_loop_quit (loop);
}

static void
large_message_on_name_vanished (GDBusConnection *connection,
                                const gchar *name,
                                gpointer user_data)
{
}

static void
test_connection_large_message (void)
{
  guint watcher_id;

  session_bus_up ();

  /* this is safe; testserver will exit once the bus goes away */
  g_assert (g_spawn_command_line_async (SRCDIR "/gdbus-testserver.py", NULL));

  watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                 "com.example.TestService",
                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                 large_message_on_name_appeared,
                                 large_message_on_name_vanished,
                                 NULL,  /* user_data */
                                 NULL); /* GDestroyNotify */
  g_main_loop_run (loop);
  g_bus_unwatch_name (watcher_id);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* all the tests use a session bus with a well-known address that we can bring up and down
   * using session_bus_up() and session_bus_down().
   */
  g_unsetenv ("DISPLAY");
  g_setenv ("DBUS_SESSION_BUS_ADDRESS", session_bus_get_temporary_address (), TRUE);

  g_test_add_func ("/gdbus/connection/flush", test_connection_flush);
  g_test_add_func ("/gdbus/connection/large_message", test_connection_large_message);
  return g_test_run();
}
