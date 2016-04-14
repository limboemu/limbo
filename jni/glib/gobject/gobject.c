/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * MT safe with regards to reference counting.
 */

#include "config.h"

#include <string.h>
#include <signal.h>

#include "gobject.h"
#include "gvaluecollector.h"
#include "gsignal.h"
#include "gparamspecs.h"
#include "gvaluetypes.h"
#include "gobject_trace.h"

#include "gobjectnotifyqueue.c"

/**
 * SECTION:objects
 * @short_description: The base object type
 * @see_also: #GParamSpecObject, g_param_spec_object()
 * @title: The Base Object Type
 *
 * GObject is the fundamental type providing the common attributes and
 * methods for all object types in GTK+, Pango and other libraries
 * based on GObject.  The GObject class provides methods for object
 * construction and destruction, property access methods, and signal
 * support.  Signals are described in detail in <xref
 * linkend="gobject-Signals"/>.
 *
 * <para id="floating-ref">
 * #GInitiallyUnowned is derived from #GObject. The only difference between
 * the two is that the initial reference of a #GInitiallyUnowned is flagged
 * as a <firstterm>floating</firstterm> reference.
 * This means that it is not specifically claimed to be "owned" by
 * any code portion. The main motivation for providing floating references is
 * C convenience. In particular, it allows code to be written as:
 * |[
 * container = create_container();
 * container_add_child (container, create_child());
 * ]|
 * If <function>container_add_child()</function> will g_object_ref_sink() the
 * passed in child, no reference of the newly created child is leaked.
 * Without floating references, <function>container_add_child()</function>
 * can only g_object_ref() the new child, so to implement this code without
 * reference leaks, it would have to be written as:
 * |[
 * Child *child;
 * container = create_container();
 * child = create_child();
 * container_add_child (container, child);
 * g_object_unref (child);
 * ]|
 * The floating reference can be converted into
 * an ordinary reference by calling g_object_ref_sink().
 * For already sunken objects (objects that don't have a floating reference
 * anymore), g_object_ref_sink() is equivalent to g_object_ref() and returns
 * a new reference.
 * Since floating references are useful almost exclusively for C convenience,
 * language bindings that provide automated reference and memory ownership
 * maintenance (such as smart pointers or garbage collection) therefore don't
 * need to expose floating references in their API.
 * </para>
 *
 * Some object implementations may need to save an objects floating state
 * across certain code portions (an example is #GtkMenu), to achive this, the
 * following sequence can be used:
 *
 * |[
 * // save floating state
 * gboolean was_floating = g_object_is_floating (object);
 * g_object_ref_sink (object);
 * // protected code portion
 * ...;
 * // restore floating state
 * if (was_floating)
 *   g_object_force_floating (object);
 * g_obejct_unref (object); // release previously acquired reference
 * ]|
 */


/* --- macros --- */
#define PARAM_SPEC_PARAM_ID(pspec)		((pspec)->param_id)
#define	PARAM_SPEC_SET_PARAM_ID(pspec, id)	((pspec)->param_id = (id))

#define OBJECT_HAS_TOGGLE_REF_FLAG 0x1
#define OBJECT_HAS_TOGGLE_REF(object) \
    ((g_datalist_get_flags (&(object)->qdata) & OBJECT_HAS_TOGGLE_REF_FLAG) != 0)
#define OBJECT_FLOATING_FLAG 0x2

#define CLASS_HAS_PROPS_FLAG 0x1
#define CLASS_HAS_PROPS(class) \
    ((class)->flags & CLASS_HAS_PROPS_FLAG)
#define CLASS_HAS_CUSTOM_CONSTRUCTOR(class) \
    ((class)->constructor != g_object_constructor)

#define CLASS_HAS_DERIVED_CLASS_FLAG 0x2
#define CLASS_HAS_DERIVED_CLASS(class) \
    ((class)->flags & CLASS_HAS_DERIVED_CLASS_FLAG)

/* --- signals --- */
enum {
  NOTIFY,
  LAST_SIGNAL
};


/* --- properties --- */
enum {
  PROP_NONE
};


/* --- prototypes --- */
static void	g_object_base_class_init		(GObjectClass	*class);
static void	g_object_base_class_finalize		(GObjectClass	*class);
static void	g_object_do_class_init			(GObjectClass	*class);
static void	g_object_init				(GObject	*object,
							 GObjectClass	*class);
static GObject*	g_object_constructor			(GType                  type,
							 guint                  n_construct_properties,
							 GObjectConstructParam *construct_params);
static void	g_object_real_dispose			(GObject	*object);
static void	g_object_finalize			(GObject	*object);
static void	g_object_do_set_property		(GObject        *object,
							 guint           property_id,
							 const GValue   *value,
							 GParamSpec     *pspec);
static void	g_object_do_get_property		(GObject        *object,
							 guint           property_id,
							 GValue         *value,
							 GParamSpec     *pspec);
static void	g_value_object_init			(GValue		*value);
static void	g_value_object_free_value		(GValue		*value);
static void	g_value_object_copy_value		(const GValue	*src_value,
							 GValue		*dest_value);
static void	g_value_object_transform_value		(const GValue	*src_value,
							 GValue		*dest_value);
static gpointer g_value_object_peek_pointer             (const GValue   *value);
static gchar*	g_value_object_collect_value		(GValue		*value,
							 guint           n_collect_values,
							 GTypeCValue    *collect_values,
							 guint           collect_flags);
static gchar*	g_value_object_lcopy_value		(const GValue	*value,
							 guint           n_collect_values,
							 GTypeCValue    *collect_values,
							 guint           collect_flags);
static void	g_object_dispatch_properties_changed	(GObject	*object,
							 guint		 n_pspecs,
							 GParamSpec    **pspecs);
static inline void         object_get_property		(GObject        *object,
							 GParamSpec     *pspec,
							 GValue         *value);
static inline void	   object_set_property		(GObject        *object,
							 GParamSpec     *pspec,
							 const GValue   *value,
							 GObjectNotifyQueue *nqueue);
static guint               object_floating_flag_handler (GObject        *object,
                                                         gint            job);

static void object_interface_check_properties           (gpointer        func_data,
							 gpointer        g_iface);


/* --- variables --- */
static GQuark	            quark_closure_array = 0;
static GQuark	            quark_weak_refs = 0;
static GQuark	            quark_toggle_refs = 0;
static GParamSpecPool      *pspec_pool = NULL;
static GObjectNotifyContext property_notify_context = { 0, };
static gulong	            gobject_signals[LAST_SIGNAL] = { 0, };
static guint (*floating_flag_handler) (GObject*, gint) = object_floating_flag_handler;
G_LOCK_DEFINE_STATIC (construction_mutex);
static GSList *construction_objects = NULL;

/* --- functions --- */
#ifdef	G_ENABLE_DEBUG
#define	IF_DEBUG(debug_type)	if (_g_type_debug_flags & G_TYPE_DEBUG_ ## debug_type)
G_LOCK_DEFINE_STATIC     (debug_objects);
static volatile GObject *g_trap_object_ref = NULL;
static guint		 debug_objects_count = 0;
static GHashTable	*debug_objects_ht = NULL;

static void
debug_objects_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GObject *object = value;

  g_message ("[%p] stale %s\tref_count=%u",
	     object,
	     G_OBJECT_TYPE_NAME (object),
	     object->ref_count);
}

static void
debug_objects_atexit (void)
{
  IF_DEBUG (OBJECTS)
    {
      G_LOCK (debug_objects);
      g_message ("stale GObjects: %u", debug_objects_count);
      g_hash_table_foreach (debug_objects_ht, debug_objects_foreach, NULL);
      G_UNLOCK (debug_objects);
    }
}
#endif	/* G_ENABLE_DEBUG */

void
g_object_type_init (void)
{
  static gboolean initialized = FALSE;
  static const GTypeFundamentalInfo finfo = {
    G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE,
  };
  static GTypeInfo info = {
    sizeof (GObjectClass),
    (GBaseInitFunc) g_object_base_class_init,
    (GBaseFinalizeFunc) g_object_base_class_finalize,
    (GClassInitFunc) g_object_do_class_init,
    NULL	/* class_destroy */,
    NULL	/* class_data */,
    sizeof (GObject),
    0		/* n_preallocs */,
    (GInstanceInitFunc) g_object_init,
    NULL,	/* value_table */
  };
  static const GTypeValueTable value_table = {
    g_value_object_init,	  /* value_init */
    g_value_object_free_value,	  /* value_free */
    g_value_object_copy_value,	  /* value_copy */
    g_value_object_peek_pointer,  /* value_peek_pointer */
    "p",			  /* collect_format */
    g_value_object_collect_value, /* collect_value */
    "p",			  /* lcopy_format */
    g_value_object_lcopy_value,	  /* lcopy_value */
  };
  GType type;
  
  g_return_if_fail (initialized == FALSE);
  initialized = TRUE;
  
  /* G_TYPE_OBJECT
   */
  info.value_table = &value_table;
  type = g_type_register_fundamental (G_TYPE_OBJECT, g_intern_static_string ("GObject"), &info, &finfo, 0);
  g_assert (type == G_TYPE_OBJECT);
  g_value_register_transform_func (G_TYPE_OBJECT, G_TYPE_OBJECT, g_value_object_transform_value);
  
#ifdef	G_ENABLE_DEBUG
  IF_DEBUG (OBJECTS)
    {
      debug_objects_ht = g_hash_table_new (g_direct_hash, NULL);
      g_atexit (debug_objects_atexit);
    }
#endif	/* G_ENABLE_DEBUG */
}

static void
g_object_base_class_init (GObjectClass *class)
{
  GObjectClass *pclass = g_type_class_peek_parent (class);

  /* Don't inherit HAS_DERIVED_CLASS flag from parent class */
  class->flags &= ~CLASS_HAS_DERIVED_CLASS_FLAG;

  if (pclass)
    pclass->flags |= CLASS_HAS_DERIVED_CLASS_FLAG;

  /* reset instance specific fields and methods that don't get inherited */
  class->construct_properties = pclass ? g_slist_copy (pclass->construct_properties) : NULL;
  class->get_property = NULL;
  class->set_property = NULL;
}

static void
g_object_base_class_finalize (GObjectClass *class)
{
  GList *list, *node;
  
  _g_signals_destroy (G_OBJECT_CLASS_TYPE (class));

  g_slist_free (class->construct_properties);
  class->construct_properties = NULL;
  list = g_param_spec_pool_list_owned (pspec_pool, G_OBJECT_CLASS_TYPE (class));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;
      
      g_param_spec_pool_remove (pspec_pool, pspec);
      PARAM_SPEC_SET_PARAM_ID (pspec, 0);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);
}

static void
g_object_notify_dispatcher (GObject     *object,
			    guint        n_pspecs,
			    GParamSpec **pspecs)
{
  G_OBJECT_GET_CLASS (object)->dispatch_properties_changed (object, n_pspecs, pspecs);
}

