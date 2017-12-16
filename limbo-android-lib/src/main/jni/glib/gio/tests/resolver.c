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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>

static GResolver *resolver;
static GCancellable *cancellable;
static GMainLoop *loop;
static int nlookups = 0;

static void G_GNUC_NORETURN
usage (void)
{
	fprintf (stderr, "Usage: resolver [-t] [-s] [hostname | IP | service/protocol/domain ] ...\n");
	fprintf (stderr, "       resolver [-t] [-s] -c [hostname | IP | service/protocol/domain ]\n");
	fprintf (stderr, "       Use -t to enable threading.\n");
	fprintf (stderr, "       Use -s to do synchronous lookups.\n");
	fprintf (stderr, "       Both together will result in simultaneous lookups in multiple threads\n");
	fprintf (stderr, "       Use -c (and only a single resolvable argument) to test GSocketConnectable.\n");
	exit (1);
}

G_LOCK_DEFINE_STATIC (response);

static void
done_lookup (void)
{
  nlookups--;
  if (nlookups == 0)
    {
      /* In the sync case we need to make sure we don't call
       * g_main_loop_quit before the loop is actually running...
       */
      g_idle_add ((GSourceFunc)g_main_loop_quit, loop);
    }
}

static void
print_resolved_name (const char *phys,
                     char       *name,
                     GError     *error)
{
  G_LOCK (response);
  printf ("Address: %s\n", phys);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      printf ("Name:    %s\n", name);
      g_free (name);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_addresses (const char *name,
                          GList      *addresses,
			  GError     *error)
{
  char *phys;
  GList *a;

  G_LOCK (response);
  printf ("Name:    %s\n", name);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      for (a = addresses; a; a = a->next)
	{
	  phys = g_inet_address_to_string (a->data);
	  printf ("Address: %s\n", phys);
	  g_free (phys);
          g_object_unref (a->data);
	}
      g_list_free (addresses);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_service (const char *service,
                        GList      *targets,
			GError     *error)
{
  GList *t;  

  G_LOCK (response);
  printf ("Service: %s\n", service);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      for (t = targets; t; t = t->next)
	{
	  printf ("%s:%u (pri %u, weight %u)\n",
		  g_srv_target_get_hostname (t->data),
		  g_srv_target_get_port (t->data),
		  g_srv_target_get_priority (t->data),
		  g_srv_target_get_weight (t->data));
          g_srv_target_free (t->data);
	}
      g_list_free (targets);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
lookup_one_sync (const char *arg)
{
  GError *error = NULL;

  if (strchr (arg, '/'))
    {
      GList *targets;
      /* service/protocol/domain */
      char **parts = g_strsplit (arg, "/", 3);

      if (!parts || !parts[2])
	usage ();

      targets = g_resolver_lookup_service (resolver,
                                           parts[0], parts[1], parts[2],
                                           cancellable, &error);
      print_resolved_service (arg, targets, error);
    }
  else if (g_hostname_is_ip_address (arg))
    {
      GInetAddress *addr = g_inet_address_new_from_string (arg);
      char *name;

      name = g_resolver_lookup_by_address (resolver, addr, cancellable, &error);
      print_resolved_name (arg, name, error);
      g_object_unref (addr);
    }
  else
    {
      GList *addresses;

      addresses = g_resolver_lookup_by_name (resolver, arg, cancellable, &error);
      print_resolved_addresses (arg, addresses, error);
    }
}

static gpointer
lookup_thread (gpointer arg)
{
  lookup_one_sync (arg);
  return NULL;
}

static void
start_threaded_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    g_thread_create (lookup_thread, argv[i], FALSE, NULL);
}

static void
start_sync_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    lookup_one_sync (argv[i]);
}

static void
lookup_by_addr_callback (GObject *source, GAsyncResult *result,
                         gpointer user_data)
{
  const char *phys = user_data;
  GError *error = NULL;
  char *name;

  name = g_resolver_lookup_by_address_finish (resolver, result, &error);
  print_resolved_name (phys, name, error);
}

static void
lookup_by_name_callback (GObject *source, GAsyncResult *result,
                         gpointer user_data)
{
  const char *name = user_data;
  GError *error = NULL;
  GList *addresses;

  addresses = g_resolver_lookup_by_name_finish (resolver, result, &error);
  print_resolved_addresses (name, addresses, error);
}

static void
lookup_service_callback (GObject *source, GAsyncResult *result,
			 gpointer user_data)
{
  const char *service = user_data;
  GError *error = NULL;
  GList *targets;

  targets = g_resolver_lookup_service_finish (resolver, result, &error);
  print_resolved_service (service, targets, error);
}

static void
start_async_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    {
      if (strchr (argv[i], '/'))
	{
	  /* service/protocol/domain */
	  char **parts = g_strsplit (argv[i], "/", 3);

	  if (!parts || !parts[2])
	    usage ();

	  g_resolver_lookup_service_async (resolver,
					   parts[0], parts[1], parts[2],
					   cancellable,
					   lookup_service_callback, argv[i]);
	}
      else if (g_hostname_is_ip_address (argv[i]))
	{
          GInetAddress *addr = g_inet_address_new_from_string (argv[i]);

	  g_resolver_lookup_by_address_async (resolver, addr, cancellable,
                                              lookup_by_addr_callback, argv[i]);
	  g_object_unref (addr);
	}
      else
	{
	  g_resolver_lookup_by_name_async (resolver, argv[i], cancellable,
                                           lookup_by_name_callback,
                                           argv[i]);
	}

      /* Stress-test the reloading code */
      g_signal_emit_by_name (resolver, "reload");
    }
}

