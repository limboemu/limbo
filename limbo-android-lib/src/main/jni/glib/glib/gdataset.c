/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gdataset.c: Generic dataset mechanism, similar to GtkObject data.
 * Copyright (C) 1998 Tim Janik
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe ; except for g_data*_foreach()
 */

#include "config.h"

#include <string.h>

#include "gdataset.h"

#include "gdatasetprivate.h"
#include "ghash.h"
#include "gquark.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gthread.h"
#include "glib_trace.h"

/**
 * SECTION: datasets
 * @title: Datasets
 * @short_description: associate groups of data elements with
 *                     particular memory locations
 *
 * Datasets associate groups of data elements with particular memory
 * locations. These are useful if you need to associate data with a
 * structure returned from an external library. Since you cannot modify
 * the structure, you use its location in memory as the key into a
 * dataset, where you can associate any number of data elements with it.
 *
 * There are two forms of most of the dataset functions. The first form
 * uses strings to identify the data elements associated with a
 * location. The second form uses #GQuark identifiers, which are
 * created with a call to g_quark_from_string() or
 * g_quark_from_static_string(). The second form is quicker, since it
 * does not require looking up the string in the hash table of #GQuark
 * identifiers.
 *
 * There is no function to create a dataset. It is automatically
 * created as soon as you add elements to it.
 *
 * To add data elements to a dataset use g_dataset_id_set_data(),
 * g_dataset_id_set_data_full(), g_dataset_set_data() and
 * g_dataset_set_data_full().
 *
 * To get data elements from a dataset use g_dataset_id_get_data() and
 * g_dataset_get_data().
 *
 * To iterate over all data elements in a dataset use
 * g_dataset_foreach() (not thread-safe).
 *
 * To remove data elements from a dataset use
 * g_dataset_id_remove_data() and g_dataset_remove_data().
 *
 * To destroy a dataset, use g_dataset_destroy().
 **/

/**
 * SECTION: datalist
 * @title: Keyed Data Lists
 * @short_description: lists of data elements which are accessible by a
 *                     string or GQuark identifier
 *
 * Keyed data lists provide lists of arbitrary data elements which can
 * be accessed either with a string or with a #GQuark corresponding to
 * the string.
 *
 * The #GQuark methods are quicker, since the strings have to be
 * converted to #GQuarks anyway.
 *
 * Data lists are used for associating arbitrary data with #GObjects,
 * using g_object_set_data() and related functions.
 *
 * To create a datalist, use g_datalist_init().
 *
 * To add data elements to a datalist use g_datalist_id_set_data(),
 * g_datalist_id_set_data_full(), g_datalist_set_data() and
 * g_datalist_set_data_full().
 *
 * To get data elements from a datalist use g_datalist_id_get_data()
 * and g_datalist_get_data().
 *
 * To iterate over all data elements in a datalist use
 * g_datalist_foreach() (not thread-safe).
 *
 * To remove data elements from a datalist use
 * g_datalist_id_remove_data() and g_datalist_remove_data().
 *
 * To remove all data elements from a datalist, use g_datalist_clear().
 **/

/**
 * GData:
 *
 * The #GData struct is an opaque data structure to represent a <link
 * linkend="glib-Keyed-Data-Lists">Keyed Data List</link>. It should
 * only be accessed via the following functions.
 **/

/**
 * GDestroyNotify:
 * @data: the data element.
 *
 * Specifies the type of function which is called when a data element
 * is destroyed. It is passed the pointer to the data element and
 * should free any memory and resources allocated for it.
 **/

/* --- defines --- */
#define	G_QUARK_BLOCK_SIZE			(512)

/* datalist pointer accesses have to be carried out atomically */
#define G_DATALIST_GET_POINTER(datalist)						\
  ((GData*) ((gsize) g_atomic_pointer_get (datalist) & ~(gsize) G_DATALIST_FLAGS_MASK))

#define G_DATALIST_SET_POINTER(datalist, pointer)       G_STMT_START {                  \
  gpointer _oldv, _newv;                                                                \
  do {                                                                                  \
    _oldv = g_atomic_pointer_get (datalist);                                            \
    _newv = (gpointer) (((gsize) _oldv & G_DATALIST_FLAGS_MASK) | (gsize) pointer);     \
  } while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, _oldv, _newv));   \
} G_STMT_END

