/* GLib testing framework examples and tests
 * Copyright (C) 2008 Red Hat, Inc
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <gio/gwin32inputstream.h>
#include <gio/gwin32outputstream.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#include <windows.h>

#define DATA "abcdefghijklmnopqrstuvwxyz"

int writer_pipe[2], reader_pipe[2];
GCancellable *writer_cancel, *reader_cancel, *main_cancel;
GMainLoop *loop;

static gpointer
writer_thread (gpointer user_data)
{
  GOutputStream *out;
  gssize nwrote, offset;
  GError *err = NULL;
  HANDLE out_handle;

  g_assert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (writer_pipe[1]),
			     GetCurrentProcess (),
			     &out_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (writer_pipe[1]);

  out = g_win32_output_stream_new (out_handle, TRUE);
  do
    {
      g_usleep (10);

      offset = 0;
      while (offset < (gssize) sizeof (DATA))
	{
	  nwrote = g_output_stream_write (out, DATA + offset,
					  sizeof (DATA) - offset,
					  writer_cancel, &err);
	  if (nwrote <= 0 || err != NULL)
	    break;
	  offset += nwrote;
	}

      g_assert (nwrote > 0 || err != NULL);
    }
  while (err == NULL);

  if (g_cancellable_is_cancelled (writer_cancel))
    {
      g_cancellable_cancel (main_cancel);
      g_object_unref (out);
      return NULL;
    }

  g_warning ("writer: %s", err->message);
  g_assert_not_reached ();
}

static gpointer
reader_thread (gpointer user_data)
{
  GInputStream *in;
  gssize nread = 0, total;
  GError *err = NULL;
  char buf[sizeof (DATA)];
  HANDLE in_handle;

  g_assert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (reader_pipe[0]),
			     GetCurrentProcess (),
			     &in_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (reader_pipe[0]);

  in = g_win32_input_stream_new (in_handle, TRUE);

  do
    {
      total = 0;
      while (total < (gssize) sizeof (DATA))
	{
	  nread = g_input_stream_read (in, buf + total, sizeof (buf) - total,
				       reader_cancel, &err);
	  if (nread <= 0 || err != NULL)
	    break;
	  total += nread;
	}

      if (err)
	break;

      if (nread == 0)
	{
	  g_assert (err == NULL);
	  /* pipe closed */
	  g_object_unref (in);
	  return NULL;
	}

      g_assert_cmpstr (buf, ==, DATA);
      g_assert (!g_cancellable_is_cancelled (reader_cancel));
    }
  while (err == NULL);

  g_warning ("reader: %s", err->message);
  g_assert_not_reached ();
}

char main_buf[sizeof (DATA)];
gssize main_len, main_offset;

static void readable (GObject *source, GAsyncResult *res, gpointer user_data);
static void writable (GObject *source, GAsyncResult *res, gpointer user_data);

static void
do_main_cancel (GOutputStream *out)
{
  g_output_stream_close (out, NULL, NULL);
  g_main_loop_quit (loop);
}

static void
readable (GObject *source, GAsyncResult *res, gpointer user_data)
{
  GInputStream *in = G_INPUT_STREAM (source);
  GOutputStream *out = user_data;
  GError *err = NULL;

  main_len = g_input_stream_read_finish (in, res, &err);

  if (g_cancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      return;
    }

  g_assert (err == NULL);

  main_offset = 0;
  g_output_stream_write_async (out, main_buf, main_len,
			       G_PRIORITY_DEFAULT, main_cancel,
			       writable, in);
}

static void
writable (GObject *source, GAsyncResult *res, gpointer user_data)
{
  GOutputStream *out = G_OUTPUT_STREAM (source);
  GInputStream *in = user_data;
  GError *err = NULL;
  gssize nwrote;

  nwrote = g_output_stream_write_finish (out, res, &err);

  if (g_cancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      return;
    }

  g_assert (err == NULL);
  g_assert_cmpint (nwrote, <=, main_len - main_offset);

  main_offset += nwrote;
  if (main_offset == main_len)
    {
      g_input_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 readable, out);
    }
  else
    {
      g_output_stream_write_async (out, main_buf + main_offset,
				   main_len - main_offset,
				   G_PRIORITY_DEFAULT, main_cancel,
				   writable, in);
    }
}

static gboolean
timeout (gpointer cancellable)
{
  g_cancellable_cancel (cancellable);
  return FALSE;
}

static void
test_pipe_io (void)
{
  GThread *writer, *reader;
  GInputStream *in;
  GOutputStream *out;
  HANDLE in_handle, out_handle;

  /* Split off two (additional) threads, a reader and a writer. From
   * the writer thread, write data synchronously in small chunks,
   * which gets read asynchronously by the main thread and then
   * written asynchronously to the reader thread, which reads it
   * synchronously. Eventually a timeout in the main thread will cause
   * it to cancel the writer thread, which will in turn cancel the
   * read op in the main thread, which will then close the pipe to
   * the reader thread, causing the read op to fail.
   */

  g_assert (_pipe (writer_pipe, 10, _O_BINARY) == 0 && _pipe (reader_pipe, 10, _O_BINARY) == 0);

  writer_cancel = g_cancellable_new ();
  reader_cancel = g_cancellable_new ();
  main_cancel = g_cancellable_new ();

  writer = g_thread_create (writer_thread, NULL, TRUE, NULL);
  reader = g_thread_create (reader_thread, NULL, TRUE, NULL);

  g_assert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (writer_pipe[0]),
			     GetCurrentProcess (),
			     &in_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (writer_pipe[0]);

  g_assert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (reader_pipe[1]),
			     GetCurrentProcess (),
			     &out_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (reader_pipe[1]);

  in = g_win32_input_stream_new (in_handle, TRUE);
  out = g_win32_output_stream_new (out_handle, TRUE);

  g_input_stream_read_async (in, main_buf, sizeof (main_buf),
			     G_PRIORITY_DEFAULT, main_cancel,
			     readable, out);

  g_timeout_add (500, timeout, writer_cancel);

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_thread_join (reader);
  g_thread_join (writer);

  g_object_unref (main_cancel);
  g_object_unref (reader_cancel);
  g_object_unref (writer_cancel);
  g_object_unref (in);
  g_object_unref (out);
}

int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32-streams/pipe-io-test", test_pipe_io);

  return g_test_run();
}
