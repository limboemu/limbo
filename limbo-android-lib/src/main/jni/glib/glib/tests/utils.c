/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */

#include "glib.h"

#include <stdarg.h>

static gboolean
strv_check (const gchar * const *strv, ...)
{
  va_list args;
  gchar *s;
  gint i;

  va_start (args, strv);
  for (i = 0; strv[i]; i++)
    {
      s = va_arg (args, gchar*);
      if (g_strcmp0 (strv[i], s) != 0)
        {
          va_end (args);
          return FALSE;
        }
    }

  va_end (args);

  return TRUE;
}

static void
test_language_names (void)
{
  const gchar * const *names;

  g_setenv ("LANGUAGE", "de:en_US", TRUE);
  names = g_get_language_names ();
  g_assert (strv_check (names, "de", "en_US", "en", "C", NULL));

  g_setenv ("LANGUAGE", "tt_RU.UTF-8@iqtelif", TRUE);
  names = g_get_language_names ();
  g_assert (strv_check (names,
                        "tt_RU.UTF-8@iqtelif",
                        "tt_RU@iqtelif",
                        "tt.UTF-8@iqtelif",
                        "tt@iqtelif",
                        "tt_RU.UTF-8",
                        "tt_RU",
                        "tt.UTF-8",
                        "tt",
                        "C",
                        NULL));
}

static void
test_version (void)
{
  g_assert (glib_check_version (GLIB_MAJOR_VERSION,
                                GLIB_MINOR_VERSION,
                                GLIB_MICRO_VERSION) == NULL);
  g_assert (glib_check_version (GLIB_MAJOR_VERSION,
                                GLIB_MINOR_VERSION,
                                0) == NULL);
  g_assert (glib_check_version (GLIB_MAJOR_VERSION - 1,
                                0,
                                0) != NULL);
  g_assert (glib_check_version (GLIB_MAJOR_VERSION + 1,
                                0,
                                0) != NULL);
  g_assert (glib_check_version (GLIB_MAJOR_VERSION,
                                GLIB_MINOR_VERSION + 1,
                                0) != NULL);
  g_assert (glib_check_version (GLIB_MAJOR_VERSION,
                                GLIB_MINOR_VERSION,
                                GLIB_MICRO_VERSION + 1) != NULL);
}

static const gchar *argv0;

static void
test_appname (void)
{
  const gchar *prgname;
  const gchar *appname;

  prgname = g_get_prgname ();
  appname = g_get_application_name ();
  g_assert_cmpstr (prgname, ==, argv0);
  g_assert_cmpstr (appname, ==, prgname);

  g_set_prgname ("prgname");

  prgname = g_get_prgname ();
  appname = g_get_application_name ();
  g_assert_cmpstr (prgname, ==, "prgname");
  g_assert_cmpstr (appname, ==, "prgname");

  g_set_application_name ("appname");

  prgname = g_get_prgname ();
  appname = g_get_application_name ();
  g_assert_cmpstr (prgname, ==, "prgname");
  g_assert_cmpstr (appname, ==, "appname");
}

static void
test_tmpdir (void)
{
  g_test_bug ("627969");
  g_assert_cmpstr (g_get_tmp_dir (), !=, "");
}

int
main (int   argc,
      char *argv[])
{
  argv0 = argv[0];

  /* for tmpdir test, need to do this early before g_get_any_init */
  g_setenv ("TMPDIR", "", TRUE);
  g_unsetenv ("TMP");
  g_unsetenv ("TEMP");

  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/utils/language-names", test_language_names);
  g_test_add_func ("/utils/version", test_version);
  g_test_add_func ("/utils/appname", test_appname);
  g_test_add_func ("/utils/tmpdir", test_tmpdir);

  return g_test_run();
}