/* --- structures --- */
typedef struct _GDataset GDataset;
struct _GData
{
  GData *next;
  GQuark id;
  gpointer data;
  GDestroyNotify destroy_func;
};

struct _GDataset
{
  gconstpointer location;
  GData        *datalist;
};


/* --- prototypes --- */
static inline GDataset*	g_dataset_lookup		(gconstpointer	  dataset_location);
static inline void	g_datalist_clear_i		(GData		**datalist);
static void		g_dataset_destroy_internal	(GDataset	 *dataset);
static inline gpointer	g_data_set_internal		(GData     	**datalist,
							 GQuark   	  key_id,
							 gpointer         data,
							 GDestroyNotify   destroy_func,
							 GDataset	 *dataset);
static void		g_data_initialize		(void);
static inline GQuark	g_quark_new			(gchar  	*string);


/* --- variables --- */
G_LOCK_DEFINE_STATIC (g_dataset_global);
static GHashTable   *g_dataset_location_ht = NULL;
static GDataset     *g_dataset_cached = NULL; /* should this be
						 threadspecific? */
G_LOCK_DEFINE_STATIC (g_quark_global);
static GHashTable   *g_quark_ht = NULL;
static gchar       **g_quarks = NULL;
static GQuark        g_quark_seq_id = 0;

/* --- functions --- */

/* HOLDS: g_dataset_global_lock */
static inline void
g_datalist_clear_i (GData **datalist)
{
  register GData *list;
  
  /* unlink *all* items before walking their destructors
   */
  list = G_DATALIST_GET_POINTER (datalist);
  G_DATALIST_SET_POINTER (datalist, NULL);
  
  while (list)
    {
      register GData *prev;
      
      prev = list;
      list = prev->next;
      
      if (prev->destroy_func)
	{
	  G_UNLOCK (g_dataset_global);
	  prev->destroy_func (prev->data);
	  G_LOCK (g_dataset_global);
	}
      
      g_slice_free (GData, prev);
    }
}

/**
 * g_datalist_clear:
 * @datalist: a datalist.
 *
 * Frees all the data elements of the datalist. The data elements'
 * destroy functions are called if they have been set.
 **/
void
g_datalist_clear (GData **datalist)
{
  g_return_if_fail (datalist != NULL);
  
  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();

  while (G_DATALIST_GET_POINTER (datalist))
    g_datalist_clear_i (datalist);
  G_UNLOCK (g_dataset_global);
}

/* HOLDS: g_dataset_global_lock */
static inline GDataset*
g_dataset_lookup (gconstpointer	dataset_location)
{
  register GDataset *dataset;
  
  if (g_dataset_cached && g_dataset_cached->location == dataset_location)
    return g_dataset_cached;
  
  dataset = g_hash_table_lookup (g_dataset_location_ht, dataset_location);
  if (dataset)
    g_dataset_cached = dataset;
  
  return dataset;
}

/* HOLDS: g_dataset_global_lock */
static void
g_dataset_destroy_internal (GDataset *dataset)
{
  register gconstpointer dataset_location;
  
  dataset_location = dataset->location;
  while (dataset)
    {
      if (!dataset->datalist)
	{
	  if (dataset == g_dataset_cached)
	    g_dataset_cached = NULL;
	  g_hash_table_remove (g_dataset_location_ht, dataset_location);
	  g_slice_free (GDataset, dataset);
	  break;
	}
      
      g_datalist_clear_i (&dataset->datalist);
      dataset = g_dataset_lookup (dataset_location);
    }
}

/**
 * g_dataset_destroy:
 * @dataset_location: the location identifying the dataset.
 *
 * Destroys the dataset, freeing all memory allocated, and calling any
 * destroy functions set for data elements.
 **/
void
g_dataset_destroy (gconstpointer  dataset_location)
{
  g_return_if_fail (dataset_location != NULL);
  
  G_LOCK (g_dataset_global);
  if (g_dataset_location_ht)
    {
      register GDataset *dataset;

      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	g_dataset_destroy_internal (dataset);
    }
  G_UNLOCK (g_dataset_global);
}

