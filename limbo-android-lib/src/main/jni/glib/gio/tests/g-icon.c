/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 *
 * Authors: David Zeuthen <davidz@redhat.com>
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static void
test_g_icon_serialize (void)
{
  GIcon *icon;
  GIcon *icon2;
  GIcon *icon3;
  GIcon *icon4;
  GIcon *icon5;
  GEmblem *emblem1;
  GEmblem *emblem2;
  const char *uri;
  GFile *location;
  char *data;
  GError *error;

  error = NULL;

  /* check that GFileIcon and GThemedIcon serialize to the encoding specified */

  uri = "file:///some/native/path/to/an/icon.png";
  location = g_file_new_for_uri (uri);
  icon = g_file_icon_new (location);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "/some/native/path/to/an/icon.png");
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);

  uri = "file:///some/native/path/to/an/icon with spaces.png";
  location = g_file_new_for_uri (uri);
  icon = g_file_icon_new (location);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "/some/native/path/to/an/icon with spaces.png");
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);

  uri = "sftp:///some/non-native/path/to/an/icon.png";
  location = g_file_new_for_uri (uri);
  icon = g_file_icon_new (location);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///some/non-native/path/to/an/icon.png");
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);

#if 0
  uri = "sftp:///some/non-native/path/to/an/icon with spaces.png";
  location = g_file_new_for_uri (uri);
  icon = g_file_icon_new (location);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///some/non-native/path/to/an/icon%20with%20spaces.png");
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);
#endif

  icon = g_themed_icon_new ("network-server");
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "network-server");
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);

  /* Check that we can serialize from well-known specified formats */
  icon = g_icon_new_for_string ("network-server%", &error);
  g_assert_no_error (error);
  icon2 = g_themed_icon_new ("network-server%");
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (icon);
  g_object_unref (icon2);

  icon = g_icon_new_for_string ("/path/to/somewhere.png", &error);
  g_assert_no_error (error);
  location = g_file_new_for_commandline_arg ("/path/to/somewhere.png");
  icon2 = g_file_icon_new (location);
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);

  icon = g_icon_new_for_string ("/path/to/somewhere with whitespace.png", &error);
  g_assert_no_error (error);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "/path/to/somewhere with whitespace.png");
  g_free (data);
  location = g_file_new_for_commandline_arg ("/path/to/somewhere with whitespace.png");
  icon2 = g_file_icon_new (location);
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (location);
  g_object_unref (icon2);
  location = g_file_new_for_commandline_arg ("/path/to/somewhere%20with%20whitespace.png");
  icon2 = g_file_icon_new (location);
  g_assert (!g_icon_equal (icon, icon2));
  g_object_unref (location);
  g_object_unref (icon2);
  g_object_unref (icon);

  icon = g_icon_new_for_string ("sftp:///path/to/somewhere.png", &error);
  g_assert_no_error (error);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///path/to/somewhere.png");
  g_free (data);
  location = g_file_new_for_commandline_arg ("sftp:///path/to/somewhere.png");
  icon2 = g_file_icon_new (location);
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (location);

#if 0
  icon = g_icon_new_for_string ("sftp:///path/to/somewhere with whitespace.png", &error);
  g_assert_no_error (error);
  data = g_icon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///path/to/somewhere%20with%20whitespace.png");
  g_free (data);
  location = g_file_new_for_commandline_arg ("sftp:///path/to/somewhere with whitespace.png");
  icon2 = g_file_icon_new (location);
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (location);
  g_object_unref (icon2);
  location = g_file_new_for_commandline_arg ("sftp:///path/to/somewhere%20with%20whitespace.png");
  icon2 = g_file_icon_new (location);
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (location);
  g_object_unref (icon2);
  g_object_unref (icon);
#endif

  /* Check that GThemedIcon serialization works */

  icon = g_themed_icon_new ("network-server");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = g_icon_to_string (icon);
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);

  icon = g_themed_icon_new ("icon name with whitespace");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = g_icon_to_string (icon);
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);

  icon = g_themed_icon_new_with_default_fallbacks ("network-server-xyz");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = g_icon_to_string (icon);
  icon2 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon, icon2));
  g_free (data);
  g_object_unref (icon);
  g_object_unref (icon2);

  /* Check that GEmblemedIcon serialization works */

  icon = g_themed_icon_new ("face-smirk");
  icon2 = g_themed_icon_new ("emblem-important");
  g_themed_icon_append_name (G_THEMED_ICON (icon2), "emblem-shared");
  location = g_file_new_for_uri ("file:///some/path/somewhere.png");
  icon3 = g_file_icon_new (location);
  g_object_unref (location);
  emblem1 = g_emblem_new_with_origin (icon2, G_EMBLEM_ORIGIN_DEVICE);
  emblem2 = g_emblem_new_with_origin (icon3, G_EMBLEM_ORIGIN_LIVEMETADATA);
  icon4 = g_emblemed_icon_new (icon, emblem1);
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon4), emblem2);
  data = g_icon_to_string (icon4);
  icon5 = g_icon_new_for_string (data, &error);
  g_assert_no_error (error);
  g_assert (g_icon_equal (icon4, icon5));
  g_object_unref (emblem1);
  g_object_unref (emblem2);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (icon3);
  g_object_unref (icon4);
  g_object_unref (icon5);
  g_free (data);
}

