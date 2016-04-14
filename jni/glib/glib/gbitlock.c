/*
 * Copyright © 2008 Ryan Lortie
 * Copyright © 2010 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "gbitlock.h"

#include <glib/gatomic.h>
#include <glib/gslist.h>
#include <glib/gthread.h>

#include "gthreadprivate.h"
#include "config.h"


#ifdef G_BIT_LOCK_FORCE_FUTEX_EMULATION
#undef HAVE_FUTEX
#endif

#ifndef HAVE_FUTEX
static GSList *g_futex_address_list = NULL;
static GMutex *g_futex_mutex = NULL;
#endif

void
_g_futex_thread_init (void) {
#ifndef HAVE_FUTEX
  g_futex_mutex = g_mutex_new ();
#endif
}

#ifdef HAVE_FUTEX
/*
 * We have headers for futex(2) on the build machine.  This does not
 * imply that every system that ever runs the resulting glib will have
 * kernel support for futex, but you'd have to have a pretty old
 * kernel in order for that not to be the case.
 *
 * If anyone actually gets bit by this, please file a bug. :)
 */
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>

/* < private >
 * g_futex_wait:
 * @address: a pointer to an integer
 * @value: the value that should be at @address
 *
 * Atomically checks that the value stored at @address is equal to
 * @value and then blocks.  If the value stored at @address is not
 * equal to @value then this function returns immediately.
 *
 * To unblock, call g_futex_wake() on @address.
 *
 * This call may spuriously unblock (for example, in response to the
 * process receiving a signal) but this is not guaranteed.  Unlike the
 * Linux system call of a similar name, there is no guarantee that a
 * waiting process will unblock due to a g_futex_wake() call in a
 * separate process.
 */
static void
g_futex_wait (const volatile gint *address,
              gint                 value)
{
  syscall (SYS_futex, address, (gsize) FUTEX_WAIT, (gsize) value, NULL);
}

/* < private >
 * g_futex_wake:
 * @address: a pointer to an integer
 *
 * Nominally, wakes one thread that is blocked in g_futex_wait() on
 * @address (if any thread is currently waiting).
 *
 * As mentioned in the documention for g_futex_wait(), spurious
 * wakeups may occur.  As such, this call may result in more than one
 * thread being woken up.
 */
static void
g_futex_wake (const volatile gint *address)
{
  syscall (SYS_futex, address, (gsize) FUTEX_WAKE, (gsize) 1, NULL);
}

#else

/* emulate futex(2) */
typedef struct
{
  const volatile gint *address;
  gint                 ref_count;
  GCond               *wait_queue;
} WaitAddress;

static WaitAddress *
g_futex_find_address (const volatile gint *address)
{
  GSList *node;

  for (node = g_futex_address_list; node; node = node->next)
    {
      WaitAddress *waiter = node->data;

      if (waiter->address == address)
        return waiter;
    }

  return NULL;
}

static void
g_futex_wait (const volatile gint *address,
              gint                 value)
{
  g_mutex_lock (g_futex_mutex);
  if G_LIKELY (g_atomic_int_get (address) == value)
    {
      WaitAddress *waiter;

      if ((waiter = g_futex_find_address (address)) == NULL)
        {
          waiter = g_slice_new (WaitAddress);
          waiter->address = address;
          waiter->wait_queue = g_cond_new ();
          waiter->ref_count = 0;
          g_futex_address_list =
            g_slist_prepend (g_futex_address_list, waiter);
        }

      waiter->ref_count++;
      g_cond_wait (waiter->wait_queue, g_futex_mutex);

      if (!--waiter->ref_count)
        {
          g_futex_address_list =
            g_slist_remove (g_futex_address_list, waiter);
          g_cond_free (waiter->wait_queue);
          g_slice_free (WaitAddress, waiter);
        }
    }
  g_mutex_unlock (g_futex_mutex);
}

