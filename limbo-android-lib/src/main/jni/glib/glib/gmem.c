/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "gmem.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "gbacktrace.h"
#include "gtestutils.h"
#include "gthread.h"
#include "glib_trace.h"


#define MEM_PROFILE_TABLE_SIZE 4096


/* notes on macros:
 * having G_DISABLE_CHECKS defined disables use of glib_mem_profiler_table and
 * g_mem_profile().
 * REALLOC_0_WORKS is defined if g_realloc (NULL, x) works.
 * SANE_MALLOC_PROTOS is defined if the systems malloc() and friends functions
 * match the corresponding GLib prototypes, keep configure.ac and gmem.h in sync here.
 * g_mem_gc_friendly is TRUE, freed memory should be 0-wiped.
 */

/* --- prototypes --- */
static gboolean g_mem_initialized = FALSE;
static void     g_mem_init_nomessage (void);


/* --- malloc wrappers --- */
#ifndef	REALLOC_0_WORKS
static gpointer
standard_realloc (gpointer mem,
		  gsize    n_bytes)
{
  if (!mem)
    return malloc (n_bytes);
  else
    return realloc (mem, n_bytes);
}
#endif	/* !REALLOC_0_WORKS */

#ifdef SANE_MALLOC_PROTOS
#  define standard_malloc	malloc
#  ifdef REALLOC_0_WORKS
#    define standard_realloc	realloc
#  endif /* REALLOC_0_WORKS */
#  define standard_free		free
#  define standard_calloc	calloc
#  define standard_try_malloc	malloc
#  define standard_try_realloc	realloc
#else	/* !SANE_MALLOC_PROTOS */
static gpointer
standard_malloc (gsize n_bytes)
{
  return malloc (n_bytes);
}
#  ifdef REALLOC_0_WORKS
static gpointer
standard_realloc (gpointer mem,
		  gsize    n_bytes)
{
  return realloc (mem, n_bytes);
}
#  endif /* REALLOC_0_WORKS */
static void
standard_free (gpointer mem)
{
  free (mem);
}
static gpointer
standard_calloc (gsize n_blocks,
		 gsize n_bytes)
{
  return calloc (n_blocks, n_bytes);
}
#define	standard_try_malloc	standard_malloc
#define	standard_try_realloc	standard_realloc
#endif	/* !SANE_MALLOC_PROTOS */


/* --- variables --- */
static GMemVTable glib_mem_vtable = {
  standard_malloc,
  standard_realloc,
  standard_free,
  standard_calloc,
  standard_try_malloc,
  standard_try_realloc,
};

/**
 * SECTION:memory
 * @Short_Description: general memory-handling
 * @Title: Memory Allocation
 * 
 * These functions provide support for allocating and freeing memory.
 * 
 * <note>
 * If any call to allocate memory fails, the application is terminated.
 * This also means that there is no need to check if the call succeeded.
 * </note>
 * 
 * <note>
 * It's important to match g_malloc() with g_free(), plain malloc() with free(),
 * and (if you're using C++) new with delete and new[] with delete[]. Otherwise
 * bad things can happen, since these allocators may use different memory
 * pools (and new/delete call constructors and destructors). See also
 * g_mem_set_vtable().
 * </note>
 */

/* --- functions --- */
/**
 * g_malloc:
 * @n_bytes: the number of bytes to allocate
 * 
 * Allocates @n_bytes bytes of memory.
 * If @n_bytes is 0 it returns %NULL.
 * 
 * Returns: a pointer to the allocated memory
 */
gpointer
g_malloc (gsize n_bytes)
{
  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    {
      gpointer mem;

      mem = glib_mem_vtable.malloc (n_bytes);
      TRACE (GLIB_MEM_ALLOC((void*) mem, (unsigned int) n_bytes, 0, 0));
      if (mem)
	return mem;

      g_error ("%s: failed to allocate %"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_bytes);
    }

  TRACE(GLIB_MEM_ALLOC((void*) NULL, (int) n_bytes, 0, 0));

  return NULL;
}

/**
 * g_malloc0:
 * @n_bytes: the number of bytes to allocate
 * 
 * Allocates @n_bytes bytes of memory, initialized to 0's.
 * If @n_bytes is 0 it returns %NULL.
 * 
 * Returns: a pointer to the allocated memory
 */
gpointer
g_malloc0 (gsize n_bytes)
{
  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    {
      gpointer mem;

      mem = glib_mem_vtable.calloc (1, n_bytes);
      TRACE (GLIB_MEM_ALLOC((void*) mem, (unsigned int) n_bytes, 1, 0));
      if (mem)
	return mem;

      g_error ("%s: failed to allocate %"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_bytes);
    }

  TRACE(GLIB_MEM_ALLOC((void*) NULL, (int) n_bytes, 1, 0));

  return NULL;
}

/**
 * g_realloc:
 * @mem: the memory to reallocate
 * @n_bytes: new size of the memory in bytes
 * 
 * Reallocates the memory pointed to by @mem, so that it now has space for
 * @n_bytes bytes of memory. It returns the new address of the memory, which may
 * have been moved. @mem may be %NULL, in which case it's considered to
 * have zero-length. @n_bytes may be 0, in which case %NULL will be returned
 * and @mem will be freed unless it is %NULL.
 * 
 * Returns: the new address of the allocated memory
 */
