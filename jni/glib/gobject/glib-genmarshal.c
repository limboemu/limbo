/* GLIB-GenMarshal - Marshaller generator for GObject library
 * Copyright (C) 2000-2001 Red Hat, Inc.
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
 */

#include "config.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GLib-Genmarshal"
#include <glib.h>
#include <glib/gprintf.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

/* --- defines --- */
#define	PRG_NAME	"glib-genmarshal"
#define	PKG_NAME	"GLib"
#define PKG_HTTP_HOME	"http://www.gtk.org"


/* --- typedefs & structures --- */
typedef struct
{
  gchar	      *keyword;		/* marhaller list keyword [MY_STRING] */
  const gchar *sig_name;	/* signature name [STRING] */
  const gchar *ctype;		/* C type name [gchar*] */
  const gchar *getter;		/* value getter function [g_value_get_string] */
} InArgument;
typedef struct
{
  gchar	      *keyword;		/* marhaller list keyword [MY_STRING] */
  const gchar *sig_name;	/* signature name [STRING] */
  const gchar *ctype;		/* C type name [gchar*] */
  const gchar *setter;		/* value setter function [g_value_set_string] */
} OutArgument;
typedef struct
{
  gchar       *ploc;
  OutArgument *rarg;
  GList       *args;	/* of type InArgument* */
} Signature;


/* --- prototypes --- */
static void	parse_args	(gint	   	*argc_p,
				 gchar	     ***argv_p);
static void	print_blurb	(FILE	       	*bout,
				 gboolean	 print_help);


/* --- variables --- */
static const GScannerConfig scanner_config_template =
{
  (
   " \t\r"		/* "\n" is statement delimiter */
   )                    /* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_0123456789"
   G_CSET_A_2_Z
   )                    /* cset_identifier_nth */,
  ( "#\n" )             /* cpair_comment_single */,

  FALSE                 /* case_sensitive */,

  TRUE                  /* skip_comment_multi */,
  TRUE                  /* skip_comment_single */,
  TRUE                  /* scan_comment_multi */,
  TRUE                  /* scan_identifier */,
  FALSE                 /* scan_identifier_1char */,
  FALSE                 /* scan_identifier_NULL */,
  TRUE                  /* scan_symbols */,
  FALSE                 /* scan_binary */,
  TRUE                  /* scan_octal */,
  TRUE                  /* scan_float */,
  TRUE                  /* scan_hex */,
  FALSE                 /* scan_hex_dollar */,
  TRUE                  /* scan_string_sq */,
  TRUE                  /* scan_string_dq */,
  TRUE                  /* numbers_2_int */,
  FALSE                 /* int_2_float */,
  FALSE                 /* identifier_2_string */,
  TRUE                  /* char_2_token */,
  FALSE                 /* symbol_2_token */,
  FALSE                 /* scope_0_fallback */,
};
static gchar		* const std_marshaller_prefix = "g_cclosure_marshal";
static gchar		*marshaller_prefix = "g_cclosure_user_marshal";
static GHashTable	*marshallers = NULL;
static FILE             *fout = NULL;
static gboolean		 gen_cheader = FALSE;
static gboolean		 gen_cbody = FALSE;
static gboolean          gen_internal = FALSE;
static gboolean		 skip_ploc = FALSE;
static gboolean		 std_includes = TRUE;
static gint              exit_status = 0;


