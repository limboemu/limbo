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

#include <string.h>
#include <stdlib.h>

#include "garray.h"

#include "gmem.h"
#include "gthread.h"
#include "gmessages.h"
#include "gqsort.h"


/**
 * SECTION: arrays
 * @title: Arrays
 * @short_description: arrays of arbitrary elements which grow
 *                     automatically as elements are added
 *
 * Arrays are similar to standard C arrays, except that they grow
 * automatically as elements are added.
 *
 * Array elements can be of any size (though all elements of one array
 * are the same size), and the array can be automatically cleared to
 * '0's and zero-terminated.
 *
 * To create a new array use g_array_new().
 *
 * To add elements to an array, use g_array_append_val(),
 * g_array_append_vals(), g_array_prepend_val(), and
 * g_array_prepend_vals().
 *
 * To access an element of an array, use g_array_index().
 *
 * To set the size of an array, use g_array_set_size().
 *
 * To free an array, use g_array_free().
 *
 * <example>
 *  <title>Using a #GArray to store #gint values</title>
 *  <programlisting>
 *   GArray *garray;
 *   gint i;
 *   /<!-- -->* We create a new array to store gint values.
 *      We don't want it zero-terminated or cleared to 0's. *<!-- -->/
 *   garray = g_array_new (FALSE, FALSE, sizeof (gint));
 *   for (i = 0; i &lt; 10000; i++)
 *     g_array_append_val (garray, i);
 *   for (i = 0; i &lt; 10000; i++)
 *     if (g_array_index (garray, gint, i) != i)
 *       g_print ("ERROR: got &percnt;d instead of &percnt;d\n",
 *                g_array_index (garray, gint, i), i);
 *   g_array_free (garray, TRUE);
 *  </programlisting>
 * </example>
 **/

#define MIN_ARRAY_SIZE  16

typedef struct _GRealArray  GRealArray;

/**
 * GArray:
 * @data: a pointer to the element data. The data may be moved as
 *        elements are added to the #GArray.
 * @len: the number of elements in the #GArray not including the
 *       possible terminating zero element.
 *
 * Contains the public fields of an <link
 * linkend="glib-arrays">Array</link>.
 **/
struct _GRealArray
{
  guint8 *data;
  guint   len;
  guint   alloc;
  guint   elt_size;
  guint   zero_terminated : 1;
  guint   clear : 1;
  volatile gint ref_count;
};

/**
 * g_array_index:
 * @a: a #GArray.
 * @t: the type of the elements.
 * @i: the index of the element to return.
 * @Returns: the element of the #GArray at the index given by @i.
 *
 * Returns the element of a #GArray at the given index. The return
 * value is cast to the given type.
 *
 * <example>
 *  <title>Getting a pointer to an element in a #GArray</title>
 *  <programlisting>
 *   EDayViewEvent *event;
 *   /<!-- -->* This gets a pointer to the 4th element
 *      in the array of EDayViewEvent structs. *<!-- -->/
 *   event = &amp;g_array_index (events, EDayViewEvent, 3);
 *  </programlisting>
 * </example>
 **/

#define g_array_elt_len(array,i) ((array)->elt_size * (i))
#define g_array_elt_pos(array,i) ((array)->data + g_array_elt_len((array),(i)))
#define g_array_elt_zero(array, pos, len) 				\
  (memset (g_array_elt_pos ((array), pos), 0,  g_array_elt_len ((array), len)))
#define g_array_zero_terminate(array) G_STMT_START{			\
  if ((array)->zero_terminated)						\
    g_array_elt_zero ((array), (array)->len, 1);			\
}G_STMT_END

static guint g_nearest_pow        (gint        num) G_GNUC_CONST;
static void  g_array_maybe_expand (GRealArray *array,
				   gint        len);

/**
 * g_array_new:
 * @zero_terminated: %TRUE if the array should have an extra element at
 *                   the end which is set to 0.
 * @clear_: %TRUE if #GArray elements should be automatically cleared
 *          to 0 when they are allocated.
 * @element_size: the size of each element in bytes.
 * @Returns: the new #GArray.
 *
 * Creates a new #GArray with a reference count of 1.
 **/
GArray*
g_array_new (gboolean zero_terminated,
	     gboolean clear,
	     guint    elt_size)
{
  return (GArray*) g_array_sized_new (zero_terminated, clear, elt_size, 0);
}

/**
 * g_array_sized_new:
 * @zero_terminated: %TRUE if the array should have an extra element at
 *                   the end with all bits cleared.
 * @clear_: %TRUE if all bits in the array should be cleared to 0 on
 *          allocation.
 * @element_size: size of each element in the array.
 * @reserved_size: number of elements preallocated.
 * @Returns: the new #GArray.
 *
 * Creates a new #GArray with @reserved_size elements preallocated and
 * a reference count of 1. This avoids frequent reallocation, if you
 * are going to add many elements to the array. Note however that the
 * size of the array is still 0.
 **/
GArray* g_array_sized_new (gboolean zero_terminated,
			   gboolean clear,
			   guint    elt_size,
			   guint    reserved_size)
{
  GRealArray *array = g_slice_new (GRealArray);

  array->data            = NULL;
  array->len             = 0;
  array->alloc           = 0;
  array->zero_terminated = (zero_terminated ? 1 : 0);
  array->clear           = (clear ? 1 : 0);
  array->elt_size        = elt_size;
  array->ref_count       = 1;

  if (array->zero_terminated || reserved_size != 0)
    {
      g_array_maybe_expand (array, reserved_size);
      g_array_zero_terminate(array);
    }

  return (GArray*) array;
}

