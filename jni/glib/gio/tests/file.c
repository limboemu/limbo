#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gio/gfiledescriptorbased.h>

static void
test_basic (void)
{
  GFile *file;
  gchar *s;

  file = g_file_new_for_path ("./some/directory/testfile");

  s = g_file_get_basename (file);
  g_assert_cmpstr (s, ==, "testfile");
  g_free (s);

  s = g_file_get_uri (file);
  g_assert (g_str_has_prefix (s, "file://"));
  g_assert (g_str_has_suffix (s, "/some/directory/testfile"));
  g_free (s);

  g_assert (g_file_has_uri_scheme (file, "file"));
  s = g_file_get_uri_scheme (file);
  g_assert_cmpstr (s, ==, "file");
  g_free (s);

  g_object_unref (file);
}

static void
test_parent (void)
{
  GFile *file;
  GFile *file2;
  GFile *parent;
  GFile *root;

  file = g_file_new_for_path ("./some/directory/testfile");
  file2 = g_file_new_for_path ("./some/directory");
  root = g_file_new_for_path ("/");

  g_assert (g_file_has_parent (file, file2));

  parent = g_file_get_parent (file);
  g_assert (g_file_equal (parent, file2));
  g_object_unref (parent);

  g_assert (g_file_get_parent (root) == NULL);

  g_object_unref (file);
  g_object_unref (file2);
  g_object_unref (root);
}

static void
test_child (void)
{
  GFile *file;
  GFile *child;
  GFile *child2;

  file = g_file_new_for_path ("./some/directory");
  child = g_file_get_child (file, "child");
  g_assert (g_file_has_parent (child, file));

  child2 = g_file_get_child_for_display_name (file, "child2", NULL);
  g_assert (g_file_has_parent (child2, file));

  g_object_unref (child);
  g_object_unref (child2);
  g_object_unref (file);
}

static void
test_type (void)
{
  GFile *file;
  GFileType type;

  file = g_file_new_for_path (SRCDIR "/file.c");
  type = g_file_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, G_FILE_TYPE_REGULAR);
  g_object_unref (file);

  file = g_file_new_for_path (SRCDIR "/schema-tests");
  type = g_file_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, G_FILE_TYPE_DIRECTORY);
  g_object_unref (file);
}


typedef struct
{
  GFile *file;
  GFileMonitor *monitor;
  GOutputStream *ostream;
  GInputStream *istream;
  GMainLoop *loop;
  gint buffersize;
  gint monitor_created;
  gint monitor_deleted;
  gint monitor_changed;
  gchar *monitor_path;
  gint pos;
  gchar *data;
  gchar *buffer;
  guint timeout;
} CreateDeleteData;

static void
monitor_changed (GFileMonitor      *monitor,
                 GFile             *file,
                 GFile             *other_file,
                 GFileMonitorEvent  event_type,
                 gpointer           user_data)
{
  CreateDeleteData *data = user_data;

  g_assert_cmpstr (data->monitor_path, ==, g_file_get_path (file));

  if (event_type == G_FILE_MONITOR_EVENT_CREATED)
    data->monitor_created++;
  if (event_type == G_FILE_MONITOR_EVENT_DELETED)
    data->monitor_deleted++;
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    data->monitor_changed++;
}


static gboolean
quit_idle (gpointer user_data)
{
  CreateDeleteData *data = user_data;

  g_source_remove (data->timeout); 
  g_main_loop_quit (data->loop);

  return FALSE;
}

static void
iclosed_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gboolean ret;

  error = NULL;
  ret = g_input_stream_close_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert (ret);

  g_assert (g_input_stream_is_closed (data->istream));

  ret = g_file_delete (data->file, NULL, &error);
  g_assert (ret);
  g_assert_no_error (error);

  /* work around file monitor bug:
   * inotify events are only processed every 1000 ms, regardless
   * of the rate limit set on the file monitor
   */
  g_timeout_add (2000, quit_idle, data);
}

static void
read_cb (GObject      *source,
         GAsyncResult *res,
         gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_input_stream_read_finish (data->istream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      g_input_stream_read_async (data->istream,
                                 data->buffer + data->pos,
                                 strlen (data->data) - data->pos,
                                 0,
                                 NULL,
                                 read_cb,
                                 data);
    }
  else
    {
      g_assert_cmpstr (data->buffer, ==, data->data);
      g_assert (!g_input_stream_is_closed (data->istream));
      g_input_stream_close_async (data->istream, 0, NULL, iclosed_cb, data);
    }
}

static void
ipending_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_input_stream_read_finish (data->istream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  g_error_free (error);
}