/* --- functions --- */
static void
put_marshal_value_getters (void)
{
  fputs ("\n", fout);
  fputs ("#ifdef G_ENABLE_DEBUG\n", fout);
  fputs ("#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)\n", fout);
  fputs ("#define g_marshal_value_peek_char(v)     g_value_get_char (v)\n", fout);
  fputs ("#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)\n", fout);
  fputs ("#define g_marshal_value_peek_int(v)      g_value_get_int (v)\n", fout);
  fputs ("#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)\n", fout);
  fputs ("#define g_marshal_value_peek_long(v)     g_value_get_long (v)\n", fout);
  fputs ("#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)\n", fout);
  fputs ("#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)\n", fout);
  fputs ("#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)\n", fout);
  fputs ("#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)\n", fout);
  fputs ("#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)\n", fout);
  fputs ("#define g_marshal_value_peek_float(v)    g_value_get_float (v)\n", fout);
  fputs ("#define g_marshal_value_peek_double(v)   g_value_get_double (v)\n", fout);
  fputs ("#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)\n", fout);
  fputs ("#define g_marshal_value_peek_param(v)    g_value_get_param (v)\n", fout);
  fputs ("#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)\n", fout);
  fputs ("#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)\n", fout);
  fputs ("#define g_marshal_value_peek_object(v)   g_value_get_object (v)\n", fout);
  fputs ("#define g_marshal_value_peek_variant(v)  g_value_get_variant (v)\n", fout);
  fputs ("#else /* !G_ENABLE_DEBUG */\n", fout);
  fputs ("/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.\n", fout);
  fputs (" *          Do not access GValues directly in your code. Instead, use the\n", fout);
  fputs (" *          g_value_get_*() functions\n", fout);
  fputs (" */\n", fout);
  fputs ("#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int\n", fout);
  fputs ("#define g_marshal_value_peek_char(v)     (v)->data[0].v_int\n", fout);
  fputs ("#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint\n", fout);
  fputs ("#define g_marshal_value_peek_int(v)      (v)->data[0].v_int\n", fout);
  fputs ("#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint\n", fout);
  fputs ("#define g_marshal_value_peek_long(v)     (v)->data[0].v_long\n", fout);
  fputs ("#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong\n", fout);
  fputs ("#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64\n", fout);
  fputs ("#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64\n", fout);
  fputs ("#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long\n", fout);
  fputs ("#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong\n", fout);
  fputs ("#define g_marshal_value_peek_float(v)    (v)->data[0].v_float\n", fout);
  fputs ("#define g_marshal_value_peek_double(v)   (v)->data[0].v_double\n", fout);
  fputs ("#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer\n", fout);
  fputs ("#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer\n", fout);
  fputs ("#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer\n", fout);
  fputs ("#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer\n", fout);
  fputs ("#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer\n", fout);
  fputs ("#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer\n", fout);
  fputs ("#endif /* !G_ENABLE_DEBUG */\n", fout);
  fputs ("\n", fout);
}

static gboolean
complete_in_arg (InArgument *iarg)
{
  static const InArgument args[] = {
    /* keyword		sig_name	ctype		getter			*/
    { "VOID",		"VOID",		"void",		NULL,			},
    { "BOOLEAN",	"BOOLEAN",	"gboolean",	"g_marshal_value_peek_boolean",	},
    { "CHAR",		"CHAR",		"gchar",	"g_marshal_value_peek_char",	},
    { "UCHAR",		"UCHAR",	"guchar",	"g_marshal_value_peek_uchar",	},
    { "INT",		"INT",		"gint",		"g_marshal_value_peek_int",	},
    { "UINT",		"UINT",		"guint",	"g_marshal_value_peek_uint",	},
    { "LONG",		"LONG",		"glong",	"g_marshal_value_peek_long",	},
    { "ULONG",		"ULONG",	"gulong",	"g_marshal_value_peek_ulong",	},
    { "INT64",		"INT64",	"gint64",       "g_marshal_value_peek_int64",	},
    { "UINT64",		"UINT64",	"guint64",	"g_marshal_value_peek_uint64",	},
    { "ENUM",		"ENUM",		"gint",		"g_marshal_value_peek_enum",	},
    { "FLAGS",		"FLAGS",	"guint",	"g_marshal_value_peek_flags",	},
    { "FLOAT",		"FLOAT",	"gfloat",	"g_marshal_value_peek_float",	},
    { "DOUBLE",		"DOUBLE",	"gdouble",	"g_marshal_value_peek_double",	},
    { "STRING",		"STRING",	"gpointer",	"g_marshal_value_peek_string",	},
    { "PARAM",		"PARAM",	"gpointer",	"g_marshal_value_peek_param",	},
    { "BOXED",		"BOXED",	"gpointer",	"g_marshal_value_peek_boxed",	},
    { "POINTER",	"POINTER",	"gpointer",	"g_marshal_value_peek_pointer",	},
    { "OBJECT",		"OBJECT",	"gpointer",	"g_marshal_value_peek_object",	},
    { "VARIANT",	"VARIANT",	"gpointer",	"g_marshal_value_peek_variant",	},
    /* deprecated: */
    { "NONE",		"VOID",		"void",		NULL,			},
    { "BOOL",		"BOOLEAN",	"gboolean",	"g_marshal_value_peek_boolean",	},
  };
  guint i;

  g_return_val_if_fail (iarg != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (args); i++)
    if (strcmp (args[i].keyword, iarg->keyword) == 0)
      {
	iarg->sig_name = args[i].sig_name;
	iarg->ctype = args[i].ctype;
	iarg->getter = args[i].getter;

	return TRUE;
      }
  return FALSE;
}