static void
test_themed_icon (void)
{
  GIcon *icon1, *icon2, *icon3;
  const gchar *const *names;
  const gchar *names2[] = { "first", "testicon", "last", NULL };
  gchar *str;
  gboolean fallbacks;

  icon1 = g_themed_icon_new ("testicon");

  g_object_get (icon1, "use-default-fallbacks", &fallbacks, NULL);
  g_assert (!fallbacks);

  names = g_themed_icon_get_names (G_THEMED_ICON (icon1));
  g_assert_cmpint (g_strv_length ((gchar **)names), ==, 1);
  g_assert_cmpstr (names[0], ==, "testicon");

  g_themed_icon_prepend_name (G_THEMED_ICON (icon1), "first");
  g_themed_icon_append_name (G_THEMED_ICON (icon1), "last");
  names = g_themed_icon_get_names (G_THEMED_ICON (icon1));
  g_assert_cmpint (g_strv_length ((gchar **)names), ==, 3);
  g_assert_cmpstr (names[0], ==, "first");
  g_assert_cmpstr (names[1], ==, "testicon");
  g_assert_cmpstr (names[2], ==, "last");
  g_assert_cmpuint (g_icon_hash (icon1), ==, 3193088045U);

  icon2 = g_themed_icon_new_from_names ((gchar**)names2, -1);
  g_assert (g_icon_equal (icon1, icon2));

  str = g_icon_to_string (icon2);
  icon3 = g_icon_new_for_string (str, NULL);
  g_assert (g_icon_equal (icon2, icon3));
  g_free (str);

  g_object_unref (icon1);
  g_object_unref (icon2);
  g_object_unref (icon3);
}

static void
test_emblemed_icon (void)
{
  GIcon *icon1, *icon2, *icon3, *icon4;
  GEmblem *emblem, *emblem1, *emblem2;
  GList *emblems;

  icon1 = g_themed_icon_new ("testicon");
  icon2 = g_themed_icon_new ("testemblem");
  emblem1 = g_emblem_new (icon2);
  emblem2 = g_emblem_new_with_origin (icon2, G_EMBLEM_ORIGIN_TAG);

  icon3 = g_emblemed_icon_new (icon1, emblem1);
  emblems = g_emblemed_icon_get_emblems (G_EMBLEMED_ICON (icon3));
  g_assert_cmpint (g_list_length (emblems), ==, 1);
  g_assert (g_emblemed_icon_get_icon (G_EMBLEMED_ICON (icon3)) == icon1);

  icon4 = g_emblemed_icon_new (icon1, emblem1);
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon4), emblem2);
  emblems = g_emblemed_icon_get_emblems (G_EMBLEMED_ICON (icon4));
  g_assert_cmpint (g_list_length (emblems), ==, 2);

  g_assert (!g_icon_equal (icon3, icon4));

  emblem = emblems->data;
  g_assert (g_emblem_get_icon (emblem) == icon2);
  g_assert (g_emblem_get_origin (emblem) == G_EMBLEM_ORIGIN_UNKNOWN);

  emblem = emblems->next->data;
  g_assert (g_emblem_get_icon (emblem) == icon2);
  g_assert (g_emblem_get_origin (emblem) == G_EMBLEM_ORIGIN_TAG);

  g_object_unref (icon1);
  g_object_unref (icon2);
  g_object_unref (icon3);
  g_object_unref (icon4);

  g_object_unref (emblem1);
  g_object_unref (emblem2);
}

static void
test_file_icon (void)
{
  GFile *file;
  GIcon *icon;
  GIcon *icon2;
  GError *error;
  GInputStream *stream;
  gchar *str;

  file = g_file_new_for_path (SRCDIR "/g-icon.c");
  icon = g_file_icon_new (file);
  g_object_unref (file);

  error = NULL;
  stream = g_loadable_icon_load (G_LOADABLE_ICON (icon), 20, NULL, NULL, &error);
  g_assert (stream != NULL);
  g_assert_no_error (error);

  g_object_unref (stream);

  str = g_icon_to_string (icon);
  icon2 = g_icon_new_for_string (str, NULL);
  g_assert (g_icon_equal (icon, icon2));
  g_free (str);

  g_object_unref (icon);
  g_object_unref (icon2);
}

int
main (int   argc,
      char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/icons/serialize", test_g_icon_serialize);
  g_test_add_func ("/icons/themed", test_themed_icon);
  g_test_add_func ("/icons/emblemed", test_emblemed_icon);
  g_test_add_func ("/icons/file", test_file_icon);

  return g_test_run();
}
