#include <gio/gio.h>

static void
test_basic (void)
{
  GNetworkAddress *address;
  guint port;
  gchar *hostname;

  address = (GNetworkAddress*)g_network_address_new ("www.gnome.org", 8080);

  g_assert_cmpstr (g_network_address_get_hostname (address), ==, "www.gnome.org");
  g_assert_cmpint (g_network_address_get_port (address), ==, 8080);

  g_object_get (address, "hostname", &hostname, "port", &port, NULL);
  g_assert_cmpstr (hostname, ==, "www.gnome.org");
  g_assert_cmpint (port, ==, 8080);
  g_free (hostname);

  g_object_unref (address);
}

typedef struct _ParseTest ParseTest;

struct _ParseTest
{
  const gchar *input;
  const gchar *hostname;
  guint16 port;
  gint error_code;
};

static ParseTest tests[] =
{
  { "www.gnome.org", "www.gnome.org", 1234, -1 },
  { "www.gnome.org:8080", "www.gnome.org", 8080, -1 },
  { "www.gnome.org:http", "www.gnome.org", 80, -1 },
  { "[2001:db8::1]", "2001:db8::1", 1234, -1 },
  { "[2001:db8::1]:888", "2001:db8::1", 888, -1 },
  { "[hostname", NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "[hostnam]e", NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:", NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:-1", NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:9999999", NULL, 0, G_IO_ERROR_INVALID_ARGUMENT }
};

static void
test_parse (gconstpointer d)
{
  const ParseTest *test = d;
  GNetworkAddress *address;
  GError *error;

  error = NULL;
  address = (GNetworkAddress*)g_network_address_parse (test->input, 1234, &error);

  if (address)
    {
      g_assert_cmpstr (g_network_address_get_hostname (address), ==, test->hostname);
      g_assert_cmpint (g_network_address_get_port (address), ==, test->port);
      g_assert_no_error (error);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, test->error_code);
    }

  if (address)
    g_object_unref (address);
  if (error)
    g_error_free (error);
}

int
main (int argc, char *argv[])
{
  gint i;
  gchar *path;

  g_type_init ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/network-address/basic", test_basic);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      path = g_strdup_printf ("/network-address/parse/%d", i);
      g_test_add_data_func (path, &tests[i], test_parse);
      g_free (path);
    }

  return g_test_run ();
}