static gboolean
complete_out_arg (OutArgument *oarg)
{
  static const OutArgument args[] = {
    /* keyword		sig_name	ctype		setter			*/
    { "VOID",		"VOID",		"void",		NULL,					     },
    { "BOOLEAN",	"BOOLEAN",	"gboolean",	"g_value_set_boolean",			     },
    { "CHAR",		"CHAR",		"gchar",	"g_value_set_char",			     },
    { "UCHAR",		"UCHAR",	"guchar",	"g_value_set_uchar",			     },
    { "INT",		"INT",		"gint",		"g_value_set_int",			     },
    { "UINT",		"UINT",		"guint",	"g_value_set_uint",			     },
    { "LONG",		"LONG",		"glong",	"g_value_set_long",			     },
    { "ULONG",		"ULONG",	"gulong",	"g_value_set_ulong",			     },
    { "INT64",		"INT64",	"gint64",	"g_value_set_int64",			     },
    { "UINT64",		"UINT64",	"guint64",	"g_value_set_uint64",			     },
    { "ENUM",		"ENUM",		"gint",		"g_value_set_enum",			     },
    { "FLAGS",		"FLAGS",	"guint",	"g_value_set_flags",			     },
    { "FLOAT",		"FLOAT",	"gfloat",	"g_value_set_float",			     },
    { "DOUBLE",		"DOUBLE",	"gdouble",	"g_value_set_double",			     },
    { "STRING",		"STRING",	"gchar*",	"g_value_take_string",			     },
    { "PARAM",		"PARAM",	"GParamSpec*",	"g_value_take_param",			     },
    { "BOXED",		"BOXED",	"gpointer",	"g_value_take_boxed",			     },
    { "POINTER",	"POINTER",	"gpointer",	"g_value_set_pointer",			     },
    { "OBJECT",		"OBJECT",	"GObject*",	"g_value_take_object",			     },
    { "VARIANT",	"VARIANT",	"GVariant*",	"g_value_take_variant",			     },
    /* deprecated: */
    { "NONE",		"VOID",		"void",		NULL,					     },
    { "BOOL",		"BOOLEAN",	"gboolean",	"g_value_set_boolean",			     },
  };
  guint i;

  g_return_val_if_fail (oarg != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (args); i++)
    if (strcmp (args[i].keyword, oarg->keyword) == 0)
      {
	oarg->sig_name = args[i].sig_name;
	oarg->ctype = args[i].ctype;
	oarg->setter = args[i].setter;

	return TRUE;
      }
  return FALSE;
}

static const gchar*
pad (const gchar *string)
{
#define PAD_LENGTH	12
  static gchar *buffer = NULL;
  gint i;

  g_return_val_if_fail (string != NULL, NULL);

  if (!buffer)
    buffer = g_new (gchar, PAD_LENGTH + 1);

  /* paranoid check */
  if (strlen (string) >= PAD_LENGTH)
    {
      g_free (buffer);
      buffer = g_strdup_printf ("%s ", string);
      g_warning ("overfull string (%u bytes) for padspace",
                 (guint) strlen (string));
      exit_status |= 2;

      return buffer;
    }

  for (i = 0; i < PAD_LENGTH; i++)
    {
      gboolean done = *string == 0;

      buffer[i] = done ? ' ' : *string++;
    }
  buffer[i] = 0;

  return buffer;
}

static const gchar*
indent (guint n_spaces)
{
  static gchar *buffer = NULL;
  static guint blength = 0;

  if (blength <= n_spaces)
    {
      blength = n_spaces + 1;
      g_free (buffer);
      buffer = g_new (gchar, blength);
    }
  memset (buffer, ' ', n_spaces);
  buffer[n_spaces] = 0;

  return buffer;
}