/**
 * g_array_ref:
 * @array: A #GArray.
 *
 * Atomically increments the reference count of @array by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #GArray.
 *
 * Since: 2.22
 **/
GArray *
g_array_ref (GArray *array)
{
  GRealArray *rarray = (GRealArray*) array;
  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (g_atomic_int_get (&rarray->ref_count) > 0, array);
  g_atomic_int_inc (&rarray->ref_count);
  return array;
}

/**
 * g_array_unref:
 * @array: A #GArray.
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, all memory allocated by the array is
 * released. This function is MT-safe and may be called from any
 * thread.
 *
 * Since: 2.22
 **/
void
g_array_unref (GArray *array)
{
  GRealArray *rarray = (GRealArray*) array;
  g_return_if_fail (array);
  g_return_if_fail (g_atomic_int_get (&rarray->ref_count) > 0);
  if (g_atomic_int_dec_and_test (&rarray->ref_count))
    g_array_free (array, TRUE);
}

/**
 * g_array_get_element_size:
 * @array: A #GArray.
 *
 * Gets the size of the elements in @array.
 *
 * Returns: Size of each element, in bytes.
 *
 * Since: 2.22
 **/
guint
g_array_get_element_size (GArray *array)
{
  GRealArray *rarray = (GRealArray*) array;

  g_return_val_if_fail (array, 0);

  return rarray->elt_size;
}

/**
 * g_array_free:
 * @array: a #GArray.
 * @free_segment: if %TRUE the actual element data is freed as well.
 * @Returns: the element data if @free_segment is %FALSE, otherwise
 *           %NULL.  The element data should be freed using g_free().
 *
 * Frees the memory allocated for the #GArray. If @free_segment is
 * %TRUE it frees the memory block holding the elements as well and
 * also each element if @array has a @element_free_func set. Pass
 * %FALSE if you want to free the #GArray wrapper but preserve the
 * underlying array for use elsewhere. If the reference count of @array
 * is greater than one, the #GArray wrapper is preserved but the size
 * of @array will be set to zero.
 *
 * <note><para>If array elements contain dynamically-allocated memory,
 * they should be freed separately.</para></note>
 **/
gchar*
g_array_free (GArray   *farray,
	      gboolean  free_segment)
{
  GRealArray *array = (GRealArray*) farray;
  gchar* segment;
  gboolean preserve_wrapper;

  g_return_val_if_fail (array, NULL);

  /* if others are holding a reference, preserve the wrapper but do free/return the data */
  preserve_wrapper = FALSE;
  if (g_atomic_int_get (&array->ref_count) > 1)
    preserve_wrapper = TRUE;

  if (free_segment)
    {
      g_free (array->data);
      segment = NULL;
    }
  else
    segment = (gchar*) array->data;

  if (preserve_wrapper)
    {
      array->data            = NULL;
      array->len             = 0;
      array->alloc           = 0;
    }
  else
    {
      g_slice_free1 (sizeof (GRealArray), array);
    }

  return segment;
}

/**
 * g_array_append_vals:
 * @array: a #GArray.
 * @data: a pointer to the elements to append to the end of the array.
 * @len: the number of elements to append.
 * @Returns: the #GArray.
 *
 * Adds @len elements onto the end of the array.
 **/
/**
 * g_array_append_val:
 * @a: a #GArray.
 * @v: the value to append to the #GArray.
 * @Returns: the #GArray.
 *
 * Adds the value on to the end of the array. The array will grow in
 * size automatically if necessary.
 *
 * <note><para>g_array_append_val() is a macro which uses a reference
 * to the value parameter @v. This means that you cannot use it with
 * literal values such as "27". You must use variables.</para></note>
 **/