static void
print_connectable_sockaddr (GSocketAddress *sockaddr,
                            GError         *error)
{
  char *phys;

  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else if (!G_IS_INET_SOCKET_ADDRESS (sockaddr))
    {
      printf ("Error:   Unexpected sockaddr type '%s'\n", g_type_name_from_instance ((GTypeInstance *)sockaddr));
      g_object_unref (sockaddr);
    }
  else
    {
      GInetSocketAddress *isa = G_INET_SOCKET_ADDRESS (sockaddr);
      phys = g_inet_address_to_string (g_inet_socket_address_get_address (isa));
      printf ("Address: %s%s%s:%d\n",
              strchr (phys, ':') ? "[" : "", phys, strchr (phys, ':') ? "]" : "",
              g_inet_socket_address_get_port (isa));
      g_free (phys);
      g_object_unref (sockaddr);
    }
}

static void
do_sync_connectable (GSocketAddressEnumerator *enumerator)
{
  GSocketAddress *sockaddr;
  GError *error = NULL;

  while ((sockaddr = g_socket_address_enumerator_next (enumerator, cancellable, &error)))
    print_connectable_sockaddr (sockaddr, error);

  g_object_unref (enumerator);
  done_lookup ();
}

static void do_async_connectable (GSocketAddressEnumerator *enumerator);

static void
got_next_async (GObject *source, GAsyncResult *result, gpointer user_data)
{
  GSocketAddressEnumerator *enumerator = G_SOCKET_ADDRESS_ENUMERATOR (source);
  GSocketAddress *sockaddr;
  GError *error = NULL;

  sockaddr = g_socket_address_enumerator_next_finish (enumerator, result, &error);
  if (sockaddr || error)
    print_connectable_sockaddr (sockaddr, error);
  if (sockaddr)
    do_async_connectable (enumerator);
  else
    {
      g_object_unref (enumerator);
      done_lookup ();
    }
}

static void
do_async_connectable (GSocketAddressEnumerator *enumerator)
{
  g_socket_address_enumerator_next_async (enumerator, cancellable,
                                          got_next_async, NULL);
}

static void
do_connectable (const char *arg, gboolean synchronous)
{
  char **parts;
  GSocketConnectable *connectable;
  GSocketAddressEnumerator *enumerator;

  if (strchr (arg, '/'))
    {
      /* service/protocol/domain */
      parts = g_strsplit (arg, "/", 3);
      if (!parts || !parts[2])
	usage ();

      connectable = g_network_service_new (parts[0], parts[1], parts[2]);
    }
  else
    {
      guint16 port;

      parts = g_strsplit (arg, ":", 2);
      if (parts && parts[1])
	{
	  arg = parts[0];
	  port = strtoul (parts[1], NULL, 10);
	}
      else
	port = 0;

      if (g_hostname_is_ip_address (arg))
	{
	  GInetAddress *addr = g_inet_address_new_from_string (arg);
	  GSocketAddress *sockaddr = g_inet_socket_address_new (addr, port);

	  g_object_unref (addr);
	  connectable = G_SOCKET_CONNECTABLE (sockaddr);
	}
      else
        connectable = g_network_address_new (arg, port);
    }

  enumerator = g_socket_connectable_enumerate (connectable);
  g_object_unref (connectable);

  if (synchronous)
    do_sync_connectable (enumerator);
  else
    do_async_connectable (enumerator);
}

#ifdef G_OS_UNIX
static int cancel_fds[2];

static void
interrupted (int sig)
{
  gssize c;

  signal (SIGINT, SIG_DFL);
  c = write (cancel_fds[1], "x", 1);
  g_assert_cmpint(c, ==, 1);
}

static gboolean
async_cancel (GIOChannel *source, GIOCondition cond, gpointer cancel)
{
  g_cancellable_cancel (cancel);
  return FALSE;
}
#endif

int
main (int argc, char **argv)
{
  gboolean threaded = FALSE, synchronous = FALSE;
  gboolean use_connectable = FALSE;
#ifdef G_OS_UNIX
  GIOChannel *chan;
  guint watch;
#endif

  /* We can't use GOptionContext because we use the arguments to
   * decide whether or not to call g_thread_init().
   */
  while (argc >= 2 && argv[1][0] == '-')
    {
      if (!strcmp (argv[1], "-t"))
        {
          g_thread_init (NULL);
          threaded = TRUE;
        }
      else if (!strcmp (argv[1], "-s"))
        synchronous = TRUE;
      else if (!strcmp (argv[1], "-c"))
        use_connectable = TRUE;
      else
        usage ();

      argv++;
      argc--;
    }
  g_type_init ();

  if (argc < 2 || (argc > 2 && use_connectable))
    usage ();

  resolver = g_resolver_get_default ();

  cancellable = g_cancellable_new ();

#ifdef G_OS_UNIX
  /* Set up cancellation; we want to cancel if the user ^C's the
   * program, but we can't cancel directly from an interrupt.
   */
  signal (SIGINT, interrupted);

  if (pipe (cancel_fds) == -1)
    {
      perror ("pipe");
      exit (1);
    }
  chan = g_io_channel_unix_new (cancel_fds[0]);
  watch = g_io_add_watch (chan, G_IO_IN, async_cancel, cancellable);
  g_io_channel_unref (chan);
#endif

  nlookups = argc - 1;
  loop = g_main_loop_new (NULL, TRUE);

  if (use_connectable)
    do_connectable (argv[1], synchronous);
  else
    {
      if (threaded && synchronous)
        start_threaded_lookups (argv + 1, argc - 1);
      else if (synchronous)
        start_sync_lookups (argv + 1, argc - 1);
      else
        start_async_lookups (argv + 1, argc - 1);
    }

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

#ifdef G_OS_UNIX
  g_source_remove (watch);
#endif
  g_object_unref (cancellable);

  return 0;
}
