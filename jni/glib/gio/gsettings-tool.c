/* GLIB - Library of useful routines for C programming
 * Copyright (C) 2010 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Matthias Clasen
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <gi18n.h>
#include <gio.h>

static gchar *
pick_word_at (const gchar  *s,
              gint          cursor,
              gint         *out_word_begins_at)
{
  gint begin;
  gint end;

  if (s[0] == '\0')
    {
      if (out_word_begins_at != NULL)
        *out_word_begins_at = -1;
      return NULL;
    }

  if (g_ascii_isspace (s[cursor]) &&
      ((cursor > 0 && g_ascii_isspace (s[cursor-1])) || cursor == 0))
    {
      if (out_word_begins_at != NULL)
        *out_word_begins_at = cursor;
      return g_strdup ("");
    }
  while (!g_ascii_isspace (s[cursor - 1]) && cursor > 0)
    cursor--;
  begin = cursor;

  end = begin;
  while (!g_ascii_isspace (s[end]) && s[end] != '\0')
    end++;

  if (out_word_begins_at != NULL)
    *out_word_begins_at = begin;

  return g_strndup (s + begin, end - begin);
}

static gint
usage (gint      *argc,
       gchar    **argv[],
       gboolean   use_stdout)
{
  GOptionContext *context;
  gchar *s;

  g_set_prgname (g_path_get_basename ((*argv)[0]));

  context = g_option_context_new (_("COMMAND"));
  g_option_context_set_help_enabled (context, FALSE);
  s = g_strdup_printf (
    _("Commands:\n"
      "  help        Show this information\n"
      "  get         Get the value of a key\n"
      "  set         Set the value of a key\n"
      "  reset       Reset the value of a key\n"
      "  monitor     Monitor a key for changes\n"
      "  writable    Check if a key is writable\n"
      "\n"
      "Use '%s COMMAND --help' to get help for individual commands.\n"),
      g_get_prgname ());
  g_option_context_set_description (context, s);
  g_free (s);
  s = g_option_context_get_help (context, FALSE, NULL);
  if (use_stdout)
    g_print ("%s", s);
  else
    g_printerr ("%s", s);
  g_free (s);
  g_option_context_free (context);

  return use_stdout ? 0 : 1;
}

static void
remove_arg (gint num, gint *argc, gchar **argv[])
{
  gint n;

  g_assert (num <= (*argc));

  for (n = num; (*argv)[n] != NULL; n++)
    (*argv)[n] = (*argv)[n+1];
  (*argv)[n] = NULL;
  (*argc) = (*argc) - 1;
}


static void
modify_argv0_for_command (gint         *argc,
                          gchar       **argv[],
                          const gchar  *command)
{
  gchar *s;

  g_assert (g_strcmp0 ((*argv)[1], command) == 0);
  remove_arg (1, argc, argv);

  s = g_strdup_printf ("%s %s", (*argv)[0], command);
  (*argv)[0] = s;
}

static gboolean
schema_exists (const gchar *name)
{
  const gchar * const *schemas;
  gint i;

  schemas = g_settings_list_schemas ();
  for (i = 0; schemas[i]; i++)
    if (g_strcmp0 (name, schemas[i]) == 0)
      return TRUE;

  return FALSE;
}

static void
list_schemas (const gchar *prefix)
{
  const gchar * const *schemas;
  gint i;

  schemas = g_settings_list_schemas ();
  for (i = 0; schemas[i]; i++)
    if (prefix == NULL || g_str_has_prefix (schemas[i], prefix))
      g_print ("%s \n", schemas[i]);
}

static gboolean
key_exists (GSettings   *settings,
            const gchar *name)
{
  gchar **keys;
  gint i;
  gboolean ret;

  ret = FALSE;

  keys = g_settings_list_keys (settings);
  for (i = 0; keys[i]; i++)
    if (g_strcmp0 (keys[i], name) == 0)
      {
        ret = TRUE;
        break;
      }
  g_strfreev (keys);

  return ret;
}

static void
list_keys (GSettings   *settings,
           const gchar *prefix)
{
  gchar **keys;
  gint i;

  keys = g_settings_list_keys (settings);
  for (i = 0; keys[i]; i++)
    {
      if (prefix == NULL || g_str_has_prefix (keys[i], prefix))
        g_print ("%s \n", keys[i]);
    }
  g_strfreev (keys);
}

static void
list_options (GOptionContext *context,
              const gchar    *prefix)
{
  /* FIXME extract options from context */
  const gchar *options[] = { "--help", "--path", NULL };
  gint i;
  for (i = 0; options[i]; i++)
    if (g_str_has_prefix (options[i], prefix))
      g_print ("%s \n", options[i]);
}