/* HOLDS: g_dataset_global_lock */
static inline gpointer
g_data_set_internal (GData	  **datalist,
		     GQuark         key_id,
		     gpointer       data,
		     GDestroyNotify destroy_func,
		     GDataset	   *dataset)
{
  register GData *list;
  
  list = G_DATALIST_GET_POINTER (datalist);
  if (!data)
    {
      register GData *prev;
      
      prev = NULL;
      while (list)
	{
	  if (list->id == key_id)
	    {
	      gpointer ret_data = NULL;

	      if (prev)
		prev->next = list->next;
	      else
		{
		  G_DATALIST_SET_POINTER (datalist, list->next);
		  
		  /* the dataset destruction *must* be done
		   * prior to invocation of the data destroy function
		   */
		  if (!list->next && dataset)
		    g_dataset_destroy_internal (dataset);
		}
	      
	      /* the GData struct *must* already be unlinked
	       * when invoking the destroy function.
	       * we use (data==NULL && destroy_func!=NULL) as
	       * a special hint combination to "steal"
	       * data without destroy notification
	       */
	      if (list->destroy_func && !destroy_func)
		{
		  G_UNLOCK (g_dataset_global);
		  list->destroy_func (list->data);
		  G_LOCK (g_dataset_global);
		}
	      else
		ret_data = list->data;
	      
              g_slice_free (GData, list);
	      
	      return ret_data;
	    }
	  
	  prev = list;
	  list = list->next;
	}
    }
  else
    {
      while (list)
	{
	  if (list->id == key_id)
	    {
	      if (!list->destroy_func)
		{
		  list->data = data;
		  list->destroy_func = destroy_func;
		}
	      else
		{
		  register GDestroyNotify dfunc;
		  register gpointer ddata;
		  
		  dfunc = list->destroy_func;
		  ddata = list->data;
		  list->data = data;
		  list->destroy_func = destroy_func;
		  
		  /* we need to have updated all structures prior to
		   * invocation of the destroy function
		   */
		  G_UNLOCK (g_dataset_global);
		  dfunc (ddata);
		  G_LOCK (g_dataset_global);
		}
	      
	      return NULL;
	    }
	  
	  list = list->next;
	}
      
      list = g_slice_new (GData);
      list->next = G_DATALIST_GET_POINTER (datalist);
      list->id = key_id;
      list->data = data;
      list->destroy_func = destroy_func;
      G_DATALIST_SET_POINTER (datalist, list);
    }

  return NULL;
}

/**
 * g_dataset_id_set_data_full:
 * @dataset_location: the location identifying the dataset.
 * @key_id: the #GQuark id to identify the data element.
 * @data: the data element.
 * @destroy_func: the function to call when the data element is
 *                removed. This function will be called with the data
 *                element and can be used to free any memory allocated
 *                for it.
 *
 * Sets the data element associated with the given #GQuark id, and also
 * the function to call when the data element is destroyed. Any
 * previous data with the same key is removed, and its destroy function
 * is called.
 **/
/**
 * g_dataset_set_data_full:
 * @l: the location identifying the dataset.
 * @k: the string to identify the data element.
 * @d: the data element.
 * @f: the function to call when the data element is removed. This
 *     function will be called with the data element and can be used to
 *     free any memory allocated for it.
 *
 * Sets the data corresponding to the given string identifier, and the
 * function to call when the data element is destroyed.
 **/
/**
 * g_dataset_id_set_data:
 * @l: the location identifying the dataset.
 * @k: the #GQuark id to identify the data element.
 * @d: the data element.
 *
 * Sets the data element associated with the given #GQuark id. Any
 * previous data with the same key is removed, and its destroy function
 * is called.
 **/
/**
 * g_dataset_set_data:
 * @l: the location identifying the dataset.
 * @k: the string to identify the data element.
 * @d: the data element.
 *
 * Sets the data corresponding to the given string identifier.
 **/
/**
 * g_dataset_id_remove_data:
 * @l: the location identifying the dataset.
 * @k: the #GQuark id identifying the data element.
 *
 * Removes a data element from a dataset. The data element's destroy
 * function is called if it has been set.
 **/
/**
 * g_dataset_remove_data:
 * @l: the location identifying the dataset.
 * @k: the string identifying the data element.
 *
 * Removes a data element corresponding to a string. Its destroy
 * function is called if it has been set.
 **/
