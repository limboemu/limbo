#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "socket-common.c"

GMainLoop *loop;

gboolean verbose = FALSE;
gboolean non_blocking = FALSE;
gboolean use_udp = FALSE;
gboolean use_source = FALSE;
int cancel_timeout = 0;
int read_timeout = 0;
gboolean unix_socket = FALSE;

static GOptionEntry cmd_entries[] = {
  {"cancel", 'c', 0, G_OPTION_ARG_INT, &cancel_timeout,
   "Cancel any op after the specified amount of seconds", NULL},
  {"udp", 'u', 0, G_OPTION_ARG_NONE, &use_udp,
   "Use udp instead of tcp", NULL},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Be verbose", NULL},
  {"non-blocking", 'n', 0, G_OPTION_ARG_NONE, &non_blocking,
   "Enable non-blocking i/o", NULL},
  {"use-source", 's', 0, G_OPTION_ARG_NONE, &use_source,
   "Use GSource to wait for non-blocking i/o", NULL},
#ifdef G_OS_UNIX
  {"unix", 'U', 0, G_OPTION_ARG_NONE, &unix_socket,
   "Use a unix socket instead of IP", NULL},
#endif
  {"timeout", 't', 0, G_OPTION_ARG_INT, &read_timeout,
   "Time out reads after the specified number of seconds", NULL},
  {NULL}
};

static gboolean
source_ready (gpointer data,
	      GIOCondition condition)
{
  g_main_loop_quit (loop);
  return FALSE;
}

static void
ensure_condition (GSocket *socket,
		  const char *where,
		  GCancellable *cancellable,
		  GIOCondition condition)
{
  GError *error = NULL;
  GSource *source;

  if (!non_blocking)
    return;

  if (use_source)
    {
      source = g_socket_create_source (socket,
                                       condition,
                                       cancellable);
      g_source_set_callback (source,
                             (GSourceFunc) source_ready,
			     NULL, NULL);
      g_source_attach (source, NULL);
      g_source_unref (source);
      g_main_loop_run (loop);
    }
  else
    {
      if (!g_socket_condition_wait (socket, condition, cancellable, &error))
	{
	  g_printerr ("condition wait error for %s: %s\n",
		      where,
		      error->message);
	  exit (1);
	}
    }
}

static gpointer
cancel_thread (gpointer data)
{
  GCancellable *cancellable = data;

  g_usleep (1000*1000*cancel_timeout);
  g_print ("Cancelling\n");
  g_cancellable_cancel (cancellable);
  return NULL;
}