static gint
handle_get (gint      *argc,
            gchar    **argv[],
            gboolean   request_completion,
            gchar     *completion_cur,
            gchar     *completion_prev)
{
  gchar *schema;
  gchar *path;
  gchar *key;
  GSettings *settings;
  GVariant *v;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "path", 'p', 0, G_OPTION_ARG_STRING, &path, N_("Specify the path for the schema"), N_("PATH") },
    { NULL }
  };
  GError *error;
  gint ret = 1;

  modify_argv0_for_command (argc, argv, "get");

  context = g_option_context_new (_("SCHEMA KEY"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context, _("Get the value of KEY"));
  g_option_context_set_description (context,
    _("Arguments:\n"
      "  SCHEMA      The id of the schema\n"
      "  KEY         The name of the key\n"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  settings = NULL;
  path = NULL;
  schema = NULL;
  key = NULL;

  error = NULL;
  if (!g_option_context_parse (context, argc, argv, NULL))
    {
      if (!request_completion)
        {
          gchar *s;
          s = g_option_context_get_help (context, FALSE, NULL);
          g_printerr ("%s", s);
          g_free (s);

          goto out;
        }
    }

  if (*argc > 1)
    schema = (*argv)[1];
  if (*argc > 2)
    key = (*argv)[2];

  if (request_completion && completion_cur[0] == '-')
    {
      list_options (context, completion_cur);
      ret = 0;
      goto out;
    }

  if (request_completion && !schema_exists (schema))
    {
      list_schemas (schema);
      ret = 0;
      goto out;
    }

  if (path)
    settings = g_settings_new_with_path (schema, path);
  else
    settings = g_settings_new (schema);

  if (request_completion && !key_exists (settings, key))
    {
      list_keys (settings, key);
      ret = 0;
      goto out;
    }

  if (!request_completion)
    {
      v = g_settings_get_value (settings, key);
      g_print ("%s\n", g_variant_print (v, FALSE));
      g_variant_unref (v);
      ret = 0;
    }

 out:
  if (settings)
    g_object_unref (settings);

  g_option_context_free (context);

  return ret;
}

static gint
handle_set (gint      *argc,
            gchar    **argv[],
            gboolean   request_completion,
            gchar     *completion_cur,
            gchar     *completion_prev)
{
  gchar *schema;
  gchar *path;
  gchar *key;
  gchar *value;
  GSettings *settings;
  GVariant *v, *default_v;
  const GVariantType *type;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "path", 'p', 0, G_OPTION_ARG_STRING, &path, N_("Specify the path for the schema"), N_("PATH") },
    { NULL }
  };
  GError *error;
  gint ret = 1;

  modify_argv0_for_command (argc, argv, "set");

  /* Translators: Please keep order of words (command parameters) */
  context = g_option_context_new (_("SCHEMA KEY VALUE"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context, _("Set the value of KEY"));
  g_option_context_set_description (context,
    _("Arguments:\n"
      "  SCHEMA      The id of the schema\n"
      "  KEY         The name of the key\n"
      "  VALUE       The value to set key to, as a serialized GVariant\n"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  settings = NULL;
  path = NULL;
  schema = NULL;
  key = NULL;

  error = NULL;
  if (!g_option_context_parse (context, argc, argv, NULL))
    {
      if (!request_completion)
        {
          gchar *s;
          s = g_option_context_get_help (context, FALSE, NULL);
          g_printerr ("%s", s);
          g_free (s);
          goto out;
        }
    }

  if (*argc > 1)
    schema = (*argv)[1];
  if (*argc > 2)
    key = (*argv)[2];
  if (*argc > 3)
    value = (*argv)[3];

  if (request_completion && completion_cur[0] == '-')
    {
      list_options (context, completion_cur);
      ret = 0;
      goto out;
    }

  if (request_completion && !schema_exists (schema))
    {
      list_schemas (schema);
      ret = 0;
      goto out;
    }

  if (path)
    settings = g_settings_new_with_path (schema, path);
  else
    settings = g_settings_new (schema);

  if (request_completion && !key_exists (settings, key))
    {
      list_keys (settings, key);
      ret = 0;
      goto out;
    }

  if (!request_completion)
    {
      default_v = g_settings_get_value (settings, key);
      type = g_variant_get_type (default_v);

      error = NULL;
      v = g_variant_parse (type, value, NULL, NULL, &error);
      g_variant_unref (default_v);
      if (v == NULL)
        {
          g_printerr ("%s\n", error->message);
          goto out;
        }

      if (!g_settings_set_value (settings, key, v))
        {
          g_printerr (_("Key %s is not writable\n"), key);
          goto out;
        }

      g_settings_sync ();
      ret = 0;
    }

 out:
  if (settings)
    g_object_unref (settings);

  g_option_context_free (context);

  return ret;
}


static gint
handle_reset (gint      *argc,
              gchar    **argv[],
              gboolean   request_completion,
              gchar     *completion_cur,
              gchar     *completion_prev)
{
  gchar *schema;
  gchar *path;
  gchar *key;
  GSettings *settings;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "path", 'p', 0, G_OPTION_ARG_STRING, &path, N_("Specify the path for the schema"), N_("PATH") },
    { NULL }
  };
  GError *error;
  gint ret = 1;

  modify_argv0_for_command (argc, argv, "reset");

  context = g_option_context_new (_("SCHEMA KEY VALUE"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context, _("Sets KEY to its default value"));
  g_option_context_set_description (context,
    _("Arguments:\n"
      "  SCHEMA      The id of the schema\n"
      "  KEY         The name of the key\n"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  settings = NULL;
  path = NULL;
  schema = NULL;
  key = NULL;

  error = NULL;
  if (!g_option_context_parse (context, argc, argv, NULL))
    {
      if (!request_completion)
        {
          gchar *s;
          s = g_option_context_get_help (context, FALSE, NULL);
          g_printerr ("%s", s);
          g_free (s);
          goto out;
        }
    }

  if (*argc > 1)
    schema = (*argv)[1];
  if (*argc > 2)
    key = (*argv)[2];

  if (request_completion && completion_cur[0] == '-')
    {
      list_options (context, completion_cur);
      ret = 0;
      goto out;
    }

  if (request_completion && !schema_exists (schema))
    {
      list_schemas (schema);
      ret = 0;
      goto out;
    }

  if (path)
    settings = g_settings_new_with_path (schema, path);
  else
    settings = g_settings_new (schema);

  if (request_completion && !key_exists (settings, key))
    {
      list_keys (settings, key);
      ret = 0;
      goto out;
    }

  if (!request_completion)
    {
      g_settings_reset (settings, key);
      g_settings_sync ();
      ret = 0;
    }

 out:
  if (settings)
    g_object_unref (settings);

  g_option_context_free (context);

  return ret;
}

static gint
handle_writable (gint   *argc,
                 gchar **argv[],
                 gboolean   request_completion,
                 gchar     *completion_cur,
                 gchar     *completion_prev)
{
  gchar *schema;
  gchar *path;
  gchar *key;
  GSettings *settings;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "path", 'p', 0, G_OPTION_ARG_STRING, &path, N_("Specify the path for the schema"), N_("PATH") },
    { NULL }
  };
  GError *error;
  gint ret = 1;

  modify_argv0_for_command (argc, argv, "writable");

  /* Translators: Please keep order of words (command parameters) */
  context = g_option_context_new (_("SCHEMA KEY"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context, _("Find out whether KEY is writable"));
  g_option_context_set_description (context,
    _("Arguments:\n"
      "  SCHEMA      The id of the schema\n"
      "  KEY         The name of the key\n"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  settings = NULL;
  path = NULL;
  schema = NULL;
  key = NULL;

  error = NULL;
  if (!g_option_context_parse (context, argc, argv, NULL))
    {
      if (!request_completion)
        {
          gchar *s;
          s = g_option_context_get_help (context, FALSE, NULL);
          g_printerr ("%s", s);
          g_free (s);
          goto out;
        }
    }

  if (*argc > 1)
    schema = (*argv)[1];
  if (*argc > 2)
    key = (*argv)[2];

  if (request_completion && completion_cur[0] == '-')
    {
      list_options (context, completion_cur);
      ret = 0;
      goto out;
    }

  if (request_completion && !schema_exists (schema))
    {
      list_schemas (schema);
      ret = 0;
      goto out;
    }

  if (path)
    settings = g_settings_new_with_path (schema, path);
  else
    settings = g_settings_new (schema);

  if (request_completion && !key_exists (settings, key))
    {
      list_keys (settings, key);
      ret = 0;
      goto out;
    }

  if (!request_completion)
    {
      if (g_settings_is_writable (settings, key))
        g_print ("true\n");
      else
        g_print ("false\n");
      ret = 0;
    }

 out:
  if (settings)
    g_object_unref (settings);

  g_option_context_free (context);

  return ret;
}

static void
key_changed (GSettings   *settings,
             const gchar *key)
{
  GVariant *v;
  gchar *value;

  v = g_settings_get_value (settings, key);
  value = g_variant_print (v, FALSE);
  g_print ("%s\n", value);
  g_free (value);
  g_variant_unref (v);
}

static gint
handle_monitor (gint      *argc,
                gchar    **argv[],
                gboolean   request_completion,
                gchar     *completion_cur,
                gchar     *completion_prev)
{
  gchar *schema;
  gchar *path;
  gchar *key;
  GSettings *settings;
  gchar *detailed_signal;
  GMainLoop *loop;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "path", 'p', 0, G_OPTION_ARG_STRING, &path, N_("Specify the path for the schema"), N_("PATH") },
    { NULL }
  };
  GError *error;
  gint ret = 1;

  modify_argv0_for_command (argc, argv, "monitor");

  context = g_option_context_new (_("SCHEMA KEY"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
    _("Monitor KEY for changes and print the changed values.\n"
      "Monitoring will continue until the process is terminated."));

  g_option_context_set_description (context,
    _("Arguments:\n"
      "  SCHEMA      The id of the schema\n"
      "  KEY         The name of the key\n"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  settings = NULL;
  path = NULL;
  schema = NULL;
  key = NULL;

  error = NULL;
  if (!g_option_context_parse (context, argc, argv, NULL))
    {
      if (!request_completion)
        {
          gchar *s;
          s = g_option_context_get_help (context, FALSE, NULL);
          g_printerr ("%s", s);
          g_free (s);
          goto out;
        }
    }

  if (*argc > 1)
    schema = (*argv)[1];
  if (*argc > 2)
    key = (*argv)[2];

  if (request_completion && completion_cur[0] == '-')
    {
      list_options (context, completion_cur);
      ret = 0;
      goto out;
    }

  if (request_completion && !schema_exists (schema))
    {
      list_schemas (schema);
      ret = 0;
      goto out;
    }

  if (path)
    settings = g_settings_new_with_path (schema, path);
  else
    settings = g_settings_new (schema);

  if (request_completion && !key_exists (settings, key))
    {
      list_keys (settings, key);
      ret = 0;
      goto out;
    }

  if (!request_completion)
    {
      detailed_signal = g_strdup_printf ("changed::%s", key);
      g_signal_connect (settings, detailed_signal,
                        G_CALLBACK (key_changed), NULL);

      loop = g_main_loop_new (NULL, FALSE);
      g_main_loop_run (loop);
      g_main_loop_unref (loop);
      ret = 0;
    }

 out:
  if (settings)
    g_object_unref (settings);

  g_option_context_free (context);

  return ret;
}

int
main (int argc, char *argv[])
{
  gboolean ret;
  gchar *command;
  gboolean request_completion;
  gchar *completion_cur;
  gchar *completion_prev;

  setlocale (LC_ALL, "");

  g_type_init ();

  ret = 1;
  completion_cur = NULL;
  completion_prev = NULL;
  request_completion = FALSE;

  if (argc < 2)
    {
      ret = usage (&argc, &argv, FALSE);
      goto out;
    }

 again:
  command = argv[1];

  if (g_strcmp0 (command, "help") == 0)
    {
      if (!request_completion)
        ret = usage (&argc, &argv, TRUE);
    }
  else if (g_strcmp0 (command, "get") == 0)
    ret = handle_get (&argc, &argv, request_completion, completion_cur, completion_prev);
  else if (g_strcmp0 (command, "set") == 0)
    ret = handle_set (&argc, &argv, request_completion, completion_cur, completion_prev);
  else if (g_strcmp0 (command, "reset") == 0)
    ret = handle_reset (&argc, &argv, request_completion, completion_cur, completion_prev);
  else if (g_strcmp0 (command, "monitor") == 0)
    ret = handle_monitor (&argc, &argv, request_completion, completion_cur, completion_prev);
  else if (g_strcmp0 (command, "writable") == 0)
    ret = handle_writable (&argc, &argv, request_completion, completion_cur, completion_prev);
  else if (g_strcmp0 (command, "complete") == 0 && argc == 4 && !request_completion)
    {
      gchar *completion_line;
      gint completion_point;
      gchar *endp;
      gchar **completion_argv;
      gint completion_argc;
      gint cur_begin;

      request_completion = TRUE;

      completion_line = argv[2];
      completion_point = strtol (argv[3], &endp, 10);
      if (endp == argv[3] || *endp != '\0')
        goto out;

      if (!g_shell_parse_argv (completion_line,
                               &completion_argc,
                               &completion_argv,
                               NULL))
        {
          /* can't parse partical cmdline, don't attempt completion */
          goto out;
        }

      completion_prev = NULL;
      completion_cur = pick_word_at (completion_line, completion_point, &cur_begin);
      if (cur_begin > 0)
        {
          gint prev_end;
          for (prev_end = cur_begin - 1; prev_end >= 0; prev_end--)
            {
              if (!g_ascii_isspace (completion_line[prev_end]))
                {
                  completion_prev = pick_word_at (completion_line, prev_end, NULL);
                  break;
                }
            }
        }

      argc = completion_argc;
      argv = completion_argv;

      ret = 0;
      goto again;
    }
  else
    {
      if (request_completion)
        {
          g_print ("help \nget \nmonitor \nwritable \nset \nreset \n");
          ret = 0;
        }
      else
        {
          g_printerr (_("Unknown command '%s'\n"), argv[1]);
          ret = usage (&argc, &argv, FALSE);
        }
    }

 out:
  g_free (completion_cur);
  g_free (completion_prev);

  return ret;
}