void
g_dataset_id_set_data_full (gconstpointer  dataset_location,
			    GQuark         key_id,
			    gpointer       data,
			    GDestroyNotify destroy_func)
{
  register GDataset *dataset;
  
  g_return_if_fail (dataset_location != NULL);
  if (!data)
    g_return_if_fail (destroy_func == NULL);
  if (!key_id)
    {
      if (data)
	g_return_if_fail (key_id > 0);
      else
	return;
    }
  
  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();
 
  dataset = g_dataset_lookup (dataset_location);
  if (!dataset)
    {
      dataset = g_slice_new (GDataset);
      dataset->location = dataset_location;
      g_datalist_init (&dataset->datalist);
      g_hash_table_insert (g_dataset_location_ht, 
			   (gpointer) dataset->location,
			   dataset);
    }
  
  g_data_set_internal (&dataset->datalist, key_id, data, destroy_func, dataset);
  G_UNLOCK (g_dataset_global);
}

/**
 * g_datalist_id_set_data_full:
 * @datalist: a datalist.
 * @key_id: the #GQuark to identify the data element.
 * @data: the data element or %NULL to remove any previous element
 *        corresponding to @key_id.
 * @destroy_func: the function to call when the data element is
 *                removed. This function will be called with the data
 *                element and can be used to free any memory allocated
 *                for it. If @data is %NULL, then @destroy_func must
 *                also be %NULL.
 *
 * Sets the data corresponding to the given #GQuark id, and the
 * function to be called when the element is removed from the datalist.
 * Any previous data with the same key is removed, and its destroy
 * function is called.
 **/
/**
 * g_datalist_set_data_full:
 * @dl: a datalist.
 * @k: the string to identify the data element.
 * @d: the data element, or %NULL to remove any previous element
 *     corresponding to @k.
 * @f: the function to call when the data element is removed. This
 *     function will be called with the data element and can be used to
 *     free any memory allocated for it. If @d is %NULL, then @f must
 *     also be %NULL.
 *
 * Sets the data element corresponding to the given string identifier,
 * and the function to be called when the data element is removed.
 **/
/**
 * g_datalist_id_set_data:
 * @dl: a datalist.
 * @q: the #GQuark to identify the data element.
 * @d: the data element, or %NULL to remove any previous element
 *     corresponding to @q.
 *
 * Sets the data corresponding to the given #GQuark id. Any previous
 * data with the same key is removed, and its destroy function is
 * called.
 **/
/**
 * g_datalist_set_data:
 * @dl: a datalist.
 * @k: the string to identify the data element.
 * @d: the data element, or %NULL to remove any previous element
 *     corresponding to @k.
 *
 * Sets the data element corresponding to the given string identifier.
 **/
/**
 * g_datalist_id_remove_data:
 * @dl: a datalist.
 * @q: the #GQuark identifying the data element.
 *
 * Removes an element, using its #GQuark identifier.
 **/
/**
 * g_datalist_remove_data:
 * @dl: a datalist.
 * @k: the string identifying the data element.
 *
 * Removes an element using its string identifier. The data element's
 * destroy function is called if it has been set.
 **/
void
g_datalist_id_set_data_full (GData	  **datalist,
			     GQuark         key_id,
			     gpointer       data,
			     GDestroyNotify destroy_func)
{
  g_return_if_fail (datalist != NULL);
  if (!data)
    g_return_if_fail (destroy_func == NULL);
  if (!key_id)
    {
      if (data)
	g_return_if_fail (key_id > 0);
      else
	return;
    }

  G_LOCK (g_dataset_global);
  if (!g_dataset_location_ht)
    g_data_initialize ();
  
  g_data_set_internal (datalist, key_id, data, destroy_func, NULL);
  G_UNLOCK (g_dataset_global);
}

/**
 * g_dataset_id_remove_no_notify:
 * @dataset_location: the location identifying the dataset.
 * @key_id: the #GQuark ID identifying the data element.
 * @Returns: the data previously stored at @key_id, or %NULL if none.
 *
 * Removes an element, without calling its destroy notification
 * function.
 **/