static void
g_object_do_class_init (GObjectClass *class)
{
  /* read the comment about typedef struct CArray; on why not to change this quark */
  quark_closure_array = g_quark_from_static_string ("GObject-closure-array");

  quark_weak_refs = g_quark_from_static_string ("GObject-weak-references");
  quark_toggle_refs = g_quark_from_static_string ("GObject-toggle-references");
  pspec_pool = g_param_spec_pool_new (TRUE);
  property_notify_context.quark_notify_queue = g_quark_from_static_string ("GObject-notify-queue");
  property_notify_context.dispatcher = g_object_notify_dispatcher;
  
  class->constructor = g_object_constructor;
  class->set_property = g_object_do_set_property;
  class->get_property = g_object_do_get_property;
  class->dispose = g_object_real_dispose;
  class->finalize = g_object_finalize;
  class->dispatch_properties_changed = g_object_dispatch_properties_changed;
  class->notify = NULL;

  /**
   * GObject::notify:
   * @gobject: the object which received the signal.
   * @pspec: the #GParamSpec of the property which changed.
   *
   * The notify signal is emitted on an object when one of its
   * properties has been changed. Note that getting this signal
   * doesn't guarantee that the value of the property has actually
   * changed, it may also be emitted when the setter for the property
   * is called to reinstate the previous value.
   *
   * This signal is typically used to obtain change notification for a
   * single property, by specifying the property name as a detail in the
   * g_signal_connect() call, like this:
   * |[
   * g_signal_connect (text_view->buffer, "notify::paste-target-list",
   *                   G_CALLBACK (gtk_text_view_target_list_notify),
   *                   text_view)
   * ]|
   * It is important to note that you must use
   * <link linkend="canonical-parameter-name">canonical</link> parameter names as
   * detail strings for the notify signal.
   */
  gobject_signals[NOTIFY] =
    g_signal_new (g_intern_static_string ("notify"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GObjectClass, notify),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__PARAM,
		  G_TYPE_NONE,
		  1, G_TYPE_PARAM);

  /* Install a check function that we'll use to verify that classes that
   * implement an interface implement all properties for that interface
   */
  g_type_add_interface_check (NULL, object_interface_check_properties);
}

static inline void
install_property_internal (GType       g_type,
			   guint       property_id,
			   GParamSpec *pspec)
{
  if (g_param_spec_pool_lookup (pspec_pool, pspec->name, g_type, FALSE))
    {
      g_warning ("When installing property: type `%s' already has a property named `%s'",
		 g_type_name (g_type),
		 pspec->name);
      return;
    }

  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (pspec_pool, pspec, g_type);
}

/**
 * g_object_class_install_property:
 * @oclass: a #GObjectClass
 * @property_id: the id for the new property
 * @pspec: the #GParamSpec for the new property
 *
 * Installs a new property. This is usually done in the class initializer.
 *
 * Note that it is possible to redefine a property in a derived class,
 * by installing a property with the same name. This can be useful at times,
 * e.g. to change the range of allowed values or the default value.
 */
void
g_object_class_install_property (GObjectClass *class,
				 guint	       property_id,
				 GParamSpec   *pspec)
{
  g_return_if_fail (G_IS_OBJECT_CLASS (class));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (CLASS_HAS_DERIVED_CLASS (class))
    g_error ("Attempt to add property %s::%s to class after it was derived",
	     G_OBJECT_CLASS_NAME (class), pspec->name);

  class->flags |= CLASS_HAS_PROPS_FLAG;

  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (class->set_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (class->get_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);	/* paranoid */
  if (pspec->flags & G_PARAM_CONSTRUCT)
    g_return_if_fail ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail (pspec->flags & G_PARAM_WRITABLE);

  install_property_internal (G_OBJECT_CLASS_TYPE (class), property_id, pspec);

  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    class->construct_properties = g_slist_prepend (class->construct_properties, pspec);

  /* for property overrides of construct properties, we have to get rid
   * of the overidden inherited construct property
   */
  pspec = g_param_spec_pool_lookup (pspec_pool, pspec->name, g_type_parent (G_OBJECT_CLASS_TYPE (class)), TRUE);
  if (pspec && pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    class->construct_properties = g_slist_remove (class->construct_properties, pspec);
}

/**
 * g_object_class_install_properties:
 * @oclass: a #GObjectClass
 * @n_pspecs: the length of the #GParamSpec<!-- -->s array
 * @pspecs: (array length=n_pspecs): the #GParamSpec<!-- -->s array
 *   defining the new properties
 *
 * Installs new properties from an array of #GParamSpec<!-- -->s. This is
 * usually done in the class initializer.
 *
 * The property id of each property is the index of each #GParamSpec in
 * the @pspecs array.
 *
 * The property id of 0 is treated specially by #GObject and it should not
 * be used to store a #GParamSpec.
 *
 * This function should be used if you plan to use a static array of
 * #GParamSpec<!-- -->s and g_object_notify_pspec(). For instance, this
 * class initialization:
 *
 * |[
 * enum {
 *   PROP_0, PROP_FOO, PROP_BAR, N_PROPERTIES
 * };
 *
 * static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
 *
 * static void
 * my_object_class_init (MyObjectClass *klass)
 * {
 *   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
 *
 *   obj_properties[PROP_FOO] =
 *     g_param_spec_int ("foo", "Foo", "Foo",
 *                       -1, G_MAXINT,
 *                       0,
 *                       G_PARAM_READWRITE);
 *
 *   obj_properties[PROP_BAR] =
 *     g_param_spec_string ("bar", "Bar", "Bar",
 *                          NULL,
 *                          G_PARAM_READWRITE);
 *
 *   gobject_class->set_property = my_object_set_property;
 *   gobject_class->get_property = my_object_get_property;
 *   g_object_class_install_properties (gobject_class,
 *                                      N_PROPERTIES,
 *                                      obj_properties);
 * }
 * ]|
 *
 * allows calling g_object_notify_by_pspec() to notify of property changes:
 *
 * |[
 * void
 * my_object_set_foo (MyObject *self, gint foo)
 * {
 *   if (self->foo != foo)
 *     {
 *       self->foo = foo;
 *       g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_FOO]);
 *     }
 *  }
 * ]|
 *
 * Since: 2.26
 */
void
g_object_class_install_properties (GObjectClass  *oclass,
                                   guint          n_pspecs,
                                   GParamSpec   **pspecs)
{
  GType oclass_type;
  gint i;

  g_return_if_fail (G_IS_OBJECT_CLASS (oclass));
  g_return_if_fail (n_pspecs > 1);
  g_return_if_fail (pspecs[0] == NULL);

  if (CLASS_HAS_DERIVED_CLASS (oclass))
    g_error ("Attempt to add properties to %s after it was derived",
             G_OBJECT_CLASS_NAME (oclass));

  oclass_type = G_OBJECT_CLASS_TYPE (oclass);

  /* we skip the first element of the array as it would have a 0 prop_id */
  for (i = 1; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      g_return_if_fail (pspec != NULL);

      if (pspec->flags & G_PARAM_WRITABLE)
        g_return_if_fail (oclass->set_property != NULL);
      if (pspec->flags & G_PARAM_READABLE)
        g_return_if_fail (oclass->get_property != NULL);
      g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);	/* paranoid */
      if (pspec->flags & G_PARAM_CONSTRUCT)
        g_return_if_fail ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);
      if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
        g_return_if_fail (pspec->flags & G_PARAM_WRITABLE);

      oclass->flags |= CLASS_HAS_PROPS_FLAG;
      install_property_internal (oclass_type, i, pspec);

      if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
        oclass->construct_properties = g_slist_prepend (oclass->construct_properties, pspec);

      /* for property overrides of construct properties, we have to get rid
       * of the overidden inherited construct property
       */
      pspec = g_param_spec_pool_lookup (pspec_pool, pspec->name, g_type_parent (G_OBJECT_CLASS_TYPE (oclass)), TRUE);
      if (pspec && pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
        oclass->construct_properties = g_slist_remove (oclass->construct_properties, pspec);
    }
}

/**
 * g_object_interface_install_property:
 * @g_iface: any interface vtable for the interface, or the default
 *  vtable for the interface.
 * @pspec: the #GParamSpec for the new property
 *
 * Add a property to an interface; this is only useful for interfaces
 * that are added to GObject-derived types. Adding a property to an
 * interface forces all objects classes with that interface to have a
 * compatible property. The compatible property could be a newly
 * created #GParamSpec, but normally
 * g_object_class_override_property() will be used so that the object
 * class only needs to provide an implementation and inherits the
 * property description, default value, bounds, and so forth from the
 * interface property.
 *
 * This function is meant to be called from the interface's default
 * vtable initialization function (the @class_init member of
 * #GTypeInfo.) It must not be called after after @class_init has
 * been called for any object types implementing this interface.
 *
 * Since: 2.4
 */
void
g_object_interface_install_property (gpointer      g_iface,
				     GParamSpec   *pspec)
{
  GTypeInterface *iface_class = g_iface;
	
  g_return_if_fail (G_TYPE_IS_INTERFACE (iface_class->g_type));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (!G_IS_PARAM_SPEC_OVERRIDE (pspec)); /* paranoid */
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);	/* paranoid */
		    
  install_property_internal (iface_class->g_type, 0, pspec);
}

/**
 * g_object_class_find_property:
 * @oclass: a #GObjectClass
 * @property_name: the name of the property to look up
 *
 * Looks up the #GParamSpec for a property of a class.
 *
 * Returns: the #GParamSpec for the property, or %NULL if the class
 *          doesn't have a property of that name
 */
GParamSpec*
g_object_class_find_property (GObjectClass *class,
			      const gchar  *property_name)
{
  GParamSpec *pspec;
  GParamSpec *redirect;
	
  g_return_val_if_fail (G_IS_OBJECT_CLASS (class), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_CLASS_TYPE (class),
				    TRUE);
  if (pspec)
    {
      redirect = g_param_spec_get_redirect_target (pspec);
      if (redirect)
	return redirect;
      else
	return pspec;
    }
  else
    return NULL;
}

/**
 * g_object_interface_find_property:
 * @g_iface: any interface vtable for the interface, or the default
 *  vtable for the interface
 * @property_name: name of a property to lookup.
 *
 * Find the #GParamSpec with the given name for an
 * interface. Generally, the interface vtable passed in as @g_iface
 * will be the default vtable from g_type_default_interface_ref(), or,
 * if you know the interface has already been loaded,
 * g_type_default_interface_peek().
 *
 * Since: 2.4
 *
 * Returns: the #GParamSpec for the property of the interface with the
 *          name @property_name, or %NULL if no such property exists.
 */