gpointer
g_realloc (gpointer mem,
	   gsize    n_bytes)
{
  gpointer newmem;

  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    {
      newmem = glib_mem_vtable.realloc (mem, n_bytes);
      TRACE (GLIB_MEM_REALLOC((void*) newmem, (void*)mem, (unsigned int) n_bytes, 0));
      if (newmem)
	return newmem;

      g_error ("%s: failed to allocate %"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_bytes);
    }

  if (mem)
    glib_mem_vtable.free (mem);

  TRACE (GLIB_MEM_REALLOC((void*) NULL, (void*)mem, 0, 0));

  return NULL;
}

/**
 * g_free:
 * @mem: the memory to free
 * 
 * Frees the memory pointed to by @mem.
 * If @mem is %NULL it simply returns.
 */
void
g_free (gpointer mem)
{
  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (mem))
    glib_mem_vtable.free (mem);
  TRACE(GLIB_MEM_FREE((void*) mem));
}

/**
 * g_try_malloc:
 * @n_bytes: number of bytes to allocate.
 * 
 * Attempts to allocate @n_bytes, and returns %NULL on failure.
 * Contrast with g_malloc(), which aborts the program on failure.
 * 
 * Returns: the allocated memory, or %NULL.
 */
gpointer
g_try_malloc (gsize n_bytes)
{
  gpointer mem;

  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    mem = glib_mem_vtable.try_malloc (n_bytes);
  else
    mem = NULL;

  TRACE (GLIB_MEM_ALLOC((void*) mem, (unsigned int) n_bytes, 0, 1));

  return mem;
}

/**
 * g_try_malloc0:
 * @n_bytes: number of bytes to allocate
 * 
 * Attempts to allocate @n_bytes, initialized to 0's, and returns %NULL on
 * failure. Contrast with g_malloc0(), which aborts the program on failure.
 * 
 * Since: 2.8
 * Returns: the allocated memory, or %NULL
 */
gpointer
g_try_malloc0 (gsize n_bytes)
{
  gpointer mem;

  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    mem = glib_mem_vtable.try_malloc (n_bytes);
  else
    mem = NULL;

  if (mem)
    memset (mem, 0, n_bytes);

  return mem;
}

/**
 * g_try_realloc:
 * @mem: previously-allocated memory, or %NULL.
 * @n_bytes: number of bytes to allocate.
 * 
 * Attempts to realloc @mem to a new size, @n_bytes, and returns %NULL
 * on failure. Contrast with g_realloc(), which aborts the program
 * on failure. If @mem is %NULL, behaves the same as g_try_malloc().
 * 
 * Returns: the allocated memory, or %NULL.
 */
gpointer
g_try_realloc (gpointer mem,
	       gsize    n_bytes)
{
  gpointer newmem;

  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();
  if (G_LIKELY (n_bytes))
    newmem = glib_mem_vtable.try_realloc (mem, n_bytes);
  else
    {
      newmem = NULL;
      if (mem)
	glib_mem_vtable.free (mem);
    }

  TRACE (GLIB_MEM_REALLOC((void*) newmem, (void*)mem, (unsigned int) n_bytes, 1));

  return newmem;
}


#define SIZE_OVERFLOWS(a,b) (G_UNLIKELY ((b) > 0 && (a) > G_MAXSIZE / (b)))

/**
 * g_malloc_n:
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_malloc(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: a pointer to the allocated memory
 */
gpointer
g_malloc_n (gsize n_blocks,
	    gsize n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    {
      if (G_UNLIKELY (!g_mem_initialized))
	g_mem_init_nomessage();

      g_error ("%s: overflow allocating %"G_GSIZE_FORMAT"*%"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_blocks, n_block_bytes);
    }

  return g_malloc (n_blocks * n_block_bytes);
}

/**
 * g_malloc0_n:
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_malloc0(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: a pointer to the allocated memory
 */
gpointer
g_malloc0_n (gsize n_blocks,
	     gsize n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    {
      if (G_UNLIKELY (!g_mem_initialized))
	g_mem_init_nomessage();

      g_error ("%s: overflow allocating %"G_GSIZE_FORMAT"*%"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_blocks, n_block_bytes);
    }

  return g_malloc0 (n_blocks * n_block_bytes);
}

/**
 * g_realloc_n:
 * @mem: the memory to reallocate
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_realloc(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: the new address of the allocated memory
 */
gpointer
g_realloc_n (gpointer mem,
	     gsize    n_blocks,
	     gsize    n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    {
      if (G_UNLIKELY (!g_mem_initialized))
	g_mem_init_nomessage();

      g_error ("%s: overflow allocating %"G_GSIZE_FORMAT"*%"G_GSIZE_FORMAT" bytes",
               G_STRLOC, n_blocks, n_block_bytes);
    }

  return g_realloc (mem, n_blocks * n_block_bytes);
}

/**
 * g_try_malloc_n:
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_try_malloc(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: the allocated memory, or %NULL.
 */