static void
generate_marshal (const gchar *signame,
		  Signature   *sig)
{
  guint ind, a;
  GList *node;
  gchar *tmp = g_strconcat (marshaller_prefix, "_", signame, NULL);
  gboolean have_std_marshaller = FALSE;

  /* here we have to make sure a marshaller named <marshaller_prefix>_<signame>
   * exists. we might have put it out already, can revert to a standard
   * marshaller provided by glib, or need to generate one.
   */

  if (g_hash_table_lookup (marshallers, tmp))
    {
      /* done, marshaller already generated */
      g_free (tmp);
      return;
    }
  else
    {
      /* need to alias/generate marshaller, register name */
      g_hash_table_insert (marshallers, tmp, tmp);
    }

  /* can we revert to a standard marshaller? */
  if (std_includes)
    {
      tmp = g_strconcat (std_marshaller_prefix, "_", signame, NULL);
      have_std_marshaller = g_hash_table_lookup (marshallers, tmp) != NULL;
      g_free (tmp);
    }

  if (gen_cheader && have_std_marshaller)
    {
      g_fprintf (fout, "#define %s_%s\t%s_%s\n", marshaller_prefix, signame, std_marshaller_prefix, signame);
    }
  if (gen_cheader && !have_std_marshaller)
    {
      ind = g_fprintf (fout, gen_internal ? "G_GNUC_INTERNAL " : "extern ");
      ind += g_fprintf (fout, "void ");
      ind += g_fprintf (fout, "%s_%s (", marshaller_prefix, signame);
      g_fprintf (fout,   "GClosure     *closure,\n");
      g_fprintf (fout, "%sGValue       *return_value,\n", indent (ind));
      g_fprintf (fout, "%sguint         n_param_values,\n", indent (ind));
      g_fprintf (fout, "%sconst GValue *param_values,\n", indent (ind));
      g_fprintf (fout, "%sgpointer      invocation_hint,\n", indent (ind));
      g_fprintf (fout, "%sgpointer      marshal_data);\n",
                 indent (ind));
    }
  if (gen_cbody && !have_std_marshaller)
    {
      /* cfile marshal header */
      g_fprintf (fout, "void\n");
      ind = g_fprintf (fout, "%s_%s (", marshaller_prefix, signame);
      g_fprintf (fout,   "GClosure     *closure,\n");
      g_fprintf (fout, "%sGValue       *return_value G_GNUC_UNUSED,\n", indent (ind));
      g_fprintf (fout, "%sguint         n_param_values,\n", indent (ind));
      g_fprintf (fout, "%sconst GValue *param_values,\n", indent (ind));
      g_fprintf (fout, "%sgpointer      invocation_hint G_GNUC_UNUSED,\n", indent (ind));
      g_fprintf (fout, "%sgpointer      marshal_data)\n", indent (ind));
      g_fprintf (fout, "{\n");

      /* cfile GMarshalFunc typedef */
      ind = g_fprintf (fout, "  typedef %s (*GMarshalFunc_%s) (", sig->rarg->ctype, signame);
      g_fprintf (fout, "%s data1,\n", pad ("gpointer"));
      for (a = 1, node = sig->args; node; node = node->next)
	{
	  InArgument *iarg = node->data;

	  if (iarg->getter)
	    g_fprintf (fout, "%s%s arg_%d,\n", indent (ind), pad (iarg->ctype), a++);
	}
      g_fprintf (fout, "%s%s data2);\n", indent (ind), pad ("gpointer"));

      /* cfile marshal variables */
      g_fprintf (fout, "  register GMarshalFunc_%s callback;\n", signame);
      g_fprintf (fout, "  register GCClosure *cc = (GCClosure*) closure;\n");
      g_fprintf (fout, "  register gpointer data1, data2;\n");
      if (sig->rarg->setter)
	g_fprintf (fout, "  %s v_return;\n", sig->rarg->ctype);

      if (sig->args || sig->rarg->setter)
	{
	  g_fprintf (fout, "\n");

	  if (sig->rarg->setter)
	    g_fprintf (fout, "  g_return_if_fail (return_value != NULL);\n");
	  if (sig->args)
	    {
	      for (a = 0, node = sig->args; node; node = node->next)
		{
		  InArgument *iarg = node->data;

		  if (iarg->getter)
		    a++;
		}
	      g_fprintf (fout, "  g_return_if_fail (n_param_values == %u);\n", 1 + a);
	    }
	}

      /* cfile marshal data1, data2 and callback setup */
      g_fprintf (fout, "\n");
      g_fprintf (fout, "  if (G_CCLOSURE_SWAP_DATA (closure))\n    {\n");
      g_fprintf (fout, "      data1 = closure->data;\n");
      g_fprintf (fout, "      data2 = g_value_peek_pointer (param_values + 0);\n");
      g_fprintf (fout, "    }\n  else\n    {\n");
      g_fprintf (fout, "      data1 = g_value_peek_pointer (param_values + 0);\n");
      g_fprintf (fout, "      data2 = closure->data;\n");
      g_fprintf (fout, "    }\n");
      g_fprintf (fout, "  callback = (GMarshalFunc_%s) (marshal_data ? marshal_data : cc->callback);\n", signame);

      /* cfile marshal callback action */
      g_fprintf (fout, "\n");
      ind = g_fprintf (fout, " %s callback (", sig->rarg->setter ? " v_return =" : "");
      g_fprintf (fout, "data1,\n");
      for (a = 1, node = sig->args; node; node = node->next)
	{
	  InArgument *iarg = node->data;

	  if (iarg->getter)
	    g_fprintf (fout, "%s%s (param_values + %d),\n", indent (ind), iarg->getter, a++);
	}
      g_fprintf (fout, "%sdata2);\n", indent (ind));

      /* cfile marshal return value storage */
      if (sig->rarg->setter)
	{
	  g_fprintf (fout, "\n");
	  g_fprintf (fout, "  %s (return_value, v_return);\n", sig->rarg->setter);
	}

      /* cfile marshal footer */
      g_fprintf (fout, "}\n");
    }
}

