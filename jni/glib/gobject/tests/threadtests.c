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

static volatile int mtsafe_call_counter = 0; /* multi thread safe call counter */
static int          unsafe_call_counter = 0; /* single-threaded call counter */

#define NUM_COUNTER_INCREMENTS 100000

static void
call_counter_init (gpointer tclass)
{
  int i;
  for (i = 0; i < NUM_COUNTER_INCREMENTS; i++)
    {
      int saved_unsafe_call_counter = unsafe_call_counter;
      g_atomic_int_add (&mtsafe_call_counter, 1); /* real call count update */
      g_thread_yield(); /* let concurrent threads corrupt the unsafe_call_counter state */
      unsafe_call_counter = 1 + saved_unsafe_call_counter; /* non-atomic counter update */
    }
}

static void interface_per_class_init () { call_counter_init (NULL); }

/* define 3 test interfaces */
typedef GTypeInterface MyFace0Interface;
G_DEFINE_INTERFACE (MyFace0, my_face0, G_TYPE_OBJECT);
static void my_face0_default_init (MyFace0Interface *iface) { call_counter_init (iface); }
typedef GTypeInterface MyFace1Interface;
G_DEFINE_INTERFACE (MyFace1, my_face1, G_TYPE_OBJECT);
static void my_face1_default_init (MyFace1Interface *iface) { call_counter_init (iface); }
typedef GTypeInterface MyFace2Interface;
G_DEFINE_INTERFACE (MyFace2, my_face2, G_TYPE_OBJECT);
static void my_face2_default_init (MyFace2Interface *iface) { call_counter_init (iface); }

/* define 3 test objects, adding interfaces 0 & 1, and adding interface 2 after class initialization */
typedef GObject         MyTester0;
typedef GObjectClass    MyTester0Class;
G_DEFINE_TYPE_WITH_CODE (MyTester0, my_tester0, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init);
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init);
                         );
static void my_tester0_init (MyTester0*t) {}
static void my_tester0_class_init (MyTester0Class*c) { call_counter_init (c); }
typedef GObject         MyTester1;
typedef GObjectClass    MyTester1Class;
G_DEFINE_TYPE_WITH_CODE (MyTester1, my_tester1, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init);
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init);
                         );
static void my_tester1_init (MyTester1*t) {}
static void my_tester1_class_init (MyTester1Class*c) { call_counter_init (c); }
typedef GObject         MyTester2;
typedef GObjectClass    MyTester2Class;
G_DEFINE_TYPE_WITH_CODE (MyTester2, my_tester2, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init);
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init);
                         );
static void my_tester2_init (MyTester2*t) {}
static void my_tester2_class_init (MyTester2Class*c) { call_counter_init (c); }

static GCond *sync_cond = NULL;
static GMutex *sync_mutex = NULL;

static gpointer
tester_init_thread (gpointer data)
{
  const GInterfaceInfo face2_interface_info = { (GInterfaceInitFunc) interface_per_class_init, NULL, NULL };
  gpointer klass;
  /* first, syncronize with other threads,
   * then run interface and class initializers,
   * using unsafe_call_counter concurrently
   */
  g_mutex_lock (sync_mutex);
  g_mutex_unlock (sync_mutex);
  /* test default interface initialization for face0 */
  g_type_default_interface_unref (g_type_default_interface_ref (my_face0_get_type()));
  /* test class initialization, face0 per-class initializer, face1 default and per-class initializer */
  klass = g_type_class_ref ((GType) data);
  /* test face2 default and per-class initializer, after class_init */
  g_type_add_interface_static (G_TYPE_FROM_CLASS (klass), my_face2_get_type(), &face2_interface_info);
  /* cleanups */
  g_type_class_unref (klass);
  return NULL;
}

static void
test_threaded_class_init (void)
{
  GThread *threads[3] = { NULL, };
  /* pause newly created threads */
  g_mutex_lock (sync_mutex);
  /* create threads */
  threads[0] = g_thread_create (tester_init_thread, (gpointer) my_tester0_get_type(), TRUE, NULL);
  threads[1] = g_thread_create (tester_init_thread, (gpointer) my_tester1_get_type(), TRUE, NULL);
  threads[2] = g_thread_create (tester_init_thread, (gpointer) my_tester2_get_type(), TRUE, NULL);
  /* execute threads */
  g_mutex_unlock (sync_mutex);
  while (g_atomic_int_get (&mtsafe_call_counter) < (3 + 3 + 3 * 3) * NUM_COUNTER_INCREMENTS)
    {
      if (g_test_verbose())
        g_print ("Initializers counted: %u\n", g_atomic_int_get (&mtsafe_call_counter));
      g_usleep (50 * 1000); /* wait for threads to complete */
    }
  if (g_test_verbose())
    g_print ("Total initializers: %u\n", g_atomic_int_get (&mtsafe_call_counter));
  /* ensure non-corrupted counter updates */
  g_assert_cmpint (g_atomic_int_get (&mtsafe_call_counter), ==, unsafe_call_counter);
}

typedef struct {
  GObject parent;
  char   *name;
} PropTester;
typedef GObjectClass    PropTesterClass;
G_DEFINE_TYPE (PropTester, prop_tester, G_TYPE_OBJECT);
#define PROP_NAME 1
static void
prop_tester_init (PropTester* t)
{
  if (t->name == NULL)
    ; /* neds unit test framework initialization: g_test_bug ("race initializing properties"); */
}
static void
prop_tester_set_property (GObject        *object,
                          guint           property_id,
                          const GValue   *value,
                          GParamSpec     *pspec)
{}
static void
prop_tester_class_init (PropTesterClass *c)
{
  int i;
  GParamSpec *param;
  GObjectClass *gobject_class = G_OBJECT_CLASS (c);

  gobject_class->set_property = prop_tester_set_property; /* silence GObject checks */

  g_mutex_lock (sync_mutex);
  g_cond_signal (sync_cond);
  g_mutex_unlock (sync_mutex);

  for (i = 0; i < 100; i++) /* wait a bit. */
    g_thread_yield();

  call_counter_init (c);
  param = g_param_spec_string ("name", "name_i18n",
			       "yet-more-wasteful-i18n",
			       NULL,
			       G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
			       G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB |
			       G_PARAM_STATIC_NICK);
  g_object_class_install_property (gobject_class, PROP_NAME, param);
}

static gpointer
object_create (gpointer data)
{
  GObject *obj = g_object_new (prop_tester_get_type(), "name", "fish", NULL);
  g_object_unref (obj);
  return NULL;
}

static void
test_threaded_object_init (void)
{
  GThread *creator;
  g_mutex_lock (sync_mutex);

  creator = g_thread_create (object_create, NULL, TRUE, NULL);
  /* really provoke the race */
  g_cond_wait (sync_cond, sync_mutex);

  object_create (NULL);
  g_mutex_unlock (sync_mutex);

  g_thread_join (creator);
}

int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  g_test_init (&argc, &argv, NULL);
  g_type_init ();

  sync_cond = g_cond_new();
  sync_mutex = g_mutex_new();

  g_test_add_func ("/GObject/threaded-class-init", test_threaded_class_init);
  g_test_add_func ("/GObject/threaded-object-init", test_threaded_object_init);

  return g_test_run();
}