static void
skipped_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_input_stream_skip_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert_cmpint (size, ==, data->pos);

  g_input_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             read_cb,
                             data);
  /* check that we get a pending error */
  g_input_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             ipending_cb,
                             data);
}

static void
opened_cb (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  GFileInputStream *base;
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  base = g_file_read_finish (data->file, res, &error);
  g_assert_no_error (error);

  if (data->buffersize == 0)
    data->istream = G_INPUT_STREAM (g_object_ref (base));
  else
    data->istream = g_buffered_input_stream_new_sized (G_INPUT_STREAM (base), data->buffersize);
  g_object_unref (base);

  data->buffer = g_new0 (gchar, strlen (data->data) + 1);

  /* copy initial segment directly, then skip */
  memcpy (data->buffer, data->data, 10);
  data->pos = 10;

  g_input_stream_skip_async (data->istream,
                             10,
                             0,
                             NULL,
                             skipped_cb,
                             data);
}

static void
oclosed_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gboolean ret;

  error = NULL;
  ret = g_output_stream_close_finish (data->ostream, res, &error);
  g_assert_no_error (error);
  g_assert (ret);
  g_assert (g_output_stream_is_closed (data->ostream));

  g_file_read_async (data->file, 0, NULL, opened_cb, data);
}

static void
written_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  gssize size;
  GError *error;

  error = NULL;
  size = g_output_stream_write_finish (data->ostream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      g_output_stream_write_async (data->ostream,
                                   data->data + data->pos,
                                   strlen (data->data) - data->pos,
                                   0,
                                   NULL,
                                   written_cb,
                                   data);
    }
  else
    {
      g_assert (!g_output_stream_is_closed (data->ostream));
      g_output_stream_close_async (data->ostream, 0, NULL, oclosed_cb, data);
    }
}

static void
opending_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  CreateDeleteData *data = user_data;
  GError *error;
  gssize size;

  error = NULL;
  size = g_output_stream_write_finish (data->ostream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  g_error_free (error);
}

static void
created_cb (GObject      *source,
            GAsyncResult *res,
            gpointer      user_data)
{
  GFileOutputStream *base;
  CreateDeleteData *data = user_data;
  GError *error;

  error = NULL;
  base = g_file_create_finish (G_FILE (source), res, &error);
  g_assert_no_error (error);
  g_assert (g_file_query_exists  (data->file, NULL));

  if (data->buffersize == 0)
    data->ostream = G_OUTPUT_STREAM (g_object_ref (base));
  else
    data->ostream = g_buffered_output_stream_new_sized (G_OUTPUT_STREAM (base), data->buffersize);
  g_object_unref (base);

  g_output_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               written_cb,
                               data);
  /* check that we get a pending error */
  g_output_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               opending_cb,
                               data);
}

static gboolean
stop_timeout (gpointer data)
{
  g_assert_not_reached ();

  return FALSE;
}

/*
 * This test does a fully async create-write-read-delete.
 * Callbackistan.
 */
static void
test_create_delete (gconstpointer d)
{
  GError *error;
  CreateDeleteData *data;
  int tmpfd;

  data = g_new0 (CreateDeleteData, 1);

  data->buffersize = GPOINTER_TO_INT (d);
  data->data = "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ0123456789";
  data->pos = 0;

  /* Using tempnam() would be easier here, but causes a compile warning */
  tmpfd = g_file_open_tmp ("g_file_create_delete_XXXXXX",
			   &data->monitor_path, NULL);
  g_assert_cmpint (tmpfd, !=, -1);
  close (tmpfd);
  remove (data->monitor_path);

  data->file = g_file_new_for_path (data->monitor_path);
  g_assert (!g_file_query_exists  (data->file, NULL));

  error = NULL;
  data->monitor = g_file_monitor_file (data->file, 0, NULL, &error);
  g_assert_no_error (error);
  g_file_monitor_set_rate_limit (data->monitor, 100);

  g_signal_connect (data->monitor, "changed", G_CALLBACK (monitor_changed), data);

  data->loop = g_main_loop_new (NULL, FALSE);

  data->timeout = g_timeout_add (5000, stop_timeout, NULL);

  g_file_create_async (data->file, 0, 0, NULL, created_cb, data);

  g_main_loop_run (data->loop);

  g_assert_cmpint (data->monitor_created, ==, 1);
  g_assert_cmpint (data->monitor_deleted, ==, 1);
  g_assert_cmpint (data->monitor_changed, >, 0);

  g_assert (!g_file_monitor_is_cancelled (data->monitor));
  g_file_monitor_cancel (data->monitor);
  g_assert (g_file_monitor_is_cancelled (data->monitor));

  g_main_loop_unref (data->loop);
  g_object_unref (data->monitor);
  g_object_unref (data->ostream);
  g_object_unref (data->istream);
  g_object_unref (data->file);
  free (data->monitor_path);
  g_free (data->buffer);
  g_free (data);
}

