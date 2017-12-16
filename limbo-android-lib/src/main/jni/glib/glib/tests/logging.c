#include <stdlib.h>
#include <glib.h>

/* Test g_warn macros */
static void
test_warnings (void)
{
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_warn_if_reached ();
    }
  g_test_trap_assert_failed();
  g_test_trap_assert_stderr ("*WARNING*test_warnings*should not be reached*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_warn_if_fail (FALSE);
    }
  g_test_trap_assert_failed();
  g_test_trap_assert_stderr ("*WARNING*test_warnings*runtime check failed*");
}

static guint log_count = 0;

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
  g_assert_cmpstr (log_domain, ==, "bu");
  g_assert_cmpint (log_level, ==, G_LOG_LEVEL_INFO);

  log_count++;
}

/* test that custom log handlers only get called for
 * their domain and level
 */
static void
test_set_handler (void)
{
  guint id;

  id = g_log_set_handler ("bu", G_LOG_LEVEL_INFO, log_handler, NULL);

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT))
    {
      g_log ("bu", G_LOG_LEVEL_DEBUG, "message");
      g_log ("ba", G_LOG_LEVEL_DEBUG, "message");
      g_log ("bu", G_LOG_LEVEL_INFO, "message");
      g_log ("ba", G_LOG_LEVEL_INFO, "message");

      g_assert_cmpint (log_count, ==, 1);
      exit (0);
    }
  g_test_trap_assert_passed ();

  g_log_remove_handler ("bu", id);
}

static void
test_default_handler (void)
{
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_error ("message1");
      exit (0);
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*message1*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_critical ("message2");
      exit (0);
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*message2*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_warning ("message3");
      exit (0);
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*message3*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_message ("message4");
      exit (0);
    }
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*Message*message4*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT))
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "message5");
      exit (0);
    }
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*INFO*message5*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT))
    {
      g_debug ("message6");
      exit (0);
    }
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*DEBUG*message6*");

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT))
    {
      g_log (G_LOG_DOMAIN, 1<<10, "message7");
      exit (0);
    }
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*LOG-0x400*message7*");
}

/* test that setting levels as fatal works */
static void
test_fatal_log_mask (void)
{
  GLogLevelFlags flags;

  flags = g_log_set_fatal_mask ("bu", G_LOG_LEVEL_INFO);
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT))
    g_log ("bu", G_LOG_LEVEL_INFO, "fatal");
  g_test_trap_assert_failed ();
  g_log_set_fatal_mask ("bu", flags);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/logging/default-handler", test_default_handler);
  g_test_add_func ("/logging/warnings", test_warnings);
  g_test_add_func ("/logging/fatal-log-mask", test_fatal_log_mask);
  g_test_add_func ("/logging/set-handler", test_set_handler);

  return g_test_run ();
}

