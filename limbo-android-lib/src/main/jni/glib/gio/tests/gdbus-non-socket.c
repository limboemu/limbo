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

#include <stdlib.h>

#ifdef G_OS_UNIX
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixconnection.h>
#endif

#include "gdbus-tests.h"

static GMainLoop *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
#ifdef G_OS_UNIX

#define MY_TYPE_IO_STREAM  (my_io_stream_get_type ())
#define MY_IO_STREAM(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MY_TYPE_IO_STREAM, MyIOStream))
#define MY_IS_IO_STREAM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MY_TYPE_IO_STREAM))

typedef struct
{
  GIOStream parent_instance;
  GInputStream *input_stream;
  GOutputStream *output_stream;
} MyIOStream;

typedef struct
{
  GIOStreamClass parent_class;
} MyIOStreamClass;

static GType my_io_stream_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (MyIOStream, my_io_stream, G_TYPE_IO_STREAM);

static void
my_io_stream_finalize (GObject *object)
{
  MyIOStream *stream = MY_IO_STREAM (object);
  g_object_unref (stream->input_stream);
  g_object_unref (stream->output_stream);
  G_OBJECT_CLASS (my_io_stream_parent_class)->finalize (object);
}

static void
my_io_stream_init (MyIOStream *stream)
{
}

static GInputStream *
my_io_stream_get_input_stream (GIOStream *_stream)
{
  MyIOStream *stream = MY_IO_STREAM (_stream);
  return stream->input_stream;
}

static GOutputStream *
my_io_stream_get_output_stream (GIOStream *_stream)
{
  MyIOStream *stream = MY_IO_STREAM (_stream);
  return stream->output_stream;
}

static void
my_io_stream_class_init (MyIOStreamClass *klass)
{
  GObjectClass *gobject_class;
  GIOStreamClass *giostream_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = my_io_stream_finalize;

  giostream_class = G_IO_STREAM_CLASS (klass);
  giostream_class->get_input_stream  = my_io_stream_get_input_stream;
  giostream_class->get_output_stream = my_io_stream_get_output_stream;
}

static GIOStream *
my_io_stream_new (GInputStream  *input_stream,
                  GOutputStream *output_stream)
{
  MyIOStream *stream;
  g_return_val_if_fail (G_IS_INPUT_STREAM (input_stream), NULL);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output_stream), NULL);
  stream = MY_IO_STREAM (g_object_new (MY_TYPE_IO_STREAM, NULL));
  stream->input_stream = g_object_ref (input_stream);
  stream->output_stream = g_object_ref (output_stream);
  return G_IO_STREAM (stream);
}

static GIOStream *
my_io_stream_new_for_fds (gint fd_in, gint fd_out)
{
  GIOStream *stream;
  GInputStream  *input_stream;
  GOutputStream *output_stream;

  input_stream = g_unix_input_stream_new (fd_in, TRUE);
  output_stream = g_unix_output_stream_new (fd_out, TRUE);
  stream = my_io_stream_new (input_stream, output_stream);
  g_object_unref (input_stream);
  g_object_unref (output_stream);
  return stream;
}

/* ---------------------------------------------------------------------------------------------------- */

static const GDBusArgInfo pokee_method_poke_out_arg0 = {
  -1,   /* ref_count */
  "result",
  "s",
  NULL  /* annotations */
};

static const GDBusArgInfo *pokee_method_poke_out_args[2] = {
  &pokee_method_poke_out_arg0,
  NULL,
};

static const GDBusArgInfo pokee_method_poke_in_arg0 = {
  -1,   /* ref_count */
  "value",
  "s",
  NULL  /* annotations */
};

static const GDBusArgInfo *pokee_method_poke_in_args[2] = {
  &pokee_method_poke_in_arg0,
  NULL,
};

static const GDBusMethodInfo pokee_method_poke = {
  -1,   /* ref_count */
  "Poke",
  (GDBusArgInfo**) pokee_method_poke_in_args,
  (GDBusArgInfo**) pokee_method_poke_out_args,
  NULL  /* annotations */
};

static const GDBusMethodInfo *pokee_methods[2] = {
  &pokee_method_poke,
  NULL
};

static const GDBusInterfaceInfo pokee_object_info = {
  -1,  /* ref_count */
  "org.gtk.GDBus.Pokee",
  (GDBusMethodInfo**) pokee_methods,
  NULL, /* signals */
  NULL, /* properties */
  NULL  /* annotations */
};