gpointer
g_try_malloc_n (gsize n_blocks,
		gsize n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    return NULL;

  return g_try_malloc (n_blocks * n_block_bytes);
}

/**
 * g_try_malloc0_n:
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_try_malloc0(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: the allocated memory, or %NULL
 */
gpointer
g_try_malloc0_n (gsize n_blocks,
		 gsize n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    return NULL;

  return g_try_malloc0 (n_blocks * n_block_bytes);
}

/**
 * g_try_realloc_n:
 * @mem: previously-allocated memory, or %NULL.
 * @n_blocks: the number of blocks to allocate
 * @n_block_bytes: the size of each block in bytes
 * 
 * This function is similar to g_try_realloc(), allocating (@n_blocks * @n_block_bytes) bytes,
 * but care is taken to detect possible overflow during multiplication.
 * 
 * Since: 2.24
 * Returns: the allocated memory, or %NULL.
 */
gpointer
g_try_realloc_n (gpointer mem,
		 gsize    n_blocks,
		 gsize    n_block_bytes)
{
  if (SIZE_OVERFLOWS (n_blocks, n_block_bytes))
    return NULL;

  return g_try_realloc (mem, n_blocks * n_block_bytes);
}



static gpointer
fallback_calloc (gsize n_blocks,
		 gsize n_block_bytes)
{
  gsize l = n_blocks * n_block_bytes;
  gpointer mem = glib_mem_vtable.malloc (l);

  if (mem)
    memset (mem, 0, l);

  return mem;
}

static gboolean vtable_set = FALSE;

/**
 * g_mem_is_system_malloc
 * 
 * Checks whether the allocator used by g_malloc() is the system's
 * malloc implementation. If it returns %TRUE memory allocated with
 * malloc() can be used interchangeable with memory allocated using g_malloc().
 * This function is useful for avoiding an extra copy of allocated memory returned
 * by a non-GLib-based API.
 *
 * A different allocator can be set using g_mem_set_vtable().
 *
 * Return value: if %TRUE, malloc() and g_malloc() can be mixed.
 **/
gboolean
g_mem_is_system_malloc (void)
{
  return !vtable_set;
}

/**
 * g_mem_set_vtable:
 * @vtable: table of memory allocation routines.
 * 
 * Sets the #GMemVTable to use for memory allocation. You can use this to provide
 * custom memory allocation routines. <emphasis>This function must be called
 * before using any other GLib functions.</emphasis> The @vtable only needs to
 * provide malloc(), realloc(), and free() functions; GLib can provide default
 * implementations of the others. The malloc() and realloc() implementations
 * should return %NULL on failure, GLib will handle error-checking for you.
 * @vtable is copied, so need not persist after this function has been called.
 */
void
g_mem_set_vtable (GMemVTable *vtable)
{
  if (!vtable_set)
    {
      if (vtable->malloc && vtable->realloc && vtable->free)
	{
	  glib_mem_vtable.malloc = vtable->malloc;
	  glib_mem_vtable.realloc = vtable->realloc;
	  glib_mem_vtable.free = vtable->free;
	  glib_mem_vtable.calloc = vtable->calloc ? vtable->calloc : fallback_calloc;
	  glib_mem_vtable.try_malloc = vtable->try_malloc ? vtable->try_malloc : glib_mem_vtable.malloc;
	  glib_mem_vtable.try_realloc = vtable->try_realloc ? vtable->try_realloc : glib_mem_vtable.realloc;
	  vtable_set = TRUE;
	}
      else
	g_warning (G_STRLOC ": memory allocation vtable lacks one of malloc(), realloc() or free()");
    }
  else
    g_warning (G_STRLOC ": memory allocation vtable can only be set once at startup");
}


/* --- memory profiling and checking --- */
#ifdef	G_DISABLE_CHECKS
/**
 * glib_mem_profiler_table:
 * 
 * A #GMemVTable containing profiling variants of the memory
 * allocation functions. Use them together with g_mem_profile()
 * in order to get information about the memory allocation pattern
 * of your program.
 */
GMemVTable *glib_mem_profiler_table = &glib_mem_vtable;
void
g_mem_profile (void)
{
}
#else	/* !G_DISABLE_CHECKS */
typedef enum {
  PROFILER_FREE		= 0,
  PROFILER_ALLOC	= 1,
  PROFILER_RELOC	= 2,
  PROFILER_ZINIT	= 4
} ProfilerJob;
static guint *profile_data = NULL;
static gsize profile_allocs = 0;
static gsize profile_zinit = 0;
static gsize profile_frees = 0;
static GMutex *gmem_profile_mutex = NULL;
#ifdef  G_ENABLE_DEBUG
static volatile gsize g_trap_free_size = 0;
static volatile gsize g_trap_realloc_size = 0;
static volatile gsize g_trap_malloc_size = 0;
#endif  /* G_ENABLE_DEBUG */

#define	PROFILE_TABLE(f1,f2,f3)   ( ( ((f3) << 2) | ((f2) << 1) | (f1) ) * (MEM_PROFILE_TABLE_SIZE + 1))

