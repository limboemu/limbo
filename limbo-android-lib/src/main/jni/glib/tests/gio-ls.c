
#include <gio/gio.h>

#define GETTEXT_PACKAGE "gio-ls"
#define N_(s) (s)
#define _(s) (s)

enum
{
  SHOW_ALL,
  SHOW_LONG
};

static void print_path (const gchar* path, guint32 flags);

static gboolean show_all = FALSE;
static gboolean show_long = FALSE;

int 
main (int argc, char *argv[])
{
  
  GOptionContext *context = NULL;
  static GOptionEntry options[] =
  {
    {"all", 'a', 0, G_OPTION_ARG_NONE, &show_all,
     N_("do not hide entries"), NULL },
    {"long", 'l', 0, G_OPTION_ARG_NONE, &show_long,
     N_("use a long listing format"), NULL },
    { NULL }
  };
  GError *error = NULL;
  int i;

  g_type_init ();

  context = g_option_context_new(_("[FILE...]"));
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
  
  if (!g_option_context_parse (context, &argc, &argv, &error)) 
    {
      g_print ("%s", error->message);
      g_error_free (error);
      
    }
  else
    {
      for (i = 1; i < argc; i++) 
        {
	  print_path (argv[i], (show_all ? SHOW_ALL : 0) | (show_long ? SHOW_LONG : 0));
	}
    }

  g_option_context_free(context);
  return 0;
}

static void 
print_path (const gchar* path, 
            guint32      flags)
{
  GFile *top;
  const gchar *short_attrs = G_FILE_ATTRIBUTE_STANDARD_NAME;
  const gchar *long_attrs = G_FILE_ATTRIBUTE_OWNER_USER "," G_FILE_ATTRIBUTE_OWNER_GROUP "," \
			    "access:*,std:*";
  const gchar *attrs;
  
  if (flags & SHOW_LONG)
    attrs = long_attrs;
  else
    attrs = short_attrs;

  top = g_file_new_for_path (path);
  if (top)
    {
      GFileInfo *info;
      GError *error = NULL;
      GFileEnumerator *enumerator = g_file_enumerate_children (top, attrs, 
                                                               G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);
      if (error)
        {
	  g_print ("%s", error->message);
	  g_error_free (error);
	}
      if (!enumerator)
        return;
 
      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
        {
	  const gchar *name = g_file_info_get_name (info);

          if (flags & SHOW_LONG)
	    {
	      const gchar *val;
	      
	      g_print ("%c%c%c%c ",
		g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY ? 'd' : '-',
		g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ) ? 'r' : '-',
		g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) ? 'w' : '-',
		g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE) ? 'x' : '-');

	      val = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_OWNER_USER);
	      g_print ("\t%15s", val ? val : "?user?");

	      val = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_OWNER_GROUP);
	      g_print ("\t%15s", val ? val : "?group?");
	    }
	    
	  g_print ("\t%s\n", name ? name : "?noname?");

	  g_object_unref (info);
	}

      g_object_unref (top);
    }
}