GArray*
g_array_append_vals (GArray       *farray,
		     gconstpointer data,
		     guint         len)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_array_maybe_expand (array, len);

  memcpy (g_array_elt_pos (array, array->len), data, 
	  g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_prepend_vals:
 * @array: a #GArray.
 * @data: a pointer to the elements to prepend to the start of the
 *        array.
 * @len: the number of elements to prepend.
 * @Returns: the #GArray.
 *
 * Adds @len elements onto the start of the array.
 *
 * This operation is slower than g_array_append_vals() since the
 * existing elements in the array have to be moved to make space for
 * the new elements.
 **/
/**
 * g_array_prepend_val:
 * @a: a #GArray.
 * @v: the value to prepend to the #GArray.
 * @Returns: the #GArray.
 *
 * Adds the value on to the start of the array. The array will grow in
 * size automatically if necessary.
 *
 * This operation is slower than g_array_append_val() since the
 * existing elements in the array have to be moved to make space for
 * the new element.
 *
 * <note><para>g_array_prepend_val() is a macro which uses a reference
 * to the value parameter @v. This means that you cannot use it with
 * literal values such as "27". You must use variables.</para></note>
 **/
GArray*
g_array_prepend_vals (GArray        *farray,
		      gconstpointer  data,
		      guint          len)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_array_maybe_expand (array, len);

  g_memmove (g_array_elt_pos (array, len), g_array_elt_pos (array, 0), 
	     g_array_elt_len (array, array->len));

  memcpy (g_array_elt_pos (array, 0), data, g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_insert_vals:
 * @array: a #GArray.
 * @index_: the index to place the elements at.
 * @data: a pointer to the elements to insert.
 * @len: the number of elements to insert.
 * @Returns: the #GArray.
 *
 * Inserts @len elements into a #GArray at the given index.
 **/
/**
 * g_array_insert_val:
 * @a: a #GArray.
 * @i: the index to place the element at.
 * @v: the value to insert into the array.
 * @Returns: the #GArray.
 *
 * Inserts an element into an array at the given index.
 *
 * <note><para>g_array_insert_val() is a macro which uses a reference
 * to the value parameter @v. This means that you cannot use it with
 * literal values such as "27". You must use variables.</para></note>
 **/
GArray*
g_array_insert_vals (GArray        *farray,
		     guint          index_,
		     gconstpointer  data,
		     guint          len)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_array_maybe_expand (array, len);

  g_memmove (g_array_elt_pos (array, len + index_), 
	     g_array_elt_pos (array, index_), 
	     g_array_elt_len (array, array->len - index_));

  memcpy (g_array_elt_pos (array, index_), data, g_array_elt_len (array, len));

  array->len += len;

  g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_set_size:
 * @array: a #GArray.
 * @length: the new size of the #GArray.
 * @Returns: the #GArray.
 *
 * Sets the size of the array, expanding it if necessary. If the array
 * was created with @clear_ set to %TRUE, the new elements are set to 0.
 **/
GArray*
g_array_set_size (GArray *farray,
		  guint   length)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  if (length > array->len)
    {
      g_array_maybe_expand (array, length - array->len);
      
      if (array->clear)
	g_array_elt_zero (array, array->len, length - array->len);
    }
  else if (G_UNLIKELY (g_mem_gc_friendly) && length < array->len)
    g_array_elt_zero (array, length, array->len - length);
  
  array->len = length;
  
  g_array_zero_terminate (array);
  
  return farray;
}

/**
 * g_array_remove_index:
 * @array: a #GArray.
 * @index_: the index of the element to remove.
 * @Returns: the #GArray.
 *
 * Removes the element at the given index from a #GArray. The following
 * elements are moved down one place.
 **/
GArray*
g_array_remove_index (GArray *farray,
		      guint   index_)
{
  GRealArray* array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  if (index_ != array->len - 1)
    g_memmove (g_array_elt_pos (array, index_),
	       g_array_elt_pos (array, index_ + 1),
	       g_array_elt_len (array, array->len - index_ - 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, 1);
  else
    g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_remove_index_fast:
 * @array: a @GArray.
 * @index_: the index of the element to remove.
 * @Returns: the #GArray.
 *
 * Removes the element at the given index from a #GArray. The last
 * element in the array is used to fill in the space, so this function
 * does not preserve the order of the #GArray. But it is faster than
 * g_array_remove_index().
 **/
GArray*
g_array_remove_index_fast (GArray *farray,
			   guint   index_)
{
  GRealArray* array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  if (index_ != array->len - 1)
    memcpy (g_array_elt_pos (array, index_), 
	    g_array_elt_pos (array, array->len - 1),
	    g_array_elt_len (array, 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, 1);
  else
    g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_remove_range:
 * @array: a @GArray.
 * @index_: the index of the first element to remove.
 * @length: the number of elements to remove.
 * @Returns: the #GArray.
 *
 * Removes the given number of elements starting at the given index
 * from a #GArray.  The following elements are moved to close the gap.
 *
 * Since: 2.4
 **/
GArray*
g_array_remove_range (GArray *farray,
                      guint   index_,
                      guint   length)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (index_ < array->len, NULL);
  g_return_val_if_fail (index_ + length <= array->len, NULL);

  if (index_ + length != array->len)
    g_memmove (g_array_elt_pos (array, index_), 
               g_array_elt_pos (array, index_ + length), 
               (array->len - (index_ + length)) * array->elt_size);

  array->len -= length;
  if (G_UNLIKELY (g_mem_gc_friendly))
    g_array_elt_zero (array, array->len, length);
  else
    g_array_zero_terminate (array);

  return farray;
}

/**
 * g_array_sort:
 * @array: a #GArray.
 * @compare_func: comparison function.
 *
 * Sorts a #GArray using @compare_func which should be a qsort()-style
 * comparison function (returns less than zero for first arg is less
 * than second arg, zero for equal, greater zero if first arg is
 * greater than second arg).
 *
 * If two array elements compare equal, their order in the sorted array
 * is undefined.
 **/
void
g_array_sort (GArray       *farray,
	      GCompareFunc  compare_func)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_if_fail (array != NULL);

  qsort (array->data,
	 array->len,
	 array->elt_size,
	 compare_func);
}

/**
 * g_array_sort_with_data:
 * @array: a #GArray.
 * @compare_func: comparison function.
 * @user_data: data to pass to @compare_func.
 *
 * Like g_array_sort(), but the comparison function receives an extra
 * user data argument.
 **/
void
g_array_sort_with_data (GArray           *farray,
			GCompareDataFunc  compare_func,
			gpointer          user_data)
{
  GRealArray *array = (GRealArray*) farray;

  g_return_if_fail (array != NULL);

  g_qsort_with_data (array->data,
		     array->len,
		     array->elt_size,
		     compare_func,
		     user_data);
}

/* Returns the smallest power of 2 greater than n, or n if
 * such power does not fit in a guint
 */
static guint
g_nearest_pow (gint num)
{
  guint n = 1;

  while (n < num && n > 0)
    n <<= 1;

  return n ? n : num;
}

static void
g_array_maybe_expand (GRealArray *array,
		      gint        len)
{
  guint want_alloc = g_array_elt_len (array, array->len + len + 
				      array->zero_terminated);

  if (want_alloc > array->alloc)
    {
      want_alloc = g_nearest_pow (want_alloc);
      want_alloc = MAX (want_alloc, MIN_ARRAY_SIZE);

      array->data = g_realloc (array->data, want_alloc);

      if (G_UNLIKELY (g_mem_gc_friendly))
        memset (array->data + array->alloc, 0, want_alloc - array->alloc);

      array->alloc = want_alloc;
    }
}

/**
 * SECTION: arrays_pointer
 * @title: Pointer Arrays
 * @short_description: arrays of pointers to any type of data, which
 *                     grow automatically as new elements are added
 *
 * Pointer Arrays are similar to Arrays but are used only for storing
 * pointers.
 *
 * <note><para>If you remove elements from the array, elements at the
 * end of the array are moved into the space previously occupied by the
 * removed element. This means that you should not rely on the index of
 * particular elements remaining the same. You should also be careful
 * when deleting elements while iterating over the array.</para></note>
 *
 * To create a pointer array, use g_ptr_array_new().
 *
 * To add elements to a pointer array, use g_ptr_array_add().
 *
 * To remove elements from a pointer array, use g_ptr_array_remove(),
 * g_ptr_array_remove_index() or g_ptr_array_remove_index_fast().
 *
 * To access an element of a pointer array, use g_ptr_array_index().
 *
 * To set the size of a pointer array, use g_ptr_array_set_size().
 *
 * To free a pointer array, use g_ptr_array_free().
 *
 * <example>
 *  <title>Using a #GPtrArray</title>
 *  <programlisting>
 *   GPtrArray *gparray;
 *   gchar *string1 = "one", *string2 = "two", *string3 = "three";
 *
 *   gparray = g_ptr_array_new (<!-- -->);
 *   g_ptr_array_add (gparray, (gpointer) string1);
 *   g_ptr_array_add (gparray, (gpointer) string2);
 *   g_ptr_array_add (gparray, (gpointer) string3);
 *
 *   if (g_ptr_array_index (gparray, 0) != (gpointer) string1)
 *     g_print ("ERROR: got &percnt;p instead of &percnt;p\n",
 *              g_ptr_array_index (gparray, 0), string1);
 *
 *   g_ptr_array_free (gparray, TRUE);
 *  </programlisting>
 * </example>
 **/

typedef struct _GRealPtrArray  GRealPtrArray;

/**
 * GPtrArray:
 * @pdata: points to the array of pointers, which may be moved when the
 *         array grows.
 * @len: number of pointers in the array.
 *
 * Contains the public fields of a pointer array.
 **/
struct _GRealPtrArray
{
  gpointer     *pdata;
  guint         len;
  guint         alloc;
  volatile gint ref_count;
  GDestroyNotify element_free_func;
};

/**
 * g_ptr_array_index:
 * @array: a #GPtrArray.
 * @index_: the index of the pointer to return.
 * @Returns: the pointer at the given index.
 *
 * Returns the pointer at the given index of the pointer array.
 **/

static void g_ptr_array_maybe_expand (GRealPtrArray *array,
				      gint           len);

/**
 * g_ptr_array_new:
 * @Returns: the new #GPtrArray.
 *
 * Creates a new #GPtrArray with a reference count of 1.
 **/
GPtrArray*
g_ptr_array_new (void)
{
  return g_ptr_array_sized_new (0);
}

/**
 * g_ptr_array_sized_new:
 * @reserved_size: number of pointers preallocated.
 * @Returns: the new #GPtrArray.
 *
 * Creates a new #GPtrArray with @reserved_size pointers preallocated
 * and a reference count of 1. This avoids frequent reallocation, if
 * you are going to add many pointers to the array. Note however that
 * the size of the array is still 0.
 **/
GPtrArray*  
g_ptr_array_sized_new (guint reserved_size)
{
  GRealPtrArray *array = g_slice_new (GRealPtrArray);

  array->pdata = NULL;
  array->len = 0;
  array->alloc = 0;
  array->ref_count = 1;
  array->element_free_func = NULL;

  if (reserved_size != 0)
    g_ptr_array_maybe_expand (array, reserved_size);

  return (GPtrArray*) array;  
}

/**
 * g_ptr_array_new_with_free_func:
 * @element_free_func: A function to free elements with destroy @array or %NULL.
 *
 * Creates a new #GPtrArray with a reference count of 1 and use @element_free_func
 * for freeing each element when the array is destroyed either via
 * g_ptr_array_unref(), when g_ptr_array_free() is called with @free_segment
 * set to %TRUE or when removing elements.
 *
 * Returns: A new #GPtrArray.
 *
 * Since: 2.22
 **/
GPtrArray *
g_ptr_array_new_with_free_func (GDestroyNotify element_free_func)
{
  GPtrArray *array;

  array = g_ptr_array_new ();
  g_ptr_array_set_free_func (array, element_free_func);
  return array;
}

/**
 * g_ptr_array_set_free_func:
 * @array: A #GPtrArray.
 * @element_free_func: A function to free elements with destroy @array or %NULL.
 *
 * Sets a function for freeing each element when @array is destroyed
 * either via g_ptr_array_unref(), when g_ptr_array_free() is called
 * with @free_segment set to %TRUE or when removing elements.
 *
 * Since: 2.22
 **/
void
g_ptr_array_set_free_func (GPtrArray        *array,
                           GDestroyNotify    element_free_func)
{
  GRealPtrArray* rarray = (GRealPtrArray*) array;

  g_return_if_fail (array);

  rarray->element_free_func = element_free_func;
}

/**
 * g_ptr_array_ref:
 * @array: A #GArray.
 *
 * Atomically increments the reference count of @array by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #GPtrArray.
 *
 * Since: 2.22
 **/
GPtrArray *
g_ptr_array_ref (GPtrArray *array)
{
  GRealPtrArray *rarray = (GRealPtrArray*) array;

  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (g_atomic_int_get (&rarray->ref_count) > 0, array);
  g_atomic_int_inc (&rarray->ref_count);
  return array;
}

/**
 * g_ptr_array_unref:
 * @array: A #GPtrArray.
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, the effect is the same as calling
 * g_ptr_array_free() with @free_segment set to %TRUE. This function
 * is MT-safe and may be called from any thread.
 *
 * Since: 2.22
 **/
void
g_ptr_array_unref (GPtrArray *array)
{
  GRealPtrArray *rarray = (GRealPtrArray*) array;

  g_return_if_fail (array);
  g_return_if_fail (g_atomic_int_get (&rarray->ref_count) > 0);
  if (g_atomic_int_dec_and_test (&rarray->ref_count))
    g_ptr_array_free (array, TRUE);
}

/**
 * g_ptr_array_free:
 * @array: a #GPtrArray.
 * @free_seg: if %TRUE the actual pointer array is freed as well.
 * @Returns: the pointer array if @free_seg is %FALSE, otherwise %NULL.
 *           The pointer array should be freed using g_free().
 *
 * Frees the memory allocated for the #GPtrArray. If @free_seg is %TRUE
 * it frees the memory block holding the elements as well. Pass %FALSE
 * if you want to free the #GPtrArray wrapper but preserve the
 * underlying array for use elsewhere. If the reference count of @array
 * is greater than one, the #GPtrArray wrapper is preserved but the
 * size of @array will be set to zero.
 *
 * <note><para>If array contents point to dynamically-allocated
 * memory, they should be freed separately if @free_seg is %TRUE and no
 * #GDestroyNotify function has been set for @array.</para></note>
 **/
gpointer*
g_ptr_array_free (GPtrArray *farray,
		  gboolean   free_segment)
{
  GRealPtrArray *array = (GRealPtrArray*) farray;
  gpointer* segment;
  gboolean preserve_wrapper;

  g_return_val_if_fail (array, NULL);

  /* if others are holding a reference, preserve the wrapper but do free/return the data */
  preserve_wrapper = FALSE;
  if (g_atomic_int_get (&array->ref_count) > 1)
    preserve_wrapper = TRUE;

  if (free_segment)
    {
      if (array->element_free_func != NULL)
        g_ptr_array_foreach (farray, (GFunc) array->element_free_func, NULL);
      g_free (array->pdata);
      segment = NULL;
    }
  else
    segment = array->pdata;

  if (preserve_wrapper)
    {
      array->pdata = NULL;
      array->len = 0;
      array->alloc = 0;
    }
  else
    {
      g_slice_free1 (sizeof (GRealPtrArray), array);
    }

  return segment;
}

static void
g_ptr_array_maybe_expand (GRealPtrArray *array,
			  gint           len)
{
  if ((array->len + len) > array->alloc)
    {
      guint old_alloc = array->alloc;
      array->alloc = g_nearest_pow (array->len + len);
      array->alloc = MAX (array->alloc, MIN_ARRAY_SIZE);
      array->pdata = g_realloc (array->pdata, sizeof (gpointer) * array->alloc);
      if (G_UNLIKELY (g_mem_gc_friendly))
        for ( ; old_alloc < array->alloc; old_alloc++)
          array->pdata [old_alloc] = NULL;
    }
}

/**
 * g_ptr_array_set_size:
 * @array: a #GPtrArray.
 * @length: the new length of the pointer array.
 *
 * Sets the size of the array. When making the array larger,
 * newly-added elements will be set to %NULL. When making it smaller,
 * if @array has a non-%NULL #GDestroyNotify function then it will be
 * called for the removed elements.
 **/
void
g_ptr_array_set_size  (GPtrArray *farray,
		       gint	  length)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;

  g_return_if_fail (array);

  if (length > array->len)
    {
      int i;
      g_ptr_array_maybe_expand (array, (length - array->len));
      /* This is not 
       *     memset (array->pdata + array->len, 0,
       *            sizeof (gpointer) * (length - array->len));
       * to make it really portable. Remember (void*)NULL needn't be
       * bitwise zero. It of course is silly not to use memset (..,0,..).
       */
      for (i = array->len; i < length; i++)
	array->pdata[i] = NULL;
    }
  else if (length < array->len)
    g_ptr_array_remove_range (farray, length, array->len - length);

  array->len = length;
}

/**
 * g_ptr_array_remove_index:
 * @array: a #GPtrArray.
 * @index_: the index of the pointer to remove.
 * @Returns: the pointer which was removed.
 *
 * Removes the pointer at the given index from the pointer array. The
 * following elements are moved down one place. If @array has a
 * non-%NULL #GDestroyNotify function it is called for the removed
 * element.
 **/
gpointer
g_ptr_array_remove_index (GPtrArray *farray,
			  guint      index_)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  gpointer result;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  result = array->pdata[index_];
  
  if (array->element_free_func != NULL)
    array->element_free_func (array->pdata[index_]);

  if (index_ != array->len - 1)
    g_memmove (array->pdata + index_, array->pdata + index_ + 1, 
               sizeof (gpointer) * (array->len - index_ - 1));
  
  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    array->pdata[array->len] = NULL;

  return result;
}

/**
 * g_ptr_array_remove_index_fast:
 * @array: a #GPtrArray.
 * @index_: the index of the pointer to remove.
 * @Returns: the pointer which was removed.
 *
 * Removes the pointer at the given index from the pointer array. The
 * last element in the array is used to fill in the space, so this
 * function does not preserve the order of the array. But it is faster
 * than g_ptr_array_remove_index(). If @array has a non-%NULL
 * #GDestroyNotify function it is called for the removed element.
 **/
gpointer
g_ptr_array_remove_index_fast (GPtrArray *farray,
			       guint      index_)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  gpointer result;

  g_return_val_if_fail (array, NULL);

  g_return_val_if_fail (index_ < array->len, NULL);

  result = array->pdata[index_];

  if (array->element_free_func != NULL)
    array->element_free_func (array->pdata[index_]);

  if (index_ != array->len - 1)
    array->pdata[index_] = array->pdata[array->len - 1];

  array->len -= 1;

  if (G_UNLIKELY (g_mem_gc_friendly))
    array->pdata[array->len] = NULL;

  return result;
}

