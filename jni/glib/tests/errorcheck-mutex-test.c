#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN
#define G_ERRORCHECK_MUTEXES

#include <glib.h>
#include <stdio.h>
#include <string.h>

static gpointer
locking_thread (gpointer mutex)
{
  g_mutex_lock ((GMutex*)mutex);

  return NULL;
}

static void
lock_locked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  g_mutex_lock (mutex);
  g_mutex_lock (mutex);
}

static void
trylock_locked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  g_mutex_lock (mutex);
  g_mutex_trylock (mutex);
}

static void
unlock_unlocked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  g_mutex_lock (mutex);
  g_mutex_unlock (mutex);
  g_mutex_unlock (mutex);
}

static void
free_locked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  g_mutex_lock (mutex);
  g_mutex_free (mutex);
}

static void
wait_on_unlocked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  GCond* cond = g_cond_new ();
  g_cond_wait (cond, mutex);
}

static void
wait_on_otherwise_locked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  GCond* cond = g_cond_new ();
  GThread* thread = g_thread_create (locking_thread, mutex, TRUE, NULL);
  g_assert (thread != NULL);
  g_usleep (G_USEC_PER_SEC);
  g_cond_wait (cond, mutex);
}

static void
timed_wait_on_unlocked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  GCond* cond = g_cond_new ();
  g_cond_timed_wait (cond, mutex, NULL);
}

static void
timed_wait_on_otherwise_locked_mutex (void)
{
  GMutex* mutex = g_mutex_new ();
  GCond* cond = g_cond_new ();
  GThread* thread = g_thread_create (locking_thread, mutex, TRUE, NULL);
  g_assert (thread != NULL);
  g_usleep (G_USEC_PER_SEC);
  g_cond_timed_wait (cond, mutex, NULL);
}

struct
{
  char *name;
  void (*func)();
} func_table[] =
{
  {"lock_locked_mutex", lock_locked_mutex},
  {"trylock_locked_mutex", trylock_locked_mutex},
  {"unlock_unlocked_mutex", unlock_unlocked_mutex},
  {"free_locked_mutex", free_locked_mutex},
  {"wait_on_unlocked_mutex", wait_on_unlocked_mutex},
  {"wait_on_otherwise_locked_mutex", wait_on_otherwise_locked_mutex},
  {"timed_wait_on_unlocked_mutex", timed_wait_on_unlocked_mutex},
  {"timed_wait_on_otherwise_locked_mutex",
   timed_wait_on_otherwise_locked_mutex}
};

int
main (int argc, char* argv[])
{
  int i;

  if (argc == 2)
    {
      for (i = 0; i < G_N_ELEMENTS (func_table); i++)
        {
          if (strcmp (func_table[i].name, argv[1]) == 0)
            {
              g_thread_init (NULL);
              func_table[i].func ();
              g_assert_not_reached ();
            }
        }
    }

  fprintf (stderr, "Usage: errorcheck-mutex-test [TEST]\n\n");
  fprintf (stderr, "   where TEST can be one of:\n\n");
  for (i = 0; i < G_N_ELEMENTS (func_table); i++)
    {
      fprintf (stderr, "      %s\n", func_table[i].name);
    }

  return 0;
}