static void
profiler_log (ProfilerJob job,
	      gsize       n_bytes,
	      gboolean    success)
{
  g_mutex_lock (gmem_profile_mutex);
  if (!profile_data)
    {
      profile_data = standard_calloc ((MEM_PROFILE_TABLE_SIZE + 1) * 8, 
                                      sizeof (profile_data[0]));
      if (!profile_data)	/* memory system kiddin' me, eh? */
	{
	  g_mutex_unlock (gmem_profile_mutex);
	  return;
	}
    }

  if (n_bytes < MEM_PROFILE_TABLE_SIZE)
    profile_data[n_bytes + PROFILE_TABLE ((job & PROFILER_ALLOC) != 0,
                                          (job & PROFILER_RELOC) != 0,
                                          success != 0)] += 1;
  else
    profile_data[MEM_PROFILE_TABLE_SIZE + PROFILE_TABLE ((job & PROFILER_ALLOC) != 0,
                                                         (job & PROFILER_RELOC) != 0,
                                                         success != 0)] += 1;
  if (success)
    {
      if (job & PROFILER_ALLOC)
        {
          profile_allocs += n_bytes;
          if (job & PROFILER_ZINIT)
            profile_zinit += n_bytes;
        }
      else
        profile_frees += n_bytes;
    }
  g_mutex_unlock (gmem_profile_mutex);
}

static void
profile_print_locked (guint   *local_data,
		      gboolean success)
{
  gboolean need_header = TRUE;
  guint i;

  for (i = 0; i <= MEM_PROFILE_TABLE_SIZE; i++)
    {
      glong t_malloc = local_data[i + PROFILE_TABLE (1, 0, success)];
      glong t_realloc = local_data[i + PROFILE_TABLE (1, 1, success)];
      glong t_free = local_data[i + PROFILE_TABLE (0, 0, success)];
      glong t_refree = local_data[i + PROFILE_TABLE (0, 1, success)];
      
      if (!t_malloc && !t_realloc && !t_free && !t_refree)
	continue;
      else if (need_header)
	{
	  need_header = FALSE;
	  g_print (" blocks of | allocated  | freed      | allocated  | freed      | n_bytes   \n");
	  g_print ("  n_bytes  | n_times by | n_times by | n_times by | n_times by | remaining \n");
	  g_print ("           | malloc()   | free()     | realloc()  | realloc()  |           \n");
	  g_print ("===========|============|============|============|============|===========\n");
	}
      if (i < MEM_PROFILE_TABLE_SIZE)
	g_print ("%10u | %10ld | %10ld | %10ld | %10ld |%+11ld\n",
		 i, t_malloc, t_free, t_realloc, t_refree,
		 (t_malloc - t_free + t_realloc - t_refree) * i);
      else if (i >= MEM_PROFILE_TABLE_SIZE)
	g_print ("   >%6u | %10ld | %10ld | %10ld | %10ld |        ***\n",
		 i, t_malloc, t_free, t_realloc, t_refree);
    }
  if (need_header)
    g_print (" --- none ---\n");
}

/**
 * g_mem_profile:
 * @void:
 * 
 * Outputs a summary of memory usage.
 * 
 * It outputs the frequency of allocations of different sizes,
 * the total number of bytes which have been allocated,
 * the total number of bytes which have been freed,
 * and the difference between the previous two values, i.e. the number of bytes
 * still in use.
 * 
 * Note that this function will not output anything unless you have
 * previously installed the #glib_mem_profiler_table with g_mem_set_vtable().
 */

void
g_mem_profile (void)
{
  guint local_data[(MEM_PROFILE_TABLE_SIZE + 1) * 8 * sizeof (profile_data[0])];
  gsize local_allocs;
  gsize local_zinit;
  gsize local_frees;

  if (G_UNLIKELY (!g_mem_initialized))
    g_mem_init_nomessage();

  g_mutex_lock (gmem_profile_mutex);

  local_allocs = profile_allocs;
  local_zinit = profile_zinit;
  local_frees = profile_frees;

  if (!profile_data)
    {
      g_mutex_unlock (gmem_profile_mutex);
      return;
    }

  memcpy (local_data, profile_data, 
	  (MEM_PROFILE_TABLE_SIZE + 1) * 8 * sizeof (profile_data[0]));
  
  g_mutex_unlock (gmem_profile_mutex);

  g_print ("GLib Memory statistics (successful operations):\n");
  profile_print_locked (local_data, TRUE);
  g_print ("GLib Memory statistics (failing operations):\n");
  profile_print_locked (local_data, FALSE);
  g_print ("Total bytes: allocated=%"G_GSIZE_FORMAT", "
           "zero-initialized=%"G_GSIZE_FORMAT" (%.2f%%), "
           "freed=%"G_GSIZE_FORMAT" (%.2f%%), "
           "remaining=%"G_GSIZE_FORMAT"\n",
	   local_allocs,
	   local_zinit,
	   ((gdouble) local_zinit) / local_allocs * 100.0,
	   local_frees,
	   ((gdouble) local_frees) / local_allocs * 100.0,
	   local_allocs - local_frees);
}

