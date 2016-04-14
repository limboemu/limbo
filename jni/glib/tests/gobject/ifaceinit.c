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
#define	G_LOG_DOMAIN "TestIfaceInit"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include <glib-object.h>

#include "testcommon.h"

/* What this test tests is the ability to add interfaces dynamically; in
 * particular adding interfaces to a class while that class is being
 * initialized.
 *
 * The test defines 5 interfaces:
 * 
 * - TestIface1 is added before the class is initialized
 * - TestIface2 is added in base_object_base_init()
 * - TestIface3 is added in test_iface1_base_init()
 * - TestIface4 is added in test_object_class_init()
 * - TestIface5 is added in test_object_test_iface1_init()
 * - TestIface6 is added after the class is initialized
 */

/* All 6 interfaces actually share the same class structure, though
 * we use separate typedefs
 */
typedef struct _TestIfaceClass TestIfaceClass;

struct _TestIfaceClass
{
  GTypeInterface base_iface;
  guint val;
  guint base_val;
  guint default_val;
};

#define TEST_TYPE_IFACE1           (test_iface1_get_type ())
#define TEST_IFACE1_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE1, TestIface1Class))
typedef struct _TestIface1      TestIface1;
typedef struct _TestIfaceClass  TestIface1Class;

static void test_iface1_base_init    (TestIface1Class *iface);
static void test_iface1_default_init (TestIface1Class *iface, gpointer class_data);

static DEFINE_IFACE(TestIface1, test_iface1, test_iface1_base_init, test_iface1_default_init)

#define TEST_TYPE_IFACE2           (test_iface2_get_type ())
#define TEST_IFACE2_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE2, TestIface2Class))
typedef struct _TestIface2      TestIface2;
typedef struct _TestIfaceClass  TestIface2Class;

static void test_iface2_base_init (TestIface2Class *iface);

static DEFINE_IFACE(TestIface2, test_iface2, test_iface2_base_init, NULL)

#define TEST_TYPE_IFACE3           (test_iface3_get_type ())
#define TEST_IFACE3_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE3, TestIface3Class))
typedef struct _TestIface3      TestIface3;
typedef struct _TestIfaceClass  TestIface3Class;

static void  test_iface3_base_init (TestIface3Class *iface);

static DEFINE_IFACE(TestIface3, test_iface3, test_iface3_base_init, NULL)

#define TEST_TYPE_IFACE4           (test_iface4_get_type ())
#define TEST_IFACE4_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE4, TestIface4Class))
typedef struct _TestIface4      TestIface4;
typedef struct _TestIfaceClass  TestIface4Class;

static void  test_iface4_base_init (TestIface4Class *iface);

static DEFINE_IFACE(TestIface4, test_iface4, test_iface4_base_init, NULL)

#define TEST_TYPE_IFACE5           (test_iface5_get_type ())
#define TEST_IFACE5_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE5, TestIface5Class))
typedef struct _TestIface5      TestIface5;
typedef struct _TestIfaceClass  TestIface5Class;

static void  test_iface5_base_init (TestIface5Class *iface);

static DEFINE_IFACE(TestIface5, test_iface5, test_iface5_base_init, NULL)

#define TEST_TYPE_IFACE6           (test_iface6_get_type ())
#define TEST_IFACE6_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE6, TestIface6Class))
typedef struct _TestIface6      TestIface6;
typedef struct _TestIfaceClass  TestIface6Class;

static void  test_iface6_base_init (TestIface6Class *iface);

static DEFINE_IFACE(TestIface6, test_iface6, test_iface6_base_init, NULL)

/*
 * BaseObject, a parent class for TestObject
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

/*
 * TestObject, a parent class for TestObject
 */
#define TEST_TYPE_OBJECT          (test_object_get_type ())
typedef struct _TestObject        TestObject;
typedef struct _TestObjectClass   TestObjectClass;

struct _TestObject
{
  BaseObject parent_instance;
};
struct _TestObjectClass
{
  BaseObjectClass parent_class;
};