/**
 * g_dataset_remove_no_notify:
 * @l: the location identifying the dataset.
 * @k: the string identifying the data element.
 *
 * Removes an element, without calling its destroy notifier.
 **/
gpointer
g_dataset_id_remove_no_notify (gconstpointer  dataset_location,
			       GQuark         key_id)
{
  gpointer ret_data = NULL;

  g_return_val_if_fail (dataset_location != NULL, NULL);
  
  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    {
      GDataset *dataset;
  
      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	ret_data = g_data_set_internal (&dataset->datalist, key_id, NULL, (GDestroyNotify) 42, dataset);
    } 
  G_UNLOCK (g_dataset_global);

  return ret_data;
}

/**
 * g_datalist_id_remove_no_notify:
 * @datalist: a datalist.
 * @key_id: the #GQuark identifying a data element.
 * @Returns: the data previously stored at @key_id, or %NULL if none.
 *
 * Removes an element, without calling its destroy notification
 * function.
 **/
/**
 * g_datalist_remove_no_notify:
 * @dl: a datalist.
 * @k: the string identifying the data element.
 *
 * Removes an element, without calling its destroy notifier.
 **/
gpointer
g_datalist_id_remove_no_notify (GData	**datalist,
				GQuark    key_id)
{
  gpointer ret_data = NULL;

  g_return_val_if_fail (datalist != NULL, NULL);

  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    ret_data = g_data_set_internal (datalist, key_id, NULL, (GDestroyNotify) 42, NULL);
  G_UNLOCK (g_dataset_global);

  return ret_data;
}

/**
 * g_dataset_id_get_data:
 * @dataset_location: the location identifying the dataset.
 * @key_id: the #GQuark id to identify the data element.
 * @Returns: the data element corresponding to the #GQuark, or %NULL if
 *           it is not found.
 *
 * Gets the data element corresponding to a #GQuark.
 **/
/**
 * g_dataset_get_data:
 * @l: the location identifying the dataset.
 * @k: the string identifying the data element.
 * @Returns: the data element corresponding to the string, or %NULL if
 *           it is not found.
 *
 * Gets the data element corresponding to a string.
 **/
gpointer
g_dataset_id_get_data (gconstpointer  dataset_location,
		       GQuark         key_id)
{
  g_return_val_if_fail (dataset_location != NULL, NULL);
  
  G_LOCK (g_dataset_global);
  if (key_id && g_dataset_location_ht)
    {
      register GDataset *dataset;
      
      dataset = g_dataset_lookup (dataset_location);
      if (dataset)
	{
	  register GData *list;
	  
	  for (list = dataset->datalist; list; list = list->next)
	    if (list->id == key_id)
	      {
		G_UNLOCK (g_dataset_global);
		return list->data;
	      }
	}
    }
  G_UNLOCK (g_dataset_global);
 
  return NULL;
}

/**
 * g_datalist_id_get_data:
 * @datalist: a datalist.
 * @key_id: the #GQuark identifying a data element.
 * @Returns: the data element, or %NULL if it is not found.
 *
 * Retrieves the data element corresponding to @key_id.
 **/
/**
 * g_datalist_get_data:
 * @dl: a datalist.
 * @k: the string identifying a data element.
 * @Returns: the data element, or %NULL if it is not found.
 *
 * Gets a data element, using its string identifer. This is slower than
 * g_datalist_id_get_data() because the string is first converted to a
 * #GQuark.
 **/
gpointer
g_datalist_id_get_data (GData	 **datalist,
			GQuark     key_id)
{
  gpointer data = NULL;
  g_return_val_if_fail (datalist != NULL, NULL);
  if (key_id)
    {
      register GData *list;
      G_LOCK (g_dataset_global);
      for (list = G_DATALIST_GET_POINTER (datalist); list; list = list->next)
	if (list->id == key_id)
	  {
            data = list->data;
            break;
          }
      G_UNLOCK (g_dataset_global);
    }
  return data;
}

/**
 * GDataForeachFunc:
 * @key_id: the #GQuark id to identifying the data element.
 * @data: the data element.
 * @user_data: user data passed to g_dataset_foreach().
 *
 * Specifies the type of function passed to g_dataset_foreach(). It is
 * called with each #GQuark id and associated data element, together
 * with the @user_data parameter supplied to g_dataset_foreach().
 **/

