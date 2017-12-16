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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

#include "gdbus-sessionbus.h"

/* ---------------------------------------------------------------------------------------------------- */
/* Utilities for bringing up and tearing down session message bus instances */

static void
watch_parent (gint fd)
{
  GPollFD fds[1];
  gint num_events;
  gchar buf[512];
  guint bytes_read;
  GArray *buses_to_kill_array;

  fds[0].fd = fd;
  fds[0].events = G_IO_HUP | G_IO_IN;
  fds[0].revents = 0;

  buses_to_kill_array = g_array_new (FALSE, TRUE, sizeof (guint));

  do
    {
      guint pid;
      guint n;

      num_events = g_poll (fds, 1, -1);
      if (num_events == 0)
        continue;

      if (fds[0].revents == G_IO_HUP)
        {
          for (n = 0; n < buses_to_kill_array->len; n++)
            {
              pid = g_array_index (buses_to_kill_array, guint, n);
              g_print ("cleaning up bus with pid %d\n", pid);
              kill (pid, SIGTERM);
            }
          g_array_free (buses_to_kill_array, TRUE);
          exit (0);
        }

      //g_debug ("data from parent");

      memset (buf, '\0', sizeof buf);
    again:
      bytes_read = read (fds[0].fd, buf, sizeof buf);
      if (bytes_read < 0 && (errno == EAGAIN || errno == EINTR))
        goto again;

      if (sscanf (buf, "add %d\n", &pid) == 1)
        {
          g_array_append_val (buses_to_kill_array, pid);
        }
      else if (sscanf (buf, "remove %d\n", &pid) == 1)
        {
          for (n = 0; n < buses_to_kill_array->len; n++)
            {
              if (g_array_index (buses_to_kill_array, guint, n) == pid)
                {
                  g_array_remove_index (buses_to_kill_array, n);
                  pid = 0;
                  break;
                }
            }
          if (pid != 0)
            {
              g_warning ("unknown pid %d to remove", pid);
            }
        }
      else
        {
          g_warning ("unknown command from parent '%s'", buf);
        }
    }
  while (TRUE);

}

static GHashTable *session_bus_address_to_pid = NULL;
static gint pipe_fds[2];