GParamSpec*
g_object_interface_find_property (gpointer      g_iface,
				  const gchar  *property_name)
{
  GTypeInterface *iface_class = g_iface;
	
  g_return_val_if_fail (G_TYPE_IS_INTERFACE (iface_class->g_type), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  
  return g_param_spec_pool_lookup (pspec_pool,
				   property_name,
				   iface_class->g_type,
				   FALSE);
}

/**
 * g_object_class_override_property:
 * @oclass: a #GObjectClass
 * @property_id: the new property ID
 * @name: the name of a property registered in a parent class or
 *  in an interface of this class.
 *
 * Registers @property_id as referring to a property with the
 * name @name in a parent class or in an interface implemented
 * by @oclass. This allows this class to <firstterm>override</firstterm>
 * a property implementation in a parent class or to provide
 * the implementation of a property from an interface.
 *
 * <note>
 * Internally, overriding is implemented by creating a property of type
 * #GParamSpecOverride; generally operations that query the properties of
 * the object class, such as g_object_class_find_property() or
 * g_object_class_list_properties() will return the overridden
 * property. However, in one case, the @construct_properties argument of
 * the @constructor virtual function, the #GParamSpecOverride is passed
 * instead, so that the @param_id field of the #GParamSpec will be
 * correct.  For virtually all uses, this makes no difference. If you
 * need to get the overridden property, you can call
 * g_param_spec_get_redirect_target().
 * </note>
 *
 * Since: 2.4
 */
void
g_object_class_override_property (GObjectClass *oclass,
				  guint         property_id,
				  const gchar  *name)
{
  GParamSpec *overridden = NULL;
  GParamSpec *new;
  GType parent_type;
  
  g_return_if_fail (G_IS_OBJECT_CLASS (oclass));
  g_return_if_fail (property_id > 0);
  g_return_if_fail (name != NULL);

  /* Find the overridden property; first check parent types
   */
  parent_type = g_type_parent (G_OBJECT_CLASS_TYPE (oclass));
  if (parent_type != G_TYPE_NONE)
    overridden = g_param_spec_pool_lookup (pspec_pool,
					   name,
					   parent_type,
					   TRUE);
  if (!overridden)
    {
      GType *ifaces;
      guint n_ifaces;
      
      /* Now check interfaces
       */
      ifaces = g_type_interfaces (G_OBJECT_CLASS_TYPE (oclass), &n_ifaces);
      while (n_ifaces-- && !overridden)
	{
	  overridden = g_param_spec_pool_lookup (pspec_pool,
						 name,
						 ifaces[n_ifaces],
						 FALSE);
	}
      
      g_free (ifaces);
    }

  if (!overridden)
    {
      g_warning ("%s: Can't find property to override for '%s::%s'",
		 G_STRFUNC, G_OBJECT_CLASS_NAME (oclass), name);
      return;
    }

  new = g_param_spec_override (name, overridden);
  g_object_class_install_property (oclass, property_id, new);
}

/**
 * g_object_class_list_properties:
 * @oclass: a #GObjectClass
 * @n_properties: return location for the length of the returned array
 *
 * Get an array of #GParamSpec* for all properties of a class.
 *
 * Returns: an array of #GParamSpec* which should be freed after use
 */
GParamSpec** /* free result */
g_object_class_list_properties (GObjectClass *class,
				guint        *n_properties_p)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (G_IS_OBJECT_CLASS (class), NULL);

  pspecs = g_param_spec_pool_list (pspec_pool,
				   G_OBJECT_CLASS_TYPE (class),
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}

/**
 * g_object_interface_list_properties:
 * @g_iface: any interface vtable for the interface, or the default
 *  vtable for the interface
 * @n_properties_p: location to store number of properties returned.
 *
 * Lists the properties of an interface.Generally, the interface
 * vtable passed in as @g_iface will be the default vtable from
 * g_type_default_interface_ref(), or, if you know the interface has
 * already been loaded, g_type_default_interface_peek().
 *
 * Since: 2.4
 *
 * Returns: a pointer to an array of pointers to #GParamSpec
 *          structures. The paramspecs are owned by GLib, but the
 *          array should be freed with g_free() when you are done with
 *          it.
 */
GParamSpec**
g_object_interface_list_properties (gpointer      g_iface,
				    guint        *n_properties_p)
{
  GTypeInterface *iface_class = g_iface;
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (G_TYPE_IS_INTERFACE (iface_class->g_type), NULL);

  pspecs = g_param_spec_pool_list (pspec_pool,
				   iface_class->g_type,
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}

static void
g_object_init (GObject		*object,
	       GObjectClass	*class)
{
  object->ref_count = 1;
  g_datalist_init (&object->qdata);

  if (CLASS_HAS_PROPS (class))
    {
      /* freeze object's notification queue, g_object_newv() preserves pairedness */
      g_object_notify_queue_freeze (object, &property_notify_context);
    }

  if (CLASS_HAS_CUSTOM_CONSTRUCTOR (class))
    {
      /* enter construction list for notify_queue_thaw() and to allow construct-only properties */
      G_LOCK (construction_mutex);
      construction_objects = g_slist_prepend (construction_objects, object);
      G_UNLOCK (construction_mutex);
    }

#ifdef	G_ENABLE_DEBUG
  IF_DEBUG (OBJECTS)
    {
      G_LOCK (debug_objects);
      debug_objects_count++;
      g_hash_table_insert (debug_objects_ht, object, object);
      G_UNLOCK (debug_objects);
    }
#endif	/* G_ENABLE_DEBUG */
}

static void
g_object_do_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_object_do_get_property (GObject     *object,
			  guint        property_id,
			  GValue      *value,
			  GParamSpec  *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_object_real_dispose (GObject *object)
{
  g_signal_handlers_destroy (object);
  g_datalist_id_set_data (&object->qdata, quark_closure_array, NULL);
  g_datalist_id_set_data (&object->qdata, quark_weak_refs, NULL);
}

static void
g_object_finalize (GObject *object)
{
  g_datalist_clear (&object->qdata);
  
#ifdef	G_ENABLE_DEBUG
  IF_DEBUG (OBJECTS)
    {
      G_LOCK (debug_objects);
      g_assert (g_hash_table_lookup (debug_objects_ht, object) == object);
      g_hash_table_remove (debug_objects_ht, object);
      debug_objects_count--;
      G_UNLOCK (debug_objects);
    }
#endif	/* G_ENABLE_DEBUG */
}


static void
g_object_dispatch_properties_changed (GObject     *object,
				      guint        n_pspecs,
				      GParamSpec **pspecs)
{
  guint i;

  for (i = 0; i < n_pspecs; i++)
    g_signal_emit (object, gobject_signals[NOTIFY], g_quark_from_string (pspecs[i]->name), pspecs[i]);
}

/**
 * g_object_run_dispose:
 * @object: a #GObject
 *
 * Releases all references to other objects. This can be used to break
 * reference cycles.
 *
 * This functions should only be called from object system implementations.
 */
void
g_object_run_dispose (GObject *object)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  g_object_ref (object);
  TRACE (GOBJECT_OBJECT_DISPOSE(object,G_TYPE_FROM_INSTANCE(object), 0));
  G_OBJECT_GET_CLASS (object)->dispose (object);
  TRACE (GOBJECT_OBJECT_DISPOSE_END(object,G_TYPE_FROM_INSTANCE(object), 0));
  g_object_unref (object);
}

/**
 * g_object_freeze_notify:
 * @object: a #GObject
 *
 * Increases the freeze count on @object. If the freeze count is
 * non-zero, the emission of "notify" signals on @object is
 * stopped. The signals are queued until the freeze count is decreased
 * to zero.
 *
 * This is necessary for accessors that modify multiple properties to prevent
 * premature notification while the object is still being modified.
 */
void
g_object_freeze_notify (GObject *object)
{
  g_return_if_fail (G_IS_OBJECT (object));

  if (g_atomic_int_get (&object->ref_count) == 0)
    return;

  g_object_ref (object);
  g_object_notify_queue_freeze (object, &property_notify_context);
  g_object_unref (object);
}

static inline void
g_object_notify_by_spec_internal (GObject    *object,
				  GParamSpec *pspec)
{
  GObjectNotifyQueue *nqueue;

  nqueue = g_object_notify_queue_freeze (object, &property_notify_context);
  g_object_notify_queue_add (object, nqueue, pspec);
  g_object_notify_queue_thaw (object, nqueue);
}

/**
 * g_object_notify:
 * @object: a #GObject
 * @property_name: the name of a property installed on the class of @object.
 *
 * Emits a "notify" signal for the property @property_name on @object.
 *
 * When possible, eg. when signaling a property change from within the class
 * that registered the property, you should use g_object_notify_by_pspec()
 * instead.
 */
void
g_object_notify (GObject     *object,
		 const gchar *property_name)
{
  GParamSpec *pspec;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  if (g_atomic_int_get (&object->ref_count) == 0)
    return;
  
  g_object_ref (object);
  /* We don't need to get the redirect target
   * (by, e.g. calling g_object_class_find_property())
   * because g_object_notify_queue_add() does that
   */
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE);

  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRFUNC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else
    g_object_notify_by_spec_internal (object, pspec);
  g_object_unref (object);
}

/**
 * g_object_notify_by_pspec:
 * @object: a #GObject
 * @pspec: the #GParamSpec of a property installed on the class of @object.
 *
 * Emits a "notify" signal for the property specified by @pspec on @object.
 *
 * This function omits the property name lookup, hence it is faster than
 * g_object_notify().
 *
 * One way to avoid using g_object_notify() from within the
 * class that registered the properties, and using g_object_notify_by_pspec()
 * instead, is to store the GParamSpec used with
 * g_object_class_install_property() inside a static array, e.g.:
 *
 *|[
 *   enum
 *   {
 *     PROP_0,
 *     PROP_FOO,
 *     PROP_LAST
 *   };
 *
 *   static GParamSpec *properties[PROP_LAST];
 *
 *   static void
 *   my_object_class_init (MyObjectClass *klass)
 *   {
 *     properties[PROP_FOO] = g_param_spec_int ("foo", "Foo", "The foo",
 *                                              0, 100,
 *                                              50,
 *                                              G_PARAM_READWRITE);
 *     g_object_class_install_property (gobject_class,
 *                                      PROP_FOO,
 *                                      properties[PROP_FOO]);
 *   }
 * ]|
 *
 * and then notify a change on the "foo" property with:
 *
 * |[
 *   g_object_notify_by_pspec (self, properties[PROP_FOO]);
 * ]|
 *
 * Since: 2.26
 */
void
g_object_notify_by_pspec (GObject    *object,
			  GParamSpec *pspec)
{

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  g_object_ref (object);
  g_object_notify_by_spec_internal (object, pspec);
  g_object_unref (object);
}

/**
 * g_object_thaw_notify:
 * @object: a #GObject
 *
 * Reverts the effect of a previous call to
 * g_object_freeze_notify(). The freeze count is decreased on @object
 * and when it reaches zero, all queued "notify" signals are emitted.
 *
 * It is an error to call this function when the freeze count is zero.
 */
void
g_object_thaw_notify (GObject *object)
{
  GObjectNotifyQueue *nqueue;
  
  g_return_if_fail (G_IS_OBJECT (object));
  if (g_atomic_int_get (&object->ref_count) == 0)
    return;
  
  g_object_ref (object);

  /* FIXME: Freezing is the only way to get at the notify queue.
   * So we freeze once and then thaw twice.
   */
  nqueue = g_object_notify_queue_freeze (object,  &property_notify_context);
  g_object_notify_queue_thaw (object, nqueue);
  g_object_notify_queue_thaw (object, nqueue);

  g_object_unref (object);
}

static inline void
object_get_property (GObject     *object,
		     GParamSpec  *pspec,
		     GValue      *value)
{
  GObjectClass *class = g_type_class_peek (pspec->owner_type);
  guint param_id = PARAM_SPEC_PARAM_ID (pspec);
  GParamSpec *redirect;

  redirect = g_param_spec_get_redirect_target (pspec);
  if (redirect)
    pspec = redirect;    
  
  class->get_property (object, param_id, value, pspec);
}

static inline void
object_set_property (GObject             *object,
		     GParamSpec          *pspec,
		     const GValue        *value,
		     GObjectNotifyQueue  *nqueue)
{
  GValue tmp_value = { 0, };
  GObjectClass *class = g_type_class_peek (pspec->owner_type);
  guint param_id = PARAM_SPEC_PARAM_ID (pspec);
  GParamSpec *redirect;
  static gchar* enable_diagnostic = NULL;

  redirect = g_param_spec_get_redirect_target (pspec);
  if (redirect)
    pspec = redirect;

  if (G_UNLIKELY (!enable_diagnostic))
    {
      enable_diagnostic = g_getenv ("G_ENABLE_DIAGNOSTIC");
      if (!enable_diagnostic)
        enable_diagnostic = "0";
    }

  if (enable_diagnostic[0] == '1')
    {
      if (pspec->flags & G_PARAM_DEPRECATED)
        g_warning ("The property %s::%s is deprecated and shouldn't be used "
                   "anymore. It will be removed in a future version.",
                   G_OBJECT_TYPE_NAME (object), pspec->name);
    }

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, pspec->value_type);
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set property `%s' of type `%s' from value of type `%s'",
	       pspec->name,
	       g_type_name (pspec->value_type),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type `%s' is invalid or out of range for property `%s' of type `%s'",
		 contents,
		 G_VALUE_TYPE_NAME (value),
		 pspec->name,
		 g_type_name (pspec->value_type));
      g_free (contents);
    }
  else
    {
      class->set_property (object, param_id, &tmp_value, pspec);
      g_object_notify_queue_add (object, nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}

static void
object_interface_check_properties (gpointer func_data,
				   gpointer g_iface)
{
  GTypeInterface *iface_class = g_iface;
  GObjectClass *class = g_type_class_peek (iface_class->g_instance_type);
  GType iface_type = iface_class->g_type;
  GParamSpec **pspecs;
  guint n;

  if (!G_IS_OBJECT_CLASS (class))
    return;

  pspecs = g_param_spec_pool_list (pspec_pool, iface_type, &n);

  while (n--)
    {
      GParamSpec *class_pspec = g_param_spec_pool_lookup (pspec_pool,
							  pspecs[n]->name,
							  G_OBJECT_CLASS_TYPE (class),
							  TRUE);
      
      if (!class_pspec)
	{
	  g_critical ("Object class %s doesn't implement property "
		      "'%s' from interface '%s'",
		      g_type_name (G_OBJECT_CLASS_TYPE (class)),
		      pspecs[n]->name,
		      g_type_name (iface_type));

	  continue;
	}

      /* The implementation paramspec must have a less restrictive
       * type than the interface parameter spec for set() and a
       * more restrictive type for get(). We just require equality,
       * rather than doing something more complicated checking
       * the READABLE and WRITABLE flags. We also simplify here
       * by only checking the value type, not the G_PARAM_SPEC_TYPE.
       */
      if (class_pspec &&
	  !g_type_is_a (pspecs[n]->value_type,
			class_pspec->value_type))
	{
	  g_critical ("Property '%s' on class '%s' has type '%s' "
		      "which is different from the type '%s', "
		      "of the property on interface '%s'\n",
		      pspecs[n]->name,
		      g_type_name (G_OBJECT_CLASS_TYPE (class)),
		      g_type_name (G_PARAM_SPEC_VALUE_TYPE (class_pspec)),
		      g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspecs[n])),
		      g_type_name (iface_type));
	}
      
#define SUBSET(a,b,mask) (((a) & ~(b) & (mask)) == 0)
      
      /* CONSTRUCT and CONSTRUCT_ONLY add restrictions.
       * READABLE and WRITABLE remove restrictions. The implementation
       * paramspec must have less restrictive flags.
       */
      if (class_pspec &&
	  (!SUBSET (class_pspec->flags,
		    pspecs[n]->flags,
		    G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY) ||
	   !SUBSET (pspecs[n]->flags,
		    class_pspec->flags,
		    G_PARAM_READABLE | G_PARAM_WRITABLE)))
	{
	  g_critical ("Flags for property '%s' on class '%s' "
		      "are not compatible with the property on"
		      "interface '%s'\n",
		      pspecs[n]->name,
		      g_type_name (G_OBJECT_CLASS_TYPE (class)),
		      g_type_name (iface_type));
	}
#undef SUBSET	  
    }
  
  g_free (pspecs);
}