#define TEST_CALLED_ONCE() G_STMT_START { \
  static gboolean called = 0;           \
  g_assert (!called);                   \
  called = TRUE;                        \
} G_STMT_END

#define CHECK_IFACE_TWICE(iface) G_STMT_START {                                 \
  static guint n_calls = 0;                                                     \
  n_calls++;                                                                    \
  g_assert (n_calls <= 2);                                                      \
  g_assert (G_TYPE_IS_INTERFACE (((GTypeInterface*) iface)->g_type));           \
  if (n_calls == 1)                                                             \
    g_assert (((GTypeInterface*) iface)->g_instance_type == 0);                 \
  else                                                                          \
    g_assert (G_TYPE_IS_OBJECT (((GTypeInterface*) iface)->g_instance_type));   \
} G_STMT_END

#define ADD_IFACE(n)  G_STMT_START {				\
  static GInterfaceInfo iface_info = {				\
    (GInterfaceInitFunc)test_object_test_iface##n##_init,	\
    NULL, NULL };						\
								\
  g_type_add_interface_static (TEST_TYPE_OBJECT,		\
			       test_iface##n##_get_type (),	\
			       &iface_info);			\
								\
} G_STMT_END

static gboolean base1, base2, base3, base4, base5, base6;
static gboolean iface1, iface2, iface3, iface4, iface5, iface6;

static void test_object_test_iface1_init (TestIface1Class *iface);
static void test_object_test_iface2_init (TestIface1Class *iface);
static void test_object_test_iface3_init (TestIface3Class *iface);
static void test_object_test_iface4_init (TestIface4Class *iface);
static void test_object_test_iface5_init (TestIface5Class *iface);
static void test_object_test_iface6_init (TestIface6Class *iface);

static GType test_object_get_type (void);

static void
test_object_test_iface1_init (TestIface1Class *iface)
{
  TEST_CALLED_ONCE();

  g_assert (iface->default_val == 0x111111);

  iface->val = 0x10001;

  ADD_IFACE(5);

  iface1 = TRUE;
}

static void
test_object_test_iface2_init (TestIface2Class *iface)
{
  TEST_CALLED_ONCE();
  
  iface->val = 0x20002;
  
  iface2 = TRUE;
}

static void
test_object_test_iface3_init (TestIface3Class *iface)
{
  TEST_CALLED_ONCE();
  
  iface->val = 0x30003;
  
  iface3 = TRUE;
}

static void
test_object_test_iface4_init (TestIface4Class *iface)
{
  TEST_CALLED_ONCE();

  iface->val = 0x40004;
  
  iface4 = TRUE;
}

static void
test_object_test_iface5_init (TestIface5Class *iface)
{
  TEST_CALLED_ONCE();

  iface->val = 0x50005;
  
  iface5 = TRUE;
}

static void
test_object_test_iface6_init (TestIface6Class *iface)
{
  TEST_CALLED_ONCE();

  iface->val = 0x60006;
  
  iface6 = TRUE;
}

static void
test_iface1_default_init (TestIface1Class *iface,
                          gpointer         class_data)
{
  TEST_CALLED_ONCE();
  g_assert (iface->base_iface.g_type == TEST_TYPE_IFACE1);
  g_assert (iface->base_iface.g_instance_type == 0);
  g_assert (iface->base_val == 0x110011);
  g_assert (iface->val == 0);
  g_assert (iface->default_val == 0);
  iface->default_val = 0x111111;
}

static void
test_iface1_base_init (TestIface1Class *iface)
{
  static guint n_calls = 0;
  n_calls++;
  g_assert (n_calls <= 2);

  if (n_calls == 1)
    {
      iface->base_val = 0x110011;
      g_assert (iface->default_val == 0);
    }
  else
    {
      g_assert (iface->base_val == 0x110011);
      g_assert (iface->default_val == 0x111111);
    }

  if (n_calls == 1)
    ADD_IFACE(3);
  
  base1 = TRUE;
}