int
main (int argc,
      char *argv[])
{
  GSocket *socket;
  GSocketAddress *src_address;
  GSocketAddress *address;
  GSocketType socket_type;
  GSocketFamily socket_family;
  GError *error = NULL;
  GOptionContext *context;
  GCancellable *cancellable;
  GSocketAddressEnumerator *enumerator;
  GSocketConnectable *connectable;

  g_thread_init (NULL);

  g_type_init ();

  context = g_option_context_new (" <hostname>[:port] - Test GSocket client stuff");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0], "Need to specify hostname / unix socket name");
      return 1;
    }

  if (cancel_timeout)
    {
      cancellable = g_cancellable_new ();
      g_thread_create (cancel_thread, cancellable, FALSE, NULL);
    }
  else
    {
      cancellable = NULL;
    }

  loop = g_main_loop_new (NULL, FALSE);

  if (use_udp)
    socket_type = G_SOCKET_TYPE_DATAGRAM;
  else
    socket_type = G_SOCKET_TYPE_STREAM;

  if (unix_socket)
    socket_family = G_SOCKET_FAMILY_UNIX;
  else
    socket_family = G_SOCKET_FAMILY_IPV4;

  socket = g_socket_new (socket_family, socket_type, 0, &error);
  if (socket == NULL)
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (read_timeout)
    g_socket_set_timeout (socket, read_timeout);

  if (unix_socket)
    {
      GSocketAddress *addr;

      addr = socket_address_from_string (argv[1]);
      if (addr == NULL)
	{
	  g_printerr ("%s: Could not parse '%s' as unix socket name\n", argv[0], argv[1]);
	  return 1;
	}
      connectable = G_SOCKET_CONNECTABLE (addr);
    }
  else
    {
      connectable = g_network_address_parse (argv[1], 7777, &error);
      if (connectable == NULL)
	{
	  g_printerr ("%s: %s\n", argv[0], error->message);
	  return 1;
	}
    }

  enumerator = g_socket_connectable_enumerate (connectable);
  while (TRUE)
    {
      address = g_socket_address_enumerator_next (enumerator, cancellable, &error);
      if (address == NULL)
	{
	  if (error == NULL)
	    g_printerr ("%s: No more addresses to try\n", argv[0]);
	  else
	    g_printerr ("%s: %s\n", argv[0], error->message);
	  return 1;
	}

      if (g_socket_connect (socket, address, cancellable, &error))
	break;
      g_printerr ("%s: Connection to %s failed: %s, trying next\n", argv[0], socket_address_to_string (address), error->message);
      g_error_free (error);
      error = NULL;

      g_object_unref (address);
    }
  g_object_unref (enumerator);
  g_object_unref (connectable);

  g_print ("Connected to %s\n",
	   socket_address_to_string (address));

  /* TODO: Test non-blocking connect */
  if (non_blocking)
    g_socket_set_blocking (socket, FALSE);

  src_address = g_socket_get_local_address (socket, &error);
  if (!src_address)
    {
      g_printerr ("Error getting local address: %s\n",
		  error->message);
      return 1;
    }
  g_print ("local address: %s\n",
	   socket_address_to_string (src_address));
  g_object_unref (src_address);

  while (TRUE)
    {
      gchar buffer[4096];
      gssize size;
      gsize to_send;

      if (fgets (buffer, sizeof buffer, stdin) == NULL)
	break;

      to_send = strlen (buffer);
      while (to_send > 0)
	{
	  ensure_condition (socket, "send", cancellable, G_IO_OUT);
	  if (use_udp)
	    size = g_socket_send_to (socket, address,
				     buffer, to_send,
				     cancellable, &error);
	  else
	    size = g_socket_send (socket, buffer, to_send,
				  cancellable, &error);

	  if (size < 0)
	    {
	      if (g_error_matches (error,
				   G_IO_ERROR,
				   G_IO_ERROR_WOULD_BLOCK))
		{
		  g_print ("socket send would block, handling\n");
		  g_error_free (error);
		  error = NULL;
		  continue;
		}
	      else
		{
		  g_printerr ("Error sending to socket: %s\n",
			      error->message);
		  return 1;
		}
	    }

	  g_print ("sent %" G_GSSIZE_FORMAT " bytes of data\n", size);

	  if (size == 0)
	    {
	      g_printerr ("Unexpected short write\n");
	      return 1;
	    }

	  to_send -= size;
	}

      ensure_condition (socket, "receive", cancellable, G_IO_IN);
      if (use_udp)
	size = g_socket_receive_from (socket, &src_address,
				      buffer, sizeof buffer,
				      cancellable, &error);
      else
	size = g_socket_receive (socket, buffer, sizeof buffer,
				 cancellable, &error);

      if (size < 0)
	{
	  g_printerr ("Error receiving from socket: %s\n",
		      error->message);
	  return 1;
	}

      if (size == 0)
	break;

      g_print ("received %" G_GSSIZE_FORMAT " bytes of data", size);
      if (use_udp)
	g_print (" from %s", socket_address_to_string (src_address));
      g_print ("\n");

      if (verbose)
	g_print ("-------------------------\n"
		 "%.*s"
		 "-------------------------\n",
		 (int)size, buffer);

    }

  g_print ("closing socket\n");

  if (!g_socket_close (socket, &error))
    {
      g_printerr ("Error closing master socket: %s\n",
		  error->message);
      return 1;
    }

  g_object_unref (G_OBJECT (socket));
  g_object_unref (G_OBJECT (address));

  return 0;
}