GType
g_object_get_type (void)
{
    return G_TYPE_OBJECT;
}

/**
 * g_object_new:
 * @object_type: the type id of the #GObject subtype to instantiate
 * @first_property_name: the name of the first property
 * @...: the value of the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Creates a new instance of a #GObject subtype and sets its properties.
 *
 * Construction parameters (see #G_PARAM_CONSTRUCT, #G_PARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: a new instance of @object_type
 */
gpointer
g_object_new (GType	   object_type,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object;
  va_list var_args;
  
  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);
  
  /* short circuit for calls supplying no properties */
  if (!first_property_name)
    return g_object_newv (object_type, 0, NULL);

  va_start (var_args, first_property_name);
  object = g_object_new_valist (object_type, first_property_name, var_args);
  va_end (var_args);
  
  return object;
}

static gboolean
slist_maybe_remove (GSList       **slist,
                    gconstpointer  data)
{
  GSList *last = NULL, *node = *slist;
  while (node)
    {
      if (node->data == data)
        {
          if (last)
            last->next = node->next;
          else
            *slist = node->next;
          g_slist_free_1 (node);
          return TRUE;
        }
      last = node;
      node = last->next;
    }
  return FALSE;
}

static inline gboolean
object_in_construction_list (GObject *object)
{
  gboolean in_construction;
  G_LOCK (construction_mutex);
  in_construction = g_slist_find (construction_objects, object) != NULL;
  G_UNLOCK (construction_mutex);
  return in_construction;
}

/**
 * g_object_newv:
 * @object_type: the type id of the #GObject subtype to instantiate
 * @n_parameters: the length of the @parameters array
 * @parameters: an array of #GParameter
 *
 * Creates a new instance of a #GObject subtype and sets its properties.
 *
 * Construction parameters (see #G_PARAM_CONSTRUCT, #G_PARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: a new instance of @object_type
 */
gpointer
g_object_newv (GType       object_type,
	       guint       n_parameters,
	       GParameter *parameters)
{
  GObjectConstructParam *cparams = NULL, *oparams;
  GObjectNotifyQueue *nqueue = NULL; /* shouldn't be initialized, just to silence compiler */
  GObject *object;
  GObjectClass *class, *unref_class = NULL;
  GSList *slist;
  guint n_total_cparams = 0, n_cparams = 0, n_oparams = 0, n_cvalues;
  GValue *cvalues;
  GList *clist = NULL;
  gboolean newly_constructed;
  guint i;

  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);

  class = g_type_class_peek_static (object_type);
  if (!class)
    class = unref_class = g_type_class_ref (object_type);
  for (slist = class->construct_properties; slist; slist = slist->next)
    {
      clist = g_list_prepend (clist, slist->data);
      n_total_cparams += 1;
    }

  if (n_parameters == 0 && n_total_cparams == 0)
    {
      /* This is a simple object with no construct properties, and
       * no properties are being set, so short circuit the parameter
       * handling. This speeds up simple object construction.
       */
      oparams = NULL;
      object = class->constructor (object_type, 0, NULL);
      goto did_construction;
    }

  /* collect parameters, sort into construction and normal ones */
  oparams = g_new (GObjectConstructParam, n_parameters);
  cparams = g_new (GObjectConstructParam, n_total_cparams);
  for (i = 0; i < n_parameters; i++)
    {
      GValue *value = &parameters[i].value;
      GParamSpec *pspec = g_param_spec_pool_lookup (pspec_pool,
						    parameters[i].name,
						    object_type,
						    TRUE);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRFUNC,
		     g_type_name (object_type),
		     parameters[i].name);
	  continue;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not writable",
		     G_STRFUNC,
		     pspec->name,
		     g_type_name (object_type));
	  continue;
	}
      if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
	{
	  GList *list = g_list_find (clist, pspec);

	  if (!list)
	    {
	      g_warning ("%s: construct property \"%s\" for object `%s' can't be set twice",
                         G_STRFUNC, pspec->name, g_type_name (object_type));
	      continue;
	    }
	  cparams[n_cparams].pspec = pspec;
	  cparams[n_cparams].value = value;
	  n_cparams++;
	  if (!list->prev)
	    clist = list->next;
	  else
	    list->prev->next = list->next;
	  if (list->next)
	    list->next->prev = list->prev;
	  g_list_free_1 (list);
	}
      else
	{
	  oparams[n_oparams].pspec = pspec;
	  oparams[n_oparams].value = value;
	  n_oparams++;
	}
    }

  /* set remaining construction properties to default values */
  n_cvalues = n_total_cparams - n_cparams;
  cvalues = g_new (GValue, n_cvalues);
  while (clist)
    {
      GList *tmp = clist->next;
      GParamSpec *pspec = clist->data;
      GValue *value = cvalues + n_total_cparams - n_cparams - 1;

      value->g_type = 0;
      g_value_init (value, pspec->value_type);
      g_param_value_set_default (pspec, value);

      cparams[n_cparams].pspec = pspec;
      cparams[n_cparams].value = value;
      n_cparams++;

      g_list_free_1 (clist);
      clist = tmp;
    }

  /* construct object from construction parameters */
  object = class->constructor (object_type, n_total_cparams, cparams);
  /* free construction values */
  g_free (cparams);
  while (n_cvalues--)
    g_value_unset (cvalues + n_cvalues);
  g_free (cvalues);

 did_construction:
  if (CLASS_HAS_CUSTOM_CONSTRUCTOR (class))
    {
      /* adjust freeze_count according to g_object_init() and remaining properties */
      G_LOCK (construction_mutex);
      newly_constructed = slist_maybe_remove (&construction_objects, object);
      G_UNLOCK (construction_mutex);
    }
  else
    newly_constructed = TRUE;

  if (CLASS_HAS_PROPS (class))
    {
      if (newly_constructed || n_oparams)
	nqueue = g_object_notify_queue_freeze (object, &property_notify_context);
      if (newly_constructed)
	g_object_notify_queue_thaw (object, nqueue);
    }

  /* run 'constructed' handler if there is one */
  if (newly_constructed && class->constructed)
    class->constructed (object);

  /* set remaining properties */
  for (i = 0; i < n_oparams; i++)
    object_set_property (object, oparams[i].pspec, oparams[i].value, nqueue);
  g_free (oparams);

  if (CLASS_HAS_PROPS (class))
    {
      /* release our own freeze count and handle notifications */
      if (newly_constructed || n_oparams)
	g_object_notify_queue_thaw (object, nqueue);
    }

  if (unref_class)
    g_type_class_unref (unref_class);

  return object;
}

/**
 * g_object_new_valist:
 * @object_type: the type id of the #GObject subtype to instantiate
 * @first_property_name: the name of the first property
 * @var_args: the value of the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Creates a new instance of a #GObject subtype and sets its properties.
 *
 * Construction parameters (see #G_PARAM_CONSTRUCT, #G_PARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: a new instance of @object_type
 */
GObject*
g_object_new_valist (GType	  object_type,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  GObjectClass *class;
  GParameter *params;
  const gchar *name;
  GObject *object;
  guint n_params = 0, n_alloced_params = 16;
  
  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);

  if (!first_property_name)
    return g_object_newv (object_type, 0, NULL);

  class = g_type_class_ref (object_type);

  params = g_new0 (GParameter, n_alloced_params);
  name = first_property_name;
  while (name)
    {
      gchar *error = NULL;
      GParamSpec *pspec = g_param_spec_pool_lookup (pspec_pool,
						    name,
						    object_type,
						    TRUE);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRFUNC,
		     g_type_name (object_type),
		     name);
	  break;
	}
      if (n_params >= n_alloced_params)
	{
	  n_alloced_params += 16;
	  params = g_renew (GParameter, params, n_alloced_params);
	}
      params[n_params].name = name;
      G_VALUE_COLLECT_INIT (&params[n_params].value, pspec->value_type,
			    var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRFUNC, error);
	  g_free (error);
          g_value_unset (&params[n_params].value);
	  break;
	}
      n_params++;
      name = va_arg (var_args, gchar*);
    }

  object = g_object_newv (object_type, n_params, params);

  while (n_params--)
    g_value_unset (&params[n_params].value);
  g_free (params);

  g_type_class_unref (class);

  return object;
}

static GObject*
g_object_constructor (GType                  type,
		      guint                  n_construct_properties,
		      GObjectConstructParam *construct_params)
{
  GObject *object;

  /* create object */
  object = (GObject*) g_type_create_instance (type);
  
  /* set construction parameters */
  if (n_construct_properties)
    {
      GObjectNotifyQueue *nqueue = g_object_notify_queue_freeze (object, &property_notify_context);
      
      /* set construct properties */
      while (n_construct_properties--)
	{
	  GValue *value = construct_params->value;
	  GParamSpec *pspec = construct_params->pspec;

	  construct_params++;
	  object_set_property (object, pspec, value, nqueue);
	}
      g_object_notify_queue_thaw (object, nqueue);
      /* the notification queue is still frozen from g_object_init(), so
       * we don't need to handle it here, g_object_newv() takes
       * care of that
       */
    }

  return object;
}

/**
 * g_object_set_valist:
 * @object: a #GObject
 * @first_property_name: name of the first property to set
 * @var_args: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Sets properties on an object.
 */