static void
g_futex_wake (const volatile gint *address)
{
  WaitAddress *waiter;

  /* need to lock here for two reasons:
   *   1) need to acquire/release lock to ensure waiter is not in
   *      the process of registering a wait
   *   2) need to -stay- locked until the end to ensure a wake()
   *      in another thread doesn't cause 'waiter' to stop existing
   */
  g_mutex_lock (g_futex_mutex);
  if ((waiter = g_futex_find_address (address)))
    g_cond_signal (waiter->wait_queue);
  g_mutex_unlock (g_futex_mutex);
}
#endif

#define CONTENTION_CLASSES 11
static volatile gint g_bit_lock_contended[CONTENTION_CLASSES];

/**
 * g_bit_lock:
 * @address: a pointer to an integer
 * @lock_bit: a bit value between 0 and 31
 *
 * Sets the indicated @lock_bit in @address.  If the bit is already
 * set, this call will block until g_bit_unlock() unsets the
 * corresponding bit.
 *
 * Attempting to lock on two different bits within the same integer is
 * not supported and will very probably cause deadlocks.
 *
 * The value of the bit that is set is (1u << @bit).  If @bit is not
 * between 0 and 31 then the result is undefined.
 *
 * This function accesses @address atomically.  All other accesses to
 * @address must be atomic in order for this function to work
 * reliably.
 *
 * Since: 2.24
 **/
void
g_bit_lock (volatile gint *address,
            gint           lock_bit)
{
  guint v;

 retry:
  v = g_atomic_int_get (address);
  if (v & (1u << lock_bit))
    /* already locked */
    {
      guint class = ((gsize) address) % G_N_ELEMENTS (g_bit_lock_contended);

      g_atomic_int_add (&g_bit_lock_contended[class], +1);
      g_futex_wait (address, v);
      g_atomic_int_add (&g_bit_lock_contended[class], -1);

      goto retry;
    }

  if (!g_atomic_int_compare_and_exchange (address, v, v | (1u << lock_bit)))
    goto retry;
}

/**
 * g_bit_trylock:
 * @address: a pointer to an integer
 * @lock_bit: a bit value between 0 and 31
 * @returns: %TRUE if the lock was acquired
 *
 * Sets the indicated @lock_bit in @address, returning %TRUE if
 * successful.  If the bit is already set, returns %FALSE immediately.
 *
 * Attempting to lock on two different bits within the same integer is
 * not supported.
 *
 * The value of the bit that is set is (1u << @bit).  If @bit is not
 * between 0 and 31 then the result is undefined.
 *
 * This function accesses @address atomically.  All other accesses to
 * @address must be atomic in order for this function to work
 * reliably.
 *
 * Since: 2.24
 **/
gboolean
g_bit_trylock (volatile gint *address,
               gint           lock_bit)
{
  guint v;

 retry:
  v = g_atomic_int_get (address);
  if (v & (1u << lock_bit))
    /* already locked */
    return FALSE;

  if (!g_atomic_int_compare_and_exchange (address, v, v | (1u << lock_bit)))
    goto retry;

  return TRUE;
}

/**
 * g_bit_unlock:
 * @address: a pointer to an integer
 * @lock_bit: a bit value between 0 and 31
 *
 * Clears the indicated @lock_bit in @address.  If another thread is
 * currently blocked in g_bit_lock() on this same bit then it will be
 * woken up.
 *
 * This function accesses @address atomically.  All other accesses to
 * @address must be atomic in order for this function to work
 * reliably.
 *
 * Since: 2.24
 **/
void
g_bit_unlock (volatile gint *address,
              gint           lock_bit)
{
  guint class = ((gsize) address) % G_N_ELEMENTS (g_bit_lock_contended);
  guint v;

 retry:
  v = g_atomic_int_get (address);
  if (!g_atomic_int_compare_and_exchange (address, v, v & ~(1u << lock_bit)))
    goto retry;

  if (g_atomic_int_get (&g_bit_lock_contended[class]))
    g_futex_wake (address);
}
