/* GMODULE - GLIB wrapper code for dynamic module loading
 * Copyright (C) 1998 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */

#include "config.h"

#include "glib.h"
#include "gmodule.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>		/* For open() and close() prototypes. */
#endif

#include "gmoduleconf.h"
#include "gstdio.h"

/* We maintain a list of modules, so we can reference count them.
 * That's needed because some platforms don't support refernce counts on
 * modules e.g. the shl_* implementation of HP-UX
 * (http://www.stat.umn.edu/~luke/xls/projects/dlbasics/dlbasics.html).
 * Also, the module for the program itself is kept seperatedly for
 * faster access and because it has special semantics.
 */


/* --- structures --- */
struct _GModule
{
  gchar	*file_name;
#if defined (G_OS_WIN32) && !defined(_WIN64)
  gchar *cp_file_name;
#endif
  gpointer handle;
  guint ref_count : 31;
  guint is_resident : 1;
  GModuleUnload unload;
  GModule *next;
};


/* --- prototypes --- */
static gpointer		_g_module_open		(const gchar	*file_name,
						 gboolean	 bind_lazy,
						 gboolean	 bind_local);
static void		_g_module_close		(gpointer	 handle,
						 gboolean	 is_unref);
static gpointer		_g_module_self		(void);
static gpointer		_g_module_symbol	(gpointer	 handle,
						 const gchar	*symbol_name);
static gchar*		_g_module_build_path	(const gchar	*directory,
						 const gchar	*module_name);
static inline void	g_module_set_error	(const gchar	*error);
static inline GModule*	g_module_find_by_handle (gpointer	 handle);
static inline GModule*	g_module_find_by_name	(const gchar	*name);


/* --- variables --- */
static GModule	     *modules = NULL;
static GModule	     *main_module = NULL;
static GStaticPrivate module_error_private = G_STATIC_PRIVATE_INIT;
static gboolean	      module_debug_initialized = FALSE;
static guint	      module_debug_flags = 0;


/* --- inline functions --- */
static inline GModule*
g_module_find_by_handle (gpointer handle)
{
  GModule *module;
  GModule *retval = NULL;
  
  if (main_module && main_module->handle == handle)
    retval = main_module;
  else
    for (module = modules; module; module = module->next)
      if (handle == module->handle)
	{
	  retval = module;
	  break;
	}

  return retval;
}

static inline GModule*
g_module_find_by_name (const gchar *name)
{
  GModule *module;
  GModule *retval = NULL;
  
  for (module = modules; module; module = module->next)
    if (strcmp (name, module->file_name) == 0)
	{
	  retval = module;
	  break;
	}

  return retval;
}

static inline void
g_module_set_error_unduped (gchar *error)
{
  g_static_private_set (&module_error_private, error, g_free);
  errno = 0;
}

static inline void
g_module_set_error (const gchar *error)
{
  g_module_set_error_unduped (g_strdup (error));
}


/* --- include platform specifc code --- */
#define	SUPPORT_OR_RETURN(rv)	{ g_module_set_error (NULL); }
#if	(G_MODULE_IMPL == G_MODULE_IMPL_DL)
#include "gmodule-dl.c"
#elif	(G_MODULE_IMPL == G_MODULE_IMPL_DLD)
#include "gmodule-dld.c"
#elif	(G_MODULE_IMPL == G_MODULE_IMPL_WIN32)
#include "gmodule-win32.c"
#elif	(G_MODULE_IMPL == G_MODULE_IMPL_DYLD)
#include "gmodule-dyld.c"
#elif	(G_MODULE_IMPL == G_MODULE_IMPL_AR)
#include "gmodule-ar.c"
#else
#undef	SUPPORT_OR_RETURN
#define	SUPPORT_OR_RETURN(rv)	{ g_module_set_error ("dynamic modules are " \
                                              "not supported by this system"); return rv; }
static gpointer
_g_module_open (const gchar	*file_name,
		gboolean	 bind_lazy,
		gboolean	 bind_local)
{
  return NULL;
}
static void
_g_module_close	(gpointer	 handle,
		 gboolean	 is_unref)
{
}
static gpointer
_g_module_self (void)
{
  return NULL;
}
static gpointer
_g_module_symbol (gpointer	 handle,
		  const gchar	*symbol_name)
{
  return NULL;
}
static gchar*
_g_module_build_path (const gchar *directory,
		      const gchar *module_name)
{
  return NULL;
}
#endif	/* no implementation */

/* --- functions --- */
gboolean
g_module_supported (void)
{
  SUPPORT_OR_RETURN (FALSE);
  
  return TRUE;
}