void
g_object_set_valist (GObject	 *object,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  GObjectNotifyQueue *nqueue;
  const gchar *name;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  g_object_ref (object);
  nqueue = g_object_notify_queue_freeze (object, &property_notify_context);
  
  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error = NULL;
      
      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRFUNC,
		     G_OBJECT_TYPE_NAME (object),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not writable",
		     G_STRFUNC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (object));
	  break;
	}
      if ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) && !object_in_construction_list (object))
        {
          g_warning ("%s: construct property \"%s\" for object `%s' can't be set after construction",
                     G_STRFUNC, pspec->name, G_OBJECT_TYPE_NAME (object));
          break;
        }

      G_VALUE_COLLECT_INIT (&value, pspec->value_type, var_args,
			    0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRFUNC, error);
	  g_free (error);
          g_value_unset (&value);
	  break;
	}
      
      object_set_property (object, pspec, &value, nqueue);
      g_value_unset (&value);
      
      name = va_arg (var_args, gchar*);
    }

  g_object_notify_queue_thaw (object, nqueue);
  g_object_unref (object);
}

/**
 * g_object_get_valist:
 * @object: a #GObject
 * @first_property_name: name of the first property to get
 * @var_args: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %NULL
 *
 * Gets properties of an object.
 *
 * In general, a copy is made of the property contents and the caller
 * is responsible for freeing the memory in the appropriate manner for
 * the type, for instance by calling g_free() or g_object_unref().
 *
 * See g_object_get().
 */
void
g_object_get_valist (GObject	 *object,
		     const gchar *first_property_name,
		     va_list	  var_args)
{
  const gchar *name;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  g_object_ref (object);
  
  name = first_property_name;
  
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error;
      
      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRFUNC,
		     G_OBJECT_TYPE_NAME (object),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: property `%s' of object class `%s' is not readable",
		     G_STRFUNC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (object));
	  break;
	}
      
      g_value_init (&value, pspec->value_type);
      
      object_get_property (object, pspec, &value);
      
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRFUNC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      
      g_value_unset (&value);
      
      name = va_arg (var_args, gchar*);
    }
  
  g_object_unref (object);
}

/**
 * g_object_set:
 * @object: a #GObject
 * @first_property_name: name of the first property to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Sets properties on an object.
 */
void
g_object_set (gpointer     _object,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object = _object;
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_set_valist (object, first_property_name, var_args);
  va_end (var_args);
}

/**
 * g_object_get:
 * @object: a #GObject
 * @first_property_name: name of the first property to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %NULL
 *
 * Gets properties of an object.
 *
 * In general, a copy is made of the property contents and the caller
 * is responsible for freeing the memory in the appropriate manner for
 * the type, for instance by calling g_free() or g_object_unref().
 *
 * <example>
 * <title>Using g_object_get(<!-- -->)</title>
 * An example of using g_object_get() to get the contents
 * of three properties - one of type #G_TYPE_INT,
 * one of type #G_TYPE_STRING, and one of type #G_TYPE_OBJECT:
 * <programlisting>
 *  gint intval;
 *  gchar *strval;
 *  GObject *objval;
 *
 *  g_object_get (my_object,
 *                "int-property", &intval,
 *                "str-property", &strval,
 *                "obj-property", &objval,
 *                NULL);
 *
 *  // Do something with intval, strval, objval
 *
 *  g_free (strval);
 *  g_object_unref (objval);
 * </programlisting>
 * </example>
 */
void
g_object_get (gpointer     _object,
	      const gchar *first_property_name,
	      ...)
{
  GObject *object = _object;
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_get_valist (object, first_property_name, var_args);
  va_end (var_args);
}

/**
 * g_object_set_property:
 * @object: a #GObject
 * @property_name: the name of the property to set
 * @value: the value
 *
 * Sets a property on an object.
 */
void
g_object_set_property (GObject	    *object,
		       const gchar  *property_name,
		       const GValue *value)
{
  GObjectNotifyQueue *nqueue;
  GParamSpec *pspec;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (object);
  nqueue = g_object_notify_queue_freeze (object, &property_notify_context);
  
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE);
  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRFUNC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: property `%s' of object class `%s' is not writable",
               G_STRFUNC,
               pspec->name,
               G_OBJECT_TYPE_NAME (object));
  else if ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) && !object_in_construction_list (object))
    g_warning ("%s: construct property \"%s\" for object `%s' can't be set after construction",
               G_STRFUNC, pspec->name, G_OBJECT_TYPE_NAME (object));
  else
    object_set_property (object, pspec, value, nqueue);
  
  g_object_notify_queue_thaw (object, nqueue);
  g_object_unref (object);
}

/**
 * g_object_get_property:
 * @object: a #GObject
 * @property_name: the name of the property to get
 * @value: return location for the property value
 *
 * Gets a property of an object.
 *
 * In general, a copy is made of the property contents and the caller is
 * responsible for freeing the memory by calling g_value_unset().
 *
 * Note that g_object_get_property() is really intended for language
 * bindings, g_object_get() is much more convenient for C programming.
 */
void
g_object_get_property (GObject	   *object,
		       const gchar *property_name,
		       GValue	   *value)
{
  GParamSpec *pspec;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (object);
  
  pspec = g_param_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE);
  if (!pspec)
    g_warning ("%s: object class `%s' has no property named `%s'",
	       G_STRFUNC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: property `%s' of object class `%s' is not readable",
               G_STRFUNC,
               pspec->name,
               G_OBJECT_TYPE_NAME (object));
  else
    {
      GValue *prop_value, tmp_value = { 0, };
      
      /* auto-conversion of the callers value type
       */
      if (G_VALUE_TYPE (value) == pspec->value_type)
	{
	  g_value_reset (value);
	  prop_value = value;
	}
      else if (!g_value_type_transformable (pspec->value_type, G_VALUE_TYPE (value)))
	{
	  g_warning ("%s: can't retrieve property `%s' of type `%s' as value of type `%s'",
		     G_STRFUNC, pspec->name,
		     g_type_name (pspec->value_type),
		     G_VALUE_TYPE_NAME (value));
	  g_object_unref (object);
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, pspec->value_type);
	  prop_value = &tmp_value;
	}
      object_get_property (object, pspec, prop_value);
      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
  
  g_object_unref (object);
}

/**
 * g_object_connect:
 * @object: a #GObject
 * @signal_spec: the spec for the first signal
 * @...: #GCallback for the first signal, followed by data for the
 *       first signal, followed optionally by more signal
 *       spec/callback/data triples, followed by %NULL
 *
 * A convenience function to connect multiple signals at once.
 *
 * The signal specs expected by this function have the form
 * "modifier::signal_name", where modifier can be one of the following:
 * <variablelist>
 * <varlistentry>
 * <term>signal</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_data (..., NULL, 0)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>object_signal</term>
 * <term>object-signal</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_object (..., 0)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>swapped_signal</term>
 * <term>swapped-signal</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_data (..., NULL, G_CONNECT_SWAPPED)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>swapped_object_signal</term>
 * <term>swapped-object-signal</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_object (..., G_CONNECT_SWAPPED)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>signal_after</term>
 * <term>signal-after</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_data (..., NULL, G_CONNECT_AFTER)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>object_signal_after</term>
 * <term>object-signal-after</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_object (..., G_CONNECT_AFTER)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>swapped_signal_after</term>
 * <term>swapped-signal-after</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_data (..., NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER)</literal>
 * </para></listitem>
 * </varlistentry>
 * <varlistentry>
 * <term>swapped_object_signal_after</term>
 * <term>swapped-object-signal-after</term>
 * <listitem><para>
 * equivalent to <literal>g_signal_connect_object (..., G_CONNECT_SWAPPED | G_CONNECT_AFTER)</literal>
 * </para></listitem>
 * </varlistentry>
 * </variablelist>
 *
 * |[
 *   menu->toplevel = g_object_connect (g_object_new (GTK_TYPE_WINDOW,
 * 						   "type", GTK_WINDOW_POPUP,
 * 						   "child", menu,
 * 						   NULL),
 * 				     "signal::event", gtk_menu_window_event, menu,
 * 				     "signal::size_request", gtk_menu_window_size_request, menu,
 * 				     "signal::destroy", gtk_widget_destroyed, &amp;menu-&gt;toplevel,
 * 				     NULL);
 * ]|
 *
 * Returns: @object
 */
gpointer
g_object_connect (gpointer     _object,
		  const gchar *signal_spec,
		  ...)
{
  GObject *object = _object;
  va_list var_args;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, object);

  va_start (var_args, signal_spec);
  while (signal_spec)
    {
      GCallback callback = va_arg (var_args, GCallback);
      gpointer data = va_arg (var_args, gpointer);
      gulong sid;

      if (strncmp (signal_spec, "signal::", 8) == 0)
	sid = g_signal_connect_data (object, signal_spec + 8,
				     callback, data, NULL,
				     0);
      else if (strncmp (signal_spec, "object_signal::", 15) == 0 ||
               strncmp (signal_spec, "object-signal::", 15) == 0)
	sid = g_signal_connect_object (object, signal_spec + 15,
				       callback, data,
				       0);
      else if (strncmp (signal_spec, "swapped_signal::", 16) == 0 ||
               strncmp (signal_spec, "swapped-signal::", 16) == 0)
	sid = g_signal_connect_data (object, signal_spec + 16,
				     callback, data, NULL,
				     G_CONNECT_SWAPPED);
      else if (strncmp (signal_spec, "swapped_object_signal::", 23) == 0 ||
               strncmp (signal_spec, "swapped-object-signal::", 23) == 0)
	sid = g_signal_connect_object (object, signal_spec + 23,
				       callback, data,
				       G_CONNECT_SWAPPED);
      else if (strncmp (signal_spec, "signal_after::", 14) == 0 ||
               strncmp (signal_spec, "signal-after::", 14) == 0)
	sid = g_signal_connect_data (object, signal_spec + 14,
				     callback, data, NULL,
				     G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "object_signal_after::", 21) == 0 ||
               strncmp (signal_spec, "object-signal-after::", 21) == 0)
	sid = g_signal_connect_object (object, signal_spec + 21,
				       callback, data,
				       G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "swapped_signal_after::", 22) == 0 ||
               strncmp (signal_spec, "swapped-signal-after::", 22) == 0)
	sid = g_signal_connect_data (object, signal_spec + 22,
				     callback, data, NULL,
				     G_CONNECT_SWAPPED | G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "swapped_object_signal_after::", 29) == 0 ||
               strncmp (signal_spec, "swapped-object-signal-after::", 29) == 0)
	sid = g_signal_connect_object (object, signal_spec + 29,
				       callback, data,
				       G_CONNECT_SWAPPED | G_CONNECT_AFTER);
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRFUNC, signal_spec);
	  break;
	}
      signal_spec = va_arg (var_args, gchar*);
    }
  va_end (var_args);

  return object;
}

/**
 * g_object_disconnect:
 * @object: a #GObject
 * @signal_spec: the spec for the first signal
 * @...: #GCallback for the first signal, followed by data for the first signal,
 *  followed optionally by more signal spec/callback/data triples,
 *  followed by %NULL
 *
 * A convenience function to disconnect multiple signals at once.
 *
 * The signal specs expected by this function have the form
 * "any_signal", which means to disconnect any signal with matching
 * callback and data, or "any_signal::signal_name", which only
 * disconnects the signal named "signal_name".
 */
void
g_object_disconnect (gpointer     _object,
		     const gchar *signal_spec,
		     ...)
{
  GObject *object = _object;
  va_list var_args;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  va_start (var_args, signal_spec);
  while (signal_spec)
    {
      GCallback callback = va_arg (var_args, GCallback);
      gpointer data = va_arg (var_args, gpointer);
      guint sid = 0, detail = 0, mask = 0;

      if (strncmp (signal_spec, "any_signal::", 12) == 0 ||
          strncmp (signal_spec, "any-signal::", 12) == 0)
	{
	  signal_spec += 12;
	  mask = G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA;
	}
      else if (strcmp (signal_spec, "any_signal") == 0 ||
               strcmp (signal_spec, "any-signal") == 0)
	{
	  signal_spec += 10;
	  mask = G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA;
	}
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRFUNC, signal_spec);
	  break;
	}

      if ((mask & G_SIGNAL_MATCH_ID) &&
	  !g_signal_parse_name (signal_spec, G_OBJECT_TYPE (object), &sid, &detail, FALSE))
	g_warning ("%s: invalid signal name \"%s\"", G_STRFUNC, signal_spec);
      else if (!g_signal_handlers_disconnect_matched (object, mask | (detail ? G_SIGNAL_MATCH_DETAIL : 0),
						      sid, detail,
						      NULL, (gpointer)callback, data))
	g_warning ("%s: signal handler %p(%p) is not connected", G_STRFUNC, callback, data);
      signal_spec = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

