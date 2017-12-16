/*
 * Copyright © 2009 Codethink Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include <glib/glib.h>
#include <gio/gio.h>

static void
test_input_filter (void)
{
  GInputStream *base, *f1, *f2;

  g_test_bug ("568394");
  base = g_memory_input_stream_new_from_data ("abcdefghijk", -1, NULL);
  f1 = g_buffered_input_stream_new (base);
  f2 = g_buffered_input_stream_new (base);

  g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (f1), FALSE);

  g_assert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f1)) == base);
  g_assert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f2)) == base);

  g_assert (!g_input_stream_is_closed (base));
  g_assert (!g_input_stream_is_closed (f1));
  g_assert (!g_input_stream_is_closed (f2));

  g_object_unref (f1);

  g_assert (!g_input_stream_is_closed (base));
  g_assert (!g_input_stream_is_closed (f2));

  g_object_unref (f2);

  g_assert (g_input_stream_is_closed (base));

  g_object_unref (base);
}

static void
test_output_filter (void)
{
  GOutputStream *base, *f1, *f2;

  base = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  f1 = g_buffered_output_stream_new (base);
  f2 = g_buffered_output_stream_new (base);

  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (f1), FALSE);

  g_assert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f1)) == base);
  g_assert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f2)) == base);

  g_assert (!g_output_stream_is_closed (base));
  g_assert (!g_output_stream_is_closed (f1));
  g_assert (!g_output_stream_is_closed (f2));

  g_object_unref (f1);

  g_assert (!g_output_stream_is_closed (base));
  g_assert (!g_output_stream_is_closed (f2));

  g_object_unref (f2);

  g_assert (g_output_stream_is_closed (base));

  g_object_unref (base);
}

gpointer expected_obj;
gpointer expected_data;
gboolean callback_happened;

#if 0
static void
in_cb (GObject      *object,
       GAsyncResult *result,
       gpointer      user_data)
{
  GError *error = NULL;

  g_assert (object == expected_obj);
  g_assert (user_data == expected_data);
  g_assert (callback_happened == FALSE);

  g_input_stream_close_finish (expected_obj, result, &error);
  g_assert (error == NULL);

  callback_happened = TRUE;
}

static void
test_input_async (void)
{
  GInputStream *base, *f1, *f2;

  base = g_memory_input_stream_new_from_data ("abcdefghijk", -1, NULL);
  f1 = g_buffered_input_stream_new (base);
  f2 = g_buffered_input_stream_new (base);

  g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (f1), FALSE);

  g_assert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f1)) == base);
  g_assert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f2)) == base);

  g_assert (!g_input_stream_is_closed (base));
  g_assert (!g_input_stream_is_closed (f1));
  g_assert (!g_input_stream_is_closed (f2));

  expected_obj = f1;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  g_input_stream_close_async (f1, 0, NULL, in_cb, expected_data);

  g_assert (callback_happened == FALSE);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  g_assert (callback_happened == TRUE);

  g_assert (!g_input_stream_is_closed (base));
  g_assert (!g_input_stream_is_closed (f2));
  g_free (expected_data);
  g_object_unref (f1);
  g_assert (!g_input_stream_is_closed (base));
  g_assert (!g_input_stream_is_closed (f2));

  expected_obj = f2;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  g_input_stream_close_async (f2, 0, NULL, in_cb, expected_data);

  g_assert (callback_happened == FALSE);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  g_assert (callback_happened == TRUE);

  g_assert (g_input_stream_is_closed (base));
  g_assert (g_input_stream_is_closed (f2));
  g_free (expected_data);
  g_object_unref (f2);

  g_assert (g_input_stream_is_closed (base));
  g_object_unref (base);
}

static void
out_cb (GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{
  GError *error = NULL;

  g_assert (object == expected_obj);
  g_assert (user_data == expected_data);
  g_assert (callback_happened == FALSE);

  g_output_stream_close_finish (expected_obj, result, &error);
  g_assert (error == NULL);

  callback_happened = TRUE;
}

static void
test_output_async (void)
{
  GOutputStream *base, *f1, *f2;

  base = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  f1 = g_buffered_output_stream_new (base);
  f2 = g_buffered_output_stream_new (base);

  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (f1), FALSE);

  g_assert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f1)) == base);
  g_assert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f2)) == base);

  g_assert (!g_output_stream_is_closed (base));
  g_assert (!g_output_stream_is_closed (f1));
  g_assert (!g_output_stream_is_closed (f2));

  expected_obj = f1;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  g_output_stream_close_async (f1, 0, NULL, out_cb, expected_data);

  g_assert (callback_happened == FALSE);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  g_assert (callback_happened == TRUE);

  g_assert (!g_output_stream_is_closed (base));
  g_assert (!g_output_stream_is_closed (f2));
  g_free (expected_data);
  g_object_unref (f1);
  g_assert (!g_output_stream_is_closed (base));
  g_assert (!g_output_stream_is_closed (f2));

  expected_obj = f2;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  g_output_stream_close_async (f2, 0, NULL, out_cb, expected_data);

  g_assert (callback_happened == FALSE);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  g_assert (callback_happened == TRUE);

  g_assert (g_output_stream_is_closed (base));
  g_assert (g_output_stream_is_closed (f2));
  g_free (expected_data);
  g_object_unref (f2);

  g_assert (g_output_stream_is_closed (base));
  g_object_unref (base);
}
#endif

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_type_init ();
  g_test_add_func ("/filter-stream/input", test_input_filter);
  g_test_add_func ("/filter-stream/output", test_output_filter);
#if 0
  g_test_add_func ("/filter-stream/async-input", test_input_async);
  g_test_add_func ("/filter-stream/async-output", test_output_async);
#endif

  return g_test_run();
}