static gpointer
profiler_try_malloc (gsize n_bytes)
{
  gsize *p;

#ifdef  G_ENABLE_DEBUG
  if (g_trap_malloc_size == n_bytes)
    G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */

  p = standard_malloc (sizeof (gsize) * 2 + n_bytes);

  if (p)
    {
      p[0] = 0;		/* free count */
      p[1] = n_bytes;	/* length */
      profiler_log (PROFILER_ALLOC, n_bytes, TRUE);
      p += 2;
    }
  else
    profiler_log (PROFILER_ALLOC, n_bytes, FALSE);
  
  return p;
}

static gpointer
profiler_malloc (gsize n_bytes)
{
  gpointer mem = profiler_try_malloc (n_bytes);

  if (!mem)
    g_mem_profile ();

  return mem;
}

static gpointer
profiler_calloc (gsize n_blocks,
		 gsize n_block_bytes)
{
  gsize l = n_blocks * n_block_bytes;
  gsize *p;

#ifdef  G_ENABLE_DEBUG
  if (g_trap_malloc_size == l)
    G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */
  
  p = standard_calloc (1, sizeof (gsize) * 2 + l);

  if (p)
    {
      p[0] = 0;		/* free count */
      p[1] = l;		/* length */
      profiler_log (PROFILER_ALLOC | PROFILER_ZINIT, l, TRUE);
      p += 2;
    }
  else
    {
      profiler_log (PROFILER_ALLOC | PROFILER_ZINIT, l, FALSE);
      g_mem_profile ();
    }

  return p;
}

static void
profiler_free (gpointer mem)
{
  gsize *p = mem;

  p -= 2;
  if (p[0])	/* free count */
    {
      g_warning ("free(%p): memory has been freed %"G_GSIZE_FORMAT" times already",
                 p + 2, p[0]);
      profiler_log (PROFILER_FREE,
		    p[1],	/* length */
		    FALSE);
    }
  else
    {
#ifdef  G_ENABLE_DEBUG
      if (g_trap_free_size == p[1])
	G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */

      profiler_log (PROFILER_FREE,
		    p[1],	/* length */
		    TRUE);
      memset (p + 2, 0xaa, p[1]);

      /* for all those that miss standard_free (p); in this place, yes,
       * we do leak all memory when profiling, and that is intentional
       * to catch double frees. patch submissions are futile.
       */
    }
  p[0] += 1;
}

static gpointer
profiler_try_realloc (gpointer mem,
		      gsize    n_bytes)
{
  gsize *p = mem;

  p -= 2;

#ifdef  G_ENABLE_DEBUG
  if (g_trap_realloc_size == n_bytes)
    G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */
  
  if (mem && p[0])	/* free count */
    {
      g_warning ("realloc(%p, %"G_GSIZE_FORMAT"): "
                 "memory has been freed %"G_GSIZE_FORMAT" times already",
                 p + 2, (gsize) n_bytes, p[0]);
      profiler_log (PROFILER_ALLOC | PROFILER_RELOC, n_bytes, FALSE);

      return NULL;
    }
  else
    {
      p = standard_realloc (mem ? p : NULL, sizeof (gsize) * 2 + n_bytes);

      if (p)
	{
	  if (mem)
	    profiler_log (PROFILER_FREE | PROFILER_RELOC, p[1], TRUE);
	  p[0] = 0;
	  p[1] = n_bytes;
	  profiler_log (PROFILER_ALLOC | PROFILER_RELOC, p[1], TRUE);
	  p += 2;
	}
      else
	profiler_log (PROFILER_ALLOC | PROFILER_RELOC, n_bytes, FALSE);

      return p;
    }
}

static gpointer
profiler_realloc (gpointer mem,
		  gsize    n_bytes)
{
  mem = profiler_try_realloc (mem, n_bytes);

  if (!mem)
    g_mem_profile ();

  return mem;
}

static GMemVTable profiler_table = {
  profiler_malloc,
  profiler_realloc,
  profiler_free,
  profiler_calloc,
  profiler_try_malloc,
  profiler_try_realloc,
};
GMemVTable *glib_mem_profiler_table = &profiler_table;

#endif	/* !G_DISABLE_CHECKS */

/* --- MemChunks --- */
/**
 * SECTION: allocators
 * @title: Memory Allocators
 * @short_description: deprecated way to allocate chunks of memory for
 *                     GList, GSList and GNode
 *
 * Prior to 2.10, #GAllocator was used as an efficient way to allocate
 * small pieces of memory for use with the #GList, #GSList and #GNode
 * data structures. Since 2.10, it has been completely replaced by the
 * <link linkend="glib-Memory-Slices">slice allocator</link> and
 * deprecated.
 **/

