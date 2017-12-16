/* GLIB - Library of useful routines for C programming
 * gmappedfile.c: Simplified wrapper around the mmap() function.
 *
 * Copyright 2005 Matthias Clasen
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

#include <errno.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include "glibconfig.h"

#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>

#define fstat(a,b) _fstati64(a,b)
#define stat _stati64

#endif

#include "gconvert.h"
#include "gerror.h"
#include "gfileutils.h"
#include "gmappedfile.h"
#include "gmem.h"
#include "gmessages.h"
#include "gstdio.h"
#include "gstrfuncs.h"
#include "gatomic.h"
#include "gbuffer.h"

#include "glibintl.h"


#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

struct _GMappedFile 
{
  gchar *contents;
  gsize  length;
  gpointer free_func;
  int    ref_count;
#ifdef G_OS_WIN32
  HANDLE mapping;
#endif
};

/* Make sure the layout of GMappedFile is the same as GBuffer's */
G_STATIC_ASSERT (G_STRUCT_OFFSET (GMappedFile, contents) ==
                 G_STRUCT_OFFSET (GBuffer, data));
G_STATIC_ASSERT (G_STRUCT_OFFSET (GMappedFile, length) ==
                 G_STRUCT_OFFSET (GBuffer, size));
G_STATIC_ASSERT (G_STRUCT_OFFSET (GMappedFile, ref_count) ==
                 G_STRUCT_OFFSET (GBuffer, ref_count));
G_STATIC_ASSERT (G_STRUCT_OFFSET (GMappedFile, free_func) ==
                 G_STRUCT_OFFSET (GBuffer, free_func));

static void
g_mapped_file_destroy (GMappedFile *file)
{
  if (file->length)
    {
#ifdef HAVE_MMAP
      munmap (file->contents, file->length);
#endif
#ifdef G_OS_WIN32
      UnmapViewOfFile (file->contents);
      CloseHandle (file->mapping);
#endif
    }

  g_slice_free (GMappedFile, file);
}

/**
 * g_mapped_file_new:
 * @filename: The path of the file to load, in the GLib filename encoding
 * @writable: whether the mapping should be writable
 * @error: return location for a #GError, or %NULL
 *
 * Maps a file into memory. On UNIX, this is using the mmap() function.
 *
 * If @writable is %TRUE, the mapped buffer may be modified, otherwise
 * it is an error to modify the mapped buffer. Modifications to the buffer 
 * are not visible to other processes mapping the same file, and are not 
 * written back to the file.
 *
 * Note that modifications of the underlying file might affect the contents
 * of the #GMappedFile. Therefore, mapping should only be used if the file 
 * will not be modified, or if all modifications of the file are done
 * atomically (e.g. using g_file_set_contents()). 
 *
 * Return value: a newly allocated #GMappedFile which must be unref'd
 *    with g_mapped_file_unref(), or %NULL if the mapping failed.
 *
 * Since: 2.8
 */
GMappedFile *
g_mapped_file_new (const gchar  *filename,
		   gboolean      writable,
		   GError      **error)
{
  GMappedFile *file;
  int fd;
  struct stat st;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (!error || *error == NULL, NULL);

  fd = g_open (filename, (writable ? O_RDWR : O_RDONLY) | _O_BINARY, 0);
  if (fd == -1)
    {
      int save_errno = errno;
      gchar *display_filename = g_filename_display_name (filename);
      
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (save_errno),
                   _("Failed to open file '%s': open() failed: %s"),
                   display_filename, 
		   g_strerror (save_errno));
      g_free (display_filename);
      return NULL;
    }

  file = g_slice_new0 (GMappedFile);
  file->ref_count = 1;
  file->free_func = g_mapped_file_destroy;

  if (fstat (fd, &st) == -1)
    {
      int save_errno = errno;
      gchar *display_filename = g_filename_display_name (filename);

      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (save_errno),
                   _("Failed to get attributes of file '%s': fstat() failed: %s"),
                   display_filename, 
		   g_strerror (save_errno));
      g_free (display_filename);
      goto out;
    }

  if (st.st_size == 0)
    {
      file->length = 0;
      file->contents = NULL;
      close (fd);
      return file;
    }

  file->contents = MAP_FAILED;