static gchar*
parse_libtool_archive (const gchar* libtool_name)
{
  const guint TOKEN_DLNAME = G_TOKEN_LAST + 1;
  const guint TOKEN_INSTALLED = G_TOKEN_LAST + 2;
  const guint TOKEN_LIBDIR = G_TOKEN_LAST + 3;
  gchar *lt_dlname = NULL;
  gboolean lt_installed = TRUE;
  gchar *lt_libdir = NULL;
  gchar *name;
  GTokenType token;
  GScanner *scanner;
  
  int fd = g_open (libtool_name, O_RDONLY, 0);
  if (fd < 0)
    {
      gchar *display_libtool_name = g_filename_display_name (libtool_name);
      g_module_set_error_unduped (g_strdup_printf ("failed to open libtool archive \"%s\"", display_libtool_name));
      g_free (display_libtool_name);
      return NULL;
    }
  /* search libtool's dlname specification  */
  scanner = g_scanner_new (NULL);
  g_scanner_input_file (scanner, fd);
  scanner->config->symbol_2_token = TRUE;
  g_scanner_scope_add_symbol (scanner, 0, "dlname", 
			      GUINT_TO_POINTER (TOKEN_DLNAME));
  g_scanner_scope_add_symbol (scanner, 0, "installed", 
			      GUINT_TO_POINTER (TOKEN_INSTALLED));
  g_scanner_scope_add_symbol (scanner, 0, "libdir", 
			      GUINT_TO_POINTER (TOKEN_LIBDIR));
  while (!g_scanner_eof (scanner))
    {
      token = g_scanner_get_next_token (scanner);
      if (token == TOKEN_DLNAME || token == TOKEN_INSTALLED || 
	  token == TOKEN_LIBDIR)
	{
	  if (g_scanner_get_next_token (scanner) != '=' ||
	      g_scanner_get_next_token (scanner) != 
	      (token == TOKEN_INSTALLED ? 
	       G_TOKEN_IDENTIFIER : G_TOKEN_STRING))
	    {
	      gchar *display_libtool_name = g_filename_display_name (libtool_name);
	      g_module_set_error_unduped (g_strdup_printf ("unable to parse libtool archive \"%s\"", display_libtool_name));
	      g_free (display_libtool_name);

	      g_free (lt_dlname);
	      g_free (lt_libdir);
	      g_scanner_destroy (scanner);
	      close (fd);

	      return NULL;
	    }
	  else
	    {
	      if (token == TOKEN_DLNAME)
		{
		  g_free (lt_dlname);
		  lt_dlname = g_strdup (scanner->value.v_string);
		}
	      else if (token == TOKEN_INSTALLED)
		lt_installed = 
		  strcmp (scanner->value.v_identifier, "yes") == 0;
	      else /* token == TOKEN_LIBDIR */
		{
		  g_free (lt_libdir);
		  lt_libdir = g_strdup (scanner->value.v_string);
		}
	    }
	}      
    }

  if (!lt_installed)
    {
      gchar *dir = g_path_get_dirname (libtool_name);
      g_free (lt_libdir);
      lt_libdir = g_strconcat (dir, G_DIR_SEPARATOR_S ".libs", NULL);
      g_free (dir);
    }

  name = g_strconcat (lt_libdir, G_DIR_SEPARATOR_S, lt_dlname, NULL);
  
  g_free (lt_dlname);
  g_free (lt_libdir);
  g_scanner_destroy (scanner);
  close (fd);

  return name;
}

static inline gboolean
str_check_suffix (const gchar* string,
		  const gchar* suffix)
{
  gsize string_len = strlen (string);    
  gsize suffix_len = strlen (suffix);    

  return string_len >= suffix_len && 
    strcmp (string + string_len - suffix_len, suffix) == 0;
}

enum
{
  G_MODULE_DEBUG_RESIDENT_MODULES = 1 << 0,
  G_MODULE_DEBUG_BIND_NOW_MODULES = 1 << 1
};

static void
_g_module_debug_init (void)
{
  const GDebugKey keys[] = {
    { "resident-modules", G_MODULE_DEBUG_RESIDENT_MODULES },
    { "bind-now-modules", G_MODULE_DEBUG_BIND_NOW_MODULES }
  };
  const gchar *env;

  env = g_getenv ("G_DEBUG");

  module_debug_flags =
    !env ? 0 : g_parse_debug_string (env, keys, G_N_ELEMENTS (keys));

  module_debug_initialized = TRUE;
}

static GStaticRecMutex g_module_global_lock = G_STATIC_REC_MUTEX_INIT;