/**
 * SECTION: memory_chunks
 * @title: Memory Chunks
 * @short_description: deprecated way to allocate groups of equal-sized
 *                     chunks of memory
 *
 * Memory chunks provide an space-efficient way to allocate equal-sized
 * pieces of memory, called atoms. However, due to the administrative
 * overhead (in particular for #G_ALLOC_AND_FREE, and when used from
 * multiple threads), they are in practise often slower than direct use
 * of g_malloc(). Therefore, memory chunks have been deprecated in
 * favor of the <link linkend="glib-Memory-Slices">slice
 * allocator</link>, which has been added in 2.10. All internal uses of
 * memory chunks in GLib have been converted to the
 * <literal>g_slice</literal> API.
 *
 * There are two types of memory chunks, #G_ALLOC_ONLY, and
 * #G_ALLOC_AND_FREE. <itemizedlist> <listitem><para> #G_ALLOC_ONLY
 * chunks only allow allocation of atoms. The atoms can never be freed
 * individually. The memory chunk can only be free in its entirety.
 * </para></listitem> <listitem><para> #G_ALLOC_AND_FREE chunks do
 * allow atoms to be freed individually. The disadvantage of this is
 * that the memory chunk has to keep track of which atoms have been
 * freed. This results in more memory being used and a slight
 * degradation in performance. </para></listitem> </itemizedlist>
 *
 * To create a memory chunk use g_mem_chunk_new() or the convenience
 * macro g_mem_chunk_create().
 *
 * To allocate a new atom use g_mem_chunk_alloc(),
 * g_mem_chunk_alloc0(), or the convenience macros g_chunk_new() or
 * g_chunk_new0().
 *
 * To free an atom use g_mem_chunk_free(), or the convenience macro
 * g_chunk_free(). (Atoms can only be freed if the memory chunk is
 * created with the type set to #G_ALLOC_AND_FREE.)
 *
 * To free any blocks of memory which are no longer being used, use
 * g_mem_chunk_clean(). To clean all memory chunks, use g_blow_chunks().
 *
 * To reset the memory chunk, freeing all of the atoms, use
 * g_mem_chunk_reset().
 *
 * To destroy a memory chunk, use g_mem_chunk_destroy().
 *
 * To help debug memory chunks, use g_mem_chunk_info() and
 * g_mem_chunk_print().
 *
 * <example>
 *  <title>Using a #GMemChunk</title>
 *  <programlisting>
 *   GMemChunk *mem_chunk;
 *   gchar *mem[10000];
 *   gint i;
 *
 *   /<!-- -->* Create a GMemChunk with atoms 50 bytes long, and memory
 *      blocks holding 100 bytes. Note that this means that only 2 atoms
 *      fit into each memory block and so isn't very efficient. *<!-- -->/
 *   mem_chunk = g_mem_chunk_new ("test mem chunk", 50, 100, G_ALLOC_AND_FREE);
 *   /<!-- -->* Now allocate 10000 atoms. *<!-- -->/
 *   for (i = 0; i &lt; 10000; i++)
 *     {
 *       mem[i] = g_chunk_new (gchar, mem_chunk);
 *       /<!-- -->* Fill in the atom memory with some junk. *<!-- -->/
 *       for (j = 0; j &lt; 50; j++)
 *         mem[i][j] = i * j;
 *     }
 *   /<!-- -->* Now free all of the atoms. Note that since we are going to
 *      destroy the GMemChunk, this wouldn't normally be used. *<!-- -->/
 *   for (i = 0; i &lt; 10000; i++)
 *     {
 *       g_mem_chunk_free (mem_chunk, mem[i]);
 *     }
 *   /<!-- -->* We are finished with the GMemChunk, so we destroy it. *<!-- -->/
 *   g_mem_chunk_destroy (mem_chunk);
 *  </programlisting>
 * </example>
 *
 * <example>
 *  <title>Using a #GMemChunk with data structures</title>
 *  <programlisting>
 *    GMemChunk *array_mem_chunk;
 *    GRealArray *array;
 *    /<!-- -->* Create a GMemChunk to hold GRealArray structures, using
 *       the g_mem_chunk_create(<!-- -->) convenience macro. We want 1024 atoms in each
 *       memory block, and we want to be able to free individual atoms. *<!-- -->/
 *    array_mem_chunk = g_mem_chunk_create (GRealArray, 1024, G_ALLOC_AND_FREE);
 *    /<!-- -->* Allocate one atom, using the g_chunk_new(<!-- -->) convenience macro. *<!-- -->/
 *    array = g_chunk_new (GRealArray, array_mem_chunk);
 *    /<!-- -->* We can now use array just like a normal pointer to a structure. *<!-- -->/
 *    array->data            = NULL;
 *    array->len             = 0;
 *    array->alloc           = 0;
 *    array->zero_terminated = (zero_terminated ? 1 : 0);
 *    array->clear           = (clear ? 1 : 0);
 *    array->elt_size        = elt_size;
 *    /<!-- -->* We can free the element, so it can be reused. *<!-- -->/
 *    g_chunk_free (array, array_mem_chunk);
 *    /<!-- -->* We destroy the GMemChunk when we are finished with it. *<!-- -->/
 *    g_mem_chunk_destroy (array_mem_chunk);
 *  </programlisting>
 * </example>
 **/

#ifndef G_ALLOC_AND_FREE

/**
 * GAllocator:
 *
 * The #GAllocator struct contains private data. and should only be
 * accessed using the following functions.
 **/
typedef struct _GAllocator GAllocator;

/**
 * GMemChunk:
 *
 * The #GMemChunk struct is an opaque data structure representing a
 * memory chunk. It should be accessed only through the use of the
 * following functions.
 **/
typedef struct _GMemChunk  GMemChunk;

