/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#ifdef GLIB_COMPILATION
#undef GLIB_COMPILATION
#endif

#include <string.h>

#include <glib.h>

#include <gstdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <io.h>			/* For read(), write() etc */
#endif

static void
test_mkstemp (void)
{
  char template[32];
  int fd;
  int i;
  const char hello[] = "Hello, World";
  const int hellolen = sizeof (hello) - 1;
  char chars[62];

  strcpy (template, "foobar");
  fd = g_mkstemp (template);
  if (fd != -1)
    g_warning ("g_mkstemp works even if template doesn't contain XXXXXX");
  close (fd);

  strcpy (template, "foobarXXX");
  fd = g_mkstemp (template);
  if (fd != -1)
    g_warning ("g_mkstemp works even if template contains less than six X");
  close (fd);

  strcpy (template, "fooXXXXXX");
  fd = g_mkstemp (template);
  g_assert (fd != -1 && "g_mkstemp didn't work for template fooXXXXXX");
  i = write (fd, hello, hellolen);
  g_assert (i != -1 && "write() failed");
  g_assert (i == hellolen && "write() has written too few bytes");

  lseek (fd, 0, 0);
  i = read (fd, chars, sizeof (chars));
  g_assert (i != -1 && "read() failed: %s");
  g_assert (i == hellolen && "read() has got wrong number of bytes");

  chars[i] = 0;
  g_assert (strcmp (chars, hello) == 0 && "read() didn't get same string back");

  close (fd);
  remove (template);

  strcpy (template, "fooXXXXXX.pdf");
  fd = g_mkstemp (template);
  g_assert (fd != -1 && "g_mkstemp didn't work for template fooXXXXXX.pdf");

  close (fd);
  remove (template);
}

static void
test_readlink (void)
{
#ifdef HAVE_SYMLINK
  FILE *file;
  int result;
  char *filename = "file-test-data";
  char *link1 = "file-test-link1";
  char *link2 = "file-test-link2";
  char *link3 = "file-test-link3";
  char *data;
  GError *error;

  file = fopen (filename, "w");
  g_assert (file != NULL && "fopen() failed");
  fclose (file);

  result = symlink (filename, link1);
  g_assert (result == 0 && "symlink() failed");
  result = symlink (link1, link2);
  g_assert (result == 0 && "symlink() failed");
  
  error = NULL;
  data = g_file_read_link (link1, &error);
  g_assert (data != NULL && "couldn't read link1");
  g_assert (strcmp (data, filename) == 0 && "link1 contains wrong data");
  g_free (data);
  
  error = NULL;
  data = g_file_read_link (link2, &error);
  g_assert (data != NULL && "couldn't read link2");
  g_assert (strcmp (data, link1) == 0 && "link2 contains wrong data");
  g_free (data);
  
  error = NULL;
  data = g_file_read_link (link3, &error);
  g_assert (data == NULL && "could read link3");
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);

  error = NULL;
  data = g_file_read_link (filename, &error);
  g_assert (data == NULL && "could read regular file as link");
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
  
  remove (filename);
  remove (link1);
  remove (link2);
#endif
}

static void
test_get_contents (void)
{
  const gchar *text = "abcdefghijklmnopqrstuvwxyz";
  const gchar *filename = "file-test-get-contents";
  gchar *contents;
  gsize len;
  FILE *f;
  GError *error = NULL;

  f = g_fopen (filename, "w");
  fwrite (text, 1, strlen (text), f);
  fclose (f);

  g_assert (g_file_test (filename, G_FILE_TEST_IS_REGULAR));

  if (! g_file_get_contents (filename, &contents, &len, &error))
    g_error ("g_file_get_contents() failed: %s", error->message);

  g_assert (strcmp (text, contents) == 0 && "content mismatch");

  g_free (contents);
}

int 
main (int argc, char *argv[])
{
  test_mkstemp ();
  test_readlink ();
  test_get_contents ();

  return 0;
}
