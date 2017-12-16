/* 
 * Copyright (C) 2008 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 * Author: Matthias Clasen
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <stdlib.h>
#include <string.h>

static char *basedir;

static GAppInfo * 
create_app_info (const char *name)
{
  GError *error;
  GAppInfo *info;

  error = NULL;
  info = g_app_info_create_from_commandline ("/usr/bin/true blah",
                                             name,
                                             G_APP_INFO_CREATE_NONE,
                                             &error);
  g_assert (error == NULL);

  /* this is necessary to ensure that the info is saved */
  g_app_info_set_as_default_for_type (info, "application/x-blah", &error);
  g_assert (error == NULL);
  g_app_info_remove_supports_type (info, "application/x-blah", &error);
  g_assert (error == NULL);
  g_app_info_reset_type_associations ("application/x-blah");
  
  return info;
}

static void
test_delete (void)
{
  GAppInfo *info;

  const char *id;
  char *filename;
  gboolean res;

  info = create_app_info ("Blah");
 
  id = g_app_info_get_id (info);
  g_assert (id != NULL);

  filename = g_build_filename (basedir, "applications", id, NULL);

  res = g_file_test (filename, G_FILE_TEST_EXISTS);
  g_assert (res);

  res = g_app_info_can_delete (info);
  g_assert (res);

  res = g_app_info_delete (info);
  g_assert (res);

  res = g_file_test (filename, G_FILE_TEST_EXISTS);
  g_assert (!res);

  g_object_unref (info);

  if (g_file_test ("/usr/share/applications/gedit.desktop", G_FILE_TEST_EXISTS))
    {
      info = (GAppInfo*)g_desktop_app_info_new ("gedit.desktop");
      g_assert (info);
     
      res = g_app_info_can_delete (info);
      g_assert (!res);
 
      res = g_app_info_delete (info);
      g_assert (!res);
    }
}

static void
test_default (void)
{
  GAppInfo *info, *info1, *info2, *info3;
  GList *list;
  GError *error = NULL;  

  info1 = create_app_info ("Blah1");
  info2 = create_app_info ("Blah2");
  info3 = create_app_info ("Blah3");

  g_app_info_set_as_default_for_type (info1, "application/x-test", &error);
  g_assert (error == NULL);

  g_app_info_set_as_default_for_type (info2, "application/x-test", &error);
  g_assert (error == NULL);

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert (g_list_length (list) == 2);
  
  /* check that both are in the list, info2 before info1 */
  info = (GAppInfo *)list->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info2)) == 0);

  info = (GAppInfo *)list->next->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info1)) == 0);

  g_list_foreach (list, (GFunc)g_object_unref, NULL);
  g_list_free (list);

  /* now try adding something at the end */
  g_app_info_add_supports_type (info3, "application/x-test", &error);
  g_assert (error == NULL);

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert (g_list_length (list) == 3);
  
  /* check that all are in the list, info2, info1, info3 */
  info = (GAppInfo *)list->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info2)) == 0);

  info = (GAppInfo *)list->next->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info1)) == 0);

  info = (GAppInfo *)list->next->next->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info3)) == 0);

  g_list_foreach (list, (GFunc)g_object_unref, NULL);
  g_list_free (list);

  /* now remove info1 again */
  g_app_info_remove_supports_type (info1, "application/x-test", &error);
  g_assert (error == NULL);

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert (g_list_length (list) == 2);
  
  /* check that both are in the list, info2 before info3 */
  info = (GAppInfo *)list->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info2)) == 0);

  info = (GAppInfo *)list->next->data;
  g_assert (g_strcmp0 (g_app_info_get_id (info), g_app_info_get_id (info3)) == 0);

  g_list_foreach (list, (GFunc)g_object_unref, NULL);
  g_list_free (list);

  /* now clean it all up */
  g_app_info_reset_type_associations ("application/x-test");

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert (list == NULL);
  
  g_app_info_delete (info1);
  g_app_info_delete (info2);
  g_app_info_delete (info3);

  g_object_unref (info1);
  g_object_unref (info2);
}

static void
cleanup_dir_recurse (GFile *parent, GFile *root)
{
  gboolean res;
  GError *error;
  GFileEnumerator *enumerator;
  GFileInfo *info;
  GFile *descend;
  char *relative_path;

  g_assert (root != NULL);

  error = NULL;
  enumerator =
    g_file_enumerate_children (parent, "*",
                               G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,
                               &error);
  if (! enumerator)
          return;
  error = NULL;
  info = g_file_enumerator_next_file (enumerator, NULL, &error);
  while ((info) && (!error))
    {
      descend = g_file_get_child (parent, g_file_info_get_name (info));
      g_assert (descend != NULL);
      relative_path = g_file_get_relative_path (root, descend);
      g_assert (relative_path != NULL);

      if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
          cleanup_dir_recurse (descend, root);

      error = NULL;
      res = g_file_delete (descend, NULL, &error);
      g_assert_cmpint (res, ==, TRUE);

      g_object_unref (descend);
      error = NULL;
      info = g_file_enumerator_next_file (enumerator, NULL, &error);
    }
  g_assert (error == NULL);

  error = NULL;
  res = g_file_enumerator_close (enumerator, NULL, &error);
  g_assert_cmpint (res, ==, TRUE);
  g_assert (error == NULL);
}

static void
cleanup_subdirs (const char *base_dir)
{
  GFile *base, *file;
 
  base = g_file_new_for_path (base_dir);  
  file = g_file_get_child (base, "applications");
  cleanup_dir_recurse (file, file);
  g_object_unref (file);
  file = g_file_get_child (base, "mime");
  cleanup_dir_recurse (file, file);
  g_object_unref (file);
}

int
main (int   argc,
      char *argv[])
{
  gint result;

  g_type_init ();
  g_test_init (&argc, &argv, NULL);
  
  basedir = g_get_current_dir ();
  g_setenv ("XDG_DATA_HOME", basedir, TRUE);
  cleanup_subdirs (basedir);
  
  g_test_add_func ("/desktop-app-info/delete", test_delete);
  g_test_add_func ("/desktop-app-info/default", test_default);

  result = g_test_run ();

  cleanup_subdirs (basedir);

  return result;
}