typedef struct {
  GObject *object;
  guint n_weak_refs;
  struct {
    GWeakNotify notify;
    gpointer    data;
  } weak_refs[1];  /* flexible array */
} WeakRefStack;

static void
weak_refs_notify (gpointer data)
{
  WeakRefStack *wstack = data;
  guint i;

  for (i = 0; i < wstack->n_weak_refs; i++)
    wstack->weak_refs[i].notify (wstack->weak_refs[i].data, wstack->object);
  g_free (wstack);
}

/**
 * g_object_weak_ref:
 * @object: #GObject to reference weakly
 * @notify: callback to invoke before the object is freed
 * @data: extra data to pass to notify
 *
 * Adds a weak reference callback to an object. Weak references are
 * used for notification when an object is finalized. They are called
 * "weak references" because they allow you to safely hold a pointer
 * to an object without calling g_object_ref() (g_object_ref() adds a
 * strong reference, that is, forces the object to stay alive).
 */
void
g_object_weak_ref (GObject    *object,
		   GWeakNotify notify,
		   gpointer    data)
{
  WeakRefStack *wstack;
  guint i;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);
  g_return_if_fail (object->ref_count >= 1);

  wstack = g_datalist_id_remove_no_notify (&object->qdata, quark_weak_refs);
  if (wstack)
    {
      i = wstack->n_weak_refs++;
      wstack = g_realloc (wstack, sizeof (*wstack) + sizeof (wstack->weak_refs[0]) * i);
    }
  else
    {
      wstack = g_renew (WeakRefStack, NULL, 1);
      wstack->object = object;
      wstack->n_weak_refs = 1;
      i = 0;
    }
  wstack->weak_refs[i].notify = notify;
  wstack->weak_refs[i].data = data;
  g_datalist_id_set_data_full (&object->qdata, quark_weak_refs, wstack, weak_refs_notify);
}

/**
 * g_object_weak_unref:
 * @object: #GObject to remove a weak reference from
 * @notify: callback to search for
 * @data: data to search for
 *
 * Removes a weak reference callback to an object.
 */
void
g_object_weak_unref (GObject    *object,
		     GWeakNotify notify,
		     gpointer    data)
{
  WeakRefStack *wstack;
  gboolean found_one = FALSE;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);

  wstack = g_datalist_id_get_data (&object->qdata, quark_weak_refs);
  if (wstack)
    {
      guint i;

      for (i = 0; i < wstack->n_weak_refs; i++)
	if (wstack->weak_refs[i].notify == notify &&
	    wstack->weak_refs[i].data == data)
	  {
	    found_one = TRUE;
	    wstack->n_weak_refs -= 1;
	    if (i != wstack->n_weak_refs)
	      wstack->weak_refs[i] = wstack->weak_refs[wstack->n_weak_refs];

	    break;
	  }
    }
  if (!found_one)
    g_warning ("%s: couldn't find weak ref %p(%p)", G_STRFUNC, notify, data);
}

/**
 * g_object_add_weak_pointer:
 * @object: The object that should be weak referenced.
 * @weak_pointer_location: The memory address of a pointer.
 *
 * Adds a weak reference from weak_pointer to @object to indicate that
 * the pointer located at @weak_pointer_location is only valid during
 * the lifetime of @object. When the @object is finalized,
 * @weak_pointer will be set to %NULL.
 */
void
g_object_add_weak_pointer (GObject  *object, 
                           gpointer *weak_pointer_location)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (weak_pointer_location != NULL);

  g_object_weak_ref (object, 
                     (GWeakNotify) g_nullify_pointer, 
                     weak_pointer_location);
}

/**
 * g_object_remove_weak_pointer:
 * @object: The object that is weak referenced.
 * @weak_pointer_location: The memory address of a pointer.
 *
 * Removes a weak reference from @object that was previously added
 * using g_object_add_weak_pointer(). The @weak_pointer_location has
 * to match the one used with g_object_add_weak_pointer().
 */
void
g_object_remove_weak_pointer (GObject  *object, 
                              gpointer *weak_pointer_location)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (weak_pointer_location != NULL);

  g_object_weak_unref (object, 
                       (GWeakNotify) g_nullify_pointer, 
                       weak_pointer_location);
}

static guint
object_floating_flag_handler (GObject        *object,
                              gint            job)
{
  switch (job)
    {
      gpointer oldvalue;
    case +1:    /* force floating if possible */
      do
        oldvalue = g_atomic_pointer_get (&object->qdata);
      while (!g_atomic_pointer_compare_and_exchange ((void**) &object->qdata, oldvalue,
                                                     (gpointer) ((gsize) oldvalue | OBJECT_FLOATING_FLAG)));
      return (gsize) oldvalue & OBJECT_FLOATING_FLAG;
    case -1:    /* sink if possible */
      do
        oldvalue = g_atomic_pointer_get (&object->qdata);
      while (!g_atomic_pointer_compare_and_exchange ((void**) &object->qdata, oldvalue,
                                                     (gpointer) ((gsize) oldvalue & ~(gsize) OBJECT_FLOATING_FLAG)));
      return (gsize) oldvalue & OBJECT_FLOATING_FLAG;
    default:    /* check floating */
      return 0 != ((gsize) g_atomic_pointer_get (&object->qdata) & OBJECT_FLOATING_FLAG);
    }
}

/**
 * g_object_is_floating:
 * @object: a #GObject
 *
 * Checks wether @object has a <link linkend="floating-ref">floating</link>
 * reference.
 *
 * Since: 2.10
 *
 * Returns: %TRUE if @object has a floating reference
 */
gboolean
g_object_is_floating (gpointer _object)
{
  GObject *object = _object;
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  return floating_flag_handler (object, 0);
}

/**
 * g_object_ref_sink:
 * @object: a #GObject
 *
 * Increase the reference count of @object, and possibly remove the
 * <link linkend="floating-ref">floating</link> reference, if @object
 * has a floating reference.
 *
 * In other words, if the object is floating, then this call "assumes
 * ownership" of the floating reference, converting it to a normal
 * reference by clearing the floating flag while leaving the reference
 * count unchanged.  If the object is not floating, then this call
 * adds a new normal reference increasing the reference count by one.
 *
 * Since: 2.10
 *
 * Returns: @object
 */
gpointer
g_object_ref_sink (gpointer _object)
{
  GObject *object = _object;
  gboolean was_floating;
  g_return_val_if_fail (G_IS_OBJECT (object), object);
  g_return_val_if_fail (object->ref_count >= 1, object);
  g_object_ref (object);
  was_floating = floating_flag_handler (object, -1);
  if (was_floating)
    g_object_unref (object);
  return object;
}

/**
 * g_object_force_floating:
 * @object: a #GObject
 *
 * This function is intended for #GObject implementations to re-enforce a
 * <link linkend="floating-ref">floating</link> object reference.
 * Doing this is seldomly required, all
 * #GInitiallyUnowned<!-- -->s are created with a floating reference which
 * usually just needs to be sunken by calling g_object_ref_sink().
 *
 * Since: 2.10
 */
void
g_object_force_floating (GObject *object)
{
  gboolean was_floating;
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count >= 1);

  was_floating = floating_flag_handler (object, +1);
}

typedef struct {
  GObject *object;
  guint n_toggle_refs;
  struct {
    GToggleNotify notify;
    gpointer    data;
  } toggle_refs[1];  /* flexible array */
} ToggleRefStack;

static void
toggle_refs_notify (GObject *object,
		    gboolean is_last_ref)
{
  ToggleRefStack *tstack = g_datalist_id_get_data (&object->qdata, quark_toggle_refs);

  /* Reentrancy here is not as tricky as it seems, because a toggle reference
   * will only be notified when there is exactly one of them.
   */
  g_assert (tstack->n_toggle_refs == 1);
  tstack->toggle_refs[0].notify (tstack->toggle_refs[0].data, tstack->object, is_last_ref);
}

/**
 * g_object_add_toggle_ref:
 * @object: a #GObject
 * @notify: a function to call when this reference is the
 *  last reference to the object, or is no longer
 *  the last reference.
 * @data: data to pass to @notify
 *
 * Increases the reference count of the object by one and sets a
 * callback to be called when all other references to the object are
 * dropped, or when this is already the last reference to the object
 * and another reference is established.
 *
 * This functionality is intended for binding @object to a proxy
 * object managed by another memory manager. This is done with two
 * paired references: the strong reference added by
 * g_object_add_toggle_ref() and a reverse reference to the proxy
 * object which is either a strong reference or weak reference.
 *
 * The setup is that when there are no other references to @object,
 * only a weak reference is held in the reverse direction from @object
 * to the proxy object, but when there are other references held to
 * @object, a strong reference is held. The @notify callback is called
 * when the reference from @object to the proxy object should be
 * <firstterm>toggled</firstterm> from strong to weak (@is_last_ref
 * true) or weak to strong (@is_last_ref false).
 *
 * Since a (normal) reference must be held to the object before
 * calling g_object_toggle_ref(), the initial state of the reverse
 * link is always strong.
 *
 * Multiple toggle references may be added to the same gobject,
 * however if there are multiple toggle references to an object, none
 * of them will ever be notified until all but one are removed.  For
 * this reason, you should only ever use a toggle reference if there
 * is important state in the proxy object.
 *
 * Since: 2.8
 */
void
g_object_add_toggle_ref (GObject       *object,
			 GToggleNotify  notify,
			 gpointer       data)
{
  ToggleRefStack *tstack;
  guint i;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);
  g_return_if_fail (object->ref_count >= 1);

  g_object_ref (object);

  tstack = g_datalist_id_remove_no_notify (&object->qdata, quark_toggle_refs);
  if (tstack)
    {
      i = tstack->n_toggle_refs++;
      /* allocate i = tstate->n_toggle_refs - 1 positions beyond the 1 declared
       * in tstate->toggle_refs */
      tstack = g_realloc (tstack, sizeof (*tstack) + sizeof (tstack->toggle_refs[0]) * i);
    }
  else
    {
      tstack = g_renew (ToggleRefStack, NULL, 1);
      tstack->object = object;
      tstack->n_toggle_refs = 1;
      i = 0;
    }

  /* Set a flag for fast lookup after adding the first toggle reference */
  if (tstack->n_toggle_refs == 1)
    g_datalist_set_flags (&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);
  
  tstack->toggle_refs[i].notify = notify;
  tstack->toggle_refs[i].data = data;
  g_datalist_id_set_data_full (&object->qdata, quark_toggle_refs, tstack,
			       (GDestroyNotify)g_free);
}

/**
 * g_object_remove_toggle_ref:
 * @object: a #GObject
 * @notify: a function to call when this reference is the
 *  last reference to the object, or is no longer
 *  the last reference.
 * @data: data to pass to @notify
 *
 * Removes a reference added with g_object_add_toggle_ref(). The
 * reference count of the object is decreased by one.
 *
 * Since: 2.8
 */
void
g_object_remove_toggle_ref (GObject       *object,
			    GToggleNotify  notify,
			    gpointer       data)
{
  ToggleRefStack *tstack;
  gboolean found_one = FALSE;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);

  tstack = g_datalist_id_get_data (&object->qdata, quark_toggle_refs);
  if (tstack)
    {
      guint i;

      for (i = 0; i < tstack->n_toggle_refs; i++)
	if (tstack->toggle_refs[i].notify == notify &&
	    tstack->toggle_refs[i].data == data)
	  {
	    found_one = TRUE;
	    tstack->n_toggle_refs -= 1;
	    if (i != tstack->n_toggle_refs)
	      tstack->toggle_refs[i] = tstack->toggle_refs[tstack->n_toggle_refs];

	    if (tstack->n_toggle_refs == 0)
	      g_datalist_unset_flags (&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);

	    g_object_unref (object);
	    
	    break;
	  }
    }
  
  if (!found_one)
    g_warning ("%s: couldn't find toggle ref %p(%p)", G_STRFUNC, notify, data);
}