static void
process_signature (Signature *sig)
{
  gchar *pname, *sname, *tmp;
  GList *node;

  /* lookup and complete info on arguments */
  if (!complete_out_arg (sig->rarg))
    {
      g_warning ("unknown type: %s", sig->rarg->keyword);
      exit_status |= 1;
      return;
    }
  for (node = sig->args; node; node = node->next)
    {
      InArgument *iarg = node->data;

      if (!complete_in_arg (iarg))
	{
	  g_warning ("unknown type: %s", iarg->keyword);
          exit_status |= 1;
	  return;
	}
    }

  /* construct requested marshaller name and technical marshaller name */
  pname = g_strconcat (sig->rarg->keyword, "_", NULL);
  sname = g_strconcat (sig->rarg->sig_name, "_", NULL);
  for (node = sig->args; node; node = node->next)
    {
      InArgument *iarg = node->data;
      gchar *tmp;

      tmp = sname;
      sname = g_strconcat (tmp, "_", iarg->sig_name, NULL);
      g_free (tmp);
      tmp = pname;
      pname = g_strconcat (tmp, "_", iarg->keyword, NULL);
      g_free (tmp);
    }

  /* introductionary comment */
  g_fprintf (fout, "\n/* %s", sig->rarg->keyword);
  for (node = sig->args; node; node = node->next)
    {
      InArgument *iarg = node->data;

      g_fprintf (fout, "%c%s", node->prev ? ',' : ':', iarg->keyword);
    }
  if (!skip_ploc)
    g_fprintf (fout, " (%s)", sig->ploc);
  g_fprintf (fout, " */\n");

  /* ensure technical marshaller exists (<marshaller_prefix>_<sname>) */
  generate_marshal (sname, sig);

  /* put out marshaller alias for requested name if required (<marshaller_prefix>_<pname>) */
  tmp = g_strconcat (marshaller_prefix, "_", pname, NULL);
  if (gen_cheader && !g_hash_table_lookup (marshallers, tmp))
    {
      g_fprintf (fout, "#define %s_%s\t%s_%s\n", marshaller_prefix, pname, marshaller_prefix, sname);

      g_hash_table_insert (marshallers, tmp, tmp);
    }
  else
    g_free (tmp);

  g_free (pname);
  g_free (sname);
}

static InArgument*
new_in_arg (const gchar *pname)
{
  InArgument *iarg = g_new0 (InArgument, 1);

  iarg->keyword = g_strdup (pname);

  return iarg;
}

static OutArgument*
new_out_arg (const gchar *pname)
{
  OutArgument *oarg = g_new0 (OutArgument, 1);

  oarg->keyword = g_strdup (pname);

  return oarg;
}