/**
 * g_dataset_foreach:
 * @dataset_location: the location identifying the dataset.
 * @func: the function to call for each data element.
 * @user_data: user data to pass to the function.
 *
 * Calls the given function for each data element which is associated
 * with the given location. Note that this function is NOT thread-safe.
 * So unless @datalist can be protected from any modifications during
 * invocation of this function, it should not be called.
 **/
void
g_dataset_foreach (gconstpointer    dataset_location,
		   GDataForeachFunc func,
		   gpointer         user_data)
{
  register GDataset *dataset;
  
  g_return_if_fail (dataset_location != NULL);
  g_return_if_fail (func != NULL);

  G_LOCK (g_dataset_global);
  if (g_dataset_location_ht)
    {
      dataset = g_dataset_lookup (dataset_location);
      G_UNLOCK (g_dataset_global);
      if (dataset)
	{
	  register GData *list, *next;
	  
	  for (list = dataset->datalist; list; list = next)
	    {
	      next = list->next;
	      func (list->id, list->data, user_data);
	    }
	}
    }
  else
    {
      G_UNLOCK (g_dataset_global);
    }
}

/**
 * g_datalist_foreach:
 * @datalist: a datalist.
 * @func: the function to call for each data element.
 * @user_data: user data to pass to the function.
 *
 * Calls the given function for each data element of the datalist. The
 * function is called with each data element's #GQuark id and data,
 * together with the given @user_data parameter. Note that this
 * function is NOT thread-safe. So unless @datalist can be protected
 * from any modifications during invocation of this function, it should
 * not be called.
 **/
void
g_datalist_foreach (GData	   **datalist,
		    GDataForeachFunc func,
		    gpointer         user_data)
{
  register GData *list, *next;

  g_return_if_fail (datalist != NULL);
  g_return_if_fail (func != NULL);
  
  for (list = G_DATALIST_GET_POINTER (datalist); list; list = next)
    {
      next = list->next;
      func (list->id, list->data, user_data);
    }
}

/**
 * g_datalist_init:
 * @datalist: a pointer to a pointer to a datalist.
 *
 * Resets the datalist to %NULL. It does not free any memory or call
 * any destroy functions.
 **/
void
g_datalist_init (GData **datalist)
{
  g_return_if_fail (datalist != NULL);

  g_atomic_pointer_set (datalist, NULL);
}

/**
 * g_datalist_set_flags:
 * @datalist: pointer to the location that holds a list
 * @flags: the flags to turn on. The values of the flags are
 *   restricted by %G_DATALIST_FLAGS_MASK (currently
 *   3; giving two possible boolean flags).
 *   A value for @flags that doesn't fit within the mask is
 *   an error.
 * 
 * Turns on flag values for a data list. This function is used
 * to keep a small number of boolean flags in an object with
 * a data list without using any additional space. It is
 * not generally useful except in circumstances where space
 * is very tight. (It is used in the base #GObject type, for
 * example.)
 *
 * Since: 2.8
 **/
void
g_datalist_set_flags (GData **datalist,
		      guint   flags)
{
  gpointer oldvalue;
  g_return_if_fail (datalist != NULL);
  g_return_if_fail ((flags & ~G_DATALIST_FLAGS_MASK) == 0);
  
  do
    {
      oldvalue = g_atomic_pointer_get (datalist);
    }
  while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, oldvalue,
                                                 (gpointer) ((gsize) oldvalue | flags)));
}

/**
 * g_datalist_unset_flags:
 * @datalist: pointer to the location that holds a list
 * @flags: the flags to turn off. The values of the flags are
 *   restricted by %G_DATALIST_FLAGS_MASK (currently
 *   3: giving two possible boolean flags).
 *   A value for @flags that doesn't fit within the mask is
 *   an error.
 * 
 * Turns off flag values for a data list. See g_datalist_unset_flags()
 *
 * Since: 2.8
 **/
void
g_datalist_unset_flags (GData **datalist,
			guint   flags)
{
  gpointer oldvalue;
  g_return_if_fail (datalist != NULL);
  g_return_if_fail ((flags & ~G_DATALIST_FLAGS_MASK) == 0);
  
  do
    {
      oldvalue = g_atomic_pointer_get (datalist);
    }
  while (!g_atomic_pointer_compare_and_exchange ((void**) datalist, oldvalue,
                                                 (gpointer) ((gsize) oldvalue & ~(gsize) flags)));
}