static void
test_iface2_base_init (TestIface2Class *iface)
{
  CHECK_IFACE_TWICE (iface);

  iface->base_val = 0x220022;
  
  base2 = TRUE;
}

static void
test_iface3_base_init (TestIface3Class *iface)
{
  CHECK_IFACE_TWICE (iface);

  iface->base_val = 0x330033;
  
  base3 = TRUE;
}

static void
test_iface4_base_init (TestIface4Class *iface)
{
  CHECK_IFACE_TWICE (iface);

  iface->base_val = 0x440044;

  base4 = TRUE;
}

static void
test_iface5_base_init (TestIface5Class *iface)
{
  CHECK_IFACE_TWICE (iface);

  iface->base_val = 0x550055;

  base5 = TRUE;
}

static void
test_iface6_base_init (TestIface6Class *iface)
{
  CHECK_IFACE_TWICE (iface);

  iface->base_val = 0x660066;
  
  base6 = TRUE;
}

static void
base_object_base_init (BaseObjectClass *class)
{
  static int n_called = 0;
  n_called++;
  
  /* The second time this is called is for TestObject */
  if (n_called == 2)
    {
      ADD_IFACE(2);
      
      /* No interface base init functions should have been called yet
       */
      g_assert (!base1 && !base2 && !base3 && !base4 && !base5 && !base6);
      g_assert (!iface1 && !iface2 && !iface3 && !iface4 && !iface5 && !iface6);
    }
}

static void
test_object_class_init (TestObjectClass *class)
{
  ADD_IFACE(4);

  /* At this point, the base init functions for all interfaces that have
   * been added should be called, but no interface init functions.
   */
  g_assert (base1 && base2 && base3 && base4 && !base5 && !base6);
  g_assert (!iface1 && !iface2 && !iface3 && !iface4 && !iface5 && !iface6);
}

static DEFINE_TYPE(BaseObject, base_object,
		   NULL, base_object_base_init, NULL,
		   G_TYPE_OBJECT)
static DEFINE_TYPE(TestObject, test_object,
		   test_object_class_init, NULL, NULL,
		   BASE_TYPE_OBJECT)

int
main (int   argc,
      char *argv[])
{
  TestObject *object;
  TestObjectClass *object_class;
  TestIfaceClass *iface;
	
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);
  g_type_init ();

  /* We force the interfaces to be registered in a different order
   * than we add them, so our logic doesn't always deal with interfaces
   * added at the end.
   */
  (void)TEST_TYPE_IFACE4;
  (void)TEST_TYPE_IFACE2;
  (void)TEST_TYPE_IFACE6;
  (void)TEST_TYPE_IFACE5;
  (void)TEST_TYPE_IFACE3;
  (void)TEST_TYPE_IFACE1;

  ADD_IFACE(1);

  object_class = g_type_class_ref (TEST_TYPE_OBJECT);

  ADD_IFACE(6);

  /* All base and interface init functions should have been called
   */
  g_assert (base1 && base2 && base3 && base4 && base5 && base6);
  g_assert (iface1 && iface2 && iface3 && iface4 && iface5 && iface6);
  
  object = g_object_new (TEST_TYPE_OBJECT, NULL);

  iface = TEST_IFACE1_GET_CLASS (object);
  g_assert (iface && iface->val == 0x10001 && iface->base_val == 0x110011);
  iface = TEST_IFACE3_GET_CLASS (object);
  g_assert (iface && iface->val == 0x30003 && iface->base_val == 0x330033);
  iface = TEST_IFACE4_GET_CLASS (object);
  g_assert (iface && iface->val == 0x40004 && iface->base_val == 0x440044);
  iface = TEST_IFACE5_GET_CLASS (object);
  g_assert (iface && iface->val == 0x50005 && iface->base_val == 0x550055);
  iface = TEST_IFACE6_GET_CLASS (object);
  g_assert (iface && iface->val == 0x60006 && iface->base_val == 0x660066);

  return 0;
}