GModule*
g_module_open (const gchar    *file_name,
	       GModuleFlags    flags)
{
  GModule *module;
  gpointer handle = NULL;
  gchar *name = NULL;
  
  SUPPORT_OR_RETURN (NULL);
  
  g_static_rec_mutex_lock (&g_module_global_lock);

  if (G_UNLIKELY (!module_debug_initialized))
    _g_module_debug_init ();

  if (module_debug_flags & G_MODULE_DEBUG_BIND_NOW_MODULES)
    flags &= ~G_MODULE_BIND_LAZY;

  if (!file_name)
    {      
      if (!main_module)
	{
	  handle = _g_module_self ();
	  if (handle)
	    {
	      main_module = g_new (GModule, 1);
	      main_module->file_name = NULL;
#if defined (G_OS_WIN32) && !defined(_WIN64)
	      main_module->cp_file_name = NULL;
#endif
	      main_module->handle = handle;
	      main_module->ref_count = 1;
	      main_module->is_resident = TRUE;
	      main_module->unload = NULL;
	      main_module->next = NULL;
	    }
	}
      else
	main_module->ref_count++;

      g_static_rec_mutex_unlock (&g_module_global_lock);
      return main_module;
    }
  
  /* we first search the module list by name */
  module = g_module_find_by_name (file_name);
  if (module)
    {
      module->ref_count++;
      
      g_static_rec_mutex_unlock (&g_module_global_lock);
      return module;
    }

  /* check whether we have a readable file right away */
  if (g_file_test (file_name, G_FILE_TEST_IS_REGULAR))
    name = g_strdup (file_name);
  /* try completing file name with standard library suffix */
  if (!name)
    {
      name = g_strconcat (file_name, "." G_MODULE_SUFFIX, NULL);
      if (!g_file_test (name, G_FILE_TEST_IS_REGULAR))
	{
	  g_free (name);
	  name = NULL;
	}
    }
  /* try completing by appending libtool suffix */
  if (!name)
    {
      name = g_strconcat (file_name, ".la", NULL);
      if (!g_file_test (name, G_FILE_TEST_IS_REGULAR))
	{
	  g_free (name);
	  name = NULL;
	}
    }
  /* we can't access() the file, lets hope the platform backends finds
   * it via library paths
   */
  if (!name)
    {
      gchar *dot = strrchr (file_name, '.');
      gchar *slash = strrchr (file_name, G_DIR_SEPARATOR);
      
      /* make sure the name has a suffix */
      if (!dot || dot < slash)
	name = g_strconcat (file_name, "." G_MODULE_SUFFIX, NULL);
      else
	name = g_strdup (file_name);
    }

  /* ok, try loading the module */
  if (name)
    {
      /* if it's a libtool archive, figure library file to load */
      if (str_check_suffix (name, ".la")) /* libtool archive? */
	{
	  gchar *real_name = parse_libtool_archive (name);

	  /* real_name might be NULL, but then module error is already set */
	  if (real_name)
	    {
	      g_free (name);
	      name = real_name;
            }
	}
      if (name)
	handle = _g_module_open (name, (flags & G_MODULE_BIND_LAZY) != 0,
			(flags & G_MODULE_BIND_LOCAL) != 0);
    }
  else
    {
      gchar *display_file_name = g_filename_display_name (file_name);
      g_module_set_error_unduped (g_strdup_printf ("unable to access file \"%s\"", display_file_name));
      g_free (display_file_name);
    }
  g_free (name);

  if (handle)
    {
      gchar *saved_error;
      GModuleCheckInit check_init;
      const gchar *check_failed = NULL;
      
      /* search the module list by handle, since file names are not unique */
      module = g_module_find_by_handle (handle);
      if (module)
	{
	  _g_module_close (module->handle, TRUE);
	  module->ref_count++;
	  g_module_set_error (NULL);
	  
	  g_static_rec_mutex_unlock (&g_module_global_lock);
	  return module;
	}
      
      saved_error = g_strdup (g_module_error ());
      g_module_set_error (NULL);
      
      module = g_new (GModule, 1);
      module->file_name = g_strdup (file_name);
#if defined (G_OS_WIN32) && !defined(_WIN64)
      module->cp_file_name = g_locale_from_utf8 (file_name, -1,
						 NULL, NULL, NULL);
#endif
      module->handle = handle;
      module->ref_count = 1;
      module->is_resident = FALSE;
      module->unload = NULL;
      module->next = modules;
      modules = module;
      
      /* check initialization */
      if (g_module_symbol (module, "g_module_check_init", (gpointer) &check_init) && check_init != NULL)
	check_failed = check_init (module);
      
      /* we don't call unload() if the initialization check failed. */
      if (!check_failed)
	g_module_symbol (module, "g_module_unload", (gpointer) &module->unload);
      
      if (check_failed)
	{
	  gchar *error;

	  error = g_strconcat ("GModule (", 
                               file_name ? file_name : "NULL", 
                               ") initialization check failed: ", 
                               check_failed, NULL);
	  g_module_close (module);
	  module = NULL;
	  g_module_set_error (error);
	  g_free (error);
	}
      else
	g_module_set_error (saved_error);

      g_free (saved_error);
    }

  if (module != NULL &&
      (module_debug_flags & G_MODULE_DEBUG_RESIDENT_MODULES))
    g_module_make_resident (module);

  g_static_rec_mutex_unlock (&g_module_global_lock);
  return module;
}

