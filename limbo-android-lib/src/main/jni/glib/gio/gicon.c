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
#include <stdlib.h>
#include <string.h>

#include "gicon.h"
#include "gthemedicon.h"
#include "gfileicon.h"
#include "gemblemedicon.h"
#include "gfile.h"
#include "gioerror.h"

#include "glibintl.h"


/* There versioning of this is implicit, version 1 would be ".1 " */
#define G_ICON_SERIALIZATION_MAGIC0 ". "

/**
 * SECTION:gicon
 * @short_description: Interface for icons
 * @include: gio/gio.h
 *
 * #GIcon is a very minimal interface for icons. It provides functions
 * for checking the equality of two icons, hashing of icons and
 * serializing an icon to and from strings.
 *
 * #GIcon does not provide the actual pixmap for the icon as this is out 
 * of GIO's scope, however implementations of #GIcon may contain the name 
 * of an icon (see #GThemedIcon), or the path to an icon (see #GLoadableIcon). 
 * 
 * To obtain a hash of a #GIcon, see g_icon_hash().
 * 
 * To check if two #GIcons are equal, see g_icon_equal().
 *
 * For serializing a #GIcon, use g_icon_to_string() and
 * g_icon_new_for_string().
 *
 * If your application or library provides one or more #GIcon
 * implementations you need to ensure that each #GType is registered
 * with the type system prior to calling g_icon_new_for_string().
 **/

typedef GIconIface GIconInterface;
G_DEFINE_INTERFACE(GIcon, g_icon, G_TYPE_OBJECT)

static void
g_icon_default_init (GIconInterface *iface)
{
}

/**
 * g_icon_hash:
 * @icon: #gconstpointer to an icon object.
 * 
 * Gets a hash for an icon.
 * 
 * Returns: a #guint containing a hash for the @icon, suitable for 
 * use in a #GHashTable or similar data structure.
 **/
guint
g_icon_hash (gconstpointer icon)
{
  GIconIface *iface;

  g_return_val_if_fail (G_IS_ICON (icon), 0);

  iface = G_ICON_GET_IFACE (icon);

  return (* iface->hash) ((GIcon *)icon);
}

/**
 * g_icon_equal:
 * @icon1: pointer to the first #GIcon.
 * @icon2: pointer to the second #GIcon.
 * 
 * Checks if two icons are equal.
 * 
 * Returns: %TRUE if @icon1 is equal to @icon2. %FALSE otherwise.
 **/
gboolean
g_icon_equal (GIcon *icon1,
	      GIcon *icon2)
{
  GIconIface *iface;

  if (icon1 == NULL && icon2 == NULL)
    return TRUE;

  if (icon1 == NULL || icon2 == NULL)
    return FALSE;
  
  if (G_TYPE_FROM_INSTANCE (icon1) != G_TYPE_FROM_INSTANCE (icon2))
    return FALSE;

  iface = G_ICON_GET_IFACE (icon1);
  
  return (* iface->equal) (icon1, icon2);
}

static gboolean
g_icon_to_string_tokenized (GIcon *icon, GString *s)
{
  char *ret;
  GPtrArray *tokens;
  gint version;
  GIconIface *icon_iface;
  int i;

  g_return_val_if_fail (icon != NULL, FALSE);
  g_return_val_if_fail (G_IS_ICON (icon), FALSE);

  ret = NULL;

  icon_iface = G_ICON_GET_IFACE (icon);
  if (icon_iface->to_tokens == NULL)
    return FALSE;

  tokens = g_ptr_array_new ();
  if (!icon_iface->to_tokens (icon, tokens, &version))
    {
      g_ptr_array_free (tokens, TRUE);
      return FALSE;
    }

  /* format: TypeName[.Version] <token_0> .. <token_N-1>
     version 0 is implicit and can be omitted
     all the tokens are url escaped to ensure they have no spaces in them */
  
  g_string_append (s, g_type_name_from_instance ((GTypeInstance *)icon));
  if (version != 0)
    g_string_append_printf (s, ".%d", version);
  
  for (i = 0; i < tokens->len; i++)
    {
      char *token;

      token = g_ptr_array_index (tokens, i);

      g_string_append_c (s, ' ');
      /* We really only need to escape spaces here, so allow lots of otherwise reserved chars */
      g_string_append_uri_escaped (s, token,
				   G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);

      g_free (token);
    }
  
  g_ptr_array_free (tokens, TRUE);
  
  return TRUE;
}

/**
 * g_icon_to_string:
 * @icon: a #GIcon.
 *
 * Generates a textual representation of @icon that can be used for
 * serialization such as when passing @icon to a different process or
 * saving it to persistent storage. Use g_icon_new_for_string() to
 * get @icon back from the returned string.
 *
 * The encoding of the returned string is proprietary to #GIcon except
 * in the following two cases
 *
 * <itemizedlist>
 * <listitem><para>
 *     If @icon is a #GFileIcon, the returned string is a native path
 *     (such as <literal>/path/to/my icon.png</literal>) without escaping
 *     if the #GFile for @icon is a native file.  If the file is not
 *     native, the returned string is the result of g_file_get_uri()
 *     (such as <literal>sftp://path/to/my%%20icon.png</literal>).
 * </para></listitem>
 * <listitem><para>
 *    If @icon is a #GThemedIcon with exactly one name, the encoding is
 *    simply the name (such as <literal>network-server</literal>).
 * </para></listitem>
 * </itemizedlist>
 *
 * Returns: An allocated NUL-terminated UTF8 string or %NULL if @icon can't
 * be serialized. Use g_free() to free.
 *
 * Since: 2.20
 */