/**
 * G_ALLOC_ONLY:
 *
 * Specifies the type of a #GMemChunk. Used in g_mem_chunk_new() and
 * g_mem_chunk_create() to specify that atoms will never be freed
 * individually.
 **/
#define G_ALLOC_ONLY	  1

/**
 * G_ALLOC_AND_FREE:
 *
 * Specifies the type of a #GMemChunk. Used in g_mem_chunk_new() and
 * g_mem_chunk_create() to specify that atoms will be freed
 * individually.
 **/
#define G_ALLOC_AND_FREE  2
#endif

struct _GMemChunk {
  guint alloc_size;           /* the size of an atom */
};

/**
 * g_mem_chunk_new:
 * @name: a string to identify the #GMemChunk. It is not copied so it
 *        should be valid for the lifetime of the #GMemChunk. It is
 *        only used in g_mem_chunk_print(), which is used for debugging.
 * @atom_size: the size, in bytes, of each element in the #GMemChunk.
 * @area_size: the size, in bytes, of each block of memory allocated to
 *             contain the atoms.
 * @type: the type of the #GMemChunk.  #G_ALLOC_AND_FREE is used if the
 *        atoms will be freed individually.  #G_ALLOC_ONLY should be
 *        used if atoms will never be freed individually.
 *        #G_ALLOC_ONLY is quicker, since it does not need to track
 *        free atoms, but it obviously wastes memory if you no longer
 *        need many of the atoms.
 * @Returns: the new #GMemChunk.
 *
 * Creates a new #GMemChunk.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
GMemChunk*
g_mem_chunk_new (const gchar  *name,
		 gint          atom_size,
		 gsize         area_size,
		 gint          type)
{
  GMemChunk *mem_chunk;
  g_return_val_if_fail (atom_size > 0, NULL);

  mem_chunk = g_slice_new (GMemChunk);
  mem_chunk->alloc_size = atom_size;
  return mem_chunk;
}

/**
 * g_mem_chunk_destroy:
 * @mem_chunk: a #GMemChunk.
 *
 * Frees all of the memory allocated for a #GMemChunk.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void
g_mem_chunk_destroy (GMemChunk *mem_chunk)
{
  g_return_if_fail (mem_chunk != NULL);
  
  g_slice_free (GMemChunk, mem_chunk);
}

/**
 * g_mem_chunk_alloc:
 * @mem_chunk: a #GMemChunk.
 * @Returns: a pointer to the allocated atom.
 *
 * Allocates an atom of memory from a #GMemChunk.
 *
 * Deprecated:2.10: Use g_slice_alloc() instead
 **/
gpointer
g_mem_chunk_alloc (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);
  
  return g_slice_alloc (mem_chunk->alloc_size);
}

/**
 * g_mem_chunk_alloc0:
 * @mem_chunk: a #GMemChunk.
 * @Returns: a pointer to the allocated atom.
 *
 * Allocates an atom of memory from a #GMemChunk, setting the memory to
 * 0.
 *
 * Deprecated:2.10: Use g_slice_alloc0() instead
 **/
gpointer
g_mem_chunk_alloc0 (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);
  
  return g_slice_alloc0 (mem_chunk->alloc_size);
}

/**
 * g_mem_chunk_free:
 * @mem_chunk: a #GMemChunk.
 * @mem: a pointer to the atom to free.
 *
 * Frees an atom in a #GMemChunk. This should only be called if the
 * #GMemChunk was created with #G_ALLOC_AND_FREE. Otherwise it will
 * simply return.
 *
 * Deprecated:2.10: Use g_slice_free1() instead
 **/
void
g_mem_chunk_free (GMemChunk *mem_chunk,
		  gpointer   mem)
{
  g_return_if_fail (mem_chunk != NULL);
  
  g_slice_free1 (mem_chunk->alloc_size, mem);
}

/**
 * g_mem_chunk_clean:
 * @mem_chunk: a #GMemChunk.
 *
 * Frees any blocks in a #GMemChunk which are no longer being used.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void	g_mem_chunk_clean	(GMemChunk *mem_chunk)	{}

/**
 * g_mem_chunk_reset:
 * @mem_chunk: a #GMemChunk.
 *
 * Resets a GMemChunk to its initial state. It frees all of the
 * currently allocated blocks of memory.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void	g_mem_chunk_reset	(GMemChunk *mem_chunk)	{}


/**
 * g_mem_chunk_print:
 * @mem_chunk: a #GMemChunk.
 *
 * Outputs debugging information for a #GMemChunk. It outputs the name
 * of the #GMemChunk (set with g_mem_chunk_new()), the number of bytes
 * used, and the number of blocks of memory allocated.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void	g_mem_chunk_print	(GMemChunk *mem_chunk)	{}


/**
 * g_mem_chunk_info:
 *
 * Outputs debugging information for all #GMemChunk objects currently
 * in use. It outputs the number of #GMemChunk objects currently
 * allocated, and calls g_mem_chunk_print() to output information on
 * each one.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void	g_mem_chunk_info	(void)			{}

/**
 * g_blow_chunks:
 *
 * Calls g_mem_chunk_clean() on all #GMemChunk objects.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void	g_blow_chunks		(void)			{}

/**
 * g_chunk_new0:
 * @type: the type of the #GMemChunk atoms, typically a structure name.
 * @chunk: a #GMemChunk.
 * @Returns: a pointer to the allocated atom, cast to a pointer to
 *           @type.
 *
 * A convenience macro to allocate an atom of memory from a #GMemChunk.
 * It calls g_mem_chunk_alloc0() and casts the returned atom to a
 * pointer to the given type, avoiding a type cast in the source code.
 *
 * Deprecated:2.10: Use g_slice_new0() instead
 **/

