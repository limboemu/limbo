
#include <locale.h>

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

static void
test_launch (void)
{
  GAppInfo *appinfo;
  GError *error;

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test.desktop");
  g_assert (appinfo != NULL);

  error = NULL;
  g_assert (g_app_info_launch (appinfo, NULL, NULL, &error));
  g_assert_no_error (error);

  g_assert (g_app_info_launch_uris (appinfo, NULL, NULL, &error));
  g_assert_no_error (error);
}

static void
test_locale (const char *locale)
{
  GAppInfo *appinfo;
  const gchar *orig;

  orig = setlocale (LC_ALL, NULL);
  g_setenv ("LANGUAGE", locale, TRUE);
  setlocale (LC_ALL, "");

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test.desktop");

  if (g_strcmp0 (locale, "C") == 0)
    {
      g_assert_cmpstr (g_app_info_get_name (appinfo), ==, "appinfo-test");
      g_assert_cmpstr (g_app_info_get_description (appinfo), ==, "GAppInfo example");
      g_assert_cmpstr (g_app_info_get_display_name (appinfo), ==, "example");
    }
  else if (g_str_has_prefix (locale, "en"))
    {
      g_assert_cmpstr (g_app_info_get_name (appinfo), ==, "appinfo-test");
      g_assert_cmpstr (g_app_info_get_description (appinfo), ==, "GAppInfo example");
      g_assert_cmpstr (g_app_info_get_display_name (appinfo), ==, "example");
    }
  else if (g_str_has_prefix (locale, "de"))
    {
      g_assert_cmpstr (g_app_info_get_name (appinfo), ==, "appinfo-test-de");
      g_assert_cmpstr (g_app_info_get_description (appinfo), ==, "GAppInfo Beispiel");
      g_assert_cmpstr (g_app_info_get_display_name (appinfo), ==, "Beispiel");
    }

  g_object_unref (appinfo);

  g_setenv ("LANGUAGE", orig, TRUE);
  setlocale (LC_ALL, "");
}

static void
test_text (void)
{
  test_locale ("C");
  test_locale ("en_US");
  test_locale ("de");
  test_locale ("de_DE.UTF-8");
}

static void
test_basic (void)
{
  GAppInfo *appinfo;
  GAppInfo *appinfo2;
  GIcon *icon, *icon2;

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test.desktop");

  g_assert (g_app_info_get_id (appinfo) == NULL);
  g_assert_cmpstr (g_app_info_get_executable (appinfo), ==, "./appinfo-test");
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, "./appinfo-test --option");

  icon = g_app_info_get_icon (appinfo);
  g_assert (G_IS_THEMED_ICON (icon));
  icon2 = g_themed_icon_new ("testicon");
  g_assert (g_icon_equal (icon, icon2));
  g_object_unref (icon2);

  appinfo2 = g_app_info_dup (appinfo);
  g_assert (g_app_info_get_id (appinfo) == g_app_info_get_id (appinfo2));
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, g_app_info_get_commandline (appinfo2));

  g_object_unref (appinfo);
  g_object_unref (appinfo2);
}

static void
test_show_in (void)
{
  GAppInfo *appinfo;

  g_desktop_app_info_set_desktop_env ("GNOME");

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test.desktop");
  g_assert (g_app_info_should_show (appinfo));
  g_object_unref (appinfo);

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test-gnome.desktop");
  g_assert (g_app_info_should_show (appinfo));
  g_object_unref (appinfo);

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test-notgnome.desktop");
  g_assert (!g_app_info_should_show (appinfo));
  g_object_unref (appinfo);
}

