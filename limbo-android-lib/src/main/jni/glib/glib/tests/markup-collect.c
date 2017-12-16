/* 
 * Copyright © 2007 Ryan Lortie
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * See the included COPYING file for more information.
 */

#include <string.h>
#include <glib.h>

static void
start (GMarkupParseContext  *context,
       const char           *element_name,
       const char          **attribute_names,
       const char          **attribute_values,
       gpointer              user_data,
       GError              **error)
{
  GString *string = user_data;
  gboolean result;

#define collect(...) \
  g_markup_collect_attributes (element_name, attribute_names, \
                               attribute_values, error, __VA_ARGS__, \
                               G_MARKUP_COLLECT_INVALID)
#define BOOL    G_MARKUP_COLLECT_BOOLEAN
#define OPTBOOL G_MARKUP_COLLECT_BOOLEAN | G_MARKUP_COLLECT_OPTIONAL
#define TRI     G_MARKUP_COLLECT_TRISTATE
#define STR     G_MARKUP_COLLECT_STRING
#define STRDUP  G_MARKUP_COLLECT_STRDUP
#define OPTSTR  G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL
#define OPTDUP  G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL
#define n(x)    ((x)?(x):"(null)")

  if (strcmp (element_name, "bool") == 0)
    {
      gboolean mb = 2, ob = 2, tri = 2;

      result = collect (BOOL,    "mb", &mb,
                        OPTBOOL, "ob", &ob,
                        TRI,     "tri", &tri);

      g_assert (result ||
                (mb == FALSE && ob == FALSE && tri != TRUE && tri != FALSE));

      if (tri != FALSE && tri != TRUE)
        tri = -1;

      g_string_append_printf (string, "<bool(%d) %d %d %d>",
                              result, mb, ob, tri);
    }

  else if (strcmp (element_name, "str") == 0)
    {
      const char *cm, *co;
      char *am, *ao;

      result = collect (STR,    "cm", &cm,
                        STRDUP, "am", &am,
                        OPTDUP, "ao", &ao,
                        OPTSTR, "co", &co);

      g_assert (result ||
                (cm == NULL && am == NULL && ao == NULL && co == NULL));

      g_string_append_printf (string, "<str(%d) %s %s %s %s>",
                              result, n (cm), n (am), n (ao), n (co));

      g_free (am);
      g_free (ao);
    }
}

static GMarkupParser parser = { start };

struct test
{
  const char   *document;
  const char   *result;
  GMarkupError  error_code;
  const char   *error_info;
};

static struct test tests[] =
{
  { "<bool mb='y'>", "<bool(1) 1 0 -1>",
    G_MARKUP_ERROR_PARSE, "'bool'" },

  { "<bool mb='false'/>", "<bool(1) 0 0 -1>" },
  { "<bool mb='true'/>", "<bool(1) 1 0 -1>" },
  { "<bool mb='t' ob='f' tri='1'/>", "<bool(1) 1 0 1>" },
  { "<bool mb='y' ob='n' tri='0'/>", "<bool(1) 1 0 0>" },

  { "<bool ob='y'/>", "<bool(0) 0 0 -1>",
    G_MARKUP_ERROR_MISSING_ATTRIBUTE, "'mb'" },

  { "<bool mb='y' mb='y'/>", "<bool(0) 0 0 -1>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'mb'" },

  { "<bool mb='y' tri='y' tri='n'/>", "<bool(0) 0 0 -1>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'tri'" },

  { "<str cm='x' am='y'/>", "<str(1) x y (null) (null)>" },

  { "<str am='x' co='y'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_MISSING_ATTRIBUTE, "'cm'" },

  { "<str am='x'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_MISSING_ATTRIBUTE, "'cm'" },

  { "<str am='x' cm='x' am='y'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'am'" },

  { "<str am='x' qm='y' cm='x'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, "'qm'" },

  { "<str am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' cm='x'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'am'" },

  { "<str cm='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x' am='x'/>", "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'am'" },

  { "<str a='x' b='x' c='x' d='x' e='x' f='x' g='x' h='x' i='x' j='x' k='x' l='x' m='x' n='x' o='x' p='x' q='x' r='x' s='x' t='x' u='x' v='x' w='x' x='x' y='x' z='x' aa='x' bb='x' cc='x' dd='x' ee='x' ff='x' gg='x' hh='x' ii='x' jj='x' kk='x' ll='x' mm='x' nn='x' oo='x' pp='x' qq='x' rr='x' ss='x' tt='x' uu='x' vv='x' ww='x' xx='x' yy='x' zz='x' am='x' cm='x'/>",
    "<str(0) (null) (null) (null) (null)>",
    G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, "'a'" },

  { "<bool mb='ja'/>", "<bool(0) 0 0 -1>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'mb'" },

  { "<bool mb='nein'/>", "<bool(0) 0 0 -1>",
    G_MARKUP_ERROR_INVALID_CONTENT, "'mb'" }
};

static void
test_collect (gconstpointer d)
{
  const struct test *test = d;

  GMarkupParseContext *ctx;
  GError *error = NULL;
  GString *string;
  gboolean result;

  string = g_string_new ("");
  ctx = g_markup_parse_context_new (&parser, 0, string, NULL);
  result = g_markup_parse_context_parse (ctx,
                                         test->document,
                                         -1, &error);
  if (result)
    result = g_markup_parse_context_end_parse (ctx, &error);

  if (result)
    {
      g_assert_no_error (error);
      g_assert_cmpint (test->error_code, ==, 0);
      g_assert_cmpstr (test->result, ==, string->str);
    }
  else
    {
      g_assert_error (error, G_MARKUP_ERROR, test->error_code);
    }

  g_markup_parse_context_free (ctx);
  g_string_free (string, TRUE);
  g_clear_error (&error);
}

int
main (int argc, char **argv)
{
  int i;
  gchar *path;

  g_test_init (&argc, &argv, NULL);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      path = g_strdup_printf ("/markup/collect/%d", i);
      g_test_add_data_func (path, &tests[i], test_collect);
      g_free (path);
    }

  return g_test_run ();
}