static guint
parse_line (GScanner  *scanner,
	    Signature *sig)
{
  /* parse identifier for return value */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  sig->rarg = new_out_arg (scanner->value.v_identifier);

  /* keep a note on the location */
  sig->ploc = g_strdup_printf ("%s:%u", scanner->input_name, scanner->line);

  /* expect ':' */
  if (g_scanner_get_next_token (scanner) != ':')
    return ':';

  /* parse first argument */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  sig->args = g_list_append (sig->args, new_in_arg (scanner->value.v_identifier));

  /* parse rest of argument list */
  while (g_scanner_peek_next_token (scanner) == ',')
    {
      /* eat comma */
      g_scanner_get_next_token (scanner);

      /* parse arg identifier */
      if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
	return G_TOKEN_IDENTIFIER;
      sig->args = g_list_append (sig->args, new_in_arg (scanner->value.v_identifier));
    }

  /* expect end of line, done */
  if (g_scanner_get_next_token (scanner) != '\n')
    return '\n';

  /* success */
  return G_TOKEN_NONE;
}

static gboolean
string_key_destroy (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
  g_free (key);

  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  const gchar *gobject_marshallers[] = {
#include	"gmarshal.strings"
  };
  GScanner *scanner;
  GSList *slist, *files = NULL;
  gint i;

  /* parse args and do fast exits */
  parse_args (&argc, &argv);

  /* list input files */
  for (i = 1; i < argc; i++)
    files = g_slist_prepend (files, argv[i]);
  if (files)
    files = g_slist_reverse (files);
  else
    files = g_slist_prepend (files, "/dev/stdin");

  /* setup auxillary structs */
  scanner = g_scanner_new (&scanner_config_template);
  fout = stdout;
  marshallers = g_hash_table_new (g_str_hash, g_str_equal);

  /* add standard marshallers of the GObject library */
  if (std_includes)
    for (i = 0; i < G_N_ELEMENTS (gobject_marshallers); i++)
      {
	gchar *tmp = g_strdup (gobject_marshallers[i]);

	g_hash_table_insert (marshallers, tmp, tmp);
      }

  /* put out initial heading */
  g_fprintf (fout, "\n");

  if (gen_cheader && std_includes)
    {
      g_fprintf (fout, "#ifndef __%s_MARSHAL_H__\n", marshaller_prefix);
      g_fprintf (fout, "#define __%s_MARSHAL_H__\n\n", marshaller_prefix);
    }

  if ((gen_cheader || gen_cbody) && std_includes)
    g_fprintf (fout, "#include\t<glib-object.h>\n\n");

  if (gen_cheader)
    g_fprintf (fout, "G_BEGIN_DECLS\n");

  /* generate necessary preprocessor directives */
  if (gen_cbody)
    put_marshal_value_getters ();

  /* process input files */
  for (slist = files; slist; slist = slist->next)
    {
      gchar *file = slist->data;
      gint fd;

      if (strcmp (file, "/dev/stdin") == 0)
	/* Mostly for Win32. This is equivalent to opening /dev/stdin */
	fd = dup (0);
      else
	fd = open (file, O_RDONLY);

      if (fd < 0)
	{
	  g_warning ("failed to open \"%s\": %s", file, g_strerror (errno));
	  exit_status |= 1;
	  continue;
	}

      /* set file name for error reports */
      scanner->input_name = file;

      /* parse & process file */
      g_scanner_input_file (scanner, fd);

      /* scanning loop, we parse the input until its end is reached,
       * or our sub routine came across invalid syntax
       */
      do
	{
	  guint expected_token = G_TOKEN_NONE;

	  switch (g_scanner_peek_next_token (scanner))
	    {
	    case '\n':
	      /* eat newline and restart */
	      g_scanner_get_next_token (scanner);
	      continue;
	    case G_TOKEN_EOF:
	      /* done */
	      break;
	    default:
	      /* parse and process signatures */
	      {
		Signature signature = { NULL, NULL, NULL };
		GList *node;

		expected_token = parse_line (scanner, &signature);

		/* once we got a valid signature, process it */
		if (expected_token == G_TOKEN_NONE)
		  process_signature (&signature);

		/* clean up signature contents */
		g_free (signature.ploc);
		if (signature.rarg)
		  g_free (signature.rarg->keyword);
		g_free (signature.rarg);
		for (node = signature.args; node; node = node->next)
		  {
		    InArgument *iarg = node->data;

		    g_free (iarg->keyword);
		    g_free (iarg);
		  }
		g_list_free (signature.args);
	      }
	      break;
	    }

	  /* bail out on errors */
	  if (expected_token != G_TOKEN_NONE)
	    {
	      g_scanner_unexp_token (scanner, expected_token, "type name", NULL, NULL, NULL, TRUE);
	      exit_status |= 1;
	      break;
	    }

	  g_scanner_peek_next_token (scanner);
	}
      while (scanner->next_token != G_TOKEN_EOF);

      close (fd);
    }

  /* put out trailer */
  if (gen_cheader)
    {
      g_fprintf (fout, "\nG_END_DECLS\n");

      if (std_includes)
	g_fprintf (fout, "\n#endif /* __%s_MARSHAL_H__ */\n", marshaller_prefix);
    }
  g_fprintf (fout, "\n");

  /* clean up */
  g_slist_free (files);
  g_scanner_destroy (scanner);
  g_hash_table_foreach_remove (marshallers, string_key_destroy, NULL);
  g_hash_table_destroy (marshallers);

  return exit_status;
}