/**
 * g_ptr_array_remove_range:
 * @array: a @GPtrArray.
 * @index_: the index of the first pointer to remove.
 * @length: the number of pointers to remove.
 *
 * Removes the given number of pointers starting at the given index
 * from a #GPtrArray.  The following elements are moved to close the
 * gap. If @array has a non-%NULL #GDestroyNotify function it is called
 * for the removed elements.
 *
 * Since: 2.4
 **/
void
g_ptr_array_remove_range (GPtrArray *farray,
                          guint      index_,
                          guint      length)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  guint n;

  g_return_if_fail (array);
  g_return_if_fail (index_ < array->len);
  g_return_if_fail (index_ + length <= array->len);

  if (array->element_free_func != NULL)
    {
      for (n = index_; n < index_ + length; n++)
        array->element_free_func (array->pdata[n]);
    }

  if (index_ + length != array->len)
    {
      g_memmove (&array->pdata[index_],
                 &array->pdata[index_ + length], 
                 (array->len - (index_ + length)) * sizeof (gpointer));
    }

  array->len -= length;
  if (G_UNLIKELY (g_mem_gc_friendly))
    {
      guint i;
      for (i = 0; i < length; i++)
        array->pdata[array->len + i] = NULL;
    }
}

/**
 * g_ptr_array_remove:
 * @array: a #GPtrArray.
 * @data: the pointer to remove.
 * @Returns: %TRUE if the pointer is removed. %FALSE if the pointer is
 *           not found in the array.
 *
 * Removes the first occurrence of the given pointer from the pointer
 * array. The following elements are moved down one place. If @array
 * has a non-%NULL #GDestroyNotify function it is called for the
 * removed element.
 *
 * It returns %TRUE if the pointer was removed, or %FALSE if the
 * pointer was not found.
 **/