/**
 * g_datalist_get_flags:
 * @datalist: pointer to the location that holds a list
 * 
 * Gets flags values packed in together with the datalist.
 * See g_datalist_set_flags().
 * 
 * Return value: the flags of the datalist
 *
 * Since: 2.8
 **/
guint
g_datalist_get_flags (GData **datalist)
{
  g_return_val_if_fail (datalist != NULL, 0);
  
  return G_DATALIST_GET_FLAGS (datalist); /* atomic macro */
}

/* HOLDS: g_dataset_global_lock */
static void
g_data_initialize (void)
{
  g_return_if_fail (g_dataset_location_ht == NULL);

  g_dataset_location_ht = g_hash_table_new (g_direct_hash, NULL);
  g_dataset_cached = NULL;
}

/**
 * SECTION: quarks
 * @title: Quarks
 * @short_description: a 2-way association between a string and a
 *                     unique integer identifier
 *
 * Quarks are associations between strings and integer identifiers.
 * Given either the string or the #GQuark identifier it is possible to
 * retrieve the other.
 *
 * Quarks are used for both <link
 * linkend="glib-datasets">Datasets</link> and <link
 * linkend="glib-keyed-data-lists">Keyed Data Lists</link>.
 *
 * To create a new quark from a string, use g_quark_from_string() or
 * g_quark_from_static_string().
 *
 * To find the string corresponding to a given #GQuark, use
 * g_quark_to_string().
 *
 * To find the #GQuark corresponding to a given string, use
 * g_quark_try_string().
 *
 * Another use for the string pool maintained for the quark functions
 * is string interning, using g_intern_string() or
 * g_intern_static_string(). An interned string is a canonical
 * representation for a string. One important advantage of interned
 * strings is that they can be compared for equality by a simple
 * pointer comparision, rather than using strcmp().
 **/

/**
 * GQuark:
 *
 * A GQuark is a non-zero integer which uniquely identifies a
 * particular string. A GQuark value of zero is associated to %NULL.
 **/

/**
 * g_quark_try_string:
 * @string: a string.
 * @Returns: the #GQuark associated with the string, or 0 if @string is
 *           %NULL or there is no #GQuark associated with it.
 *
 * Gets the #GQuark associated with the given string, or 0 if string is
 * %NULL or it has no associated #GQuark.
 *
 * If you want the GQuark to be created if it doesn't already exist,
 * use g_quark_from_string() or g_quark_from_static_string().
 **/
GQuark
g_quark_try_string (const gchar *string)
{
  GQuark quark = 0;

  if (string == NULL)
    return 0;

  G_LOCK (g_quark_global);
  if (g_quark_ht)
    quark = GPOINTER_TO_UINT (g_hash_table_lookup (g_quark_ht, string));
  G_UNLOCK (g_quark_global);
  
  return quark;
}

#define QUARK_STRING_BLOCK_SIZE (4096 - sizeof (gsize))
static char *quark_block = NULL;
static int quark_block_offset = 0;

/* HOLDS: g_quark_global_lock */
static char *
quark_strdup(const gchar *string)
{
  gchar *copy;
  gsize len;

  len = strlen (string) + 1;

  /* For strings longer than half the block size, fall back
     to strdup so that we fill our blocks at least 50%. */
  if (len > QUARK_STRING_BLOCK_SIZE / 2)
    return g_strdup (string);

  if (quark_block == NULL ||
      QUARK_STRING_BLOCK_SIZE - quark_block_offset < len)
    {
      quark_block = g_malloc (QUARK_STRING_BLOCK_SIZE);
      quark_block_offset = 0;
    }

  copy = quark_block + quark_block_offset;
  memcpy (copy, string, len);
  quark_block_offset += len;

  return copy;
}