/**
 * g_object_ref:
 * @object: a #GObject
 *
 * Increases the reference count of @object.
 *
 * Returns: the same @object
 */
gpointer
g_object_ref (gpointer _object)
{
  GObject *object = _object;
  gint old_val;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);
  
#ifdef  G_ENABLE_DEBUG
  if (g_trap_object_ref == object)
    G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */


  old_val = g_atomic_int_exchange_and_add ((int *)&object->ref_count, 1);

  if (old_val == 1 && OBJECT_HAS_TOGGLE_REF (object))
    toggle_refs_notify (object, FALSE);

  TRACE (GOBJECT_OBJECT_REF(object,G_TYPE_FROM_INSTANCE(object),old_val));

  return object;
}

/**
 * g_object_unref:
 * @object: a #GObject
 *
 * Decreases the reference count of @object. When its reference count
 * drops to 0, the object is finalized (i.e. its memory is freed).
 */
void
g_object_unref (gpointer _object)
{
  GObject *object = _object;
  gint old_ref;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);
  
#ifdef  G_ENABLE_DEBUG
  if (g_trap_object_ref == object)
    G_BREAKPOINT ();
#endif  /* G_ENABLE_DEBUG */

  /* here we want to atomically do: if (ref_count>1) { ref_count--; return; } */
 retry_atomic_decrement1:
  old_ref = g_atomic_int_get (&object->ref_count);
  if (old_ref > 1)
    {
      /* valid if last 2 refs are owned by this call to unref and the toggle_ref */
      gboolean has_toggle_ref = OBJECT_HAS_TOGGLE_REF (object);

      if (!g_atomic_int_compare_and_exchange ((int *)&object->ref_count, old_ref, old_ref - 1))
	goto retry_atomic_decrement1;

      TRACE (GOBJECT_OBJECT_UNREF(object,G_TYPE_FROM_INSTANCE(object),old_ref));

      /* if we went from 2->1 we need to notify toggle refs if any */
      if (old_ref == 2 && has_toggle_ref) /* The last ref being held in this case is owned by the toggle_ref */
	toggle_refs_notify (object, TRUE);
    }
  else
    {
      /* we are about tp remove the last reference */
      TRACE (GOBJECT_OBJECT_DISPOSE(object,G_TYPE_FROM_INSTANCE(object), 1));
      G_OBJECT_GET_CLASS (object)->dispose (object);
      TRACE (GOBJECT_OBJECT_DISPOSE_END(object,G_TYPE_FROM_INSTANCE(object), 1));

      /* may have been re-referenced meanwhile */
    retry_atomic_decrement2:
      old_ref = g_atomic_int_get ((int *)&object->ref_count);
      if (old_ref > 1)
        {
          /* valid if last 2 refs are owned by this call to unref and the toggle_ref */
          gboolean has_toggle_ref = OBJECT_HAS_TOGGLE_REF (object);

          if (!g_atomic_int_compare_and_exchange ((int *)&object->ref_count, old_ref, old_ref - 1))
	    goto retry_atomic_decrement2;

	  TRACE (GOBJECT_OBJECT_UNREF(object,G_TYPE_FROM_INSTANCE(object),old_ref));

          /* if we went from 2->1 we need to notify toggle refs if any */
          if (old_ref == 2 && has_toggle_ref) /* The last ref being held in this case is owned by the toggle_ref */
	    toggle_refs_notify (object, TRUE);

	  return;
	}

      /* we are still in the process of taking away the last ref */
      g_datalist_id_set_data (&object->qdata, quark_closure_array, NULL);
      g_signal_handlers_destroy (object);
      g_datalist_id_set_data (&object->qdata, quark_weak_refs, NULL);
      
      /* decrement the last reference */
      old_ref = g_atomic_int_exchange_and_add ((int *)&object->ref_count, -1);

      TRACE (GOBJECT_OBJECT_UNREF(object,G_TYPE_FROM_INSTANCE(object),old_ref));

      /* may have been re-referenced meanwhile */
      if (G_LIKELY (old_ref == 1))
	{
	  TRACE (GOBJECT_OBJECT_FINALIZE(object,G_TYPE_FROM_INSTANCE(object)));
          G_OBJECT_GET_CLASS (object)->finalize (object);

	  TRACE (GOBJECT_OBJECT_FINALIZE_END(object,G_TYPE_FROM_INSTANCE(object)));

#ifdef	G_ENABLE_DEBUG
          IF_DEBUG (OBJECTS)
	    {
	      /* catch objects not chaining finalize handlers */
	      G_LOCK (debug_objects);
	      g_assert (g_hash_table_lookup (debug_objects_ht, object) == NULL);
	      G_UNLOCK (debug_objects);
	    }
#endif	/* G_ENABLE_DEBUG */
          g_type_free_instance ((GTypeInstance*) object);
	}
    }
}

/**
 * g_object_get_qdata:
 * @object: The GObject to get a stored user data pointer from
 * @quark: A #GQuark, naming the user data pointer
 * 
 * This function gets back user data pointers stored via
 * g_object_set_qdata().
 * 
 * Returns: The user data pointer set, or %NULL
 */