gboolean
g_ptr_array_remove (GPtrArray *farray,
		    gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  guint i;

  g_return_val_if_fail (array, FALSE);

  for (i = 0; i < array->len; i += 1)
    {
      if (array->pdata[i] == data)
	{
	  g_ptr_array_remove_index (farray, i);
	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * g_ptr_array_remove_fast:
 * @array: a #GPtrArray.
 * @data: the pointer to remove.
 * @Returns: %TRUE if the pointer was found in the array.
 *
 * Removes the first occurrence of the given pointer from the pointer
 * array. The last element in the array is used to fill in the space,
 * so this function does not preserve the order of the array. But it is
 * faster than g_ptr_array_remove(). If @array has a non-%NULL
 * #GDestroyNotify function it is called for the removed element.
 *
 * It returns %TRUE if the pointer was removed, or %FALSE if the
 * pointer was not found.
 **/
gboolean
g_ptr_array_remove_fast (GPtrArray *farray,
			 gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;
  guint i;

  g_return_val_if_fail (array, FALSE);

  for (i = 0; i < array->len; i += 1)
    {
      if (array->pdata[i] == data)
	{
	  g_ptr_array_remove_index_fast (farray, i);
	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * g_ptr_array_add:
 * @array: a #GPtrArray.
 * @data: the pointer to add.
 *
 * Adds a pointer to the end of the pointer array. The array will grow
 * in size automatically if necessary.
 **/
void
g_ptr_array_add (GPtrArray *farray,
		 gpointer   data)
{
  GRealPtrArray* array = (GRealPtrArray*) farray;

  g_return_if_fail (array);

  g_ptr_array_maybe_expand (array, 1);

  array->pdata[array->len++] = data;
}

/**
 * g_ptr_array_sort:
 * @array: a #GPtrArray.
 * @compare_func: comparison function.
 *
 * Sorts the array, using @compare_func which should be a qsort()-style
 * comparison function (returns less than zero for first arg is less
 * than second arg, zero for equal, greater than zero if irst arg is
 * greater than second arg).
 *
 * If two array elements compare equal, their order in the sorted array
 * is undefined.
 *
 * <note><para>The comparison function for g_ptr_array_sort() doesn't
 * take the pointers from the array as arguments, it takes pointers to
 * the pointers in the array.</para></note>
 **/
void
g_ptr_array_sort (GPtrArray    *array,
		  GCompareFunc  compare_func)
{
  g_return_if_fail (array != NULL);

  qsort (array->pdata,
	 array->len,
	 sizeof (gpointer),
	 compare_func);
}

/**
 * g_ptr_array_sort_with_data:
 * @array: a #GPtrArray.
 * @compare_func: comparison function.
 * @user_data: data to pass to @compare_func.
 *
 * Like g_ptr_array_sort(), but the comparison function has an extra
 * user data argument.
 *
 * <note><para>The comparison function for g_ptr_array_sort_with_data()
 * doesn't take the pointers from the array as arguments, it takes
 * pointers to the pointers in the array.</para></note>
 **/
void
g_ptr_array_sort_with_data (GPtrArray        *array,
			    GCompareDataFunc  compare_func,
			    gpointer          user_data)
{
  g_return_if_fail (array != NULL);

  g_qsort_with_data (array->pdata,
		     array->len,
		     sizeof (gpointer),
		     compare_func,
		     user_data);
}

/**
 * g_ptr_array_foreach:
 * @array: a #GPtrArray
 * @func: the function to call for each array element
 * @user_data: user data to pass to the function
 * 
 * Calls a function for each element of a #GPtrArray.
 *
 * Since: 2.4
 **/
void
g_ptr_array_foreach (GPtrArray *array,
                     GFunc      func,
                     gpointer   user_data)
{
  guint i;

  g_return_if_fail (array);

  for (i = 0; i < array->len; i++)
    (*func) (array->pdata[i], user_data);
}

/**
 * SECTION: arrays_byte
 * @title: Byte Arrays
 * @short_description: arrays of bytes, which grow automatically as
 *                     elements are added
 *
 * #GByteArray is based on #GArray, to provide arrays of bytes which
 * grow automatically as elements are added.
 *
 * To create a new #GByteArray use g_byte_array_new().
 *
 * To add elements to a #GByteArray, use g_byte_array_append(), and
 * g_byte_array_prepend().
 *
 * To set the size of a #GByteArray, use g_byte_array_set_size().
 *
 * To free a #GByteArray, use g_byte_array_free().
 *
 * <example>
 *  <title>Using a #GByteArray</title>
 *  <programlisting>
 *   GByteArray *gbarray;
 *   gint i;
 *
 *   gbarray = g_byte_array_new (<!-- -->);
 *   for (i = 0; i &lt; 10000; i++)
 *     g_byte_array_append (gbarray, (guint8*) "abcd", 4);
 *
 *   for (i = 0; i &lt; 10000; i++)
 *     {
 *       g_assert (gbarray->data[4*i] == 'a');
 *       g_assert (gbarray->data[4*i+1] == 'b');
 *       g_assert (gbarray->data[4*i+2] == 'c');
 *       g_assert (gbarray->data[4*i+3] == 'd');
 *     }
 *
 *   g_byte_array_free (gbarray, TRUE);
 *  </programlisting>
 * </example>
 **/

/**
 * GByteArray:
 * @data: a pointer to the element data. The data may be moved as
 *        elements are added to the #GByteArray.
 * @len: the number of elements in the #GByteArray.
 *
 * The <structname>GByteArray</structname> struct allows access to the
 * public fields of a <structname>GByteArray</structname>.
 **/

/**
 * g_byte_array_new:
 * @Returns: the new #GByteArray.
 *
 * Creates a new #GByteArray with a reference count of 1.
 **/
GByteArray* g_byte_array_new (void)
{
  return (GByteArray*) g_array_sized_new (FALSE, FALSE, 1, 0);
}

/**
 * g_byte_array_sized_new:
 * @reserved_size: number of bytes preallocated.
 * @Returns: the new #GByteArray.
 *
 * Creates a new #GByteArray with @reserved_size bytes preallocated.
 * This avoids frequent reallocation, if you are going to add many
 * bytes to the array. Note however that the size of the array is still
 * 0.
 **/
GByteArray* g_byte_array_sized_new (guint reserved_size)
{
  return (GByteArray*) g_array_sized_new (FALSE, FALSE, 1, reserved_size);
}

/**
 * g_byte_array_free:
 * @array: a #GByteArray.
 * @free_segment: if %TRUE the actual byte data is freed as well.
 * @Returns: the element data if @free_segment is %FALSE, otherwise
 *           %NULL.  The element data should be freed using g_free().
 *
 * Frees the memory allocated by the #GByteArray. If @free_segment is
 * %TRUE it frees the actual byte data. If the reference count of
 * @array is greater than one, the #GByteArray wrapper is preserved but
 * the size of @array will be set to zero.
 **/
guint8*	    g_byte_array_free     (GByteArray *array,
			           gboolean    free_segment)
{
  return (guint8*) g_array_free ((GArray*) array, free_segment);
}

/**
 * g_byte_array_ref:
 * @array: A #GByteArray.
 *
 * Atomically increments the reference count of @array by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #GByteArray.
 *
 * Since: 2.22
 **/
GByteArray *
g_byte_array_ref (GByteArray *array)
{
  return (GByteArray *) g_array_ref ((GArray *) array);
}

/**
 * g_byte_array_unref:
 * @array: A #GByteArray.
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, all memory allocated by the array is
 * released. This function is MT-safe and may be called from any
 * thread.
 *
 * Since: 2.22
 **/
void
g_byte_array_unref (GByteArray *array)
{
  g_array_unref ((GArray *) array);
}

/**
 * g_byte_array_append:
 * @array: a #GByteArray.
 * @data: the byte data to be added.
 * @len: the number of bytes to add.
 * @Returns: the #GByteArray.
 *
 * Adds the given bytes to the end of the #GByteArray. The array will
 * grow in size automatically if necessary.
 **/
GByteArray* g_byte_array_append   (GByteArray   *array,
				   const guint8 *data,
				   guint         len)
{
  g_array_append_vals ((GArray*) array, (guint8*)data, len);

  return array;
}

/**
 * g_byte_array_prepend:
 * @array: a #GByteArray.
 * @data: the byte data to be added.
 * @len: the number of bytes to add.
 * @Returns: the #GByteArray.
 *
 * Adds the given data to the start of the #GByteArray. The array will
 * grow in size automatically if necessary.
 **/
GByteArray* g_byte_array_prepend  (GByteArray   *array,
				   const guint8 *data,
				   guint         len)
{
  g_array_prepend_vals ((GArray*) array, (guint8*)data, len);

  return array;
}

/**
 * g_byte_array_set_size:
 * @array: a #GByteArray.
 * @length: the new size of the #GByteArray.
 * @Returns: the #GByteArray.
 *
 * Sets the size of the #GByteArray, expanding it if necessary.
 **/
GByteArray* g_byte_array_set_size (GByteArray *array,
				   guint       length)
{
  g_array_set_size ((GArray*) array, length);

  return array;
}

/**
 * g_byte_array_remove_index:
 * @array: a #GByteArray.
 * @index_: the index of the byte to remove.
 * @Returns: the #GByteArray.
 *
 * Removes the byte at the given index from a #GByteArray. The
 * following bytes are moved down one place.
 **/
GByteArray* g_byte_array_remove_index (GByteArray *array,
				       guint       index_)
{
  g_array_remove_index ((GArray*) array, index_);

  return array;
}

/**
 * g_byte_array_remove_index_fast:
 * @array: a #GByteArray.
 * @index_: the index of the byte to remove.
 * @Returns: the #GByteArray.
 *
 * Removes the byte at the given index from a #GByteArray. The last
 * element in the array is used to fill in the space, so this function
 * does not preserve the order of the #GByteArray. But it is faster
 * than g_byte_array_remove_index().
 **/
GByteArray* g_byte_array_remove_index_fast (GByteArray *array,
					    guint       index_)
{
  g_array_remove_index_fast ((GArray*) array, index_);

  return array;
}

/**
 * g_byte_array_remove_range:
 * @array: a @GByteArray.
 * @index_: the index of the first byte to remove.
 * @length: the number of bytes to remove.
 * @Returns: the #GByteArray.
 *
 * Removes the given number of bytes starting at the given index from a
 * #GByteArray.  The following elements are moved to close the gap.
 *
 * Since: 2.4
 **/
GByteArray*
g_byte_array_remove_range (GByteArray *array,
                           guint       index_,
                           guint       length)
{
  g_return_val_if_fail (array, NULL);
  g_return_val_if_fail (index_ < array->len, NULL);
  g_return_val_if_fail (index_ + length <= array->len, NULL);

  return (GByteArray *)g_array_remove_range ((GArray*) array, index_, length);
}

/**
 * g_byte_array_sort:
 * @array: a #GByteArray.
 * @compare_func: comparison function.
 *
 * Sorts a byte array, using @compare_func which should be a
 * qsort()-style comparison function (returns less than zero for first
 * arg is less than second arg, zero for equal, greater than zero if
 * first arg is greater than second arg).
 *
 * If two array elements compare equal, their order in the sorted array
 * is undefined.
 **/
void
g_byte_array_sort (GByteArray   *array,
		   GCompareFunc  compare_func)
{
  g_array_sort ((GArray *) array, compare_func);
}

/**
 * g_byte_array_sort_with_data:
 * @array: a #GByteArray.
 * @compare_func: comparison function.
 * @user_data: data to pass to @compare_func.
 *
 * Like g_byte_array_sort(), but the comparison function takes an extra
 * user data argument.
 **/
void
g_byte_array_sort_with_data (GByteArray       *array,
			     GCompareDataFunc  compare_func,
			     gpointer          user_data)
{
  g_array_sort_with_data ((GArray *) array, compare_func, user_data);
}