static void
parse_args (gint    *argc_p,
	    gchar ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  guint i, e;

  for (i = 1; i < argc; i++)
    {
      if (strcmp ("--header", argv[i]) == 0)
	{
	  gen_cheader = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--body", argv[i]) == 0)
	{
	  gen_cbody = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--skip-source", argv[i]) == 0)
	{
	  skip_ploc = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--nostdinc", argv[i]) == 0)
	{
	  std_includes = FALSE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--stdinc", argv[i]) == 0)
	{
	  std_includes = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--internal", argv[i]) == 0)
	{
	  gen_internal = TRUE;
	  argv[i] = NULL;
	}
      else if ((strcmp ("--prefix", argv[i]) == 0) ||
	       (strncmp ("--prefix=", argv[i], 9) == 0))
	{
          gchar *equal = argv[i] + 8;

	  if (*equal == '=')
	    marshaller_prefix = g_strdup (equal + 1);
	  else if (i + 1 < argc)
	    {
	      marshaller_prefix = g_strdup (argv[i + 1]);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
          strcmp ("-?", argv[i]) == 0 ||
	  strcmp ("--help", argv[i]) == 0)
	{
	  print_blurb (stderr, TRUE);
	  argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("-v", argv[i]) == 0 ||
	       strcmp ("--version", argv[i]) == 0)
	{
	  print_blurb (stderr, FALSE);
	  argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;

	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);

	  argv[i] = NULL;
	}
    }

  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;
}

static void
print_blurb (FILE    *bout,
	     gboolean print_help)
{
  if (!print_help)
    {
      g_fprintf (bout, "%s version ", PRG_NAME);
      g_fprintf (bout, "%u.%u.%u", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      g_fprintf (bout, "\n");
      g_fprintf (bout, "%s comes with ABSOLUTELY NO WARRANTY.\n", PRG_NAME);
      g_fprintf (bout, "You may redistribute copies of %s under the terms of\n", PRG_NAME);
      g_fprintf (bout, "the GNU General Public License which can be found in the\n");
      g_fprintf (bout, "%s source package. Sources, examples and contact\n", PKG_NAME);
      g_fprintf (bout, "information are available at %s\n", PKG_HTTP_HOME);
    }
  else
    {
      g_fprintf (bout, "Usage:\n");
      g_fprintf (bout, "  %s [OPTION...] [FILES...]\n\n", PRG_NAME);
      g_fprintf (bout, "Help Options:\n");
      g_fprintf (bout, "  -h, --help                 Show this help message\n\n");
      g_fprintf (bout, "Utility Options:\n");
      g_fprintf (bout, "  --header                   Generate C headers\n");
      g_fprintf (bout, "  --body                     Generate C code\n");
      g_fprintf (bout, "  --prefix=string            Specify marshaller prefix\n");
      g_fprintf (bout, "  --skip-source              Skip source location comments\n");
      g_fprintf (bout, "  --stdinc, --nostdinc       Include/use standard marshallers\n");
      g_fprintf (bout, "  --internal                 Mark generated functions as internal\n");
      g_fprintf (bout, "  -v, --version              Print version informations\n");
      g_fprintf (bout, "  --g-fatal-warnings         Make warnings fatal (abort)\n");
    }
}
