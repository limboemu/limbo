#include <gio/gio.h>

static void
test_write (void)
{
  GOutputStream *base;
  GOutputStream *out;
  GError *error;
  const gchar buffer[] = "abcdefghijklmnopqrstuvwxyz";

  base = g_memory_output_stream_new (g_malloc0 (20), 20, g_realloc, g_free);
  out = g_buffered_output_stream_new (base);

  g_assert_cmpint (g_buffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 4096);
  g_assert (!g_buffered_output_stream_get_auto_grow (G_BUFFERED_OUTPUT_STREAM (out)));
  g_buffered_output_stream_set_buffer_size (G_BUFFERED_OUTPUT_STREAM (out), 16);
  g_assert_cmpint (g_buffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 16);

  error = NULL;
  g_assert_cmpint (g_output_stream_write (out, buffer, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  g_assert_cmpint (g_output_stream_write (out, buffer + 10, 10, NULL, &error), ==, 6);
  g_assert_no_error (error);

  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);
  g_assert (g_output_stream_flush (out, NULL, &error));
  g_assert_no_error (error);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 16);

  g_assert_cmpstr (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (base)), ==, "abcdefghijklmnop");

  g_object_unref (out);
  g_object_unref (base);
}

static void
test_grow (void)
{
  GOutputStream *base;
  GOutputStream *out;
  GError *error;
  const gchar buffer[] = "abcdefghijklmnopqrstuvwxyz";
  gint size;
  gboolean grow;

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = g_buffered_output_stream_new_sized (base, 16);

  g_buffered_output_stream_set_auto_grow (G_BUFFERED_OUTPUT_STREAM (out), TRUE);

  g_object_get (out, "buffer-size", &size, "auto-grow", &grow, NULL);
  g_assert_cmpint (size, ==, 16);
  g_assert (grow);

  error = NULL;
  g_assert_cmpint (g_output_stream_write (out, buffer, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (g_buffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 16);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  g_assert_cmpint (g_output_stream_write (out, buffer + 10, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (g_buffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), >=, 20);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  g_assert (g_output_stream_flush (out, NULL, &error));
  g_assert_no_error (error);

  g_assert_cmpstr (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (base)), ==, "abcdefghijklmnopqrst");

  g_object_unref (out);
  g_object_unref (base);
}

static void
test_close (void)
{
  GOutputStream *base;
  GOutputStream *out;
  GError *error;

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = g_buffered_output_stream_new (base);

  g_assert (g_filter_output_stream_get_close_base_stream (G_FILTER_OUTPUT_STREAM (out)));

  error = NULL;
  g_assert (g_output_stream_close (out, NULL, &error));
  g_assert_no_error (error);
  g_assert (g_output_stream_is_closed (base));

  g_object_unref (out);
  g_object_unref (base);

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = g_buffered_output_stream_new (base);

  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (out), FALSE);

  error = NULL;
  g_assert (g_output_stream_close (out, NULL, &error));
  g_assert_no_error (error);
  g_assert (!g_output_stream_is_closed (base));

  g_object_unref (out);
  g_object_unref (base);
}

int
main (int argc, char *argv[])
{
  g_type_init ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/buffered-output-stream/write", test_write);
  g_test_add_func ("/buffered-output-stream/grow", test_grow);
  g_test_add_func ("/filter-output-stream/close", test_close);

  return g_test_run ();
}