#ifdef HAVE_MMAP
  if (st.st_size > G_MAXSIZE)
    {
      errno = EINVAL;
    }
  else
    {      
      file->length = (gsize) st.st_size;
      file->contents = (gchar *) mmap (NULL,  file->length,
				       writable ? PROT_READ|PROT_WRITE : PROT_READ,
				       MAP_PRIVATE, fd, 0);
    }
#endif
#ifdef G_OS_WIN32
  file->length = st.st_size;
  file->mapping = CreateFileMapping ((HANDLE) _get_osfhandle (fd), NULL,
				     writable ? PAGE_WRITECOPY : PAGE_READONLY,
				     0, 0,
				     NULL);
  if (file->mapping != NULL)
    {
      file->contents = MapViewOfFile (file->mapping,
				      writable ? FILE_MAP_COPY : FILE_MAP_READ,
				      0, 0,
				      0);
      if (file->contents == NULL)
	{
	  file->contents = MAP_FAILED;
	  CloseHandle (file->mapping);
	  file->mapping = NULL;
	}
    }
#endif

  
  if (file->contents == MAP_FAILED)
    {
      int save_errno = errno;
      gchar *display_filename = g_filename_display_name (filename);
      
      g_set_error (error,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to map file '%s': mmap() failed: %s"),
		   display_filename,
		   g_strerror (save_errno));
      g_free (display_filename);
      goto out;
    }

  close (fd);
  return file;

 out:
  close (fd);
  g_slice_free (GMappedFile, file);

  return NULL;
}

/**
 * g_mapped_file_get_length:
 * @file: a #GMappedFile
 *
 * Returns the length of the contents of a #GMappedFile.
 *
 * Returns: the length of the contents of @file.
 *
 * Since: 2.8
 */
gsize
g_mapped_file_get_length (GMappedFile *file)
{
  g_return_val_if_fail (file != NULL, 0);

  return file->length;
}

/**
 * g_mapped_file_get_contents:
 * @file: a #GMappedFile
 *
 * Returns the contents of a #GMappedFile. 
 *
 * Note that the contents may not be zero-terminated,
 * even if the #GMappedFile is backed by a text file.
 *
 * If the file is empty then %NULL is returned.
 *
 * Returns: the contents of @file, or %NULL.
 *
 * Since: 2.8
 */
gchar *
g_mapped_file_get_contents (GMappedFile *file)
{
  g_return_val_if_fail (file != NULL, NULL);

  return file->contents;
}

/**
 * g_mapped_file_free:
 * @file: a #GMappedFile
 *
 * This call existed before #GMappedFile had refcounting and is currently
 * exactly the same as g_mapped_file_unref().
 *
 * Since: 2.8
 * Deprecated:2.22: Use g_mapped_file_unref() instead.
 */
void
g_mapped_file_free (GMappedFile *file)
{
  g_mapped_file_unref (file);
}

/**
 * g_mapped_file_ref:
 * @file: a #GMappedFile
 *
 * Increments the reference count of @file by one.  It is safe to call
 * this function from any thread.
 *
 * Return value: the passed in #GMappedFile.
 *
 * Since: 2.22
 **/
GMappedFile *
g_mapped_file_ref (GMappedFile *file)
{
  g_return_val_if_fail (file != NULL, NULL);
  g_return_val_if_fail (file->ref_count > 0, file);

  g_atomic_int_inc (&file->ref_count);

  return file;
}

/**
 * g_mapped_file_unref:
 * @file: a #GMappedFile
 *
 * Decrements the reference count of @file by one.  If the reference count
 * drops to 0, unmaps the buffer of @file and frees it.
 *
 * It is safe to call this function from any thread.
 *
 * Since 2.22
 **/
void
g_mapped_file_unref (GMappedFile *file)
{
  g_return_if_fail (file != NULL);
  g_return_if_fail (file->ref_count > 0);

  if (g_atomic_int_dec_and_test (&file->ref_count))
    g_mapped_file_destroy (file);
}
