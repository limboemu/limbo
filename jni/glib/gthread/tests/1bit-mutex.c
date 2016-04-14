/*
 * Copyright © 2008 Ryan Lortie
 * Copyright © 2010 Codethink Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

/* LOCKS should be more than the number of contention
 * counters in gthread.c in order to ensure we exercise
 * the case where they overlap.
 */
#define LOCKS      48
#define ITERATIONS 10000
#define THREADS    100


#if TEST_EMULATED_FUTEX
  /* this is defined for the 1bit-mutex-emufutex test.
   *
   * we want to test the emulated futex even if futex(2) is available.
   */

  /* side-step some glib build stuff */
  #ifndef DISABLE_VISIBILITY
  #define DISABLE_VISIBILITY
  #endif
  #define GLIB_COMPILATION

  /* rebuild gbitlock.c without futex support,
     defining our own version of the g_bit_*lock symbols
   */
  #define g_bit_lock            _emufutex_g_bit_lock
  #define g_bit_trylock         _emufutex_g_bit_trylock
  #define g_bit_unlock          _emufutex_g_bit_unlock
  #define _g_futex_thread_init  _emufutex_g_futex_thread_init

  #define G_BIT_LOCK_FORCE_FUTEX_EMULATION

  #include <glib/gbitlock.c>
#endif

#include <glib.h>

volatile GThread *owners[LOCKS];
volatile gint     locks[LOCKS];
volatile gint     bits[LOCKS];

static void
acquire (int nr)
{
  GThread *self;

  self = g_thread_self ();

  if (!g_bit_trylock (&locks[nr], bits[nr]))
    {
      if (g_test_verbose ())
        g_print ("thread %p going to block on lock %d\n", self, nr);
      g_bit_lock (&locks[nr], bits[nr]);
    }

  g_assert (owners[nr] == NULL);   /* hopefully nobody else is here */
  owners[nr] = self;

  /* let some other threads try to ruin our day */
  g_thread_yield ();
  g_thread_yield ();
  g_thread_yield ();

  g_assert (owners[nr] == self);   /* hopefully this is still us... */
  owners[nr] = NULL;               /* make way for the next guy */
  g_bit_unlock (&locks[nr], bits[nr]);
}

static gpointer
thread_func (gpointer data)
{
  gint i;

  for (i = 0; i < ITERATIONS; i++)
    acquire (g_random_int () % LOCKS);

  return NULL;
}

static void
testcase (void)
{
  GThread *threads[THREADS];
  int i;

  g_thread_init (NULL);

#ifdef TEST_EMULATED_FUTEX
  _g_futex_thread_init ();
  #define SUFFIX "-emufutex"

  /* ensure that we are using the emulated futex by checking
   * (at compile-time) for the existance of 'g_futex_mutex'
   */
  g_assert (g_futex_mutex != NULL);
#else
  #define SUFFIX ""
#endif

  for (i = 0; i < LOCKS; i++)
    bits[i] = g_random_int () % 32;

  for (i = 0; i < THREADS; i++)
    threads[i] = g_thread_create (thread_func, NULL, TRUE, NULL);

  for (i = 0; i < THREADS; i++)
    g_thread_join (threads[i]);

  for (i = 0; i < LOCKS; i++)
    {
      g_assert (owners[i] == NULL);
      g_assert (locks[i] == 0);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glib/1bit-mutex" SUFFIX, testcase);

  return g_test_run ();
}