#if defined (G_OS_WIN32) && !defined(_WIN64)

#undef g_module_open

GModule*
g_module_open (const gchar    *file_name,
	       GModuleFlags    flags)
{
  gchar *utf8_file_name = g_locale_to_utf8 (file_name, -1, NULL, NULL, NULL);
  GModule *retval = g_module_open_utf8 (utf8_file_name, flags);

  g_free (utf8_file_name);

  return retval;
}

#endif

gboolean
g_module_close (GModule	       *module)
{
  SUPPORT_OR_RETURN (FALSE);
  
  g_return_val_if_fail (module != NULL, FALSE);
  g_return_val_if_fail (module->ref_count > 0, FALSE);
  
  g_static_rec_mutex_lock (&g_module_global_lock);

  module->ref_count--;
  
  if (!module->ref_count && !module->is_resident && module->unload)
    {
      GModuleUnload unload;

      unload = module->unload;
      module->unload = NULL;
      unload (module);
    }

  if (!module->ref_count && !module->is_resident)
    {
      GModule *last;
      GModule *node;
      
      last = NULL;
      
      node = modules;
      while (node)
	{
	  if (node == module)
	    {
	      if (last)
		last->next = node->next;
	      else
		modules = node->next;
	      break;
	    }
	  last = node;
	  node = last->next;
	}
      module->next = NULL;
      
      _g_module_close (module->handle, FALSE);
      g_free (module->file_name);
#if defined (G_OS_WIN32) && !defined(_WIN64)
      g_free (module->cp_file_name);
#endif
      g_free (module);
    }
  
  g_static_rec_mutex_unlock (&g_module_global_lock);
  return g_module_error() == NULL;
}

void
g_module_make_resident (GModule *module)
{
  g_return_if_fail (module != NULL);

  module->is_resident = TRUE;
}

G_CONST_RETURN gchar*
g_module_error (void)
{
  return g_static_private_get (&module_error_private);
}

gboolean
g_module_symbol (GModule	*module,
		 const gchar	*symbol_name,
		 gpointer	*symbol)
{
  const gchar *module_error;

  if (symbol)
    *symbol = NULL;
  SUPPORT_OR_RETURN (FALSE);
  
  g_return_val_if_fail (module != NULL, FALSE);
  g_return_val_if_fail (symbol_name != NULL, FALSE);
  g_return_val_if_fail (symbol != NULL, FALSE);
  
  g_static_rec_mutex_lock (&g_module_global_lock);

#ifdef	G_MODULE_NEED_USCORE
  {
    gchar *name;

    name = g_strconcat ("_", symbol_name, NULL);
    *symbol = _g_module_symbol (module->handle, name);
    g_free (name);
  }
#else	/* !G_MODULE_NEED_USCORE */
  *symbol = _g_module_symbol (module->handle, symbol_name);
#endif	/* !G_MODULE_NEED_USCORE */
  
  module_error = g_module_error ();
  if (module_error)
    {
      gchar *error;

      error = g_strconcat ("`", symbol_name, "': ", module_error, NULL);
      g_module_set_error (error);
      g_free (error);
      *symbol = NULL;
    }
  
  g_static_rec_mutex_unlock (&g_module_global_lock);
  return !module_error;
}

G_CONST_RETURN gchar*
g_module_name (GModule *module)
{
  g_return_val_if_fail (module != NULL, NULL);
  
  if (module == main_module)
    return "main";
  
  return module->file_name;
}

#if defined (G_OS_WIN32) && !defined(_WIN64)

#undef g_module_name

G_CONST_RETURN gchar*
g_module_name (GModule *module)
{
  g_return_val_if_fail (module != NULL, NULL);
  
  if (module == main_module)
    return "main";
  
  return module->cp_file_name;
}

#endif

gchar*
g_module_build_path (const gchar *directory,
		     const gchar *module_name)
{
  g_return_val_if_fail (module_name != NULL, NULL);
  
  return _g_module_build_path (directory, module_name);
}