/**
 * g_chunk_free:
 * @mem: a pointer to the atom to be freed.
 * @mem_chunk: a #GMemChunk.
 *
 * A convenience macro to free an atom of memory from a #GMemChunk. It
 * simply switches the arguments and calls g_mem_chunk_free() It is
 * included simply to complement the other convenience macros,
 * g_chunk_new() and g_chunk_new0().
 *
 * Deprecated:2.10: Use g_slice_free() instead
 **/

/**
 * g_chunk_new:
 * @type: the type of the #GMemChunk atoms, typically a structure name.
 * @chunk: a #GMemChunk.
 * @Returns: a pointer to the allocated atom, cast to a pointer to
 *           @type.
 *
 * A convenience macro to allocate an atom of memory from a #GMemChunk.
 * It calls g_mem_chunk_alloc() and casts the returned atom to a
 * pointer to the given type, avoiding a type cast in the source code.
 *
 * Deprecated:2.10: Use g_slice_new() instead
 **/

/**
 * g_mem_chunk_create:
 * @type: the type of the atoms, typically a structure name.
 * @pre_alloc: the number of atoms to store in each block of memory.
 * @alloc_type: the type of the #GMemChunk.  #G_ALLOC_AND_FREE is used
 *              if the atoms will be freed individually.  #G_ALLOC_ONLY
 *              should be used if atoms will never be freed
 *              individually.  #G_ALLOC_ONLY is quicker, since it does
 *              not need to track free atoms, but it obviously wastes
 *              memory if you no longer need many of the atoms.
 * @Returns: the new #GMemChunk.
 *
 * A convenience macro for creating a new #GMemChunk. It calls
 * g_mem_chunk_new(), using the given type to create the #GMemChunk
 * name. The atom size is determined using
 * <function>sizeof()</function>, and the area size is calculated by
 * multiplying the @pre_alloc parameter with the atom size.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/


/**
 * g_allocator_new:
 * @name: the name of the #GAllocator. This name is used to set the
 *        name of the #GMemChunk used by the #GAllocator, and is only
 *        used for debugging.
 * @n_preallocs: the number of elements in each block of memory
 *               allocated.  Larger blocks mean less calls to
 *               g_malloc(), but some memory may be wasted.  (GLib uses
 *               128 elements per block by default.) The value must be
 *               between 1 and 65535.
 * @Returns: a new #GAllocator.
 *
 * Creates a new #GAllocator.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
GAllocator*
g_allocator_new (const gchar *name,
		 guint        n_preallocs)
{
  static struct _GAllocator {
    gchar      *name;
    guint16     n_preallocs;
    guint       is_unused : 1;
    guint       type : 4;
    GAllocator *last;
    GMemChunk  *mem_chunk;
    gpointer    free_list;
  } dummy = {
    "GAllocator is deprecated", 1, TRUE, 0, NULL, NULL, NULL,
  };
  /* some (broken) GAllocator uses depend on non-NULL allocators */
  return (void*) &dummy;
}

/**
 * g_allocator_free:
 * @allocator: a #GAllocator.
 *
 * Frees all of the memory allocated by the #GAllocator.
 *
 * Deprecated:2.10: Use the <link linkend="glib-Memory-Slices">slice
 *                  allocator</link> instead
 **/
void
g_allocator_free (GAllocator *allocator)
{
}

#ifdef ENABLE_GC_FRIENDLY_DEFAULT
gboolean g_mem_gc_friendly = TRUE;
#else
/**
 * g_mem_gc_friendly:
 * 
 * This variable is %TRUE if the <envar>G_DEBUG</envar> environment variable
 * includes the key <link linkend="G_DEBUG">gc-friendly</link>.
 */
gboolean g_mem_gc_friendly = FALSE;
#endif

static void
g_mem_init_nomessage (void)
{
  gchar buffer[1024];
  const gchar *val;
  const GDebugKey keys[] = {
    { "gc-friendly", 1 },
  };
  gint flags;
  if (g_mem_initialized)
    return;
  /* don't use g_malloc/g_message here */
  val = _g_getenv_nomalloc ("G_DEBUG", buffer);
  flags = !val ? 0 : g_parse_debug_string (val, keys, G_N_ELEMENTS (keys));
  if (flags & 1)        /* gc-friendly */
    {
      g_mem_gc_friendly = TRUE;
    }
  g_mem_initialized = TRUE;
}

void
_g_mem_thread_init_noprivate_nomessage (void)
{
  /* we may only create mutexes here, locking/
   * unlocking a mutex does not yet work.
   */
  g_mem_init_nomessage();
#ifndef G_DISABLE_CHECKS
  gmem_profile_mutex = g_mutex_new ();
#endif
}