const gchar *
session_bus_up_with_address (const gchar *given_address)
{
  gchar *address;
  int stdout_fd;
  GError *error;
  gchar *argv[] = {"dbus-daemon", "--print-address", "--config-file=foo", NULL};
  GPid pid;
  gchar buf[512];
  ssize_t bytes_read;
  gchar *config_file_name;
  gint config_file_fd;
  GString *config_file_contents;

  address = NULL;
  error = NULL;
  config_file_name = NULL;
  config_file_fd = -1;
  argv[2] = NULL;

  config_file_fd = g_file_open_tmp ("g-dbus-tests-XXXXXX",
                                    &config_file_name,
                                    &error);
  if (config_file_fd < 0)
    {
      g_warning ("Error creating temporary config file: %s", error->message);
      g_error_free (error);
      goto out;
    }

  config_file_contents = g_string_new (NULL);
  g_string_append        (config_file_contents, "<busconfig>\n");
  g_string_append        (config_file_contents, "  <type>session</type>\n");
  g_string_append_printf (config_file_contents, "  <listen>%s</listen>\n", given_address);
  g_string_append        (config_file_contents,
                          "  <policy context=\"default\">\n"
                          "    <!-- Allow everything to be sent -->\n"
                          "    <allow send_destination=\"*\" eavesdrop=\"true\"/>\n"
                          "    <!-- Allow everything to be received -->\n"
                          "    <allow eavesdrop=\"true\"/>\n"
                          "    <!-- Allow anyone to own anything -->\n"
                          "    <allow own=\"*\"/>\n"
                          "  </policy>\n");
  g_string_append        (config_file_contents, "</busconfig>\n");

  if (write (config_file_fd, config_file_contents->str, config_file_contents->len) != (gssize) config_file_contents->len)
    {
      g_warning ("Error writing %d bytes to config file: %m", (gint) config_file_contents->len);
      g_string_free (config_file_contents, TRUE);
      goto out;
    }
  g_string_free (config_file_contents, TRUE);

  argv[2] = g_strdup_printf ("--config-file=%s", config_file_name);

  if (session_bus_address_to_pid == NULL)
    {
      /* keep a mapping from session bus address to the pid */
      session_bus_address_to_pid = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      /* fork a child to clean up session buses when we are killed */
      if (pipe (pipe_fds) != 0)
        {
          g_warning ("pipe() failed: %m");
          g_assert_not_reached ();
        }
      switch (fork ())
        {
        case -1:
          g_warning ("fork() failed: %m");
          g_assert_not_reached ();
          break;

        case 0:
          /* child */
          close (pipe_fds[1]);
          watch_parent (pipe_fds[0]);
          break;

        default:
          /* parent */
          close (pipe_fds[0]);
          break;
        }

      //atexit (cleanup_session_buses);
      /* TODO: need to handle the cases where we crash */
    }
  else
    {
      /* check if we already have a bus running for this address */
      if (g_hash_table_lookup (session_bus_address_to_pid, given_address) != NULL)
        {
          g_warning ("Already have a bus instance for the given address %s", given_address);
          goto out;
        }
    }

  if (!g_spawn_async_with_pipes (NULL,
                                 argv,
                                 NULL,
                                 G_SPAWN_SEARCH_PATH,
                                 NULL,
                                 NULL,
                                 &pid,
                                 NULL,
                                 &stdout_fd,
                                 NULL,
                                 &error))
    {
      g_warning ("Error spawning dbus-daemon: %s", error->message);
      g_error_free (error);
      goto out;
    }

  memset (buf, '\0', sizeof buf);
 again:
  bytes_read = read (stdout_fd, buf, sizeof buf);
  if (bytes_read < 0 && (errno == EAGAIN || errno == EINTR))
    goto again;
  close (stdout_fd);

  if (bytes_read == 0 || bytes_read == sizeof buf)
    {
      g_warning ("Error reading address from dbus daemon, %d bytes read", (gint) bytes_read);
      kill (SIGTERM, pid);
      goto out;
    }

  address = g_strdup (buf);
  g_strstrip (address);

  /* write the pid to the child so it can kill it when we die */
  g_snprintf (buf, sizeof buf, "add %d\n", (guint) pid);
  write (pipe_fds[1], buf, strlen (buf));

  /* start dbus-monitor */
  if (g_getenv ("G_DBUS_MONITOR") != NULL)
    {
      g_spawn_command_line_async ("dbus-monitor --session", NULL);
      usleep (500 * 1000);
    }

  g_hash_table_insert (session_bus_address_to_pid, address, GUINT_TO_POINTER (pid));

 out:
  if (config_file_fd > 0)
    {
      if (close (config_file_fd) != 0)
        {
          g_warning ("Error closing fd for config file %s: %m", config_file_name);
        }
      g_assert (config_file_name != NULL);
      if (unlink (config_file_name) != 0)
        {
          g_warning ("Error unlinking config file %s: %m", config_file_name);
        }
    }
  g_free (argv[2]);
  g_free (config_file_name);
  return address;
}

void
session_bus_down_with_address (const gchar *address)
{
  gpointer value;
  GPid pid;
  gchar buf[512];

  g_assert (address != NULL);
  g_assert (session_bus_address_to_pid != NULL);

  value = g_hash_table_lookup (session_bus_address_to_pid, address);
  g_assert (value != NULL);

  pid = GPOINTER_TO_UINT (g_hash_table_lookup (session_bus_address_to_pid, address));

  kill (pid, SIGTERM);

  /* write the pid to the child so it won't kill it when we die */
  g_snprintf (buf, sizeof buf, "remove %d\n", (guint) pid);
  write (pipe_fds[1], buf, strlen (buf));

  g_hash_table_remove (session_bus_address_to_pid, address);
}

static gchar *temporary_address = NULL;
static gchar *temporary_address_used_by_bus = NULL;

const gchar *
session_bus_get_temporary_address (void)
{
  if (temporary_address == NULL)
    {
      /* TODO: maybe use a more random name etc etc */
      temporary_address = g_strdup_printf ("unix:path=/tmp/g-dbus-tests-pid-%d", getpid ());
    }

  return temporary_address;
}

const gchar *
session_bus_up (void)
{
  if (temporary_address_used_by_bus != NULL)
    {
      g_warning ("There is already a session bus up");
      goto out;
    }

  temporary_address_used_by_bus = g_strdup (session_bus_up_with_address (session_bus_get_temporary_address ()));

 out:
  return temporary_address_used_by_bus;
}

void
session_bus_down (void)
{
  if (temporary_address_used_by_bus == NULL)
    {
      g_warning ("There is not a session bus up");
    }
  else
    {
      session_bus_down_with_address (temporary_address_used_by_bus);
      g_free (temporary_address_used_by_bus);
      temporary_address_used_by_bus = NULL;
    }
}
