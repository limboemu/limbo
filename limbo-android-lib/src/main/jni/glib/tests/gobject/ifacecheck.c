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
#define	G_LOG_DOMAIN "TestIfaceCheck"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include <string.h>

#include <glib-object.h>

#include "testcommon.h"

/* This test tests g_type_add_interface_check_func(), which allows
 * installing a post-initialization check function.
 */

#define TEST_TYPE_IFACE           (test_iface_get_type ())
#define TEST_IFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE, TestIfaceClass))
typedef struct _TestIfaceClass  TestIfaceClass;

struct _TestIfaceClass
{
  GTypeInterface base_iface;
  GString *history;
};

static void
test_iface_base_init (TestIfaceClass *iface)
{
  iface->history = g_string_new (iface->history ? iface->history->str : NULL);
}

static DEFINE_IFACE(TestIface, test_iface, test_iface_base_init, NULL)

/*
 * TestObject1
 */
#define TEST_TYPE_OBJECT1         (test_object1_get_type ())
typedef struct _GObject           TestObject1;
typedef struct _GObjectClass      TestObject1Class;

static DEFINE_TYPE_FULL (TestObject1, test_object1,
			 NULL, NULL, NULL,
			 G_TYPE_OBJECT,
			 INTERFACE (NULL, TEST_TYPE_IFACE))
     
/*
 * TestObject2
 */
#define TEST_TYPE_OBJECT2         (test_object2_get_type ())
typedef struct _GObject           TestObject2;
typedef struct _GObjectClass      TestObject2Class;

static DEFINE_TYPE_FULL (TestObject2, test_object2,
			 NULL, NULL, NULL,
			 G_TYPE_OBJECT,
			 INTERFACE (NULL, TEST_TYPE_IFACE))
     
/*
 * TestObject3
 */
#define TEST_TYPE_OBJECT3         (test_object3_get_type ())
typedef struct _GObject           TestObject3;
typedef struct _GObjectClass      TestObject3Class;

static DEFINE_TYPE_FULL (TestObject3, test_object3,
			 NULL, NULL, NULL,
			 G_TYPE_OBJECT,
			 INTERFACE (NULL, TEST_TYPE_IFACE))
     
/*
 * TestObject4
 */
#define TEST_TYPE_OBJECT4         (test_object4_get_type ())
typedef struct _GObject           TestObject4;
typedef struct _GObjectClass      TestObject4Class;


static DEFINE_TYPE_FULL (TestObject4, test_object4,
			 NULL, NULL, NULL,
			 G_TYPE_OBJECT, {})

static void
check_func (gpointer check_data,
	    gpointer g_iface)
{
  TestIfaceClass *iface = g_iface;

  g_string_append (iface->history, check_data);
}

int
main (int   argc,
      char *argv[])
{
  TestIfaceClass *iface;
  GObject *object;
  char *string1 = "A";
  char *string2 = "B";

  g_type_init ();
  
  /* Basic check of interfaces added before class_init time
   */
  g_type_add_interface_check (string1, check_func);

  object = g_object_new (TEST_TYPE_OBJECT1, NULL);
  iface = TEST_IFACE_GET_CLASS (object);
    g_assert (strcmp (iface->history->str, "A") == 0);
  g_object_unref (object);

  /* Add a second check function
   */
  g_type_add_interface_check (string2, check_func);

  object = g_object_new (TEST_TYPE_OBJECT2, NULL);
  iface = TEST_IFACE_GET_CLASS (object);
  g_assert (strcmp (iface->history->str, "AB") == 0);
  g_object_unref (object);

  /* Remove the first check function
   */
  g_type_remove_interface_check (string1, check_func);

  object = g_object_new (TEST_TYPE_OBJECT3, NULL);
  iface = TEST_IFACE_GET_CLASS (object);
  g_assert (strcmp (iface->history->str, "B") == 0);
  g_object_unref (object);

  /* Test interfaces added after class_init time
   */
  g_type_class_ref (TEST_TYPE_OBJECT4);
  {
    static GInterfaceInfo const iface = {
      NULL, NULL, NULL
    };
    
    g_type_add_interface_static (TEST_TYPE_OBJECT4, TEST_TYPE_IFACE, &iface);
  }
  
  object = g_object_new (TEST_TYPE_OBJECT4, NULL);
  iface = TEST_IFACE_GET_CLASS (object);
  g_assert (strcmp (iface->history->str, "B") == 0);
  g_object_unref (object);
    
  return 0;
}
