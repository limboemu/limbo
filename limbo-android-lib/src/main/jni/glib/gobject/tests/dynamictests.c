/* GLib testing framework examples and tests
 * Copyright (C) 2008 Imendio AB
 * Authors: Tim Janik
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <glib.h>
#include <glib-object.h>

/* This test tests the macros for defining dynamic types.
 */

static GMutex *sync_mutex = NULL;
static gboolean loaded = FALSE;

/* MODULE */
typedef struct _TestModule      TestModule;
typedef struct _TestModuleClass TestModuleClass;

#define TEST_TYPE_MODULE              (test_module_get_type ())
#define TEST_MODULE(module)           (G_TYPE_CHECK_INSTANCE_CAST ((module), TEST_TYPE_MODULE, TestModule))
#define TEST_MODULE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), TEST_TYPE_MODULE, TestModuleClass))
#define TEST_IS_MODULE(module)        (G_TYPE_CHECK_INSTANCE_TYPE ((module), TEST_TYPE_MODULE))
#define TEST_IS_MODULE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), TEST_TYPE_MODULE))
#define TEST_MODULE_GET_CLASS(module) (G_TYPE_INSTANCE_GET_CLASS ((module), TEST_TYPE_MODULE, TestModuleClass))
typedef void (*TestModuleRegisterFunc) (GTypeModule *module);

struct _TestModule
{
  GTypeModule parent_instance;

  TestModuleRegisterFunc register_func;
};

struct _TestModuleClass
{
  GTypeModuleClass parent_class;
};

static GType test_module_get_type (void);

static gboolean
test_module_load (GTypeModule *module)
{
  TestModule *test_module = TEST_MODULE (module);

  test_module->register_func (module);

  return TRUE;
}

static void
test_module_unload (GTypeModule *module)
{
}

static void
test_module_class_init (TestModuleClass *class)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

  module_class->load = test_module_load;
  module_class->unload = test_module_unload;
}

static GType test_module_get_type (void)
{
  static GType object_type = 0;

  if (!object_type) {
    static const GTypeInfo object_info =
      {
	sizeof (TestModuleClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) test_module_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,
	sizeof (TestModule),
	0,
	(GInstanceInitFunc)NULL
      };
    object_type = g_type_register_static (G_TYPE_TYPE_MODULE, "TestModule", &object_info, 0);
  }
  return object_type;
}


GTypeModule *
test_module_new (TestModuleRegisterFunc register_func)
{
  TestModule *test_module = g_object_new (TEST_TYPE_MODULE, NULL);
  GTypeModule *module = G_TYPE_MODULE (test_module);

  test_module->register_func = register_func;

  /* Register the types initially */
  g_type_module_use (module);
  g_type_module_unuse (module);

  return G_TYPE_MODULE (module);
}



#define DYNAMIC_OBJECT_TYPE (dynamic_object_get_type ())

typedef GObject DynamicObject;
typedef struct _DynamicObjectClass DynamicObjectClass;

struct _DynamicObjectClass
{
  GObjectClass parent_class;
  guint val;
};

G_DEFINE_DYNAMIC_TYPE(DynamicObject, dynamic_object, G_TYPE_OBJECT);

static void
dynamic_object_class_init (DynamicObjectClass *class)
{
  class->val = 42;
  g_assert (loaded == FALSE);
  loaded = TRUE;
}

static void
dynamic_object_class_finalize (DynamicObjectClass *class)
{
  g_assert (loaded == TRUE);
  loaded = FALSE;
}

static void
dynamic_object_init (DynamicObject *dynamic_object)
{
}


static void
module_register (GTypeModule *module)
{
  dynamic_object_register_type (module);
}

#define N_THREADS 100
#define N_REFS 10000

static gpointer
ref_unref_thread (gpointer data)
{
  gint i;
  /* first, syncronize with other threads,
   */
  if (g_test_verbose())
    g_print ("WAITING!\n");
  g_mutex_lock (sync_mutex);
  g_mutex_unlock (sync_mutex);
  if (g_test_verbose ())
    g_print ("STARTING\n");

  /* ref/unref the klass 10000000 times */
  for (i = N_REFS; i; i--) {
    if (g_test_verbose ())
      if (i % 10)
	g_print ("%d\n", i);
    g_type_class_unref (g_type_class_ref ((GType) data));
  }

  if (g_test_verbose())
    g_print ("DONE !\n");

  return NULL;
}

static void
test_multithreaded_dynamic_type_init (void)
{
  GTypeModule *module;
  DynamicObjectClass *class;
  /* Create N_THREADS threads that are going to just ref/unref a class */
  GThread *threads[N_THREADS];
  guint i;

  module = test_module_new (module_register);
  /* Not loaded until we call ref for the first time */
  class = g_type_class_peek (DYNAMIC_OBJECT_TYPE);
  g_assert (class == NULL);
  g_assert (!loaded);

  /* pause newly created threads */
  g_mutex_lock (sync_mutex);

  /* create threads */
  for (i = 0; i < N_THREADS; i++) {
    threads[i] = g_thread_create (ref_unref_thread, (gpointer) DYNAMIC_OBJECT_TYPE, TRUE, NULL);
  }

  /* execute threads */
  g_mutex_unlock (sync_mutex);

  for (i = 0; i < N_THREADS; i++) {
    g_thread_join (threads[i]);
  }
}

int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  g_test_init (&argc, &argv, NULL);
  g_type_init ();

  sync_mutex = g_mutex_new();

  g_test_add_func ("/GObject/threaded-dynamic-ref-unref-init", test_multithreaded_dynamic_type_init);

  return g_test_run();
}