gchar *
g_icon_to_string (GIcon *icon)
{
  gchar *ret;

  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (G_IS_ICON (icon), NULL);

  ret = NULL;

  if (G_IS_FILE_ICON (icon))
    {
      GFile *file;

      file = g_file_icon_get_file (G_FILE_ICON (icon));
      if (g_file_is_native (file))
	{
	  ret = g_file_get_path (file);
	  if (!g_utf8_validate (ret, -1, NULL))
	    {
	      g_free (ret);
	      ret = NULL;
	    }
	}
      else
        ret = g_file_get_uri (file);
    }
  else if (G_IS_THEMED_ICON (icon))
    {
      const char * const *names;

      names = g_themed_icon_get_names (G_THEMED_ICON (icon));
      if (names != NULL &&
	  names[0] != NULL &&
	  names[0][0] != '.' && /* Allowing icons starting with dot would break G_ICON_SERIALIZATION_MAGIC0 */
	  g_utf8_validate (names[0], -1, NULL) && /* Only return utf8 strings */
	  names[1] == NULL)
	ret = g_strdup (names[0]);
    }

  if (ret == NULL)
    {
      GString *s;

      s = g_string_new (G_ICON_SERIALIZATION_MAGIC0);

      if (g_icon_to_string_tokenized (icon, s))
	ret = g_string_free (s, FALSE);
      else
	g_string_free (s, TRUE);
    }

  return ret;
}

static GIcon *
g_icon_new_from_tokens (char   **tokens,
			GError **error)
{
  GIcon *icon;
  char *typename, *version_str;
  GType type;
  gpointer klass;
  GIconIface *icon_iface;
  gint version;
  char *endp;
  int num_tokens;
  int i;

  icon = NULL;
  klass = NULL;

  num_tokens = g_strv_length (tokens);

  if (num_tokens < 1)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Wrong number of tokens (%d)"),
                   num_tokens);
      goto out;
    }
  
  typename = tokens[0];
  version_str = strchr (typename, '.');
  if (version_str)
    {
      *version_str = 0;
      version_str += 1;
    }
  
  
  type = g_type_from_name (tokens[0]);
  if (type == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("No type for class name %s"),
                   tokens[0]);
      goto out;
    }

  if (!g_type_is_a (type, G_TYPE_ICON))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s does not implement the GIcon interface"),
                   tokens[0]);
      goto out;
    }

  klass = g_type_class_ref (type);
  if (klass == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s is not classed"),
                   tokens[0]);
      goto out;
    }

  version = 0;
  if (version_str)
    {
      version = strtol (version_str, &endp, 10);
      if (endp == NULL || *endp != '\0')
	{
	  g_set_error (error,
		       G_IO_ERROR,
		       G_IO_ERROR_INVALID_ARGUMENT,
		       _("Malformed version number: %s"),
		       version_str);
	  goto out;
	}
    }

  icon_iface = g_type_interface_peek (klass, G_TYPE_ICON);
  g_assert (icon_iface != NULL);

  if (icon_iface->from_tokens == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s does not implement from_tokens() on the GIcon interface"),
                   tokens[0]);
      goto out;
    }

  for (i = 1;  i < num_tokens; i++)
    {
      char *escaped;

      escaped = tokens[i];
      tokens[i] = g_uri_unescape_string (escaped, NULL);
      g_free (escaped);
    }
  
  icon = icon_iface->from_tokens (tokens + 1, num_tokens - 1, version, error);

 out:
  if (klass != NULL)
    g_type_class_unref (klass);
  return icon;
}

static void
ensure_builtin_icon_types (void)
{
  static volatile GType t;
  t = g_themed_icon_get_type ();
  t = g_file_icon_get_type ();
  t = g_emblemed_icon_get_type ();
  t = g_emblem_get_type ();
}

/**
 * g_icon_new_for_string:
 * @str: A string obtained via g_icon_to_string().
 * @error: Return location for error.
 *
 * Generate a #GIcon instance from @str. This function can fail if
 * @str is not valid - see g_icon_to_string() for discussion.
 *
 * If your application or library provides one or more #GIcon
 * implementations you need to ensure that each #GType is registered
 * with the type system prior to calling g_icon_new_for_string().
 *
 * Returns: An object implementing the #GIcon interface or %NULL if
 * @error is set.
 *
 * Since: 2.20
 **/
GIcon *
g_icon_new_for_string (const gchar   *str,
                       GError       **error)
{
  GIcon *icon;

  g_return_val_if_fail (str != NULL, NULL);

  ensure_builtin_icon_types ();

  icon = NULL;

  if (*str == '.')
    {
      if (g_str_has_prefix (str, G_ICON_SERIALIZATION_MAGIC0))
	{
	  gchar **tokens;
	  
	  /* handle tokenized encoding */
	  tokens = g_strsplit (str + sizeof (G_ICON_SERIALIZATION_MAGIC0) - 1, " ", 0);
	  icon = g_icon_new_from_tokens (tokens, error);
	  g_strfreev (tokens);
	}
      else
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_INVALID_ARGUMENT,
			     _("Can't handle the supplied version the icon encoding"));
    }
  else
    {
      gchar *scheme;

      /* handle special GFileIcon and GThemedIcon cases */
      scheme = g_uri_parse_scheme (str);
      if (scheme != NULL || str[0] == '/')
        {
          GFile *location;
          location = g_file_new_for_commandline_arg (str);
          icon = g_file_icon_new (location);
          g_object_unref (location);
        }
      else
	icon = g_themed_icon_new (str);
      g_free (scheme);
    }

  return icon;
}
