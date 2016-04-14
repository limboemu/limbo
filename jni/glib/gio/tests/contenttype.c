#include <gio/gio.h>

static void
test_guess (void)
{
  gchar *res;
  gchar *expected;
  gboolean uncertain;

  res = g_content_type_guess ("/etc/", NULL, 0, &uncertain);
  expected = g_content_type_from_mime_type ("inode/directory");
  g_assert (g_content_type_equals (expected, res));
  g_free (res);
  g_free (expected);

  res = g_content_type_guess ("foo.txt", NULL, 0, &uncertain);
  expected = g_content_type_from_mime_type ("text/plain");
  g_assert (g_content_type_equals (expected, res));
  g_free (res);
  g_free (expected);
}

static void
test_unknown (void)
{
  gchar *unknown;
  gchar *str;

  unknown = g_content_type_from_mime_type ("application/octet-stream");
  g_assert (g_content_type_is_unknown (unknown));
  str = g_content_type_get_mime_type (unknown);
  g_assert_cmpstr (str, ==, "application/octet-stream");
  g_free (str);
  g_free (unknown);
}

static void
test_subtype (void)
{
  gchar *plain;
  gchar *xml;

  plain = g_content_type_from_mime_type ("text/plain");
  xml = g_content_type_from_mime_type ("application/xml");

  g_assert (g_content_type_is_a (xml, plain));

  g_free (plain);
  g_free (xml);
}

static gint
find_mime (gconstpointer a, gconstpointer b)
{
  if (g_content_type_equals (a, b))
    return 0;
  return 1;
}

static void
test_list (void)
{
  GList *types;
  gchar *plain;
  gchar *xml;

  plain = g_content_type_from_mime_type ("text/plain");
  xml = g_content_type_from_mime_type ("application/xml");

  types = g_content_types_get_registered ();

  g_assert (g_list_length (types) > 1);

  /* just check that some types are in the list */
  g_assert (g_list_find_custom (types, plain, find_mime) != NULL);
  g_assert (g_list_find_custom (types, xml, find_mime) != NULL);

  g_list_foreach (types, (GFunc)g_free, NULL);
  g_list_free (types);

  g_free (plain);
  g_free (xml);
}

static void
test_executable (void)
{
  gchar *type;

  type = g_content_type_from_mime_type ("application/x-executable");
  g_assert (g_content_type_can_be_executable (type));
  g_free (type);

  type = g_content_type_from_mime_type ("text/plain");
  g_assert (g_content_type_can_be_executable (type));
  g_free (type);

  type = g_content_type_from_mime_type ("image/png");
  g_assert (!g_content_type_can_be_executable (type));
  g_free (type);
}

static void
test_description (void)
{
  gchar *type;
  gchar *desc;

  type = g_content_type_from_mime_type ("text/plain");
  desc = g_content_type_get_description (type);
  g_assert (desc != NULL);

  g_free (desc);
  g_free (type);
}

int
main (int argc, char *argv[])
{
  g_type_init ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/contenttype/guess", test_guess);
  g_test_add_func ("/contenttype/unknown", test_unknown);
  g_test_add_func ("/contenttype/subtype", test_subtype);
  g_test_add_func ("/contenttype/list", test_list);
  g_test_add_func ("/contenttype/executable", test_executable);
  g_test_add_func ("/contenttype/description", test_description);

  return g_test_run ();
}
