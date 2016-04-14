#include <glib.h>

static void
test_overwrite (void)
{
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      GError *error;
      error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
      g_set_error_literal (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "bla");
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*set over the top*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      GError *dest;
      GError *src;
      dest = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
      src = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, "bla");
      g_propagate_error (&dest, src);
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*set over the top*");
}

static void
test_prefix (void)
{
  GError *error;
  GError *dest, *src;

  error = NULL;
  g_prefix_error (&error, "foo %d %s: ", 1, "two");
  g_assert (error == NULL);

  error = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_prefix_error (&error, "foo %d %s: ", 1, "two");
  g_assert_cmpstr (error->message, ==, "foo 1 two: bla");
  g_error_free (error);

  dest = NULL;
  src = g_error_new_literal (G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "bla");
  g_propagate_prefixed_error (&dest, src, "foo %d %s: ", 1, "two");
  g_assert_cmpstr (dest->message, ==, "foo 1 two: bla");
  g_error_free (dest);
}

static void
test_literal (void)
{
  GError *error;

  error = NULL;
  g_set_error_literal (&error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY, "%s %d %x");
  g_assert_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY);
  g_assert_cmpstr (error->message, ==, "%s %d %x");
  g_error_free (error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/error/overwrite", test_overwrite);
  g_test_add_func ("/error/prefix", test_prefix);
  g_test_add_func ("/error/literal", test_literal);

  return g_test_run ();
}

