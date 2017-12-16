/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gcontenttypeprivate.h"
#include "gthemedicon.h"
#include "gicon.h"
#include "gfile.h"
#include "gfileenumerator.h"
#include "gfileinfo.h"
#include "glibintl.h"


/**
 * SECTION:gcontenttype
 * @short_description: Platform-specific content typing
 * @include: gio/gio.h
 *
 * A content type is a platform specific string that defines the type
 * of a file. On unix it is a mime type, on win32 it is an extension string
 * like ".doc", ".txt" or a percieved string like "audio". Such strings
 * can be looked up in the registry at HKEY_CLASSES_ROOT.
 **/

#ifdef G_OS_WIN32

#include <windows.h>

static char *
get_registry_classes_key (const char    *subdir,
                          const wchar_t *key_name)
{
  wchar_t *wc_key;
  HKEY reg_key = NULL;
  DWORD key_type;
  DWORD nbytes;
  char *value_utf8;

  value_utf8 = NULL;
  
  nbytes = 0;
  wc_key = g_utf8_to_utf16 (subdir, -1, NULL, NULL, NULL);
  if (RegOpenKeyExW (HKEY_CLASSES_ROOT, wc_key, 0,
                     KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS &&
      RegQueryValueExW (reg_key, key_name, 0,
                        &key_type, NULL, &nbytes) == ERROR_SUCCESS &&
      (key_type == REG_SZ || key_type == REG_EXPAND_SZ))
    {
      wchar_t *wc_temp = g_new (wchar_t, (nbytes+1)/2 + 1);
      RegQueryValueExW (reg_key, key_name, 0,
                        &key_type, (LPBYTE) wc_temp, &nbytes);
      wc_temp[nbytes/2] = '\0';
      if (key_type == REG_EXPAND_SZ)
        {
          wchar_t dummy[1];
          int len = ExpandEnvironmentStringsW (wc_temp, dummy, 1);
          if (len > 0)
            {
              wchar_t *wc_temp_expanded = g_new (wchar_t, len);
              if (ExpandEnvironmentStringsW (wc_temp, wc_temp_expanded, len) == len)
                value_utf8 = g_utf16_to_utf8 (wc_temp_expanded, -1, NULL, NULL, NULL);
              g_free (wc_temp_expanded);
            }
        }
      else
        {
          value_utf8 = g_utf16_to_utf8 (wc_temp, -1, NULL, NULL, NULL);
        }
      g_free (wc_temp);
      
    }
  g_free (wc_key);
  
  if (reg_key != NULL)
    RegCloseKey (reg_key);

  return value_utf8;
}

gboolean
g_content_type_equals (const gchar *type1,
                       const gchar *type2)
{
  char *progid1, *progid2;
  gboolean res;
  
  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  if (g_ascii_strcasecmp (type1, type2) == 0)
    return TRUE;

  res = FALSE;
  progid1 = get_registry_classes_key (type1, NULL);
  progid2 = get_registry_classes_key (type2, NULL);
  if (progid1 != NULL && progid2 != NULL &&
      strcmp (progid1, progid2) == 0)
    res = TRUE;
  g_free (progid1);
  g_free (progid2);
  
  return res;
}

gboolean
g_content_type_is_a (const gchar *type,
                     const gchar *supertype)
{
  gboolean res;
  char *value_utf8;

  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (supertype != NULL, FALSE);

  if (g_content_type_equals (type, supertype))
    return TRUE;

  res = FALSE;
  value_utf8 = get_registry_classes_key (type, L"PerceivedType");
  if (value_utf8 && strcmp (value_utf8, supertype) == 0)
    res = TRUE;
  g_free (value_utf8);
  
  return res;
}

gboolean
g_content_type_is_unknown (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  return strcmp ("*", type) == 0;
}

gchar *
g_content_type_get_description (const gchar *type)
{
  char *progid;
  char *description;

  g_return_val_if_fail (type != NULL, NULL);

  progid = get_registry_classes_key (type, NULL);
  if (progid)
    {
      description = get_registry_classes_key (progid, NULL);
      g_free (progid);

      if (description)
        return description;
    }

  if (g_content_type_is_unknown (type))
    return g_strdup (_("Unknown type"));
  return g_strdup_printf (_("%s filetype"), type);
}

gchar *
g_content_type_get_mime_type (const gchar *type)
{
  char *mime;

  g_return_val_if_fail (type != NULL, NULL);

  mime = get_registry_classes_key (type, L"Content Type");
  if (mime)
    return mime;
  else if (g_content_type_is_unknown (type))
    return g_strdup ("application/octet-stream");
  else if (*type == '.')
    return g_strdup_printf ("application/x-ext-%s", type+1);
  /* TODO: Map "image" to "image/ *", etc? */

  return g_strdup ("application/octet-stream");
}

G_LOCK_DEFINE_STATIC (_type_icons);
static GHashTable *_type_icons = NULL;

GIcon *
g_content_type_get_icon (const gchar *type)
{
  GIcon *themed_icon;
  char *name = NULL;

  g_return_val_if_fail (type != NULL, NULL);

  /* In the Registry icons are the default value of
     HKEY_CLASSES_ROOT\<progid>\DefaultIcon with typical values like:
     <type>: <value>
     REG_EXPAND_SZ: %SystemRoot%\System32\Wscript.exe,3
     REG_SZ: shimgvw.dll,3
  */
  G_LOCK (_type_icons);
  if (!_type_icons)
    _type_icons = g_hash_table_new (g_str_hash, g_str_equal);
  name = g_hash_table_lookup (_type_icons, type);
  if (!name && type[0] == '.')
    {
      /* double lookup by extension */
      gchar *key = get_registry_classes_key (type, NULL);
      if (!key)
        key = g_strconcat (type+1, "file\\DefaultIcon", NULL);
      else
        {
          gchar *key2 = g_strconcat (key, "\\DefaultIcon", NULL);
          g_free (key);
          key = key2;
        }
      name = get_registry_classes_key (key, NULL);
      if (name && strcmp (name, "%1") == 0)
        {
          g_free (name);
          name = NULL;
        }
      if (name)
        g_hash_table_insert (_type_icons, g_strdup (type), g_strdup (name));
      g_free (key);
    }

  /* icon-name similar to how it was with gtk-2-12 */
  if (name)
    {
      themed_icon = g_themed_icon_new (name);
    }
  else
    {
      /* if not found an icon fall back to gtk-builtins */
      name = strcmp (type, "inode/directory") == 0 ? "gtk-directory" : 
                           g_content_type_can_be_executable (type) ? "gtk-execute" : "gtk-file";
      g_hash_table_insert (_type_icons, g_strdup (type), g_strdup (name));
      themed_icon = g_themed_icon_new_with_default_fallbacks (name);
    }
  G_UNLOCK (_type_icons);

  return G_ICON (themed_icon);
}

gboolean
g_content_type_can_be_executable (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  if (strcmp (type, ".exe") == 0 ||
      strcmp (type, ".com") == 0 ||
      strcmp (type, ".bat") == 0)
    return TRUE;

  /* TODO: Also look at PATHEXT, which lists the extensions for
   * "scripts" in addition to those for true binary executables.
   *
   * (PATHEXT=.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH for me
   * right now, for instance). And in a sense, all associated file
   * types are "executable" on Windows... You can just type foo.jpg as
   * a command name in cmd.exe, and it will run the application
   * associated with .jpg. Hard to say what this API actually means
   * with "executable".
   */

  return FALSE;
}

static gboolean
looks_like_text (const guchar *data, 
                 gsize         data_size)
{
  gsize i;
  guchar c;
  for (i = 0; i < data_size; i++)
    {
      c = data[i];
      if (g_ascii_iscntrl (c) && !g_ascii_isspace (c) && c != '\b')
        return FALSE;
    }
  return TRUE;
}

gchar *
g_content_type_from_mime_type (const gchar *mime_type)
{
  char *key, *content_type;

  g_return_val_if_fail (mime_type != NULL, NULL);

  key = g_strconcat ("MIME\\DataBase\\Content Type\\", mime_type, NULL);
  content_type = get_registry_classes_key (key, L"Extension");
  g_free (key);

  return content_type;
}

gchar *
g_content_type_guess (const gchar  *filename,
                      const guchar *data,
                      gsize         data_size,
                      gboolean     *result_uncertain)
{
  char *basename;
  char *type;
  char *dot;

  type = NULL;

  if (result_uncertain)
    *result_uncertain = FALSE;

  if (filename)
    {
      basename = g_path_get_basename (filename);
      dot = strrchr (basename, '.');
      if (dot)
        type = g_strdup (dot);
      g_free (basename);
    }

  if (type)
    return type;

  if (data && looks_like_text (data, data_size))
    return g_strdup (".txt");

  return g_strdup ("*");
}

GList *
g_content_types_get_registered (void)
{
  DWORD index;
  wchar_t keyname[256];
  DWORD key_len;
  char *key_utf8;
  GList *types;

  types = NULL;
  index = 0;
  key_len = 256;
  while (RegEnumKeyExW(HKEY_CLASSES_ROOT,
                       index,
                       keyname,
                       &key_len,
                       NULL,
                       NULL,
                       NULL,
                       NULL) == ERROR_SUCCESS)
    {
      key_utf8 = g_utf16_to_utf8 (keyname, -1, NULL, NULL, NULL);
      if (key_utf8)
        {
          if (*key_utf8 == '.')
            types = g_list_prepend (types, key_utf8);
          else
            g_free (key_utf8);
        }
      index++;
      key_len = 256;
    }
  
  return g_list_reverse (types);
}

gchar **
g_content_type_guess_for_tree (GFile *root)
{
  /* FIXME: implement */
  return NULL;
}

#else /* !G_OS_WIN32 - Unix specific version */

#include <dirent.h>

#define XDG_PREFIX _gio_xdg
#include "xdgmime/xdgmime.h"

/* We lock this mutex whenever we modify global state in this module.  */
G_LOCK_DEFINE_STATIC (gio_xdgmime);

gsize
_g_unix_content_type_get_sniff_len (void)
{
  gsize size;

  G_LOCK (gio_xdgmime);
  size = xdg_mime_get_max_buffer_extents ();
  G_UNLOCK (gio_xdgmime);

  return size;
}

gchar *
_g_unix_content_type_unalias (const gchar *type)
{
  gchar *res;

  G_LOCK (gio_xdgmime);
  res = g_strdup (xdg_mime_unalias_mime_type (type));
  G_UNLOCK (gio_xdgmime);

  return res;
}

gchar **
_g_unix_content_type_get_parents (const gchar *type)
{
  const gchar *umime;
  gchar **parents;
  GPtrArray *array;
  int i;

  array = g_ptr_array_new ();

  G_LOCK (gio_xdgmime);

  umime = xdg_mime_unalias_mime_type (type);

  g_ptr_array_add (array, g_strdup (umime));

  parents = xdg_mime_list_mime_parents (umime);
  for (i = 0; parents && parents[i] != NULL; i++)
    g_ptr_array_add (array, g_strdup (parents[i]));

  free (parents);

  G_UNLOCK (gio_xdgmime);

  g_ptr_array_add (array, NULL);

  return (gchar **)g_ptr_array_free (array, FALSE);
}

/**
 * g_content_type_equals:
 * @type1: a content type string
 * @type2: a content type string
 *
 * Compares two content types for equality.
 *
 * Returns: %TRUE if the two strings are identical or equivalent,
 *     %FALSE otherwise.
 */
gboolean
g_content_type_equals (const gchar *type1,
                       const gchar *type2)
{
  gboolean res;

  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  G_LOCK (gio_xdgmime);
  res = xdg_mime_mime_type_equal (type1, type2);
  G_UNLOCK (gio_xdgmime);

  return res;
}

/**
 * g_content_type_is_a:
 * @type: a content type string
 * @supertype: a content type string
 *
 * Determines if @type is a subset of @supertype.
 *
 * Returns: %TRUE if @type is a kind of @supertype,
 *     %FALSE otherwise.
 */
gboolean
g_content_type_is_a (const gchar *type,
                     const gchar *supertype)
{
  gboolean res;

  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (supertype != NULL, FALSE);

  G_LOCK (gio_xdgmime);
  res = xdg_mime_mime_type_subclass (type, supertype);
  G_UNLOCK (gio_xdgmime);

  return res;
}

/**
 * g_content_type_is_unknown:
 * @type: a content type string
 *
 * Checks if the content type is the generic "unknown" type.
 * On UNIX this is the "application/octet-stream" mimetype,
 * while on win32 it is "*".
 *
 * Returns: %TRUE if the type is the unknown type.
 */
gboolean
g_content_type_is_unknown (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  return strcmp (XDG_MIME_TYPE_UNKNOWN, type) == 0;
}


typedef enum {
  MIME_TAG_TYPE_OTHER,
  MIME_TAG_TYPE_COMMENT
} MimeTagType;

typedef struct {
  int current_type;
  int current_lang_level;
  int comment_lang_level;
  char *comment;
} MimeParser;


static int
language_level (const char *lang)
{
  const char * const *lang_list;
  int i;

  /* The returned list is sorted from most desirable to least
     desirable and always contains the default locale "C". */
  lang_list = g_get_language_names ();

  for (i = 0; lang_list[i]; i++)
    if (strcmp (lang_list[i], lang) == 0)
      return 1000-i;

  return 0;
}

static void
mime_info_start_element (GMarkupParseContext  *context,
                         const gchar          *element_name,
                         const gchar         **attribute_names,
                         const gchar         **attribute_values,
                         gpointer              user_data,
                         GError              **error)
{
  int i;
  const char *lang;
  MimeParser *parser = user_data;

  if (strcmp (element_name, "comment") == 0)
    {
      lang = "C";
      for (i = 0; attribute_names[i]; i++)
        if (strcmp (attribute_names[i], "xml:lang") == 0)
          {
            lang = attribute_values[i];
            break;
          }

      parser->current_lang_level = language_level (lang);
      parser->current_type = MIME_TAG_TYPE_COMMENT;
    }
  else
    parser->current_type = MIME_TAG_TYPE_OTHER;
}

static void
mime_info_end_element (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       gpointer              user_data,
                       GError              **error)
{
  MimeParser *parser = user_data;

  parser->current_type = MIME_TAG_TYPE_OTHER;
}

static void
mime_info_text (GMarkupParseContext  *context,
                const gchar          *text,
                gsize                 text_len,
                gpointer              user_data,
                GError              **error)
{
  MimeParser *parser = user_data;

  if (parser->current_type == MIME_TAG_TYPE_COMMENT &&
      parser->current_lang_level > parser->comment_lang_level)
    {
      g_free (parser->comment);
      parser->comment = g_strndup (text, text_len);
      parser->comment_lang_level = parser->current_lang_level;
    }
}

static char *
load_comment_for_mime_helper (const char *dir,
                              const char *basename)
{
  GMarkupParseContext *context;
  char *filename, *data;
  gsize len;
  gboolean res;
  MimeParser parse_data = {0};
  GMarkupParser parser = {
    mime_info_start_element,
    mime_info_end_element,
    mime_info_text
  };

  filename = g_build_filename (dir, "mime", basename, NULL);

  res = g_file_get_contents (filename,  &data,  &len,  NULL);
  g_free (filename);
  if (!res)
    return NULL;

  context = g_markup_parse_context_new   (&parser, 0, &parse_data, NULL);
  res = g_markup_parse_context_parse (context, data, len, NULL);
  g_free (data);
  g_markup_parse_context_free (context);

  if (!res)
    return NULL;

  return parse_data.comment;
}


static char *
load_comment_for_mime (const char *mimetype)
{
  const char * const* dirs;
  char *basename;
  char *comment;
  int i;

  basename = g_strdup_printf ("%s.xml", mimetype);

  comment = load_comment_for_mime_helper (g_get_user_data_dir (), basename);
  if (comment)
    {
      g_free (basename);
      return comment;
    }

  dirs = g_get_system_data_dirs ();

  for (i = 0; dirs[i] != NULL; i++)
    {
      comment = load_comment_for_mime_helper (dirs[i], basename);
      if (comment)
        {
          g_free (basename);
          return comment;
        }
    }
  g_free (basename);

  return g_strdup_printf (_("%s type"), mimetype);
}

/**
 * g_content_type_get_description:
 * @type: a content type string
 *
 * Gets the human readable description of the content type.
 *
 * Returns: a short description of the content type @type. Free the
 *     returned string with g_free()
 */
gchar *
g_content_type_get_description (const gchar *type)
{
  static GHashTable *type_comment_cache = NULL;
  gchar *comment;

  g_return_val_if_fail (type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  type = xdg_mime_unalias_mime_type (type);

  if (type_comment_cache == NULL)
    type_comment_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  comment = g_hash_table_lookup (type_comment_cache, type);
  comment = g_strdup (comment);
  G_UNLOCK (gio_xdgmime);

  if (comment != NULL)
    return comment;

  comment = load_comment_for_mime (type);

  G_LOCK (gio_xdgmime);
  g_hash_table_insert (type_comment_cache,
                       g_strdup (type),
                       g_strdup (comment));
  G_UNLOCK (gio_xdgmime);

  return comment;
}

/**
 * g_content_type_get_mime_type:
 * @type: a content type string
 *
 * Gets the mime type for the content type, if one is registered.
 *
 * Returns: (allow-none): the registered mime type for the given @type,
 *     or %NULL if unknown.
 */
char *
g_content_type_get_mime_type (const char *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return g_strdup (type);
}

/**
 * g_content_type_get_icon:
 * @type: a content type string
 *
 * Gets the icon for a content type.
 *
 * Returns: #GIcon corresponding to the content type. Free the returned
 *     object with g_object_unref()
 */
GIcon *
g_content_type_get_icon (const gchar *type)
{
  char *mimetype_icon, *generic_mimetype_icon, *q;
  char *xdg_mimetype_icon, *legacy_mimetype_icon;
  char *xdg_mimetype_generic_icon;
  char *icon_names[5];
  int n = 0;
  const char *p;
  GIcon *themed_icon;

  g_return_val_if_fail (type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  xdg_mimetype_icon = g_strdup (xdg_mime_get_icon (type));
  xdg_mimetype_generic_icon = g_strdup (xdg_mime_get_generic_icon (type));
  G_UNLOCK (gio_xdgmime);

  mimetype_icon = g_strdup (type);

  while ((q = strchr (mimetype_icon, '/')) != NULL)
    *q = '-';

  p = strchr (type, '/');
  if (p == NULL)
    p = type + strlen (type);

  /* Not all icons have migrated to the new icon theme spec, look for old names too */
  legacy_mimetype_icon = g_strconcat ("gnome-mime-", mimetype_icon, NULL);

  generic_mimetype_icon = g_malloc (p - type + strlen ("-x-generic") + 1);
  memcpy (generic_mimetype_icon, type, p - type);
  memcpy (generic_mimetype_icon + (p - type), "-x-generic", strlen ("-x-generic"));
  generic_mimetype_icon[(p - type) + strlen ("-x-generic")] = 0;

  if (xdg_mimetype_icon)
    icon_names[n++] = xdg_mimetype_icon;

  icon_names[n++] = mimetype_icon;
  icon_names[n++] = legacy_mimetype_icon;

  if (xdg_mimetype_generic_icon)
    icon_names[n++] = xdg_mimetype_generic_icon;

  icon_names[n++] = generic_mimetype_icon;

  themed_icon = g_themed_icon_new_from_names (icon_names, n);

  g_free (xdg_mimetype_icon);
  g_free (xdg_mimetype_generic_icon);
  g_free (mimetype_icon);
  g_free (legacy_mimetype_icon);
  g_free (generic_mimetype_icon);

  return themed_icon;
}

/**
 * g_content_type_can_be_executable:
 * @type: a content type string
 *
 * Checks if a content type can be executable. Note that for instance
 * things like text files can be executables (i.e. scripts and batch files).
 *
 * Returns: %TRUE if the file type corresponds to a type that
 *     can be executable, %FALSE otherwise.
 */
gboolean
g_content_type_can_be_executable (const gchar *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  if (g_content_type_is_a (type, "application/x-executable")  ||
      g_content_type_is_a (type, "text/plain"))
    return TRUE;

  return FALSE;
}

static gboolean
looks_like_text (const guchar *data, gsize data_size)
{
  gsize i;
  char c;

  for (i = 0; i < data_size; i++)
    {
      c = data[i];

      if (g_ascii_iscntrl (c) &&
          !g_ascii_isspace (c) &&
          c != '\b')
        return FALSE;
    }
  return TRUE;
}

/**
 * g_content_type_from_mime_type:
 * @mime_type: a mime type string
 *
 * Tries to find a content type based on the mime type name.
 *
 * Returns: (allow-none): Newly allocated string with content type
 *     or %NULL. Free with g_free()
 *
 * Since: 2.18
 **/
gchar *
g_content_type_from_mime_type (const gchar *mime_type)
{
  char *umime;

  g_return_val_if_fail (mime_type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  /* mime type and content type are same on unixes */
  umime = g_strdup (xdg_mime_unalias_mime_type (mime_type));
  G_UNLOCK (gio_xdgmime);

  return umime;
}

/**
 * g_content_type_guess:
 * @filename: (allow-none): a string, or %NULL
 * @data: (allow-none) (array length=data_size): a stream of data, or %NULL
 * @data_size: the size of @data
 * @result_uncertain: (allow-none) (out): return location for the certainty
 *     of the result, or %NULL
 *
 * Guesses the content type based on example data. If the function is
 * uncertain, @result_uncertain will be set to %TRUE. Either @filename
 * or @data may be %NULL, in which case the guess will be based solely
 * on the other argument.
 *
 * Returns: a string indicating a guessed content type for the
 *     given data. Free with g_free()
 */
gchar *
g_content_type_guess (const gchar  *filename,
                      const guchar *data,
                      gsize         data_size,
                      gboolean     *result_uncertain)
{
  char *basename;
  const char *name_mimetypes[10], *sniffed_mimetype;
  char *mimetype;
  int i;
  int n_name_mimetypes;
  int sniffed_prio;

  sniffed_prio = 0;
  n_name_mimetypes = 0;
  sniffed_mimetype = XDG_MIME_TYPE_UNKNOWN;

  if (result_uncertain)
    *result_uncertain = FALSE;

  G_LOCK (gio_xdgmime);

  if (filename)
    {
      i = strlen (filename);
      if (filename[i - 1] == '/')
        {
          name_mimetypes[0] = "inode/directory";
          name_mimetypes[1] = NULL;
          n_name_mimetypes = 1;
          if (result_uncertain)
            *result_uncertain = TRUE;
        }
      else
        {
          basename = g_path_get_basename (filename);
          n_name_mimetypes = xdg_mime_get_mime_types_from_file_name (basename, name_mimetypes, 10);
          g_free (basename);
        }
    }

  /* Got an extension match, and no conflicts. This is it. */
  if (n_name_mimetypes == 1)
    {
      G_UNLOCK (gio_xdgmime);
      return g_strdup (name_mimetypes[0]);
    }

  if (data)
    {
      sniffed_mimetype = xdg_mime_get_mime_type_for_data (data, data_size, &sniffed_prio);
      if (sniffed_mimetype == XDG_MIME_TYPE_UNKNOWN &&
          data &&
          looks_like_text (data, data_size))
        sniffed_mimetype = "text/plain";

      /* For security reasons we don't ever want to sniff desktop files
       * where we know the filename and it doesn't have a .desktop extension.
       * This is because desktop files allow executing any application and
       * we don't want to make it possible to hide them looking like something
       * else.
       */
      if (filename != NULL &&
          strcmp (sniffed_mimetype, "application/x-desktop") == 0)
        sniffed_mimetype = "text/plain";
    }

  if (n_name_mimetypes == 0)
    {
      if (sniffed_mimetype == XDG_MIME_TYPE_UNKNOWN &&
          result_uncertain)
        *result_uncertain = TRUE;

      mimetype = g_strdup (sniffed_mimetype);
    }
  else
    {
      mimetype = NULL;
      if (sniffed_mimetype != XDG_MIME_TYPE_UNKNOWN)
        {
          if (sniffed_prio >= 80) /* High priority sniffing match, use that */
            mimetype = g_strdup (sniffed_mimetype);
          else
            {
              /* There are conflicts between the name matches and we
               * have a sniffed type, use that as a tie breaker.
               */
              for (i = 0; i < n_name_mimetypes; i++)
                {
                  if ( xdg_mime_mime_type_subclass (name_mimetypes[i], sniffed_mimetype))
                    {
                      /* This nametype match is derived from (or the same as)
                       * the sniffed type). This is probably it.
                       */
                      mimetype = g_strdup (name_mimetypes[i]);
                      break;
                    }
                }
            }
        }

      if (mimetype == NULL)
        {
          /* Conflicts, and sniffed type was no help or not there.
           * Guess on the first one
           */
          mimetype = g_strdup (name_mimetypes[0]);
          if (result_uncertain)
            *result_uncertain = TRUE;
        }
    }

  G_UNLOCK (gio_xdgmime);

  return mimetype;
}

static void
enumerate_mimetypes_subdir (const char *dir,
                            const char *prefix,
                            GHashTable *mimetypes)
{
  DIR *d;
  struct dirent *ent;
  char *mimetype;

  d = opendir (dir);
  if (d)
    {
      while ((ent = readdir (d)) != NULL)
        {
          if (g_str_has_suffix (ent->d_name, ".xml"))
            {
              mimetype = g_strdup_printf ("%s/%.*s", prefix, (int) strlen (ent->d_name) - 4, ent->d_name);
              g_hash_table_replace (mimetypes, mimetype, NULL);
            }
        }
      closedir (d);
    }
}

static void
enumerate_mimetypes_dir (const char *dir,
                         GHashTable *mimetypes)
{
  DIR *d;
  struct dirent *ent;
  char *mimedir;
  char *name;

  mimedir = g_build_filename (dir, "mime", NULL);

  d = opendir (mimedir);
  if (d)
    {
      while ((ent = readdir (d)) != NULL)
        {
          if (strcmp (ent->d_name, "packages") != 0)
            {
              name = g_build_filename (mimedir, ent->d_name, NULL);
              if (g_file_test (name, G_FILE_TEST_IS_DIR))
                enumerate_mimetypes_subdir (name, ent->d_name, mimetypes);
              g_free (name);
            }
        }
      closedir (d);
    }

  g_free (mimedir);
}

/**
 * g_content_types_get_registered:
 *
 * Gets a list of strings containing all the registered content types
 * known to the system. The list and its data should be freed using
 * <programlisting>
 * g_list_foreach (list, g_free, NULL);
 * g_list_free (list);
 * </programlisting>
 *
 * Returns: #GList of the registered content types
 */
GList *
g_content_types_get_registered (void)
{
  const char * const* dirs;
  GHashTable *mimetypes;
  GHashTableIter iter;
  gpointer key;
  int i;
  GList *l;

  mimetypes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  enumerate_mimetypes_dir (g_get_user_data_dir (), mimetypes);
  dirs = g_get_system_data_dirs ();

  for (i = 0; dirs[i] != NULL; i++)
    enumerate_mimetypes_dir (dirs[i], mimetypes);

  l = NULL;
  g_hash_table_iter_init (&iter, mimetypes);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      l = g_list_prepend (l, key);
      g_hash_table_iter_steal (&iter);
    }

  g_hash_table_destroy (mimetypes);

  return l;
}


/* tree magic data */
static GList *tree_matches = NULL;
static gboolean need_reload = FALSE;

G_LOCK_DEFINE_STATIC (gio_treemagic);

typedef struct
{
  gchar *path;
  GFileType type;
  guint match_case : 1;
  guint executable : 1;
  guint non_empty  : 1;
  guint on_disc    : 1;
  gchar *mimetype;
  GList *matches;
} TreeMatchlet;

typedef struct
{
  gchar *contenttype;
  gint priority;
  GList *matches;
} TreeMatch;


static void
tree_matchlet_free (TreeMatchlet *matchlet)
{
  g_list_foreach (matchlet->matches, (GFunc)tree_matchlet_free, NULL);
  g_list_free (matchlet->matches);
  g_free (matchlet->path);
  g_free (matchlet->mimetype);
  g_slice_free (TreeMatchlet, matchlet);
}

static void
tree_match_free (TreeMatch *match)
{
  g_list_foreach (match->matches, (GFunc)tree_matchlet_free, NULL);
  g_list_free (match->matches);
  g_free (match->contenttype);
  g_slice_free (TreeMatch, match);
}

static TreeMatch *
parse_header (gchar *line)
{
  gint len;
  gchar *s;
  TreeMatch *match;

  len = strlen (line);

  if (line[0] != '[' || line[len - 1] != ']')
    return NULL;

  line[len - 1] = 0;
  s = strchr (line, ':');

  match = g_slice_new0 (TreeMatch);
  match->priority = atoi (line + 1);
  match->contenttype = g_strdup (s + 1);

  return match;
}

static TreeMatchlet *
parse_match_line (gchar *line,
                  gint  *depth)
{
  gchar *s, *p;
  TreeMatchlet *matchlet;
  gchar **parts;
  gint i;

  matchlet = g_slice_new0 (TreeMatchlet);

  if (line[0] == '>')
    {
      *depth = 0;
      s = line;
    }
  else
    {
      *depth = atoi (line);
      s = strchr (line, '>');
    }
  s += 2;
  p = strchr (s, '"');
  *p = 0;

  matchlet->path = g_strdup (s);
  s = p + 1;
  parts = g_strsplit (s, ",", 0);
  if (strcmp (parts[0], "=file") == 0)
    matchlet->type = G_FILE_TYPE_REGULAR;
  else if (strcmp (parts[0], "=directory") == 0)
    matchlet->type = G_FILE_TYPE_DIRECTORY;
  else if (strcmp (parts[0], "=link") == 0)
    matchlet->type = G_FILE_TYPE_SYMBOLIC_LINK;
  else
    matchlet->type = G_FILE_TYPE_UNKNOWN;
  for (i = 1; parts[i]; i++)
    {
      if (strcmp (parts[i], "executable") == 0)
        matchlet->executable = 1;
      else if (strcmp (parts[i], "match-case") == 0)
        matchlet->match_case = 1;
      else if (strcmp (parts[i], "non-empty") == 0)
        matchlet->non_empty = 1;
      else if (strcmp (parts[i], "on-disc") == 0)
        matchlet->on_disc = 1;
      else
        matchlet->mimetype = g_strdup (parts[i]);
    }

  g_strfreev (parts);

  return matchlet;
}

static gint
cmp_match (gconstpointer a, gconstpointer b)
{
  const TreeMatch *aa = (const TreeMatch *)a;
  const TreeMatch *bb = (const TreeMatch *)b;

  return bb->priority - aa->priority;
}

static void
insert_match (TreeMatch *match)
{
  tree_matches = g_list_insert_sorted (tree_matches, match, cmp_match);
}

static void
insert_matchlet (TreeMatch    *match,
                 TreeMatchlet *matchlet,
                 gint          depth)
{
  if (depth == 0)
    match->matches = g_list_append (match->matches, matchlet);
  else
    {
      GList *last;
      TreeMatchlet *m;

      last = g_list_last (match->matches);
      if (!last)
        {
          tree_matchlet_free (matchlet);
          g_warning ("can't insert tree matchlet at depth %d", depth);
          return;
        }

      m = (TreeMatchlet *) last->data;
      while (--depth > 0)
        {
          last = g_list_last (m->matches);
          if (!last)
            {
              tree_matchlet_free (matchlet);
              g_warning ("can't insert tree matchlet at depth %d", depth);
              return;
            }

          m = (TreeMatchlet *) last->data;
        }
      m->matches = g_list_append (m->matches, matchlet);
    }
}

static void
read_tree_magic_from_directory (const gchar *prefix)
{
  gchar *filename;
  gchar *text;
  gsize len;
  gchar **lines;
  gint i;
  TreeMatch *match;
  TreeMatchlet *matchlet;
  gint depth;

  filename = g_build_filename (prefix, "mime", "treemagic", NULL);

  if (g_file_get_contents (filename, &text, &len, NULL))
    {
      if (strcmp (text, "MIME-TreeMagic") == 0)
        {
          lines = g_strsplit (text + strlen ("MIME-TreeMagic") + 2, "\n", 0);
          match = NULL;
          for (i = 0; lines[i] && lines[i][0]; i++)
            {
              if (lines[i][0] == '[')
                {
                  match = parse_header (lines[i]);
                  insert_match (match);
                }
              else
                {
                  matchlet = parse_match_line (lines[i], &depth);
                  insert_matchlet (match, matchlet, depth);
                }
            }

          g_strfreev (lines);
        }
      else
        g_warning ("%s: header not found, skipping\n", filename);

      g_free (text);
    }

  g_free (filename);
}


static void
xdg_mime_reload (void *user_data)
{
  need_reload = TRUE;
}

static void
tree_magic_shutdown (void)
{
  g_list_foreach (tree_matches, (GFunc)tree_match_free, NULL);
  g_list_free (tree_matches);
  tree_matches = NULL;
}

static void
tree_magic_init (void)
{
  static gboolean initialized = FALSE;
  const gchar *dir;
  const gchar * const * dirs;
  int i;

  if (!initialized)
    {
      initialized = TRUE;

      xdg_mime_register_reload_callback (xdg_mime_reload, NULL, NULL);
      need_reload = TRUE;
    }

  if (need_reload)
    {
      need_reload = FALSE;

      tree_magic_shutdown ();

      dir = g_get_user_data_dir ();
      read_tree_magic_from_directory (dir);
      dirs = g_get_system_data_dirs ();
      for (i = 0; dirs[i]; i++)
        read_tree_magic_from_directory (dirs[i]);
    }
}

/* a filtering enumerator */

typedef struct
{
  gchar *path;
  gint depth;
  gboolean ignore_case;
  gchar **components;
  gchar **case_components;
  GFileEnumerator **enumerators;
  GFile **children;
} Enumerator;

static gboolean
component_match (Enumerator  *e,
                 gint         depth,
                 const gchar *name)
{
  gchar *case_folded, *key;
  gboolean found;

  if (strcmp (name, e->components[depth]) == 0)
    return TRUE;

  if (!e->ignore_case)
    return FALSE;

  case_folded = g_utf8_casefold (name, -1);
  key = g_utf8_collate_key (case_folded, -1);

  found = strcmp (key, e->case_components[depth]) == 0;

  g_free (case_folded);
  g_free (key);

  return found;
}

static GFile *
next_match_recurse (Enumerator *e,
                    gint        depth)
{
  GFile *file;
  GFileInfo *info;
  const gchar *name;

  while (TRUE)
    {
      if (e->enumerators[depth] == NULL)
        {
          if (depth > 0)
            {
              file = next_match_recurse (e, depth - 1);
              if (file)
                {
                  e->children[depth] = file;
                  e->enumerators[depth] = g_file_enumerate_children (file,
                                                                     G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                                     G_FILE_QUERY_INFO_NONE,
                                                                     NULL,
                                                                     NULL);
                }
            }
          if (e->enumerators[depth] == NULL)
            return NULL;
        }

      while ((info = g_file_enumerator_next_file (e->enumerators[depth], NULL, NULL)))
        {
          name = g_file_info_get_name (info);
          if (component_match (e, depth, name))
            {
              file = g_file_get_child (e->children[depth], name);
              g_object_unref (info);
              return file;
            }
          g_object_unref (info);
        }

      g_object_unref (e->enumerators[depth]);
      e->enumerators[depth] = NULL;
      g_object_unref (e->children[depth]);
      e->children[depth] = NULL;
    }
}

static GFile *
enumerator_next (Enumerator *e)
{
  return next_match_recurse (e, e->depth - 1);
}

static Enumerator *
enumerator_new (GFile      *root,
                const char *path,
                gboolean    ignore_case)
{
  Enumerator *e;
  gint i;
  gchar *case_folded;

  e = g_new0 (Enumerator, 1);
  e->path = g_strdup (path);
  e->ignore_case = ignore_case;

  e->components = g_strsplit (e->path, G_DIR_SEPARATOR_S, -1);
  e->depth = g_strv_length (e->components);
  if (e->ignore_case)
    {
      e->case_components = g_new0 (char *, e->depth + 1);
      for (i = 0; e->components[i]; i++)
        {
          case_folded = g_utf8_casefold (e->components[i], -1);
          e->case_components[i] = g_utf8_collate_key (case_folded, -1);
          g_free (case_folded);
        }
    }

  e->children = g_new0 (GFile *, e->depth);
  e->children[0] = g_object_ref (root);
  e->enumerators = g_new0 (GFileEnumerator *, e->depth);
  e->enumerators[0] = g_file_enumerate_children (root,
                                                 G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 NULL,
                                                 NULL);

  return e;
}

static void
enumerator_free (Enumerator *e)
{
  gint i;

  for (i = 0; i < e->depth; i++)
    {
      if (e->enumerators[i])
        g_object_unref (e->enumerators[i]);
      if (e->children[i])
        g_object_unref (e->children[i]);
    }

  g_free (e->enumerators);
  g_free (e->children);
  g_strfreev (e->components);
  if (e->case_components)
    g_strfreev (e->case_components);
  g_free (e->path);
  g_free (e);
}

static gboolean
matchlet_match (TreeMatchlet *matchlet,
                GFile        *root)
{
  GFile *file;
  GFileInfo *info;
  gboolean result;
  const gchar *attrs;
  Enumerator *e;
  GList *l;

  e = enumerator_new (root, matchlet->path, !matchlet->match_case);

  do
    {
      file = enumerator_next (e);
      if (!file)
        {
          enumerator_free (e);
          return FALSE;
        }

      if (matchlet->mimetype)
        attrs = G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE ","
                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE;
      else
        attrs = G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE;
      info = g_file_query_info (file,
                                attrs,
                                G_FILE_QUERY_INFO_NONE,
                                NULL,
                                NULL);
      if (info)
        {
          result = TRUE;

          if (matchlet->type != G_FILE_TYPE_UNKNOWN &&
              g_file_info_get_file_type (info) != matchlet->type)
            result = FALSE;

          if (matchlet->executable &&
              !g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE))
            result = FALSE;
        }
      else
        result = FALSE;

      if (result && matchlet->non_empty)
        {
          GFileEnumerator *child_enum;
          GFileInfo *child_info;

          child_enum = g_file_enumerate_children (file,
                                                  G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                  G_FILE_QUERY_INFO_NONE,
                                                  NULL,
                                                  NULL);

          if (child_enum)
            {
              child_info = g_file_enumerator_next_file (child_enum, NULL, NULL);
              if (child_info)
                g_object_unref (child_info);
              else
                result = FALSE;
              g_object_unref (child_enum);
            }
          else
            result = FALSE;
        }

      if (result && matchlet->mimetype)
        {
          if (strcmp (matchlet->mimetype, g_file_info_get_content_type (info)) != 0)
            result = FALSE;
        }

      g_object_unref (info);
      g_object_unref (file);
    }
  while (!result);

  enumerator_free (e);

  if (!matchlet->matches)
    return TRUE;

  for (l = matchlet->matches; l; l = l->next)
    {
      TreeMatchlet *submatchlet;

      submatchlet = l->data;
      if (matchlet_match (submatchlet, root))
        return TRUE;
    }

  return FALSE;
}

static void
match_match (TreeMatch    *match,
             GFile        *root,
             GPtrArray    *types)
{
  GList *l;

  for (l = match->matches; l; l = l->next)
    {
      TreeMatchlet *matchlet = l->data;
      if (matchlet_match (matchlet, root))
        {
          g_ptr_array_add (types, g_strdup (match->contenttype));
          break;
        }
    }
}

/**
 * g_content_type_guess_for_tree:
 * @root: the root of the tree to guess a type for
 *
 * Tries to guess the type of the tree with root @root, by
 * looking at the files it contains. The result is an array
 * of content types, with the best guess coming first.
 *
 * The types returned all have the form x-content/foo, e.g.
 * x-content/audio-cdda (for audio CDs) or x-content/image-dcf
 * (for a camera memory card). See the <ulink url="http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec">shared-mime-info</ulink>
 * specification for more on x-content types.
 *
 * This function is useful in the implementation of
 * g_mount_guess_content_type().
 *
 * Returns: an %NULL-terminated array of zero or more content types,
 *     or %NULL. Free with g_strfreev()
 *
 * Since: 2.18
 */
gchar **
g_content_type_guess_for_tree (GFile *root)
{
  GPtrArray *types;
  GList *l;

  types = g_ptr_array_new ();

  G_LOCK (gio_treemagic);

  tree_magic_init ();
  for (l = tree_matches; l; l = l->next)
    {
      TreeMatch *match = l->data;
      match_match (match, root, types);
    }

  G_UNLOCK (gio_treemagic);

  g_ptr_array_add (types, NULL);

  return (gchar **)g_ptr_array_free (types, FALSE);
}

#endif /* Unix version */