typedef struct
{
  GFile *file;
  const gchar *data;
  GMainLoop *loop;
  gboolean again;
} ReplaceLoadData;

static void replaced_cb (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data);

static void
loaded_cb (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  ReplaceLoadData *data = user_data;
  gboolean ret;
  GError *error;
  gchar *contents;
  gsize length;

  error = NULL;
  ret = g_file_load_contents_finish (data->file, res, &contents, &length, NULL, &error);
  g_assert (ret);
  g_assert_no_error (error);
  g_assert_cmpint (length, ==, strlen (data->data));
  g_assert_cmpstr (contents, ==, data->data);

  g_free (contents);

  if (data->again)
    {
      data->again = FALSE;
      data->data = "pi pa po";

      g_file_replace_contents_async (data->file,
                                     data->data,
                                     strlen (data->data),
                                     NULL,
                                     FALSE,
                                     0,
                                     NULL,
                                     replaced_cb,
                                     data);
    }
  else
    {
       error = NULL;
       ret = g_file_delete (data->file, NULL, &error);
       g_assert_no_error (error);
       g_assert (ret);
       g_assert (!g_file_query_exists (data->file, NULL));

       g_main_loop_quit (data->loop);
    }
}

static void
replaced_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  ReplaceLoadData *data = user_data;
  gboolean ret;
  GError *error;

  error = NULL;
  ret = g_file_replace_contents_finish (data->file, res, NULL, &error);
  g_assert_no_error (error);

  g_file_load_contents_async (data->file, NULL, loaded_cb, data);
}

static void
test_replace_load (void)
{
  ReplaceLoadData *data;
  gchar *path;
  int tmpfd;

  data = g_new0 (ReplaceLoadData, 1);
  data->again = TRUE;
  data->data =
    "/**\n"
    " * g_file_replace_contents_async:\n"
    " * @file: input #GFile.\n"
    " * @contents: string of contents to replace the file with.\n"
    " * @length: the length of @contents in bytes.\n"
    " * @etag: (allow-none): a new <link linkend=\"gfile-etag\">entity tag</link> for the @file, or %NULL\n"
    " * @make_backup: %TRUE if a backup should be created.\n"
    " * @flags: a set of #GFileCreateFlags.\n"
    " * @cancellable: optional #GCancellable object, %NULL to ignore.\n"
    " * @callback: a #GAsyncReadyCallback to call when the request is satisfied\n"
    " * @user_data: the data to pass to callback function\n"
    " * \n"
    " * Starts an asynchronous replacement of @file with the given \n"
    " * @contents of @length bytes. @etag will replace the document's\n"
    " * current entity tag.\n"
    " * \n"
    " * When this operation has completed, @callback will be called with\n"
    " * @user_user data, and the operation can be finalized with \n"
    " * g_file_replace_contents_finish().\n"
    " * \n"
    " * If @cancellable is not %NULL, then the operation can be cancelled by\n"
    " * triggering the cancellable object from another thread. If the operation\n"
    " * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. \n"
    " * \n"
    " * If @make_backup is %TRUE, this function will attempt to \n"
    " * make a backup of @file.\n"
    " **/\n";

  /* Using tempnam() would be easier here, but causes a compile warning */
  tmpfd = g_file_open_tmp ("g_file_replace_load_XXXXXX",
			   &path, NULL);
  g_assert_cmpint (tmpfd, !=, -1);
  close (tmpfd);
  remove (path);

  data->file = g_file_new_for_path (path);
  g_assert (!g_file_query_exists (data->file, NULL));

  data->loop = g_main_loop_new (NULL, FALSE);

  g_file_replace_contents_async (data->file,
                                 data->data,
                                 strlen (data->data),
                                 NULL,
                                 FALSE,
                                 0,
                                 NULL,
                                 replaced_cb,
                                 data);

  g_main_loop_run (data->loop);

  g_main_loop_unref (data->loop);
  g_object_unref (data->file);
  g_free (data);
  free (path);
}

int
main (int argc, char *argv[])
{
  g_type_init ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/file/basic", test_basic);
  g_test_add_func ("/file/parent", test_parent);
  g_test_add_func ("/file/child", test_child);
  g_test_add_func ("/file/type", test_type);
  g_test_add_data_func ("/file/async-create-delete/0", GINT_TO_POINTER (0), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/1", GINT_TO_POINTER (1), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/10", GINT_TO_POINTER (10), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/25", GINT_TO_POINTER (25), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/4096", GINT_TO_POINTER (4096), test_create_delete);
  g_test_add_func ("/file/replace-load", test_replace_load);

  return g_test_run ();
}
