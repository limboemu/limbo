#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "socket-common.c"

GMainLoop *loop;

int port = 7777;
gboolean verbose = FALSE;
gboolean dont_reuse_address = FALSE;
gboolean non_blocking = FALSE;
gboolean use_udp = FALSE;
gboolean use_source = FALSE;
int cancel_timeout = 0;
int read_timeout = 0;
int delay = 0;
gboolean unix_socket = FALSE;

static GOptionEntry cmd_entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_INT, &port,
   "Local port to bind to", NULL},
  {"cancel", 'c', 0, G_OPTION_ARG_INT, &cancel_timeout,
   "Cancel any op after the specified amount of seconds", NULL},
  {"udp", 'u', 0, G_OPTION_ARG_NONE, &use_udp,
   "Use udp instead of tcp", NULL},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Be verbose", NULL},
  {"no-reuse", 0, 0, G_OPTION_ARG_NONE, &dont_reuse_address,
   "Don't SOADDRREUSE", NULL},
  {"non-blocking", 'n', 0, G_OPTION_ARG_NONE, &non_blocking,
   "Enable non-blocking i/o", NULL},
  {"use-source", 's', 0, G_OPTION_ARG_NONE, &use_source,
   "Use GSource to wait for non-blocking i/o", NULL},
#ifdef G_OS_UNIX
  {"unix", 'U', 0, G_OPTION_ARG_NONE, &unix_socket,
   "Use a unix socket instead of IP", NULL},
#endif
  {"delay", 'd', 0, G_OPTION_ARG_INT, &delay,
   "Delay responses by the specified number of seconds", NULL},
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
  GSocket *socket, *new_socket, *recv_socket;
  GSocketAddress *src_address;
  GSocketAddress *address;
  GSocketType socket_type;
  GSocketFamily socket_family;
  GError *error = NULL;
  GOptionContext *context;
  GCancellable *cancellable;
  char *display_addr;

  g_thread_init (NULL);

  g_type_init ();

  context = g_option_context_new (" - Test GSocket server stuff");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (unix_socket && argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0], "Need to specify unix socket name");
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

  if (non_blocking)
    g_socket_set_blocking (socket, FALSE);

  if (unix_socket)
    {
      src_address = socket_address_from_string (argv[1]);
      if (src_address == NULL)
	{
	  g_printerr ("%s: Could not parse '%s' as unix socket name\n", argv[0], argv[1]);
	  return 1;
	}
    }
  else
    {
      src_address = g_inet_socket_address_new (g_inet_address_new_any (G_SOCKET_FAMILY_IPV4), port);
    }

  if (!g_socket_bind (socket, src_address, !dont_reuse_address, &error))
    {
      g_printerr ("Can't bind socket: %s\n", error->message);
      return 1;
    }
  g_object_unref (src_address);

  if (!use_udp)
    {
      if (!g_socket_listen (socket, &error))
	{
	  g_printerr ("Can't listen on socket: %s\n", error->message);
	  return 1;
	}

      address = g_socket_get_local_address (socket, &error);
      if (!address)
	{
	  g_printerr ("Error getting local address: %s\n",
		      error->message);
	  return 1;
	}
      display_addr = socket_address_to_string (address);
      g_print ("listening on %s...\n", display_addr);
      g_free (display_addr);

      ensure_condition (socket, "accept", cancellable, G_IO_IN);
      new_socket = g_socket_accept (socket, cancellable, &error);
      if (!new_socket)
	{
	  g_printerr ("Error accepting socket: %s\n",
		      error->message);
	  return 1;
	}

      if (non_blocking)
	g_socket_set_blocking (new_socket, FALSE);
      if (read_timeout)
	g_socket_set_timeout (new_socket, read_timeout);

      address = g_socket_get_remote_address (new_socket, &error);
      if (!address)
	{
	  g_printerr ("Error getting remote address: %s\n",
		      error->message);
	  return 1;
	}

      display_addr = socket_address_to_string (address);
      g_print ("got a new connection from %s\n", display_addr);
      g_free(display_addr);
      g_object_unref (address);

      recv_socket = new_socket;
    }
  else
    {
      recv_socket = socket;
      new_socket = NULL;
    }


  while (TRUE)
    {
      gchar buffer[4096];
      gssize size;
      gsize to_send;

      ensure_condition (recv_socket, "receive", cancellable, G_IO_IN);
      if (use_udp)
	size = g_socket_receive_from (recv_socket, &address,
				      buffer, sizeof buffer,
				      cancellable, &error);
      else
	size = g_socket_receive (recv_socket, buffer, sizeof buffer,
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
	g_print (" from %s", socket_address_to_string (address));
      g_print ("\n");

      if (verbose)
	g_print ("-------------------------\n"
		 "%.*s\n"
		 "-------------------------\n",
		 (int)size, buffer);

      to_send = size;

      if (delay)
	{
	  if (verbose)
	    g_print ("delaying %d seconds before response\n", delay);
	  g_usleep (1000 * 1000 * delay);
	}

      while (to_send > 0)
	{
	  ensure_condition (recv_socket, "send", cancellable, G_IO_OUT);
	  if (use_udp)
	    size = g_socket_send_to (recv_socket, address,
				     buffer, to_send, cancellable, &error);
	  else
	    size = g_socket_send (recv_socket, buffer, to_send,
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
    }

  g_print ("connection closed\n");

  if (new_socket)
    {
      if (!g_socket_close (new_socket, &error))
	{
	  g_printerr ("Error closing connection socket: %s\n",
		      error->message);
	  return 1;
	}

      g_object_unref (G_OBJECT (new_socket));
    }

  if (!g_socket_close (socket, &error))
    {
      g_printerr ("Error closing master socket: %s\n",
		  error->message);
      return 1;
    }

  g_object_unref (G_OBJECT (socket));

  return 0;
}