/* HOLDS: g_quark_global_lock */
static inline GQuark
g_quark_from_string_internal (const gchar *string, 
			      gboolean     duplicate)
{
  GQuark quark = 0;
  
  if (g_quark_ht)
    quark = GPOINTER_TO_UINT (g_hash_table_lookup (g_quark_ht, string));
  
  if (!quark)
    {
      quark = g_quark_new (duplicate ? quark_strdup (string) : (gchar *)string);
      TRACE(GLIB_QUARK_NEW(string, quark));
    }

  return quark;
}

/**
 * g_quark_from_string:
 * @string: a string.
 * @Returns: the #GQuark identifying the string, or 0 if @string is
 *           %NULL.
 *
 * Gets the #GQuark identifying the given string. If the string does
 * not currently have an associated #GQuark, a new #GQuark is created,
 * using a copy of the string.
 **/
GQuark
g_quark_from_string (const gchar *string)
{
  GQuark quark;
  
  if (!string)
    return 0;
  
  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, TRUE);
  G_UNLOCK (g_quark_global);
  
  return quark;
}

/**
 * g_quark_from_static_string:
 * @string: a string.
 * @Returns: the #GQuark identifying the string, or 0 if @string is
 *           %NULL.
 *
 * Gets the #GQuark identifying the given (static) string. If the
 * string does not currently have an associated #GQuark, a new #GQuark
 * is created, linked to the given string.
 *
 * Note that this function is identical to g_quark_from_string() except
 * that if a new #GQuark is created the string itself is used rather
 * than a copy. This saves memory, but can only be used if the string
 * will <emphasis>always</emphasis> exist. It can be used with
 * statically allocated strings in the main program, but not with
 * statically allocated memory in dynamically loaded modules, if you
 * expect to ever unload the module again (e.g. do not use this
 * function in GTK+ theme engines).
 **/
GQuark
g_quark_from_static_string (const gchar *string)
{
  GQuark quark;
  
  if (!string)
    return 0;
  
  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, FALSE);
  G_UNLOCK (g_quark_global);

  return quark;
}

/**
 * g_quark_to_string:
 * @quark: a #GQuark.
 * @Returns: the string associated with the #GQuark.
 *
 * Gets the string associated with the given #GQuark.
 **/
G_CONST_RETURN gchar*
g_quark_to_string (GQuark quark)
{
  gchar* result = NULL;

  G_LOCK (g_quark_global);
  if (quark < g_quark_seq_id)
    result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}

/* HOLDS: g_quark_global_lock */
static inline GQuark
g_quark_new (gchar *string)
{
  GQuark quark;
  
  if (g_quark_seq_id % G_QUARK_BLOCK_SIZE == 0)
    g_quarks = g_renew (gchar*, g_quarks, g_quark_seq_id + G_QUARK_BLOCK_SIZE);
  if (!g_quark_ht)
    {
      g_assert (g_quark_seq_id == 0);
      g_quark_ht = g_hash_table_new (g_str_hash, g_str_equal);
      g_quarks[g_quark_seq_id++] = NULL;
    }

  quark = g_quark_seq_id++;
  g_quarks[quark] = string;
  g_hash_table_insert (g_quark_ht, string, GUINT_TO_POINTER (quark));
  
  return quark;
}

/**
 * g_intern_string:
 * @string: a string
 * 
 * Returns a canonical representation for @string. Interned strings can
 * be compared for equality by comparing the pointers, instead of using strcmp().
 * 
 * Returns: a canonical representation for the string
 *
 * Since: 2.10
 */
G_CONST_RETURN gchar*
g_intern_string (const gchar *string)
{
  const gchar *result;
  GQuark quark;

  if (!string)
    return NULL;

  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, TRUE);
  result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}

/**
 * g_intern_static_string:
 * @string: a static string
 * 
 * Returns a canonical representation for @string. Interned strings can
 * be compared for equality by comparing the pointers, instead of using strcmp().
 * g_intern_static_string() does not copy the string, therefore @string must
 * not be freed or modified. 
 * 
 * Returns: a canonical representation for the string
 *
 * Since: 2.10
 */
G_CONST_RETURN gchar*
g_intern_static_string (const gchar *string)
{
  GQuark quark;
  const gchar *result;

  if (!string)
    return NULL;

  G_LOCK (g_quark_global);
  quark = g_quark_from_string_internal (string, FALSE);
  result = g_quarks[quark];
  G_UNLOCK (g_quark_global);

  return result;
}
