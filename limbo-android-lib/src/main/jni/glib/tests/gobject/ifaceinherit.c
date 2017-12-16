/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001, 2003 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#undef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN "TestIfaceInherit"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include <glib-object.h>

#include "testcommon.h"
#include "testmodule.h"

/* This test tests inheritance of interface. We two object
 * class BaseObject and DerivedObject we add an interface
 * to BaseObject:
 *
 * I1) Before DerivedObject is registered
 * I2) After DerivedObject is registered, but before
 *     DerivedObject is class initialized
 * I3) During DerivedObject's class_init
 * I4) After DerivedObject's class init
 *
 * We also do some tests of overriding.
 * 
 * I5) We add an interface to BaseObject, then add the same
 *     interface to DerivedObject. (Note that this is only legal
 *     before DerivedObject's class_init; the results of
 *     g_type_interface_peek() are not allowed to change from one
 *     non-NULL vtable to another non-NULL vtable)
 */
   
/*
 * BaseObject, a parent class for DerivedObject
 */
#define BASE_TYPE_OBJECT          (base_object_get_type ())
typedef struct _BaseObject        BaseObject;
typedef struct _BaseObjectClass   BaseObjectClass;

struct _BaseObject
{
  GObject parent_instance;
};
struct _BaseObjectClass
{
  GObjectClass parent_class;
};

static GType base_object_get_type ();
static GType derived_object_get_type ();

/*
 * DerivedObject, the child class of DerivedObject
 */
#define DERIVED_TYPE_OBJECT          (derived_object_get_type ())
typedef struct _DerivedObject        DerivedObject;
typedef struct _DerivedObjectClass   DerivedObjectClass;

struct _DerivedObject
{
  BaseObject parent_instance;
};
struct _DerivedObjectClass
{
  BaseObjectClass parent_class;
};

/*
 * The interfaces
 */
typedef struct _TestIfaceClass TestIfaceClass;
typedef struct _TestIfaceClass TestIface1Class;
typedef struct _TestIfaceClass TestIface2Class;
typedef struct _TestIfaceClass TestIface3Class;
typedef struct _TestIfaceClass TestIface4Class;
typedef struct _TestIfaceClass TestIface5Class;

struct _TestIfaceClass
{
  GTypeInterface base_iface;
  guint val;
};

#define TEST_TYPE_IFACE1 (test_iface1_get_type ())
#define TEST_TYPE_IFACE2 (test_iface2_get_type ())
#define TEST_TYPE_IFACE3 (test_iface3_get_type ())
#define TEST_TYPE_IFACE4 (test_iface4_get_type ())
#define TEST_TYPE_IFACE5 (test_iface5_get_type ())

static DEFINE_IFACE (TestIface1, test_iface1,  NULL, NULL)
static DEFINE_IFACE (TestIface2, test_iface2,  NULL, NULL)
static DEFINE_IFACE (TestIface3, test_iface3,  NULL, NULL)
static DEFINE_IFACE (TestIface4, test_iface4,  NULL, NULL)
static DEFINE_IFACE (TestIface5, test_iface5,  NULL, NULL)

static void
add_interface (GType              object_type,
	       GType              iface_type,
	       GInterfaceInitFunc init_func)
{
  GInterfaceInfo iface_info = {	NULL, NULL, NULL };

  iface_info.interface_init = init_func;
								
  g_type_add_interface_static (object_type, iface_type, &iface_info);
}

static void
init_base_interface (TestIfaceClass *iface)
{
  iface->val = 21;
}

static void
add_base_interface (GType object_type,
		    GType iface_type)
{
  add_interface (object_type, iface_type,
		 (GInterfaceInitFunc)init_base_interface);
}

static gboolean
interface_is_base (GType object_type,
		   GType iface_type)
{
  gpointer g_class = g_type_class_peek (object_type);
  TestIfaceClass *iface = g_type_interface_peek (g_class, iface_type);
  return iface && iface->val == 21;
}

static void
init_derived_interface (TestIfaceClass *iface)
{
  iface->val = 42;
}

static void
add_derived_interface (GType object_type,
		       GType iface_type)
{
  add_interface (object_type, iface_type,
		 (GInterfaceInitFunc)init_derived_interface);
}

static gboolean
interface_is_derived (GType object_type,
		      GType iface_type)
{
  gpointer g_class = g_type_class_peek (object_type);
  TestIfaceClass *iface = g_type_interface_peek (g_class, iface_type);
  return iface && iface->val == 42;
}

static void
derived_object_class_init (BaseObjectClass *class)
{
  add_base_interface (BASE_TYPE_OBJECT, TEST_TYPE_IFACE3);
}

static DEFINE_TYPE(BaseObject, base_object,
		   NULL, NULL, NULL,
		   G_TYPE_OBJECT)
static DEFINE_TYPE(DerivedObject, derived_object,
		   derived_object_class_init, NULL, NULL,
		   BASE_TYPE_OBJECT)

int
main (int   argc,
      char *argv[])
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);
  g_type_init ();

  /* Register BaseObject */
  BASE_TYPE_OBJECT;
  
  add_base_interface (BASE_TYPE_OBJECT, TEST_TYPE_IFACE5);

  /* Class init BaseObject */
  g_type_class_ref (BASE_TYPE_OBJECT);
  
  add_base_interface (BASE_TYPE_OBJECT, TEST_TYPE_IFACE1);

  /* Register DerivedObject */
  DERIVED_TYPE_OBJECT;

  add_base_interface (BASE_TYPE_OBJECT, TEST_TYPE_IFACE2);
  add_derived_interface (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE5);

  /* Class init DerivedObject */
  g_type_class_ref (DERIVED_TYPE_OBJECT);
  
  add_base_interface (BASE_TYPE_OBJECT, TEST_TYPE_IFACE4);

  /* Check that all the non-overridden interfaces were properly inherited
   */
  g_assert (interface_is_base (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE1));
  g_assert (interface_is_base (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE2));
  g_assert (interface_is_base (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE3));
  g_assert (interface_is_base (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE4));

  /* Check that all the overridden interfaces were properly overridden
   */
  g_assert (interface_is_derived (DERIVED_TYPE_OBJECT, TEST_TYPE_IFACE5));

  return 0;
}