static void
pokee_method_call (GDBusConnection       *connection,
                   const gchar           *sender,
                   const gchar           *object_path,
                   const gchar           *interface_name,
                   const gchar           *method_name,
                   GVariant              *parameters,
                   GDBusMethodInvocation *invocation,
                   gpointer               user_data)
{
  const gchar *str;
  gchar *ret;

  g_assert_cmpstr (method_name, ==, "Poke");

  g_variant_get (parameters, "(&s)", &str);
  ret = g_strdup_printf ("You poked me with: `%s'", str);
  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", ret));
  g_free (ret);
}

static const GDBusInterfaceVTable pokee_vtable = {
  pokee_method_call,
  NULL, /* get_property */
  NULL  /* set_property */
};

static void
test_non_socket (void)
{
  gint in_pipes[2];
  gint out_pipes[2];
  GIOStream *stream;
  GDBusConnection *connection;
  GError *error;
  gchar *guid;
  pid_t child;
  gint read_fd;
  gint write_fd;
  GVariant *ret;
  const gchar *str;

  g_assert_cmpint (pipe (in_pipes), ==, 0);
  g_assert_cmpint (pipe (out_pipes), ==, 0);

  switch ((child = fork ()))
    {
    case -1:
      g_assert_not_reached ();
      break;

    case 0:
      /* child */
      read_fd  =  in_pipes[0];
      write_fd = out_pipes[1];
      g_assert_cmpint (close ( in_pipes[1]), ==, 0); /* close unused write end */
      g_assert_cmpint (close (out_pipes[0]), ==, 0); /* close unused read end */
      stream = my_io_stream_new_for_fds (read_fd, write_fd);
      guid = g_dbus_generate_guid ();
      error = NULL;
      /* We need to delay message processing to avoid the race
       * described in
       *
       *  https://bugzilla.gnome.org/show_bug.cgi?id=627188
       *
       * This is because (early) dispatching is done on the IO thread
       * (method_call() isn't called until we're in the right thread
       * though) so in rare cases the parent sends the message before
       * we (the child) register the object
       */
      connection = g_dbus_connection_new_sync (stream,
                                               guid,
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER |
                                               G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING,
                                               NULL, /* GDBusAuthObserver */
                                               NULL,
                                               &error);
      g_free (guid);
      g_assert_no_error (error);
      g_object_unref (stream);

      /* make sure we exit along with the parent */
      g_dbus_connection_set_exit_on_close (connection, TRUE);

      error = NULL;
      g_dbus_connection_register_object (connection,
                                         "/pokee",
                                         (GDBusInterfaceInfo *) &pokee_object_info,
                                         &pokee_vtable,
                                         NULL, /* user_data */
                                         NULL, /* user_data_free_func */
                                         &error);
      g_assert_no_error (error);

      /* and now start message processing */
      g_dbus_connection_start_message_processing (connection);

      g_main_loop_run (loop);

      g_assert_not_reached ();
      break;

    default:
      /* parent continues below */
      break;
    }

  /* parent */
  read_fd  = out_pipes[0];
  write_fd =  in_pipes[1];
  g_assert_cmpint (close (out_pipes[1]), ==, 0); /* close unused write end */
  g_assert_cmpint (close ( in_pipes[0]), ==, 0); /* close unused read end */
  stream = my_io_stream_new_for_fds (read_fd, write_fd);
  error = NULL;
  connection = g_dbus_connection_new_sync (stream,
                                           NULL, /* guid */
                                           G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                           NULL, /* GDBusAuthObserver */
                                           NULL,
                                           &error);
  g_assert_no_error (error);
  g_object_unref (stream);

  /* poke the child */
  error = NULL;
  ret = g_dbus_connection_call_sync (connection,
                                     NULL, /* name */
                                     "/pokee",
                                     "org.gtk.GDBus.Pokee",
                                     "Poke",
                                     g_variant_new ("(s)", "I am the POKER!"),
                                     G_VARIANT_TYPE ("(s)"), /* return type */
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL, /* cancellable */
                                     &error);
  g_assert_no_error (error);
  g_variant_get (ret, "(&s)", &str);
  g_assert_cmpstr (str, ==, "You poked me with: `I am the POKER!'");
  g_variant_unref (ret);

  g_object_unref (connection);

  g_assert_cmpint (kill (child, SIGTERM), ==, 0);
}

#else /* G_OS_UNIX */

static void
test_non_socket (void)
{
  /* TODO: test this with e.g. GWin32InputStream/GWin32OutputStream */
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  gint ret;

  g_type_init ();
  g_thread_init (NULL);
  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = g_main_loop_new (NULL, FALSE);

  g_test_add_func ("/gdbus/non-socket", test_non_socket);

  ret = g_test_run();

  g_main_loop_unref (loop);

  return ret;
}