gpointer
g_object_get_qdata (GObject *object,
		    GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  
  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

/**
 * g_object_set_qdata:
 * @object: The GObject to set store a user data pointer
 * @quark: A #GQuark, naming the user data pointer
 * @data: An opaque user data pointer
 *
 * This sets an opaque, named pointer on an object.
 * The name is specified through a #GQuark (retrived e.g. via
 * g_quark_from_static_string()), and the pointer
 * can be gotten back from the @object with g_object_get_qdata()
 * until the @object is finalized.
 * Setting a previously set user data pointer, overrides (frees)
 * the old pointer set, using #NULL as pointer essentially
 * removes the data stored.
 */
void
g_object_set_qdata (GObject *object,
		    GQuark   quark,
		    gpointer data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data (&object->qdata, quark, data);
}

/**
 * g_object_set_qdata_full:
 * @object: The GObject to set store a user data pointer
 * @quark: A #GQuark, naming the user data pointer
 * @data: An opaque user data pointer
 * @destroy: Function to invoke with @data as argument, when @data
 *           needs to be freed
 *
 * This function works like g_object_set_qdata(), but in addition,
 * a void (*destroy) (gpointer) function may be specified which is
 * called with @data as argument when the @object is finalized, or
 * the data is being overwritten by a call to g_object_set_qdata()
 * with the same @quark.
 */
void
g_object_set_qdata_full (GObject       *object,
			 GQuark		quark,
			 gpointer	data,
			 GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data_full (&object->qdata, quark, data,
			       data ? destroy : (GDestroyNotify) NULL);
}

/**
 * g_object_steal_qdata:
 * @object: The GObject to get a stored user data pointer from
 * @quark: A #GQuark, naming the user data pointer
 *
 * This function gets back user data pointers stored via
 * g_object_set_qdata() and removes the @data from object
 * without invoking its destroy() function (if any was
 * set).
 * Usually, calling this function is only required to update
 * user data pointers with a destroy notifier, for example:
 * |[
 * void
 * object_add_to_user_list (GObject     *object,
 *                          const gchar *new_string)
 * {
 *   // the quark, naming the object data
 *   GQuark quark_string_list = g_quark_from_static_string ("my-string-list");
 *   // retrive the old string list
 *   GList *list = g_object_steal_qdata (object, quark_string_list);
 *
 *   // prepend new string
 *   list = g_list_prepend (list, g_strdup (new_string));
 *   // this changed 'list', so we need to set it again
 *   g_object_set_qdata_full (object, quark_string_list, list, free_string_list);
 * }
 * static void
 * free_string_list (gpointer data)
 * {
 *   GList *node, *list = data;
 *
 *   for (node = list; node; node = node->next)
 *     g_free (node->data);
 *   g_list_free (list);
 * }
 * ]|
 * Using g_object_get_qdata() in the above example, instead of
 * g_object_steal_qdata() would have left the destroy function set,
 * and thus the partial string list would have been freed upon
 * g_object_set_qdata_full().
 *
 * Returns: The user data pointer set, or %NULL
 */
gpointer
g_object_steal_qdata (GObject *object,
		      GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (quark > 0, NULL);
  
  return g_datalist_id_remove_no_notify (&object->qdata, quark);
}

/**
 * g_object_get_data:
 * @object: #GObject containing the associations
 * @key: name of the key for that association
 * 
 * Gets a named field from the objects table of associations (see g_object_set_data()).
 * 
 * Returns: the data if found, or %NULL if no such data exists.
 */
gpointer
g_object_get_data (GObject     *object,
                   const gchar *key)
{
  GQuark quark;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  quark = g_quark_try_string (key);

  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

/**
 * g_object_set_data:
 * @object: #GObject containing the associations.
 * @key: name of the key
 * @data: data to associate with that key
 *
 * Each object carries around a table of associations from
 * strings to pointers.  This function lets you set an association.
 *
 * If the object already had an association with that name,
 * the old association will be destroyed.
 */
void
g_object_set_data (GObject     *object,
                   const gchar *key,
                   gpointer     data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data (&object->qdata, g_quark_from_string (key), data);
}

/**
 * g_object_set_data_full:
 * @object: #GObject containing the associations
 * @key: name of the key
 * @data: data to associate with that key
 * @destroy: function to call when the association is destroyed
 *
 * Like g_object_set_data() except it adds notification
 * for when the association is destroyed, either by setting it
 * to a different value or when the object is destroyed.
 *
 * Note that the @destroy callback is not called if @data is %NULL.
 */
void
g_object_set_data_full (GObject       *object,
                        const gchar   *key,
                        gpointer       data,
                        GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data_full (&object->qdata, g_quark_from_string (key), data,
			       data ? destroy : (GDestroyNotify) NULL);
}

/**
 * g_object_steal_data:
 * @object: #GObject containing the associations
 * @key: name of the key
 *
 * Remove a specified datum from the object's data associations,
 * without invoking the association's destroy handler.
 *
 * Returns: the data if found, or %NULL if no such data exists.
 */
gpointer
g_object_steal_data (GObject     *object,
                     const gchar *key)
{
  GQuark quark;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  quark = g_quark_try_string (key);

  return quark ? g_datalist_id_remove_no_notify (&object->qdata, quark) : NULL;
}

static void
g_value_object_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
g_value_object_free_value (GValue *value)
{
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
}

static void
g_value_object_copy_value (const GValue *src_value,
			   GValue	*dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static void
g_value_object_transform_value (const GValue *src_value,
				GValue       *dest_value)
{
  if (src_value->data[0].v_pointer && g_type_is_a (G_OBJECT_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value)))
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gpointer
g_value_object_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
g_value_object_collect_value (GValue	  *value,
			      guint        n_collect_values,
			      GTypeCValue *collect_values,
			      guint        collect_flags)
{
  if (collect_values[0].v_pointer)
    {
      GObject *object = collect_values[0].v_pointer;
      
      if (object->g_type_instance.g_class == NULL)
	return g_strconcat ("invalid unclassed object pointer for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!g_value_type_compatible (G_OBJECT_TYPE (object), G_VALUE_TYPE (value)))
	return g_strconcat ("invalid object type `",
			    G_OBJECT_TYPE_NAME (object),
			    "' for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = g_object_ref (object);
    }
  else
    value->data[0].v_pointer = NULL;
  
  return NULL;
}

static gchar*
g_value_object_lcopy_value (const GValue *value,
			    guint        n_collect_values,
			    GTypeCValue *collect_values,
			    guint        collect_flags)
{
  GObject **object_p = collect_values[0].v_pointer;
  
  if (!object_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));

  if (!value->data[0].v_pointer)
    *object_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *object_p = value->data[0].v_pointer;
  else
    *object_p = g_object_ref (value->data[0].v_pointer);
  
  return NULL;
}

/**
 * g_value_set_object:
 * @value: a valid #GValue of %G_TYPE_OBJECT derived type
 * @v_object: object value to be set
 *
 * Set the contents of a %G_TYPE_OBJECT derived #GValue to @v_object.
 *
 * g_value_set_object() increases the reference count of @v_object
 * (the #GValue holds a reference to @v_object).  If you do not wish
 * to increase the reference count of the object (i.e. you wish to
 * pass your current reference to the #GValue because you no longer
 * need it), use g_value_take_object() instead.
 *
 * It is important that your #GValue holds a reference to @v_object (either its
 * own, or one it has taken) to ensure that the object won't be destroyed while
 * the #GValue still exists).
 */
void
g_value_set_object (GValue   *value,
		    gpointer  v_object)
{
  GObject *old;
	
  g_return_if_fail (G_VALUE_HOLDS_OBJECT (value));

  old = value->data[0].v_pointer;
  
  if (v_object)
    {
      g_return_if_fail (G_IS_OBJECT (v_object));
      g_return_if_fail (g_value_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object;
      g_object_ref (value->data[0].v_pointer);
    }
  else
    value->data[0].v_pointer = NULL;
  
  if (old)
    g_object_unref (old);
}

/**
 * g_value_set_object_take_ownership:
 * @value: a valid #GValue of %G_TYPE_OBJECT derived type
 * @v_object: object value to be set
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Deprecated: 2.4: Use g_value_take_object() instead.
 */
void
g_value_set_object_take_ownership (GValue  *value,
				   gpointer v_object)
{
  g_value_take_object (value, v_object);
}

/**
 * g_value_take_object:
 * @value: a valid #GValue of %G_TYPE_OBJECT derived type
 * @v_object: object value to be set
 *
 * Sets the contents of a %G_TYPE_OBJECT derived #GValue to @v_object
 * and takes over the ownership of the callers reference to @v_object;
 * the caller doesn't have to unref it any more (i.e. the reference
 * count of the object is not increased).
 *
 * If you want the #GValue to hold its own reference to @v_object, use
 * g_value_set_object() instead.
 *
 * Since: 2.4
 */
void
g_value_take_object (GValue  *value,
		     gpointer v_object)
{
  g_return_if_fail (G_VALUE_HOLDS_OBJECT (value));

  if (value->data[0].v_pointer)
    {
      g_object_unref (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
    }

  if (v_object)
    {
      g_return_if_fail (G_IS_OBJECT (v_object));
      g_return_if_fail (g_value_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object; /* we take over the reference count */
    }
}

/**
 * g_value_get_object:
 * @value: a valid #GValue of %G_TYPE_OBJECT derived type
 * 
 * Get the contents of a %G_TYPE_OBJECT derived #GValue.
 * 
 * Returns: object contents of @value
 */
gpointer
g_value_get_object (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_OBJECT (value), NULL);
  
  return value->data[0].v_pointer;
}

/**
 * g_value_dup_object:
 * @value: a valid #GValue whose type is derived from %G_TYPE_OBJECT
 *
 * Get the contents of a %G_TYPE_OBJECT derived #GValue, increasing
 * its reference count.
 *
 * Returns: object content of @value, should be unreferenced when no
 *          longer needed.
 */
gpointer
g_value_dup_object (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_OBJECT (value), NULL);
  
  return value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
}

/**
 * g_signal_connect_object:
 * @instance: the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #GCallback to connect.
 * @gobject: the object to pass as data to @c_handler.
 * @connect_flags: a combination of #GConnnectFlags.
 *
 * This is similar to g_signal_connect_data(), but uses a closure which
 * ensures that the @gobject stays alive during the call to @c_handler
 * by temporarily adding a reference count to @gobject.
 *
 * Note that there is a bug in GObject that makes this function
 * much less useful than it might seem otherwise. Once @gobject is
 * disposed, the callback will no longer be called, but, the signal
 * handler is <emphasis>not</emphasis> currently disconnected. If the
 * @instance is itself being freed at the same time than this doesn't
 * matter, since the signal will automatically be removed, but
 * if @instance persists, then the signal handler will leak. You
 * should not remove the signal yourself because in a future versions of
 * GObject, the handler <emphasis>will</emphasis> automatically
 * be disconnected.
 *
 * It's possible to work around this problem in a way that will
 * continue to work with future versions of GObject by checking
 * that the signal handler is still connected before disconnected it:
 * <informalexample><programlisting>
 *  if (g_signal_handler_is_connected (instance, id))
 *    g_signal_handler_disconnect (instance, id);
 * </programlisting></informalexample>
 *
 * Returns: the handler id.
 */
gulong
g_signal_connect_object (gpointer      instance,
			 const gchar  *detailed_signal,
			 GCallback     c_handler,
			 gpointer      gobject,
			 GConnectFlags connect_flags)
{
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), 0);
  g_return_val_if_fail (detailed_signal != NULL, 0);
  g_return_val_if_fail (c_handler != NULL, 0);

  if (gobject)
    {
      GClosure *closure;

      g_return_val_if_fail (G_IS_OBJECT (gobject), 0);

      closure = ((connect_flags & G_CONNECT_SWAPPED) ? g_cclosure_new_object_swap : g_cclosure_new_object) (c_handler, gobject);

      return g_signal_connect_closure (instance, detailed_signal, closure, connect_flags & G_CONNECT_AFTER);
    }
  else
    return g_signal_connect_data (instance, detailed_signal, c_handler, NULL, NULL, connect_flags);
}

typedef struct {
  GObject  *object;
  guint     n_closures;
  GClosure *closures[1]; /* flexible array */
} CArray;
/* don't change this structure without supplying an accessor for
 * watched closures, e.g.:
 * GSList* g_object_list_watched_closures (GObject *object)
 * {
 *   CArray *carray;
 *   g_return_val_if_fail (G_IS_OBJECT (object), NULL);
 *   carray = g_object_get_data (object, "GObject-closure-array");
 *   if (carray)
 *     {
 *       GSList *slist = NULL;
 *       guint i;
 *       for (i = 0; i < carray->n_closures; i++)
 *         slist = g_slist_prepend (slist, carray->closures[i]);
 *       return slist;
 *     }
 *   return NULL;
 * }
 */

static void
object_remove_closure (gpointer  data,
		       GClosure *closure)
{
  GObject *object = data;
  CArray *carray = g_object_get_qdata (object, quark_closure_array);
  guint i;
  
  for (i = 0; i < carray->n_closures; i++)
    if (carray->closures[i] == closure)
      {
	carray->n_closures--;
	if (i < carray->n_closures)
	  carray->closures[i] = carray->closures[carray->n_closures];
	return;
      }
  g_assert_not_reached ();
}

static void
destroy_closure_array (gpointer data)
{
  CArray *carray = data;
  GObject *object = carray->object;
  guint i, n = carray->n_closures;
  
  for (i = 0; i < n; i++)
    {
      GClosure *closure = carray->closures[i];
      
      /* removing object_remove_closure() upfront is probably faster than
       * letting it fiddle with quark_closure_array which is empty anyways
       */
      g_closure_remove_invalidate_notifier (closure, object, object_remove_closure);
      g_closure_invalidate (closure);
    }
  g_free (carray);
}

/**
 * g_object_watch_closure:
 * @object: GObject restricting lifetime of @closure
 * @closure: GClosure to watch
 *
 * This function essentially limits the life time of the @closure to
 * the life time of the object. That is, when the object is finalized,
 * the @closure is invalidated by calling g_closure_invalidate() on
 * it, in order to prevent invocations of the closure with a finalized
 * (nonexisting) object. Also, g_object_ref() and g_object_unref() are
 * added as marshal guards to the @closure, to ensure that an extra
 * reference count is held on @object during invocation of the
 * @closure.  Usually, this function will be called on closures that
 * use this @object as closure data.
 */
void
g_object_watch_closure (GObject  *object,
			GClosure *closure)
{
  CArray *carray;
  guint i;
  
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (object->ref_count > 0);	/* this doesn't work on finalizing objects */
  
  g_closure_add_invalidate_notifier (closure, object, object_remove_closure);
  g_closure_add_marshal_guards (closure,
				object, (GClosureNotify) g_object_ref,
				object, (GClosureNotify) g_object_unref);
  carray = g_datalist_id_remove_no_notify (&object->qdata, quark_closure_array);
  if (!carray)
    {
      carray = g_renew (CArray, NULL, 1);
      carray->object = object;
      carray->n_closures = 1;
      i = 0;
    }
  else
    {
      i = carray->n_closures++;
      carray = g_realloc (carray, sizeof (*carray) + sizeof (carray->closures[0]) * i);
    }
  carray->closures[i] = closure;
  g_datalist_id_set_data_full (&object->qdata, quark_closure_array, carray, destroy_closure_array);
}

/**
 * g_closure_new_object:
 * @sizeof_closure: the size of the structure to allocate, must be at least
 *  <literal>sizeof (GClosure)</literal>
 * @object: a #GObject pointer to store in the @data field of the newly
 *  allocated #GClosure
 *
 * A variant of g_closure_new_simple() which stores @object in the
 * @data field of the closure and calls g_object_watch_closure() on
 * @object and the created closure. This function is mainly useful
 * when implementing new types of closures.
 *
 * Returns: a newly allocated #GClosure
 */
GClosure*
g_closure_new_object (guint    sizeof_closure,
		      GObject *object)
{
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */

  closure = g_closure_new_simple (sizeof_closure, object);
  g_object_watch_closure (object, closure);

  return closure;
}

/**
 * g_cclosure_new_object:
 * @callback_func: the function to invoke
 * @object: a #GObject pointer to pass to @callback_func
 *
 * A variant of g_cclosure_new() which uses @object as @user_data and
 * calls g_object_watch_closure() on @object and the created
 * closure. This function is useful when you have a callback closely
 * associated with a #GObject, and want the callback to no longer run
 * after the object is is freed.
 *
 * Returns: a new #GCClosure
 */
GClosure*
g_cclosure_new_object (GCallback callback_func,
		       GObject  *object)
{
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */
  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new (callback_func, object, NULL);
  g_object_watch_closure (object, closure);

  return closure;
}

/**
 * g_cclosure_new_object_swap:
 * @callback_func: the function to invoke
 * @object: a #GObject pointer to pass to @callback_func
 *
 * A variant of g_cclosure_new_swap() which uses @object as @user_data
 * and calls g_object_watch_closure() on @object and the created
 * closure. This function is useful when you have a callback closely
 * associated with a #GObject, and want the callback to no longer run
 * after the object is is freed.
 *
 * Returns: a new #GCClosure
 */
GClosure*
g_cclosure_new_object_swap (GCallback callback_func,
			    GObject  *object)
{
  GClosure *closure;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);     /* this doesn't work on finalizing objects */
  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new_swap (callback_func, object, NULL);
  g_object_watch_closure (object, closure);

  return closure;
}

gsize
g_object_compat_control (gsize           what,
                         gpointer        data)
{
  switch (what)
    {
      gpointer *pp;
    case 1:     /* floating base type */
      return G_TYPE_INITIALLY_UNOWNED;
    case 2:     /* FIXME: remove this once GLib/Gtk+ break ABI again */
      floating_flag_handler = (guint(*)(GObject*,gint)) data;
      return 1;
    case 3:     /* FIXME: remove this once GLib/Gtk+ break ABI again */
      pp = data;
      *pp = floating_flag_handler;
      return 1;
    default:
      return 0;
    }
}

G_DEFINE_TYPE (GInitiallyUnowned, g_initially_unowned, G_TYPE_OBJECT);

static void
g_initially_unowned_init (GInitiallyUnowned *object)
{
  g_object_force_floating (object);
}

static void
g_initially_unowned_class_init (GInitiallyUnownedClass *klass)
{
}