static void
test_commandline (void)
{
  GAppInfo *appinfo;
  GError *error;

  error = NULL;
  appinfo = g_app_info_create_from_commandline ("./appinfo-test --option",
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                &error);
  g_assert (appinfo != NULL);
  g_assert_no_error (error);
  g_assert_cmpstr (g_app_info_get_name (appinfo), ==, "cmdline-app-test");
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, "./appinfo-test --option %u");
  g_assert (g_app_info_supports_uris (appinfo));
  g_assert (!g_app_info_supports_files (appinfo));

  g_object_unref (appinfo);

  error = NULL;
  appinfo = g_app_info_create_from_commandline ("./appinfo-test --option",
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_NONE,
                                                &error);
  g_assert (appinfo != NULL);
  g_assert_no_error (error);
  g_assert_cmpstr (g_app_info_get_name (appinfo), ==, "cmdline-app-test");
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, "./appinfo-test --option %f");
  g_assert (!g_app_info_supports_uris (appinfo));
  g_assert (g_app_info_supports_files (appinfo));

  g_object_unref (appinfo);
}

static void
test_launch_context (void)
{
  GAppLaunchContext *context;
  GAppInfo *appinfo;
  gchar *str;

  context = g_app_launch_context_new ();
  appinfo = g_app_info_create_from_commandline ("./appinfo-test --option",
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                NULL);

  str = g_app_launch_context_get_display (context, appinfo, NULL);
  g_assert (str == NULL);

  str = g_app_launch_context_get_startup_notify_id (context, appinfo, NULL);
  g_assert (str == NULL);

  g_object_unref (appinfo);
  g_object_unref (context);
}

static void
test_tryexec (void)
{
  GAppInfo *appinfo;

  appinfo = (GAppInfo*)g_desktop_app_info_new_from_filename (SRCDIR "/appinfo-test2.desktop");

  g_assert (appinfo == NULL);
}

/* Test that we can set an appinfo as default for a mime type or
 * file extension, and also add and remove handled mime types.
 */
static void
test_associations (void)
{
  GAppInfo *appinfo;
  GAppInfo *appinfo2;
  GError *error;
  gboolean result;
  GList *list;

  appinfo = g_app_info_create_from_commandline ("./appinfo-test --option",
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                NULL);

  error = NULL;
  result = g_app_info_set_as_default_for_type (appinfo, "application/x-glib-test", &error);

  g_assert (result);
  g_assert_no_error (error);

  appinfo2 = g_app_info_get_default_for_type ("application/x-glib-test", FALSE);

  g_assert (appinfo2);
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, g_app_info_get_commandline (appinfo2));

  g_object_unref (appinfo2);

  result = g_app_info_set_as_default_for_extension (appinfo, "gio-tests", &error);
  g_assert (result);
  g_assert_no_error (error);

  appinfo2 = g_app_info_get_default_for_type ("application/x-extension-gio-tests", FALSE);

  g_assert (appinfo2);
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, g_app_info_get_commandline (appinfo2));

  g_object_unref (appinfo2);

  result = g_app_info_add_supports_type (appinfo, "application/x-gio-test", &error);
  g_assert (result);
  g_assert_no_error (error);

  list = g_app_info_get_all_for_type ("application/x-gio-test");
  g_assert_cmpint (g_list_length (list), ==, 1);
  appinfo2 = list->data;
  g_assert_cmpstr (g_app_info_get_commandline (appinfo), ==, g_app_info_get_commandline (appinfo2));
  g_object_unref (appinfo2);
  g_list_free (list);

  g_assert (g_app_info_can_remove_supports_type (appinfo));
  g_assert (g_app_info_remove_supports_type (appinfo, "application/x-gio-test", &error));
  g_assert_no_error (error);

  g_assert (g_app_info_can_delete (appinfo));
  g_assert (g_app_info_delete (appinfo));
  g_object_unref (appinfo);
}

int
main (int argc, char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/appinfo/basic", test_basic);
  g_test_add_func ("/appinfo/text", test_text);
  g_test_add_func ("/appinfo/launch", test_launch);
  g_test_add_func ("/appinfo/show-in", test_show_in);
  g_test_add_func ("/appinfo/commandline", test_commandline);
  g_test_add_func ("/appinfo/launch-context", test_launch_context);
  g_test_add_func ("/appinfo/tryexec", test_tryexec);
  g_test_add_func ("/appinfo/associations", test_associations);

  return g_test_run ();
}

