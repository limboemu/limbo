/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gthread.c: MT safety related functions
 * Copyright 1998 Sebastian Wilhelmi; University of Karlsruhe
 *                Owen Taylor
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Prelude {{{1 ----------------------------------------------------------- */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

/* implement gthread.h's inline functions */
#define G_IMPLEMENT_INLINES 1
#define __G_THREAD_C__

#include "config.h"

#include "gthread.h"
#include "gthreadprivate.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef G_OS_WIN32
#include <sys/time.h>
#include <time.h>
#else
#include <windows.h>
#endif /* G_OS_WIN32 */

#include <string.h>

#include "garray.h"
#include "gslist.h"
#include "gtestutils.h"
#include "gtimer.h"


/**
 * SECTION: threads
 * @title: Threads
 * @short_description: thread abstraction; including threads, different
 *                     mutexes, conditions and thread private data
 * @see_also: #GThreadPool, #GAsyncQueue
 *
 * Threads act almost like processes, but unlike processes all threads
 * of one process share the same memory. This is good, as it provides
 * easy communication between the involved threads via this shared
 * memory, and it is bad, because strange things (so called
 * "Heisenbugs") might happen if the program is not carefully designed.
 * In particular, due to the concurrent nature of threads, no
 * assumptions on the order of execution of code running in different
 * threads can be made, unless order is explicitly forced by the
 * programmer through synchronization primitives.
 *
 * The aim of the thread related functions in GLib is to provide a
 * portable means for writing multi-threaded software. There are
 * primitives for mutexes to protect the access to portions of memory
 * (#GMutex, #GStaticMutex, #G_LOCK_DEFINE, #GStaticRecMutex and
 * #GStaticRWLock). There are primitives for condition variables to
 * allow synchronization of threads (#GCond).  There are primitives for
 * thread-private data - data that every thread has a private instance
 * of (#GPrivate, #GStaticPrivate). Last but definitely not least there
 * are primitives to portably create and manage threads (#GThread).
 *
 * The threading system is initialized with g_thread_init(), which
 * takes an optional custom thread implementation or %NULL for the
 * default implementation. If you want to call g_thread_init() with a
 * non-%NULL argument this must be done before executing any other GLib
 * functions (except g_mem_set_vtable()). This is a requirement even if
 * no threads are in fact ever created by the process.
 *
 * Calling g_thread_init() with a %NULL argument is somewhat more
 * relaxed. You may call any other glib functions in the main thread
 * before g_thread_init() as long as g_thread_init() is not called from
 * a glib callback, or with any locks held. However, many libraries
 * above glib does not support late initialization of threads, so doing
 * this should be avoided if possible.
 *
 * Please note that since version 2.24 the GObject initialization
 * function g_type_init() initializes threads (with a %NULL argument),
 * so most applications, including those using Gtk+ will run with
 * threads enabled. If you want a special thread implementation, make
 * sure you call g_thread_init() before g_type_init() is called.
 *
 * After calling g_thread_init(), GLib is completely thread safe (all
 * global data is automatically locked), but individual data structure
 * instances are not automatically locked for performance reasons. So,
 * for example you must coordinate accesses to the same #GHashTable
 * from multiple threads.  The two notable exceptions from this rule
 * are #GMainLoop and #GAsyncQueue, which <emphasis>are</emphasis>
 * threadsafe and need no further application-level locking to be
 * accessed from multiple threads.
 *
 * To help debugging problems in multithreaded applications, GLib
 * supports error-checking mutexes that will give you helpful error
 * messages on common problems. To use error-checking mutexes, define
 * the symbol #G_ERRORCHECK_MUTEXES when compiling the application.
 **/

/**
 * G_THREADS_IMPL_POSIX:
 *
 * This macro is defined if POSIX style threads are used.
 **/

/**
 * G_THREADS_ENABLED:
 *
 * This macro is defined if GLib was compiled with thread support. This
 * does not necessarily mean that there is a thread implementation
 * available, but it does mean that the infrastructure is in place and
 * that once you provide a thread implementation to g_thread_init(),
 * GLib will be multi-thread safe. If #G_THREADS_ENABLED is not
 * defined, then Glib is not, and cannot be, multi-thread safe.
 **/

/**
 * G_THREADS_IMPL_NONE:
 *
 * This macro is defined if no thread implementation is used. You can,
 * however, provide one to g_thread_init() to make GLib multi-thread
 * safe.
 **/

/* G_LOCK Documentation {{{1 ---------------------------------------------- */

/* IMPLEMENTATION NOTE:
 *
 * G_LOCK_DEFINE and friends are convenience macros defined in
 * gthread.h.  Their documentation lives here.
 */

/**
 * G_LOCK_DEFINE:
 * @name: the name of the lock.
 *
 * The %G_LOCK_* macros provide a convenient interface to #GStaticMutex
 * with the advantage that they will expand to nothing in programs
 * compiled against a thread-disabled GLib, saving code and memory
 * there. #G_LOCK_DEFINE defines a lock. It can appear anywhere
 * variable definitions may appear in programs, i.e. in the first block
 * of a function or outside of functions. The @name parameter will be
 * mangled to get the name of the #GStaticMutex. This means that you
 * can use names of existing variables as the parameter - e.g. the name
 * of the variable you intent to protect with the lock. Look at our
 * <function>give_me_next_number()</function> example using the
 * %G_LOCK_* macros:
 *
 * <example>
 *  <title>Using the %G_LOCK_* convenience macros</title>
 *  <programlisting>
 *   G_LOCK_DEFINE (current_number);
 *
 *   int
 *   give_me_next_number (void)
 *   {
 *     static int current_number = 0;
 *     int ret_val;
 *
 *     G_LOCK (current_number);
 *     ret_val = current_number = calc_next_number (current_number);
 *     G_UNLOCK (current_number);
 *
 *     return ret_val;
 *   }
 *  </programlisting>
 * </example>
 **/

/**
 * G_LOCK_DEFINE_STATIC:
 * @name: the name of the lock.
 *
 * This works like #G_LOCK_DEFINE, but it creates a static object.
 **/

/**
 * G_LOCK_EXTERN:
 * @name: the name of the lock.
 *
 * This declares a lock, that is defined with #G_LOCK_DEFINE in another
 * module.
 **/

/**
 * G_LOCK:
 * @name: the name of the lock.
 *
 * Works like g_mutex_lock(), but for a lock defined with
 * #G_LOCK_DEFINE.
 **/

/**
 * G_TRYLOCK:
 * @name: the name of the lock.
 * @Returns: %TRUE, if the lock could be locked.
 *
 * Works like g_mutex_trylock(), but for a lock defined with
 * #G_LOCK_DEFINE.
 **/

/**
 * G_UNLOCK:
 * @name: the name of the lock.
 *
 * Works like g_mutex_unlock(), but for a lock defined with
 * #G_LOCK_DEFINE.
 **/

/* GThreadError {{{1 ------------------------------------------------------- */
/**
 * GThreadError:
 * @G_THREAD_ERROR_AGAIN: a thread couldn't be created due to resource
 *                        shortage. Try again later.
 *
 * Possible errors of thread related functions.
 **/

/**
 * G_THREAD_ERROR:
 *
 * The error domain of the GLib thread subsystem.
 **/
GQuark
g_thread_error_quark (void)
{
  return g_quark_from_static_string ("g_thread_error");
}

/* Miscellaneous Structures {{{1 ------------------------------------------ */
/* Keep this in sync with GRealThread in gmain.c! */
typedef struct _GRealThread GRealThread;
struct  _GRealThread
{
  GThread thread;
  gpointer private_data;
  GRealThread *next;
  gpointer retval;
  GSystemThread system_thread;
};

typedef struct _GStaticPrivateNode GStaticPrivateNode;
struct _GStaticPrivateNode
{
  gpointer       data;
  GDestroyNotify destroy;
};

static void    g_thread_cleanup (gpointer data);
static void    g_thread_fail (void);
static guint64 gettime (void);

guint64        (*g_thread_gettime) (void) = gettime;

/* Global Variables {{{1 -------------------------------------------------- */

static GSystemThread zero_thread; /* This is initialized to all zero */
gboolean g_thread_use_default_impl = TRUE;

/**
 * g_thread_supported:
 * @Returns: %TRUE, if the thread system is initialized.
 *
 * This function returns %TRUE if the thread system is initialized, and
 * %FALSE if it is not.
 *
 * <note><para>This function is actually a macro. Apart from taking the
 * address of it you can however use it as if it was a
 * function.</para></note>
 **/

/* IMPLEMENTATION NOTE:
 *
 * g_thread_supported() is just returns g_threads_got_initialized
 */
gboolean g_threads_got_initialized = FALSE;


/* Thread Implementation Virtual Function Table {{{1 ---------------------- */
/* Virtual Function Table Documentation {{{2 ------------------------------ */
/**
 * GThreadFunctions:
 * @mutex_new: virtual function pointer for g_mutex_new()
 * @mutex_lock: virtual function pointer for g_mutex_lock()
 * @mutex_trylock: virtual function pointer for g_mutex_trylock()
 * @mutex_unlock: virtual function pointer for g_mutex_unlock()
 * @mutex_free: virtual function pointer for g_mutex_free()
 * @cond_new: virtual function pointer for g_cond_new()
 * @cond_signal: virtual function pointer for g_cond_signal()
 * @cond_broadcast: virtual function pointer for g_cond_broadcast()
 * @cond_wait: virtual function pointer for g_cond_wait()
 * @cond_timed_wait: virtual function pointer for g_cond_timed_wait()
 * @cond_free: virtual function pointer for g_cond_free()
 * @private_new: virtual function pointer for g_private_new()
 * @private_get: virtual function pointer for g_private_get()
 * @private_set: virtual function pointer for g_private_set()
 * @thread_create: virtual function pointer for g_thread_create()
 * @thread_yield: virtual function pointer for g_thread_yield()
 * @thread_join: virtual function pointer for g_thread_join()
 * @thread_exit: virtual function pointer for g_thread_exit()
 * @thread_set_priority: virtual function pointer for
 *                       g_thread_set_priority()
 * @thread_self: virtual function pointer for g_thread_self()
 * @thread_equal: used internally by recursive mutex locks and by some
 *                assertion checks
 *
 * This function table is used by g_thread_init() to initialize the
 * thread system. The functions in the table are directly used by their
 * g_* prepended counterparts (described in this document).  For
 * example, if you call g_mutex_new() then mutex_new() from the table
 * provided to g_thread_init() will be called.
 *
 * <note><para>Do not use this struct unless you know what you are
 * doing.</para></note>
 **/

/* IMPLEMENTATION NOTE:
 *
 * g_thread_functions_for_glib_use is a global symbol that gets used by
 * most of the "primative" threading calls.  g_mutex_lock(), for
 * example, is just a macro that calls the appropriate virtual function
 * out of this table.
 *
 * For that reason, all of those macros are documented here.
 */
GThreadFunctions g_thread_functions_for_glib_use = {
/* GMutex Virtual Functions {{{2 ------------------------------------------ */

/**
 * GMutex:
 *
 * The #GMutex struct is an opaque data structure to represent a mutex
 * (mutual exclusion). It can be used to protect data against shared
 * access. Take for example the following function:
 *
 * <example>
 *  <title>A function which will not work in a threaded environment</title>
 *  <programlisting>
 *   int
 *   give_me_next_number (void)
 *   {
 *     static int current_number = 0;
 *
 *     /<!-- -->* now do a very complicated calculation to calculate the new
 *      * number, this might for example be a random number generator
 *      *<!-- -->/
 *     current_number = calc_next_number (current_number);
 *
 *     return current_number;
 *   }
 *  </programlisting>
 * </example>
 *
 * It is easy to see that this won't work in a multi-threaded
 * application. There current_number must be protected against shared
 * access. A first naive implementation would be:
 *
 * <example>
 *  <title>The wrong way to write a thread-safe function</title>
 *  <programlisting>
 *   int
 *   give_me_next_number (void)
 *   {
 *     static int current_number = 0;
 *     int ret_val;
 *     static GMutex * mutex = NULL;
 *
 *     if (!mutex) mutex = g_mutex_new (<!-- -->);
 *
 *     g_mutex_lock (mutex);
 *     ret_val = current_number = calc_next_number (current_number);
 *     g_mutex_unlock (mutex);
 *
 *     return ret_val;
 *   }
 *  </programlisting>
 * </example>
 *
 * This looks like it would work, but there is a race condition while
 * constructing the mutex and this code cannot work reliable. Please do
 * not use such constructs in your own programs! One working solution
 * is:
 *
 * <example>
 *  <title>A correct thread-safe function</title>
 *  <programlisting>
 *   static GMutex *give_me_next_number_mutex = NULL;
 *
 *   /<!-- -->* this function must be called before any call to
 *    * give_me_next_number(<!-- -->)
 *    *
 *    * it must be called exactly once.
 *    *<!-- -->/
 *   void
 *   init_give_me_next_number (void)
 *   {
 *     g_assert (give_me_next_number_mutex == NULL);
 *     give_me_next_number_mutex = g_mutex_new (<!-- -->);
 *   }
 *
 *   int
 *   give_me_next_number (void)
 *   {
 *     static int current_number = 0;
 *     int ret_val;
 *
 *     g_mutex_lock (give_me_next_number_mutex);
 *     ret_val = current_number = calc_next_number (current_number);
 *     g_mutex_unlock (give_me_next_number_mutex);
 *
 *     return ret_val;
 *   }
 *  </programlisting>
 * </example>
 *
 * #GStaticMutex provides a simpler and safer way of doing this.
 *
 * If you want to use a mutex, and your code should also work without
 * calling g_thread_init() first, then you can not use a #GMutex, as
 * g_mutex_new() requires that the thread system be initialized. Use a
 * #GStaticMutex instead.
 *
 * A #GMutex should only be accessed via the following functions.
 *
 * <note><para>All of the <function>g_mutex_*</function> functions are
 * actually macros. Apart from taking their addresses, you can however
 * use them as if they were functions.</para></note>
 **/

/**
 * g_mutex_new:
 * @Returns: a new #GMutex.
 *
 * Creates a new #GMutex.
 *
 * <note><para>This function will abort if g_thread_init() has not been
 * called yet.</para></note>
 **/
  (GMutex*(*)())g_thread_fail,

/**
 * g_mutex_lock:
 * @mutex: a #GMutex.
 *
 * Locks @mutex. If @mutex is already locked by another thread, the
 * current thread will block until @mutex is unlocked by the other
 * thread.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will do nothing.
 *
 * <note><para>#GMutex is neither guaranteed to be recursive nor to be
 * non-recursive, i.e. a thread could deadlock while calling
 * g_mutex_lock(), if it already has locked @mutex. Use
 * #GStaticRecMutex, if you need recursive mutexes.</para></note>
 **/
  NULL,

/**
 * g_mutex_trylock:
 * @mutex: a #GMutex.
 * @Returns: %TRUE, if @mutex could be locked.
 *
 * Tries to lock @mutex. If @mutex is already locked by another thread,
 * it immediately returns %FALSE. Otherwise it locks @mutex and returns
 * %TRUE.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will immediately return %TRUE.
 *
 * <note><para>#GMutex is neither guaranteed to be recursive nor to be
 * non-recursive, i.e. the return value of g_mutex_trylock() could be
 * both %FALSE or %TRUE, if the current thread already has locked
 * @mutex. Use #GStaticRecMutex, if you need recursive
 * mutexes.</para></note>
 **/
  NULL,

/**
 * g_mutex_unlock:
 * @mutex: a #GMutex.
 *
 * Unlocks @mutex. If another thread is blocked in a g_mutex_lock()
 * call for @mutex, it will be woken and can lock @mutex itself.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will do nothing.
 **/
  NULL,

/**
 * g_mutex_free:
 * @mutex: a #GMutex.
 *
 * Destroys @mutex.
 *
 * <note><para>Calling g_mutex_free() on a locked mutex may result in
 * undefined behaviour.</para></note>
 **/
  NULL,

/* GCond Virtual Functions {{{2 ------------------------------------------ */

/**
 * GCond:
 *
 * The #GCond struct is an opaque data structure that represents a
 * condition. Threads can block on a #GCond if they find a certain
 * condition to be false. If other threads change the state of this
 * condition they signal the #GCond, and that causes the waiting
 * threads to be woken up.
 *
 * <example>
 *  <title>
 *   Using GCond to block a thread until a condition is satisfied
 *  </title>
 *  <programlisting>
 *   GCond* data_cond = NULL; /<!-- -->* Must be initialized somewhere *<!-- -->/
 *   GMutex* data_mutex = NULL; /<!-- -->* Must be initialized somewhere *<!-- -->/
 *   gpointer current_data = NULL;
 *
 *   void
 *   push_data (gpointer data)
 *   {
 *     g_mutex_lock (data_mutex);
 *     current_data = data;
 *     g_cond_signal (data_cond);
 *     g_mutex_unlock (data_mutex);
 *   }
 *
 *   gpointer
 *   pop_data (void)
 *   {
 *     gpointer data;
 *
 *     g_mutex_lock (data_mutex);
 *     while (!current_data)
 *       g_cond_wait (data_cond, data_mutex);
 *     data = current_data;
 *     current_data = NULL;
 *     g_mutex_unlock (data_mutex);
 *
 *     return data;
 *   }
 *  </programlisting>
 * </example>
 *
 * Whenever a thread calls <function>pop_data()</function> now, it will
 * wait until current_data is non-%NULL, i.e. until some other thread
 * has called <function>push_data()</function>.
 *
 * <note><para>It is important to use the g_cond_wait() and
 * g_cond_timed_wait() functions only inside a loop which checks for the
 * condition to be true.  It is not guaranteed that the waiting thread
 * will find the condition fulfilled after it wakes up, even if the
 * signaling thread left the condition in that state: another thread may
 * have altered the condition before the waiting thread got the chance
 * to be woken up, even if the condition itself is protected by a
 * #GMutex, like above.</para></note>
 *
 * A #GCond should only be accessed via the following functions.
 *
 * <note><para>All of the <function>g_cond_*</function> functions are
 * actually macros. Apart from taking their addresses, you can however
 * use them as if they were functions.</para></note>
 **/

/**
 * g_cond_new:
 * @Returns: a new #GCond.
 *
 * Creates a new #GCond. This function will abort, if g_thread_init()
 * has not been called yet.
 **/
  (GCond*(*)())g_thread_fail,

/**
 * g_cond_signal:
 * @cond: a #GCond.
 *
 * If threads are waiting for @cond, exactly one of them is woken up.
 * It is good practice to hold the same lock as the waiting thread
 * while calling this function, though not required.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will do nothing.
 **/
  NULL,

/**
 * g_cond_broadcast:
 * @cond: a #GCond.
 *
 * If threads are waiting for @cond, all of them are woken up. It is
 * good practice to lock the same mutex as the waiting threads, while
 * calling this function, though not required.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will do nothing.
 **/
  NULL,

/**
 * g_cond_wait:
 * @cond: a #GCond.
 * @mutex: a #GMutex, that is currently locked.
 *
 * Waits until this thread is woken up on @cond. The @mutex is unlocked
 * before falling asleep and locked again before resuming.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will immediately return.
 **/
  NULL,

/**
 * g_cond_timed_wait:
 * @cond: a #GCond.
 * @mutex: a #GMutex that is currently locked.
 * @abs_time: a #GTimeVal, determining the final time.
 * @Returns: %TRUE if @cond was signalled, or %FALSE on timeout.
 *
 * Waits until this thread is woken up on @cond, but not longer than
 * until the time specified by @abs_time. The @mutex is unlocked before
 * falling asleep and locked again before resuming.
 *
 * If @abs_time is %NULL, g_cond_timed_wait() acts like g_cond_wait().
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will immediately return %TRUE.
 *
 * To easily calculate @abs_time a combination of g_get_current_time()
 * and g_time_val_add() can be used.
 **/
  NULL,

/**
 * g_cond_free:
 * @cond: a #GCond.
 *
 * Destroys the #GCond.
 **/
  NULL,

/* GPrivate Virtual Functions {{{2 --------------------------------------- */

/**
 * GPrivate:
 *
 * The #GPrivate struct is an opaque data structure to represent a
 * thread private data key. Threads can thereby obtain and set a
 * pointer which is private to the current thread. Take our
 * <function>give_me_next_number(<!-- -->)</function> example from
 * above.  Suppose we don't want <literal>current_number</literal> to be
 * shared between the threads, but instead to be private to each thread.
 * This can be done as follows:
 *
 * <example>
 *  <title>Using GPrivate for per-thread data</title>
 *  <programlisting>
 *   GPrivate* current_number_key = NULL; /<!-- -->* Must be initialized somewhere
 *                                           with g_private_new (g_free); *<!-- -->/
 *
 *   int
 *   give_me_next_number (void)
 *   {
 *     int *current_number = g_private_get (current_number_key);
 *
 *     if (!current_number)
 *       {
 *         current_number = g_new (int, 1);
 *         *current_number = 0;
 *         g_private_set (current_number_key, current_number);
 *       }
 *
 *     *current_number = calc_next_number (*current_number);
 *
 *     return *current_number;
 *   }
 *  </programlisting>
 * </example>
 *
 * Here the pointer belonging to the key
 * <literal>current_number_key</literal> is read. If it is %NULL, it has
 * not been set yet. Then get memory for an integer value, assign this
 * memory to the pointer and write the pointer back. Now we have an
 * integer value that is private to the current thread.
 *
 * The #GPrivate struct should only be accessed via the following
 * functions.
 *
 * <note><para>All of the <function>g_private_*</function> functions are
 * actually macros. Apart from taking their addresses, you can however
 * use them as if they were functions.</para></note>
 **/

/**
 * g_private_new:
 * @destructor: a function to destroy the data keyed to #GPrivate when
 *              a thread ends.
 * @Returns: a new #GPrivate.
 *
 * Creates a new #GPrivate. If @destructor is non-%NULL, it is a
 * pointer to a destructor function. Whenever a thread ends and the
 * corresponding pointer keyed to this instance of #GPrivate is
 * non-%NULL, the destructor is called with this pointer as the
 * argument.
 *
 * <note><para>@destructor is used quite differently from @notify in
 * g_static_private_set().</para></note>
 *
 * <note><para>A #GPrivate can not be freed. Reuse it instead, if you
 * can, to avoid shortage, or use #GStaticPrivate.</para></note>
 *
 * <note><para>This function will abort if g_thread_init() has not been
 * called yet.</para></note>
 **/
  (GPrivate*(*)(GDestroyNotify))g_thread_fail,

/**
 * g_private_get:
 * @private_key: a #GPrivate.
 * @Returns: the corresponding pointer.
 *
 * Returns the pointer keyed to @private_key for the current thread. If
 * g_private_set() hasn't been called for the current @private_key and
 * thread yet, this pointer will be %NULL.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will return the value of @private_key
 * casted to #gpointer. Note however, that private data set
 * <emphasis>before</emphasis> g_thread_init() will
 * <emphasis>not</emphasis> be retained <emphasis>after</emphasis> the
 * call. Instead, %NULL will be returned in all threads directly after
 * g_thread_init(), regardless of any g_private_set() calls issued
 * before threading system intialization.
 **/
  NULL,

/**
 * g_private_set:
 * @private_key: a #GPrivate.
 * @data: the new pointer.
 *
 * Sets the pointer keyed to @private_key for the current thread.
 *
 * This function can be used even if g_thread_init() has not yet been
 * called, and, in that case, will set @private_key to @data casted to
 * #GPrivate*. See g_private_get() for resulting caveats.
 **/
  NULL,

/* GThread Virtual Functions {{{2 ---------------------------------------- */
/**
 * GThread:
 *
 * The #GThread struct represents a running thread. It has three public
 * read-only members, but the underlying struct is bigger, so you must
 * not copy this struct.
 *
 * <note><para>Resources for a joinable thread are not fully released
 * until g_thread_join() is called for that thread.</para></note>
 **/

/**
 * GThreadFunc:
 * @data: data passed to the thread.
 * @Returns: the return value of the thread, which will be returned by
 *           g_thread_join().
 *
 * Specifies the type of the @func functions passed to
 * g_thread_create() or g_thread_create_full().
 **/

/**
 * GThreadPriority:
 * @G_THREAD_PRIORITY_LOW: a priority lower than normal
 * @G_THREAD_PRIORITY_NORMAL: the default priority
 * @G_THREAD_PRIORITY_HIGH: a priority higher than normal
 * @G_THREAD_PRIORITY_URGENT: the highest priority
 *
 * Specifies the priority of a thread.
 *
 * <note><para>It is not guaranteed that threads with different priorities
 * really behave accordingly. On some systems (e.g. Linux) there are no
 * thread priorities. On other systems (e.g. Solaris) there doesn't
 * seem to be different scheduling for different priorities. All in all
 * try to avoid being dependent on priorities.</para></note>
 **/

/**
 * g_thread_create:
 * @func: a function to execute in the new thread.
 * @data: an argument to supply to the new thread.
 * @joinable: should this thread be joinable?
 * @error: return location for error.
 * @Returns: the new #GThread on success.
 *
 * This function creates a new thread with the default priority.
 *
 * If @joinable is %TRUE, you can wait for this threads termination
 * calling g_thread_join(). Otherwise the thread will just disappear
 * when it terminates.
 *
 * The new thread executes the function @func with the argument @data.
 * If the thread was created successfully, it is returned.
 *
 * @error can be %NULL to ignore errors, or non-%NULL to report errors.
 * The error is set, if and only if the function returns %NULL.
 **/
  (void(*)(GThreadFunc, gpointer, gulong,
	   gboolean, gboolean, GThreadPriority,
	   gpointer, GError**))g_thread_fail,

/**
 * g_thread_yield:
 *
 * Gives way to other threads waiting to be scheduled.
 *
 * This function is often used as a method to make busy wait less evil.
 * But in most cases you will encounter, there are better methods to do
 * that. So in general you shouldn't use this function.
 **/
  NULL,

  NULL,                                        /* thread_join */
  NULL,                                        /* thread_exit */
  NULL,                                        /* thread_set_priority */
  NULL,                                        /* thread_self */
  NULL                                         /* thread_equal */
};

/* Local Data {{{1 -------------------------------------------------------- */

static GMutex   *g_once_mutex = NULL;
static GCond    *g_once_cond = NULL;
static GPrivate *g_thread_specific_private = NULL;
static GRealThread *g_thread_all_threads = NULL;
static GSList   *g_thread_free_indeces = NULL;
static GSList*   g_once_init_list = NULL;

G_LOCK_DEFINE_STATIC (g_thread);

/* Initialisation {{{1 ---------------------------------------------------- */

#ifdef G_THREADS_ENABLED
/**
 * g_thread_init:
 * @vtable: a function table of type #GThreadFunctions, that provides
 *          the entry points to the thread system to be used.
 *
 * If you use GLib from more than one thread, you must initialize the
 * thread system by calling g_thread_init(). Most of the time you will
 * only have to call <literal>g_thread_init (NULL)</literal>.
 *
 * <note><para>Do not call g_thread_init() with a non-%NULL parameter unless
 * you really know what you are doing.</para></note>
 *
 * <note><para>g_thread_init() must not be called directly or indirectly as a
 * callback from GLib. Also no mutexes may be currently locked while
 * calling g_thread_init().</para></note>
 *
 * <note><para>g_thread_init() changes the way in which #GTimer measures
 * elapsed time. As a consequence, timers that are running while
 * g_thread_init() is called may report unreliable times.</para></note>
 *
 * Calling g_thread_init() multiple times is allowed (since version
 * 2.24), but nothing happens except for the first call. If the
 * argument is non-%NULL on such a call a warning will be printed, but
 * otherwise the argument is ignored.
 *
 * If no thread system is available and @vtable is %NULL or if not all
 * elements of @vtable are non-%NULL, then g_thread_init() will abort.
 *
 * <note><para>To use g_thread_init() in your program, you have to link with
 * the libraries that the command <command>pkg-config --libs
 * gthread-2.0</command> outputs. This is not the case for all the
 * other thread related functions of GLib. Those can be used without
 * having to link with the thread libraries.</para></note>
 **/

/* This must be called only once, before any threads are created.
 * It will only be called from g_thread_init() in -lgthread.
 */
void
g_thread_init_glib (void)
{
  /* We let the main thread (the one that calls g_thread_init) inherit
   * the static_private data set before calling g_thread_init
   */
  GRealThread* main_thread = (GRealThread*) g_thread_self ();

  /* mutex and cond creation works without g_threads_got_initialized */
  g_once_mutex = g_mutex_new ();
  g_once_cond = g_cond_new ();

  /* we may only create mutex and cond in here */
  _g_mem_thread_init_noprivate_nomessage ();

  /* setup the basic threading system */
  g_threads_got_initialized = TRUE;
  g_thread_specific_private = g_private_new (g_thread_cleanup);
  g_private_set (g_thread_specific_private, main_thread);
  G_THREAD_UF (thread_self, (&main_thread->system_thread));

  /* complete memory system initialization, g_private_*() works now */
  _g_slice_thread_init_nomessage ();

  /* accomplish log system initialization to enable messaging */
  _g_messages_thread_init_nomessage ();

  /* we may run full-fledged initializers from here */
  _g_atomic_thread_init ();
  _g_convert_thread_init ();
  _g_rand_thread_init ();
  _g_main_thread_init ();
  _g_utils_thread_init ();
  _g_futex_thread_init ();
#ifdef G_OS_WIN32
  _g_win32_thread_init ();
#endif
}
#endif /* G_THREADS_ENABLED */

/* The following sections implement: GOnce, GStaticMutex, GStaticRecMutex,
 * GStaticPrivate, 
 **/

/* GOnce {{{1 ------------------------------------------------------------- */

/**
 * GOnce:
 * @status: the status of the #GOnce
 * @retval: the value returned by the call to the function, if @status
 *          is %G_ONCE_STATUS_READY
 *
 * A #GOnce struct controls a one-time initialization function. Any
 * one-time initialization function must have its own unique #GOnce
 * struct.
 *
 * Since: 2.4
 **/

/**
 * G_ONCE_INIT:
 *
 * A #GOnce must be initialized with this macro before it can be used.
 *
 * <informalexample>
 *  <programlisting>
 *   GOnce my_once = G_ONCE_INIT;
 *  </programlisting>
 * </informalexample>
 *
 * Since: 2.4
 **/

/**
 * GOnceStatus:
 * @G_ONCE_STATUS_NOTCALLED: the function has not been called yet.
 * @G_ONCE_STATUS_PROGRESS: the function call is currently in progress.
 * @G_ONCE_STATUS_READY: the function has been called.
 *
 * The possible statuses of a one-time initialization function
 * controlled by a #GOnce struct.
 *
 * Since: 2.4
 **/

/**
 * g_once:
 * @once: a #GOnce structure
 * @func: the #GThreadFunc function associated to @once. This function
 *        is called only once, regardless of the number of times it and
 *        its associated #GOnce struct are passed to g_once().
 * @arg: data to be passed to @func
 *
 * The first call to this routine by a process with a given #GOnce
 * struct calls @func with the given argument. Thereafter, subsequent
 * calls to g_once()  with the same #GOnce struct do not call @func
 * again, but return the stored result of the first call. On return
 * from g_once(), the status of @once will be %G_ONCE_STATUS_READY.
 *
 * For example, a mutex or a thread-specific data key must be created
 * exactly once. In a threaded environment, calling g_once() ensures
 * that the initialization is serialized across multiple threads.
 *
 * <note><para>Calling g_once() recursively on the same #GOnce struct in
 * @func will lead to a deadlock.</para></note>
 *
 * <informalexample>
 *  <programlisting>
 *   gpointer
 *   get_debug_flags (void)
 *   {
 *     static GOnce my_once = G_ONCE_INIT;
 *
 *     g_once (&my_once, parse_debug_flags, NULL);
 *
 *     return my_once.retval;
 *   }
 *  </programlisting>
 * </informalexample>
 *
 * Since: 2.4
 **/
gpointer
g_once_impl (GOnce       *once,
	     GThreadFunc  func,
	     gpointer     arg)
{
  g_mutex_lock (g_once_mutex);

  while (once->status == G_ONCE_STATUS_PROGRESS)
    g_cond_wait (g_once_cond, g_once_mutex);

  if (once->status != G_ONCE_STATUS_READY)
    {
      once->status = G_ONCE_STATUS_PROGRESS;
      g_mutex_unlock (g_once_mutex);

      once->retval = func (arg);

      g_mutex_lock (g_once_mutex);
      once->status = G_ONCE_STATUS_READY;
      g_cond_broadcast (g_once_cond);
    }

  g_mutex_unlock (g_once_mutex);

  return once->retval;
}

/**
 * g_once_init_enter:
 * @value_location: location of a static initializable variable
 *                  containing 0.
 * @Returns: %TRUE if the initialization section should be entered,
 *           %FALSE and blocks otherwise
 *
 * Function to be called when starting a critical initialization
 * section. The argument @value_location must point to a static
 * 0-initialized variable that will be set to a value other than 0 at
 * the end of the initialization section. In combination with
 * g_once_init_leave() and the unique address @value_location, it can
 * be ensured that an initialization section will be executed only once
 * during a program's life time, and that concurrent threads are
 * blocked until initialization completed. To be used in constructs
 * like this:
 *
 * <informalexample>
 *  <programlisting>
 *   static gsize initialization_value = 0;
 *
 *   if (g_once_init_enter (&amp;initialization_value))
 *     {
 *       gsize setup_value = 42; /<!-- -->* initialization code here *<!-- -->/
 *
 *       g_once_init_leave (&amp;initialization_value, setup_value);
 *     }
 *
 *   /<!-- -->* use initialization_value here *<!-- -->/
 *  </programlisting>
 * </informalexample>
 *
 * Since: 2.14
 **/
gboolean
g_once_init_enter_impl (volatile gsize *value_location)
{
  gboolean need_init = FALSE;
  g_mutex_lock (g_once_mutex);
  if (g_atomic_pointer_get (value_location) == NULL)
    {
      if (!g_slist_find (g_once_init_list, (void*) value_location))
        {
          need_init = TRUE;
          g_once_init_list = g_slist_prepend (g_once_init_list, (void*) value_location);
        }
      else
        do
          g_cond_wait (g_once_cond, g_once_mutex);
        while (g_slist_find (g_once_init_list, (void*) value_location));
    }
  g_mutex_unlock (g_once_mutex);
  return need_init;
}

/**
 * g_once_init_leave:
 * @value_location: location of a static initializable variable
 *                  containing 0.
 * @initialization_value: new non-0 value for *@value_location.
 *
 * Counterpart to g_once_init_enter(). Expects a location of a static
 * 0-initialized initialization variable, and an initialization value
 * other than 0. Sets the variable to the initialization value, and
 * releases concurrent threads blocking in g_once_init_enter() on this
 * initialization variable.
 *
 * Since: 2.14
 **/
void
g_once_init_leave (volatile gsize *value_location,
                   gsize           initialization_value)
{
  g_return_if_fail (g_atomic_pointer_get (value_location) == NULL);
  g_return_if_fail (initialization_value != 0);
  g_return_if_fail (g_once_init_list != NULL);

  g_atomic_pointer_set ((void**)value_location, (void*) initialization_value);
  g_mutex_lock (g_once_mutex);
  g_once_init_list = g_slist_remove (g_once_init_list, (void*) value_location);
  g_cond_broadcast (g_once_cond);
  g_mutex_unlock (g_once_mutex);
}

/* GStaticMutex {{{1 ------------------------------------------------------ */

/**
 * GStaticMutex:
 *
 * A #GStaticMutex works like a #GMutex, but it has one significant
 * advantage. It doesn't need to be created at run-time like a #GMutex,
 * but can be defined at compile-time. Here is a shorter, easier and
 * safer version of our <function>give_me_next_number()</function>
 * example:
 *
 * <example>
 *  <title>
 *   Using <structname>GStaticMutex</structname>
 *   to simplify thread-safe programming
 *  </title>
 *  <programlisting>
 *   int
 *   give_me_next_number (void)
 *   {
 *     static int current_number = 0;
 *     int ret_val;
 *     static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
 *
 *     g_static_mutex_lock (&amp;mutex);
 *     ret_val = current_number = calc_next_number (current_number);
 *     g_static_mutex_unlock (&amp;mutex);
 *
 *     return ret_val;
 *   }
 *  </programlisting>
 * </example>
 *
 * Sometimes you would like to dynamically create a mutex. If you don't
 * want to require prior calling to g_thread_init(), because your code
 * should also be usable in non-threaded programs, you are not able to
 * use g_mutex_new() and thus #GMutex, as that requires a prior call to
 * g_thread_init(). In theses cases you can also use a #GStaticMutex.
 * It must be initialized with g_static_mutex_init() before using it
 * and freed with with g_static_mutex_free() when not needed anymore to
 * free up any allocated resources.
 *
 * Even though #GStaticMutex is not opaque, it should only be used with
 * the following functions, as it is defined differently on different
 * platforms.
 *
 * All of the <function>g_static_mutex_*</function> functions apart
 * from <function>g_static_mutex_get_mutex</function> can also be used
 * even if g_thread_init() has not yet been called. Then they do
 * nothing, apart from <function>g_static_mutex_trylock</function>,
 * which does nothing but returning %TRUE.
 *
 * <note><para>All of the <function>g_static_mutex_*</function>
 * functions are actually macros. Apart from taking their addresses, you
 * can however use them as if they were functions.</para></note>
 **/

/**
 * G_STATIC_MUTEX_INIT:
 *
 * A #GStaticMutex must be initialized with this macro, before it can
 * be used. This macro can used be to initialize a variable, but it
 * cannot be assigned to a variable. In that case you have to use
 * g_static_mutex_init().
 *
 * <informalexample>
 *  <programlisting>
 *   GStaticMutex my_mutex = G_STATIC_MUTEX_INIT;
 *  </programlisting>
 * </informalexample>
 **/

/**
 * g_static_mutex_init:
 * @mutex: a #GStaticMutex to be initialized.
 *
 * Initializes @mutex. Alternatively you can initialize it with
 * #G_STATIC_MUTEX_INIT.
 **/
void
g_static_mutex_init (GStaticMutex *mutex)
{
  static const GStaticMutex init_mutex = G_STATIC_MUTEX_INIT;

  g_return_if_fail (mutex);

  *mutex = init_mutex;
}

/* IMPLEMENTATION NOTE:
 *
 * On some platforms a GStaticMutex is actually a normal GMutex stored
 * inside of a structure instead of being allocated dynamically.  We can
 * only do this for platforms on which we know, in advance, how to
 * allocate (size) and initialise (value) that memory.
 *
 * On other platforms, a GStaticMutex is nothing more than a pointer to
 * a GMutex.  In that case, the first access we make to the static mutex
 * must first allocate the normal GMutex and store it into the pointer.
 *
 * configure.ac writes macros into glibconfig.h to determine if
 * g_static_mutex_get_mutex() accesses the sturcture in memory directly
 * (on platforms where we are able to do that) or if it ends up here,
 * where we may have to allocate the GMutex before returning it.
 */

/**
 * g_static_mutex_get_mutex:
 * @mutex: a #GStaticMutex.
 * @Returns: the #GMutex corresponding to @mutex.
 *
 * For some operations (like g_cond_wait()) you must have a #GMutex
 * instead of a #GStaticMutex. This function will return the
 * corresponding #GMutex for @mutex.
 **/
GMutex *
g_static_mutex_get_mutex_impl (GMutex** mutex)
{
  if (!g_thread_supported ())
    return NULL;

  g_assert (g_once_mutex);

  g_mutex_lock (g_once_mutex);

  if (!(*mutex))
    g_atomic_pointer_set (mutex, g_mutex_new());

  g_mutex_unlock (g_once_mutex);

  return *mutex;
}

/* IMPLEMENTATION NOTE:
 *
 * g_static_mutex_lock(), g_static_mutex_trylock() and
 * g_static_mutex_unlock() are all preprocessor macros that wrap the
 * corresponding g_mutex_*() function around a call to
 * g_static_mutex_get_mutex().
 */

/**
 * g_static_mutex_lock:
 * @mutex: a #GStaticMutex.
 *
 * Works like g_mutex_lock(), but for a #GStaticMutex.
 **/

/**
 * g_static_mutex_trylock:
 * @mutex: a #GStaticMutex.
 * @Returns: %TRUE, if the #GStaticMutex could be locked.
 *
 * Works like g_mutex_trylock(), but for a #GStaticMutex.
 **/

/**
 * g_static_mutex_unlock:
 * @mutex: a #GStaticMutex.
 *
 * Works like g_mutex_unlock(), but for a #GStaticMutex.
 **/

/**
 * g_static_mutex_free:
 * @mutex: a #GStaticMutex to be freed.
 *
 * Releases all resources allocated to @mutex.
 *
 * You don't have to call this functions for a #GStaticMutex with an
 * unbounded lifetime, i.e. objects declared 'static', but if you have
 * a #GStaticMutex as a member of a structure and the structure is
 * freed, you should also free the #GStaticMutex.
 *
 * <note><para>Calling g_static_mutex_free() on a locked mutex may
 * result in undefined behaviour.</para></note>
 **/
void
g_static_mutex_free (GStaticMutex* mutex)
{
  GMutex **runtime_mutex;

  g_return_if_fail (mutex);

  /* The runtime_mutex is the first (or only) member of GStaticMutex,
   * see both versions (of glibconfig.h) in configure.ac. Note, that
   * this variable is NULL, if g_thread_init() hasn't been called or
   * if we're using the default thread implementation and it provides
   * static mutexes. */
  runtime_mutex = ((GMutex**)mutex);

  if (*runtime_mutex)
    g_mutex_free (*runtime_mutex);

  *runtime_mutex = NULL;
}

/* ------------------------------------------------------------------------ */

/**
 * GStaticRecMutex:
 *
 * A #GStaticRecMutex works like a #GStaticMutex, but it can be locked
 * multiple times by one thread. If you enter it n times, you have to
 * unlock it n times again to let other threads lock it. An exception
 * is the function g_static_rec_mutex_unlock_full(): that allows you to
 * unlock a #GStaticRecMutex completely returning the depth, (i.e. the
 * number of times this mutex was locked). The depth can later be used
 * to restore the state of the #GStaticRecMutex by calling
 * g_static_rec_mutex_lock_full().
 *
 * Even though #GStaticRecMutex is not opaque, it should only be used
 * with the following functions.
 *
 * All of the <function>g_static_rec_mutex_*</function> functions can
 * be used even if g_thread_init() has not been called. Then they do
 * nothing, apart from <function>g_static_rec_mutex_trylock</function>,
 * which does nothing but returning %TRUE.
 **/

/**
 * G_STATIC_REC_MUTEX_INIT:
 *
 * A #GStaticRecMutex must be initialized with this macro before it can
 * be used. This macro can used be to initialize a variable, but it
 * cannot be assigned to a variable. In that case you have to use
 * g_static_rec_mutex_init().
 *
 * <informalexample>
 *  <programlisting>
 *   GStaticRecMutex my_mutex = G_STATIC_REC_MUTEX_INIT;
 * </programlisting>
 </informalexample>
 **/

/**
 * g_static_rec_mutex_init:
 * @mutex: a #GStaticRecMutex to be initialized.
 *
 * A #GStaticRecMutex must be initialized with this function before it
 * can be used. Alternatively you can initialize it with
 * #G_STATIC_REC_MUTEX_INIT.
 **/
void
g_static_rec_mutex_init (GStaticRecMutex *mutex)
{
  static const GStaticRecMutex init_mutex = G_STATIC_REC_MUTEX_INIT;

  g_return_if_fail (mutex);

  *mutex = init_mutex;
}

/**
 * g_static_rec_mutex_lock:
 * @mutex: a #GStaticRecMutex to lock.
 *
 * Locks @mutex. If @mutex is already locked by another thread, the
 * current thread will block until @mutex is unlocked by the other
 * thread. If @mutex is already locked by the calling thread, this
 * functions increases the depth of @mutex and returns immediately.
 **/
void
g_static_rec_mutex_lock (GStaticRecMutex* mutex)
{
  GSystemThread self;

  g_return_if_fail (mutex);

  if (!g_thread_supported ())
    return;

  G_THREAD_UF (thread_self, (&self));

  if (g_system_thread_equal (self, mutex->owner))
    {
      mutex->depth++;
      return;
    }
  g_static_mutex_lock (&mutex->mutex);
  g_system_thread_assign (mutex->owner, self);
  mutex->depth = 1;
}

/**
 * g_static_rec_mutex_trylock:
 * @mutex: a #GStaticRecMutex to lock.
 * @Returns: %TRUE, if @mutex could be locked.
 *
 * Tries to lock @mutex. If @mutex is already locked by another thread,
 * it immediately returns %FALSE. Otherwise it locks @mutex and returns
 * %TRUE. If @mutex is already locked by the calling thread, this
 * functions increases the depth of @mutex and immediately returns
 * %TRUE.
 **/
gboolean
g_static_rec_mutex_trylock (GStaticRecMutex* mutex)
{
  GSystemThread self;

  g_return_val_if_fail (mutex, FALSE);

  if (!g_thread_supported ())
    return TRUE;

  G_THREAD_UF (thread_self, (&self));

  if (g_system_thread_equal (self, mutex->owner))
    {
      mutex->depth++;
      return TRUE;
    }

  if (!g_static_mutex_trylock (&mutex->mutex))
    return FALSE;

  g_system_thread_assign (mutex->owner, self);
  mutex->depth = 1;
  return TRUE;
}

/**
 * g_static_rec_mutex_unlock:
 * @mutex: a #GStaticRecMutex to unlock.
 *
 * Unlocks @mutex. Another thread will be allowed to lock @mutex only
 * when it has been unlocked as many times as it had been locked
 * before. If @mutex is completely unlocked and another thread is
 * blocked in a g_static_rec_mutex_lock() call for @mutex, it will be
 * woken and can lock @mutex itself.
 **/
void
g_static_rec_mutex_unlock (GStaticRecMutex* mutex)
{
  g_return_if_fail (mutex);

  if (!g_thread_supported ())
    return;

  if (mutex->depth > 1)
    {
      mutex->depth--;
      return;
    }
  g_system_thread_assign (mutex->owner, zero_thread);
  g_static_mutex_unlock (&mutex->mutex);
}

/**
 * g_static_rec_mutex_lock_full:
 * @mutex: a #GStaticRecMutex to lock.
 * @depth: number of times this mutex has to be unlocked to be
 *         completely unlocked.
 *
 * Works like calling g_static_rec_mutex_lock() for @mutex @depth times.
 **/
void
g_static_rec_mutex_lock_full   (GStaticRecMutex *mutex,
				guint            depth)
{
  GSystemThread self;
  g_return_if_fail (mutex);

  if (!g_thread_supported ())
    return;

  if (depth == 0)
    return;

  G_THREAD_UF (thread_self, (&self));

  if (g_system_thread_equal (self, mutex->owner))
    {
      mutex->depth += depth;
      return;
    }
  g_static_mutex_lock (&mutex->mutex);
  g_system_thread_assign (mutex->owner, self);
  mutex->depth = depth;
}

/**
 * g_static_rec_mutex_unlock_full:
 * @mutex: a #GStaticRecMutex to completely unlock.
 * @Returns: number of times @mutex has been locked by the current
 *           thread.
 *
 * Completely unlocks @mutex. If another thread is blocked in a
 * g_static_rec_mutex_lock() call for @mutex, it will be woken and can
 * lock @mutex itself. This function returns the number of times that
 * @mutex has been locked by the current thread. To restore the state
 * before the call to g_static_rec_mutex_unlock_full() you can call
 * g_static_rec_mutex_lock_full() with the depth returned by this
 * function.
 **/
guint
g_static_rec_mutex_unlock_full (GStaticRecMutex *mutex)
{
  guint depth;

  g_return_val_if_fail (mutex, 0);

  if (!g_thread_supported ())
    return 1;

  depth = mutex->depth;

  g_system_thread_assign (mutex->owner, zero_thread);
  mutex->depth = 0;
  g_static_mutex_unlock (&mutex->mutex);

  return depth;
}

/**
 * g_static_rec_mutex_free:
 * @mutex: a #GStaticRecMutex to be freed.
 *
 * Releases all resources allocated to a #GStaticRecMutex.
 *
 * You don't have to call this functions for a #GStaticRecMutex with an
 * unbounded lifetime, i.e. objects declared 'static', but if you have
 * a #GStaticRecMutex as a member of a structure and the structure is
 * freed, you should also free the #GStaticRecMutex.
 **/
void
g_static_rec_mutex_free (GStaticRecMutex *mutex)
{
  g_return_if_fail (mutex);

  g_static_mutex_free (&mutex->mutex);
}

/* GStaticPrivate {{{1 ---------------------------------------------------- */

/**
 * GStaticPrivate:
 *
 * A #GStaticPrivate works almost like a #GPrivate, but it has one
 * significant advantage. It doesn't need to be created at run-time
 * like a #GPrivate, but can be defined at compile-time. This is
 * similar to the difference between #GMutex and #GStaticMutex. Now
 * look at our <function>give_me_next_number()</function> example with
 * #GStaticPrivate:
 *
 * <example>
 *  <title>Using GStaticPrivate for per-thread data</title>
 *  <programlisting>
 *   int
 *   give_me_next_number (<!-- -->)
 *   {
 *     static GStaticPrivate current_number_key = G_STATIC_PRIVATE_INIT;
 *     int *current_number = g_static_private_get (&amp;current_number_key);
 *
 *     if (!current_number)
 *       {
 *         current_number = g_new (int,1);
 *         *current_number = 0;
 *         g_static_private_set (&amp;current_number_key, current_number, g_free);
 *       }
 *
 *     *current_number = calc_next_number (*current_number);
 *
 *     return *current_number;
 *   }
 *  </programlisting>
 * </example>
 **/

/**
 * G_STATIC_PRIVATE_INIT:
 *
 * Every #GStaticPrivate must be initialized with this macro, before it
 * can be used.
 *
 * <informalexample>
 *  <programlisting>
 *   GStaticPrivate my_private = G_STATIC_PRIVATE_INIT;
 *  </programlisting>
 * </informalexample>
 **/

/**
 * g_static_private_init:
 * @private_key: a #GStaticPrivate to be initialized.
 *
 * Initializes @private_key. Alternatively you can initialize it with
 * #G_STATIC_PRIVATE_INIT.
 **/
void
g_static_private_init (GStaticPrivate *private_key)
{
  private_key->index = 0;
}

/**
 * g_static_private_get:
 * @private_key: a #GStaticPrivate.
 * @Returns: the corresponding pointer.
 *
 * Works like g_private_get() only for a #GStaticPrivate.
 *
 * This function works even if g_thread_init() has not yet been called.
 **/
gpointer
g_static_private_get (GStaticPrivate *private_key)
{
  GRealThread *self = (GRealThread*) g_thread_self ();
  GArray *array;

  array = self->private_data;
  if (!array)
    return NULL;

  if (!private_key->index)
    return NULL;
  else if (private_key->index <= array->len)
    return g_array_index (array, GStaticPrivateNode,
			  private_key->index - 1).data;
  else
    return NULL;
}

/**
 * g_static_private_set:
 * @private_key: a #GStaticPrivate.
 * @data: the new pointer.
 * @notify: a function to be called with the pointer whenever the
 *          current thread ends or sets this pointer again.
 *
 * Sets the pointer keyed to @private_key for the current thread and
 * the function @notify to be called with that pointer (%NULL or
 * non-%NULL), whenever the pointer is set again or whenever the
 * current thread ends.
 *
 * This function works even if g_thread_init() has not yet been called.
 * If g_thread_init() is called later, the @data keyed to @private_key
 * will be inherited only by the main thread, i.e. the one that called
 * g_thread_init().
 *
 * <note><para>@notify is used quite differently from @destructor in
 * g_private_new().</para></note>
 **/
void
g_static_private_set (GStaticPrivate *private_key,
		      gpointer        data,
		      GDestroyNotify  notify)
{
  GRealThread *self = (GRealThread*) g_thread_self ();
  GArray *array;
  static guint next_index = 0;
  GStaticPrivateNode *node;

  array = self->private_data;
  if (!array)
    {
      array = g_array_new (FALSE, TRUE, sizeof (GStaticPrivateNode));
      self->private_data = array;
    }

  if (!private_key->index)
    {
      G_LOCK (g_thread);

      if (!private_key->index)
	{
	  if (g_thread_free_indeces)
	    {
	      private_key->index =
		GPOINTER_TO_UINT (g_thread_free_indeces->data);
	      g_thread_free_indeces =
		g_slist_delete_link (g_thread_free_indeces,
				     g_thread_free_indeces);
	    }
	  else
	    private_key->index = ++next_index;
	}

      G_UNLOCK (g_thread);
    }

  if (private_key->index > array->len)
    g_array_set_size (array, private_key->index);

  node = &g_array_index (array, GStaticPrivateNode, private_key->index - 1);
  if (node->destroy)
    {
      gpointer ddata = node->data;
      GDestroyNotify ddestroy = node->destroy;

      node->data = data;
      node->destroy = notify;

      ddestroy (ddata);
    }
  else
    {
      node->data = data;
      node->destroy = notify;
    }
}

/**
 * g_static_private_free:
 * @private_key: a #GStaticPrivate to be freed.
 *
 * Releases all resources allocated to @private_key.
 *
 * You don't have to call this functions for a #GStaticPrivate with an
 * unbounded lifetime, i.e. objects declared 'static', but if you have
 * a #GStaticPrivate as a member of a structure and the structure is
 * freed, you should also free the #GStaticPrivate.
 **/
void
g_static_private_free (GStaticPrivate *private_key)
{
  guint idx = private_key->index;
  GRealThread *thread;

  if (!idx)
    return;

  private_key->index = 0;

  G_LOCK (g_thread);

  thread = g_thread_all_threads;
  while (thread)
    {
      GArray *array = thread->private_data;
      thread = thread->next;

      if (array && idx <= array->len)
	{
	  GStaticPrivateNode *node = &g_array_index (array,
						     GStaticPrivateNode,
						     idx - 1);
	  gpointer ddata = node->data;
	  GDestroyNotify ddestroy = node->destroy;

	  node->data = NULL;
	  node->destroy = NULL;

          if (ddestroy)
            {
              G_UNLOCK (g_thread);
              ddestroy (ddata);
              G_LOCK (g_thread);
            }
	}
    }
  g_thread_free_indeces = g_slist_prepend (g_thread_free_indeces,
					   GUINT_TO_POINTER (idx));
  G_UNLOCK (g_thread);
}

/* GThread Extra Functions {{{1 ------------------------------------------- */
static void
g_thread_cleanup (gpointer data)
{
  if (data)
    {
      GRealThread* thread = data;
      if (thread->private_data)
	{
	  GArray* array = thread->private_data;
	  guint i;

	  for (i = 0; i < array->len; i++ )
	    {
	      GStaticPrivateNode *node =
		&g_array_index (array, GStaticPrivateNode, i);
	      if (node->destroy)
		node->destroy (node->data);
	    }
	  g_array_free (array, TRUE);
	}

      /* We only free the thread structure, if it isn't joinable. If
         it is, the structure is freed in g_thread_join */
      if (!thread->thread.joinable)
	{
	  GRealThread *t, *p;

	  G_LOCK (g_thread);
	  for (t = g_thread_all_threads, p = NULL; t; p = t, t = t->next)
	    {
	      if (t == thread)
		{
		  if (p)
		    p->next = t->next;
		  else
		    g_thread_all_threads = t->next;
		  break;
		}
	    }
	  G_UNLOCK (g_thread);

	  /* Just to make sure, this isn't used any more */
	  g_system_thread_assign (thread->system_thread, zero_thread);
          g_free (thread);
	}
    }
}

static void
g_thread_fail (void)
{
  g_error ("The thread system is not yet initialized.");
}

#define G_NSEC_PER_SEC 1000000000

static guint64
gettime (void)
{
#ifdef G_OS_WIN32
  guint64 v;

  /* Returns 100s of nanoseconds since start of 1601 */
  GetSystemTimeAsFileTime ((FILETIME *)&v);

  /* Offset to Unix epoch */
  v -= G_GINT64_CONSTANT (116444736000000000);
  /* Convert to nanoseconds */
  v *= 100;

  return v;
#else
  struct timeval tv;

  gettimeofday (&tv, NULL);

  return (guint64) tv.tv_sec * G_NSEC_PER_SEC + tv.tv_usec * (G_NSEC_PER_SEC / G_USEC_PER_SEC); 
#endif
}

static gpointer
g_thread_create_proxy (gpointer data)
{
  GRealThread* thread = data;

  g_assert (data);

  /* This has to happen before G_LOCK, as that might call g_thread_self */
  g_private_set (g_thread_specific_private, data);

  /* the lock makes sure, that thread->system_thread is written,
     before thread->thread.func is called. See g_thread_create. */
  G_LOCK (g_thread);
  G_UNLOCK (g_thread);

  thread->retval = thread->thread.func (thread->thread.data);

  return NULL;
}

/**
 * g_thread_create_full:
 * @func: a function to execute in the new thread.
 * @data: an argument to supply to the new thread.
 * @stack_size: a stack size for the new thread.
 * @joinable: should this thread be joinable?
 * @bound: should this thread be bound to a system thread?
 * @priority: a priority for the thread.
 * @error: return location for error.
 * @Returns: the new #GThread on success.
 *
 * This function creates a new thread with the priority @priority. If
 * the underlying thread implementation supports it, the thread gets a
 * stack size of @stack_size or the default value for the current
 * platform, if @stack_size is 0.
 *
 * If @joinable is %TRUE, you can wait for this threads termination
 * calling g_thread_join(). Otherwise the thread will just disappear
 * when it terminates. If @bound is %TRUE, this thread will be
 * scheduled in the system scope, otherwise the implementation is free
 * to do scheduling in the process scope. The first variant is more
 * expensive resource-wise, but generally faster. On some systems (e.g.
 * Linux) all threads are bound.
 *
 * The new thread executes the function @func with the argument @data.
 * If the thread was created successfully, it is returned.
 *
 * @error can be %NULL to ignore errors, or non-%NULL to report errors.
 * The error is set, if and only if the function returns %NULL.
 *
 * <note><para>It is not guaranteed that threads with different priorities
 * really behave accordingly. On some systems (e.g. Linux) there are no
 * thread priorities. On other systems (e.g. Solaris) there doesn't
 * seem to be different scheduling for different priorities. All in all
 * try to avoid being dependent on priorities. Use
 * %G_THREAD_PRIORITY_NORMAL here as a default.</para></note>
 *
 * <note><para>Only use g_thread_create_full() if you really can't use
 * g_thread_create() instead. g_thread_create() does not take
 * @stack_size, @bound, and @priority as arguments, as they should only
 * be used in cases in which it is unavoidable.</para></note>
 **/
GThread*
g_thread_create_full (GThreadFunc       func,
		      gpointer          data,
		      gulong            stack_size,
		      gboolean          joinable,
		      gboolean 	        bound,
		      GThreadPriority   priority,
		      GError          **error)
{
  GRealThread* result;
  GError *local_error = NULL;
  g_return_val_if_fail (func, NULL);
  g_return_val_if_fail (priority >= G_THREAD_PRIORITY_LOW, NULL);
  g_return_val_if_fail (priority <= G_THREAD_PRIORITY_URGENT, NULL);

  result = g_new0 (GRealThread, 1);

  result->thread.joinable = joinable;
  result->thread.priority = priority;
  result->thread.func = func;
  result->thread.data = data;
  result->private_data = NULL;
  G_LOCK (g_thread);
  G_THREAD_UF (thread_create, (g_thread_create_proxy, result,
			       stack_size, joinable, bound, priority,
			       &result->system_thread, &local_error));
  if (!local_error)
    {
      result->next = g_thread_all_threads;
      g_thread_all_threads = result;
    }
  G_UNLOCK (g_thread);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      g_free (result);
      return NULL;
    }

  return (GThread*) result;
}

/**
 * g_thread_exit:
 * @retval: the return value of this thread.
 *
 * Exits the current thread. If another thread is waiting for that
 * thread using g_thread_join() and the current thread is joinable, the
 * waiting thread will be woken up and get @retval as the return value
 * of g_thread_join(). If the current thread is not joinable, @retval
 * is ignored. Calling
 *
 * <informalexample>
 *  <programlisting>
 *   g_thread_exit (retval);
 *  </programlisting>
 * </informalexample>
 *
 * is equivalent to returning @retval from the function @func, as given
 * to g_thread_create().
 *
 * <note><para>Never call g_thread_exit() from within a thread of a
 * #GThreadPool, as that will mess up the bookkeeping and lead to funny
 * and unwanted results.</para></note>
 **/
void
g_thread_exit (gpointer retval)
{
  GRealThread* real = (GRealThread*) g_thread_self ();
  real->retval = retval;
  G_THREAD_CF (thread_exit, (void)0, ());
}

/**
 * g_thread_join:
 * @thread: a #GThread to be waited for.
 * @Returns: the return value of the thread.
 *
 * Waits until @thread finishes, i.e. the function @func, as given to
 * g_thread_create(), returns or g_thread_exit() is called by @thread.
 * All resources of @thread including the #GThread struct are released.
 * @thread must have been created with @joinable=%TRUE in
 * g_thread_create(). The value returned by @func or given to
 * g_thread_exit() by @thread is returned by this function.
 **/
gpointer
g_thread_join (GThread* thread)
{
  GRealThread* real = (GRealThread*) thread;
  GRealThread *p, *t;
  gpointer retval;

  g_return_val_if_fail (thread, NULL);
  g_return_val_if_fail (thread->joinable, NULL);
  g_return_val_if_fail (!g_system_thread_equal (real->system_thread,
						zero_thread), NULL);

  G_THREAD_UF (thread_join, (&real->system_thread));

  retval = real->retval;

  G_LOCK (g_thread);
  for (t = g_thread_all_threads, p = NULL; t; p = t, t = t->next)
    {
      if (t == (GRealThread*) thread)
	{
	  if (p)
	    p->next = t->next;
	  else
	    g_thread_all_threads = t->next;
	  break;
	}
    }
  G_UNLOCK (g_thread);

  /* Just to make sure, this isn't used any more */
  thread->joinable = 0;
  g_system_thread_assign (real->system_thread, zero_thread);

  /* the thread structure for non-joinable threads is freed upon
     thread end. We free the memory here. This will leave a loose end,
     if a joinable thread is not joined. */

  g_free (thread);

  return retval;
}

/**
 * g_thread_set_priority:
 * @thread: a #GThread.
 * @priority: a new priority for @thread.
 *
 * Changes the priority of @thread to @priority.
 *
 * <note><para>It is not guaranteed that threads with different
 * priorities really behave accordingly. On some systems (e.g. Linux)
 * there are no thread priorities. On other systems (e.g. Solaris) there
 * doesn't seem to be different scheduling for different priorities. All
 * in all try to avoid being dependent on priorities.</para></note>
 **/
void
g_thread_set_priority (GThread* thread,
		       GThreadPriority priority)
{
  GRealThread* real = (GRealThread*) thread;

  g_return_if_fail (thread);
  g_return_if_fail (!g_system_thread_equal (real->system_thread, zero_thread));
  g_return_if_fail (priority >= G_THREAD_PRIORITY_LOW);
  g_return_if_fail (priority <= G_THREAD_PRIORITY_URGENT);

  thread->priority = priority;

  G_THREAD_CF (thread_set_priority, (void)0,
	       (&real->system_thread, priority));
}

/**
 * g_thread_self:
 * @Returns: the current thread.
 *
 * This functions returns the #GThread corresponding to the calling
 * thread.
 **/
GThread*
g_thread_self (void)
{
  GRealThread* thread = g_private_get (g_thread_specific_private);

  if (!thread)
    {
      /* If no thread data is available, provide and set one.  This
         can happen for the main thread and for threads, that are not
         created by GLib. */
      thread = g_new0 (GRealThread, 1);
      thread->thread.joinable = FALSE; /* This is a save guess */
      thread->thread.priority = G_THREAD_PRIORITY_NORMAL; /* This is
							     just a guess */
      thread->thread.func = NULL;
      thread->thread.data = NULL;
      thread->private_data = NULL;

      if (g_thread_supported ())
	G_THREAD_UF (thread_self, (&thread->system_thread));

      g_private_set (g_thread_specific_private, thread);

      G_LOCK (g_thread);
      thread->next = g_thread_all_threads;
      g_thread_all_threads = thread;
      G_UNLOCK (g_thread);
    }

  return (GThread*)thread;
}

/* GStaticRWLock {{{1 ----------------------------------------------------- */

/**
 * GStaticRWLock:
 *
 * The #GStaticRWLock struct represents a read-write lock. A read-write
 * lock can be used for protecting data that some portions of code only
 * read from, while others also write. In such situations it is
 * desirable that several readers can read at once, whereas of course
 * only one writer may write at a time. Take a look at the following
 * example:
 *
 * <example>
 *  <title>An array with access functions</title>
 *  <programlisting>
 *   GStaticRWLock rwlock = G_STATIC_RW_LOCK_INIT;
 *   GPtrArray *array;
 *
 *   gpointer
 *   my_array_get (guint index)
 *   {
 *     gpointer retval = NULL;
 *
 *     if (!array)
 *       return NULL;
 *
 *     g_static_rw_lock_reader_lock (&amp;rwlock);
 *     if (index &lt; array->len)
 *       retval = g_ptr_array_index (array, index);
 *     g_static_rw_lock_reader_unlock (&amp;rwlock);
 *
 *     return retval;
 *   }
 *
 *   void
 *   my_array_set (guint index, gpointer data)
 *   {
 *     g_static_rw_lock_writer_lock (&amp;rwlock);
 *
 *     if (!array)
 *       array = g_ptr_array_new (<!-- -->);
 *
 *     if (index >= array->len)
 *       g_ptr_array_set_size (array, index+1);
 *     g_ptr_array_index (array, index) = data;
 *
 *     g_static_rw_lock_writer_unlock (&amp;rwlock);
 *   }
 *  </programlisting>
 * </example>
 *
 * This example shows an array which can be accessed by many readers
 * (the <function>my_array_get()</function> function) simultaneously,
 * whereas the writers (the <function>my_array_set()</function>
 * function) will only be allowed once at a time and only if no readers
 * currently access the array. This is because of the potentially
 * dangerous resizing of the array. Using these functions is fully
 * multi-thread safe now.
 *
 * Most of the time, writers should have precedence over readers. That
 * means, for this implementation, that as soon as a writer wants to
 * lock the data, no other reader is allowed to lock the data, whereas,
 * of course, the readers that already have locked the data are allowed
 * to finish their operation. As soon as the last reader unlocks the
 * data, the writer will lock it.
 *
 * Even though #GStaticRWLock is not opaque, it should only be used
 * with the following functions.
 *
 * All of the <function>g_static_rw_lock_*</function> functions can be
 * used even if g_thread_init() has not been called. Then they do
 * nothing, apart from <function>g_static_rw_lock_*_trylock</function>,
 * which does nothing but returning %TRUE.
 *
 * <note><para>A read-write lock has a higher overhead than a mutex. For
 * example, both g_static_rw_lock_reader_lock() and
 * g_static_rw_lock_reader_unlock() have to lock and unlock a
 * #GStaticMutex, so it takes at least twice the time to lock and unlock
 * a #GStaticRWLock that it does to lock and unlock a #GStaticMutex. So
 * only data structures that are accessed by multiple readers, and which
 * keep the lock for a considerable time justify a #GStaticRWLock. The
 * above example most probably would fare better with a
 * #GStaticMutex.</para></note>
 **/

/**
 * G_STATIC_RW_LOCK_INIT:
 *
 * A #GStaticRWLock must be initialized with this macro before it can
 * be used. This macro can used be to initialize a variable, but it
 * cannot be assigned to a variable. In that case you have to use
 * g_static_rw_lock_init().
 *
 * <informalexample>
 *  <programlisting>
 *   GStaticRWLock my_lock = G_STATIC_RW_LOCK_INIT;
 *  </programlisting>
 * </informalexample>
 **/

/**
 * g_static_rw_lock_init:
 * @lock: a #GStaticRWLock to be initialized.
 *
 * A #GStaticRWLock must be initialized with this function before it
 * can be used. Alternatively you can initialize it with
 * #G_STATIC_RW_LOCK_INIT.
 **/
void
g_static_rw_lock_init (GStaticRWLock* lock)
{
  static const GStaticRWLock init_lock = G_STATIC_RW_LOCK_INIT;

  g_return_if_fail (lock);

  *lock = init_lock;
}

inline static void
g_static_rw_lock_wait (GCond** cond, GStaticMutex* mutex)
{
  if (!*cond)
      *cond = g_cond_new ();
  g_cond_wait (*cond, g_static_mutex_get_mutex (mutex));
}

inline static void
g_static_rw_lock_signal (GStaticRWLock* lock)
{
  if (lock->want_to_write && lock->write_cond)
    g_cond_signal (lock->write_cond);
  else if (lock->want_to_read && lock->read_cond)
    g_cond_broadcast (lock->read_cond);
}

/**
 * g_static_rw_lock_reader_lock:
 * @lock: a #GStaticRWLock to lock for reading.
 *
 * Locks @lock for reading. There may be unlimited concurrent locks for
 * reading of a #GStaticRWLock at the same time.  If @lock is already
 * locked for writing by another thread or if another thread is already
 * waiting to lock @lock for writing, this function will block until
 * @lock is unlocked by the other writing thread and no other writing
 * threads want to lock @lock. This lock has to be unlocked by
 * g_static_rw_lock_reader_unlock().
 *
 * #GStaticRWLock is not recursive. It might seem to be possible to
 * recursively lock for reading, but that can result in a deadlock, due
 * to writer preference.
 **/
void
g_static_rw_lock_reader_lock (GStaticRWLock* lock)
{
  g_return_if_fail (lock);

  if (!g_threads_got_initialized)
    return;

  g_static_mutex_lock (&lock->mutex);
  lock->want_to_read++;
  while (lock->have_writer || lock->want_to_write)
    g_static_rw_lock_wait (&lock->read_cond, &lock->mutex);
  lock->want_to_read--;
  lock->read_counter++;
  g_static_mutex_unlock (&lock->mutex);
}

/**
 * g_static_rw_lock_reader_trylock:
 * @lock: a #GStaticRWLock to lock for reading.
 * @Returns: %TRUE, if @lock could be locked for reading.
 *
 * Tries to lock @lock for reading. If @lock is already locked for
 * writing by another thread or if another thread is already waiting to
 * lock @lock for writing, immediately returns %FALSE. Otherwise locks
 * @lock for reading and returns %TRUE. This lock has to be unlocked by
 * g_static_rw_lock_reader_unlock().
 **/
gboolean
g_static_rw_lock_reader_trylock (GStaticRWLock* lock)
{
  gboolean ret_val = FALSE;

  g_return_val_if_fail (lock, FALSE);

  if (!g_threads_got_initialized)
    return TRUE;

  g_static_mutex_lock (&lock->mutex);
  if (!lock->have_writer && !lock->want_to_write)
    {
      lock->read_counter++;
      ret_val = TRUE;
    }
  g_static_mutex_unlock (&lock->mutex);
  return ret_val;
}

/**
 * g_static_rw_lock_reader_unlock:
 * @lock: a #GStaticRWLock to unlock after reading.
 *
 * Unlocks @lock. If a thread waits to lock @lock for writing and all
 * locks for reading have been unlocked, the waiting thread is woken up
 * and can lock @lock for writing.
 **/
void
g_static_rw_lock_reader_unlock  (GStaticRWLock* lock)
{
  g_return_if_fail (lock);

  if (!g_threads_got_initialized)
    return;

  g_static_mutex_lock (&lock->mutex);
  lock->read_counter--;
  if (lock->read_counter == 0)
    g_static_rw_lock_signal (lock);
  g_static_mutex_unlock (&lock->mutex);
}

/**
 * g_static_rw_lock_writer_lock:
 * @lock: a #GStaticRWLock to lock for writing.
 *
 * Locks @lock for writing. If @lock is already locked for writing or
 * reading by other threads, this function will block until @lock is
 * completely unlocked and then lock @lock for writing. While this
 * functions waits to lock @lock, no other thread can lock @lock for
 * reading. When @lock is locked for writing, no other thread can lock
 * @lock (neither for reading nor writing). This lock has to be
 * unlocked by g_static_rw_lock_writer_unlock().
 **/
void
g_static_rw_lock_writer_lock (GStaticRWLock* lock)
{
  g_return_if_fail (lock);

  if (!g_threads_got_initialized)
    return;

  g_static_mutex_lock (&lock->mutex);
  lock->want_to_write++;
  while (lock->have_writer || lock->read_counter)
    g_static_rw_lock_wait (&lock->write_cond, &lock->mutex);
  lock->want_to_write--;
  lock->have_writer = TRUE;
  g_static_mutex_unlock (&lock->mutex);
}

/**
 * g_static_rw_lock_writer_trylock:
 * @lock: a #GStaticRWLock to lock for writing.
 * @Returns: %TRUE, if @lock could be locked for writing.
 *
 * Tries to lock @lock for writing. If @lock is already locked (for
 * either reading or writing) by another thread, it immediately returns
 * %FALSE. Otherwise it locks @lock for writing and returns %TRUE. This
 * lock has to be unlocked by g_static_rw_lock_writer_unlock().
 **/
gboolean
g_static_rw_lock_writer_trylock (GStaticRWLock* lock)
{
  gboolean ret_val = FALSE;

  g_return_val_if_fail (lock, FALSE);

  if (!g_threads_got_initialized)
    return TRUE;

  g_static_mutex_lock (&lock->mutex);
  if (!lock->have_writer && !lock->read_counter)
    {
      lock->have_writer = TRUE;
      ret_val = TRUE;
    }
  g_static_mutex_unlock (&lock->mutex);
  return ret_val;
}

/**
 * g_static_rw_lock_writer_unlock:
 * @lock: a #GStaticRWLock to unlock after writing.
 *
 * Unlocks @lock. If a thread is waiting to lock @lock for writing and
 * all locks for reading have been unlocked, the waiting thread is
 * woken up and can lock @lock for writing. If no thread is waiting to
 * lock @lock for writing, and some thread or threads are waiting to
 * lock @lock for reading, the waiting threads are woken up and can
 * lock @lock for reading.
 **/
void
g_static_rw_lock_writer_unlock (GStaticRWLock* lock)
{
  g_return_if_fail (lock);

  if (!g_threads_got_initialized)
    return;

  g_static_mutex_lock (&lock->mutex);
  lock->have_writer = FALSE;
  g_static_rw_lock_signal (lock);
  g_static_mutex_unlock (&lock->mutex);
}

/**
 * g_static_rw_lock_free:
 * @lock: a #GStaticRWLock to be freed.
 *
 * Releases all resources allocated to @lock.
 *
 * You don't have to call this functions for a #GStaticRWLock with an
 * unbounded lifetime, i.e. objects declared 'static', but if you have
 * a #GStaticRWLock as a member of a structure, and the structure is
 * freed, you should also free the #GStaticRWLock.
 **/
void
g_static_rw_lock_free (GStaticRWLock* lock)
{
  g_return_if_fail (lock);

  if (lock->read_cond)
    {
      g_cond_free (lock->read_cond);
      lock->read_cond = NULL;
    }
  if (lock->write_cond)
    {
      g_cond_free (lock->write_cond);
      lock->write_cond = NULL;
    }
  g_static_mutex_free (&lock->mutex);
}

/* Unsorted {{{1 ---------------------------------------------------------- */

/**
 * g_thread_foreach
 * @thread_func: function to call for all GThread structures
 * @user_data:   second argument to @thread_func
 *
 * Call @thread_func on all existing #GThread structures. Note that
 * threads may decide to exit while @thread_func is running, so
 * without intimate knowledge about the lifetime of foreign threads,
 * @thread_func shouldn't access the GThread* pointer passed in as
 * first argument. However, @thread_func will not be called for threads
 * which are known to have exited already.
 *
 * Due to thread lifetime checks, this function has an execution complexity
 * which is quadratic in the number of existing threads.
 *
 * Since: 2.10
 */
void
g_thread_foreach (GFunc    thread_func,
                  gpointer user_data)
{
  GSList *slist = NULL;
  GRealThread *thread;
  g_return_if_fail (thread_func != NULL);
  /* snapshot the list of threads for iteration */
  G_LOCK (g_thread);
  for (thread = g_thread_all_threads; thread; thread = thread->next)
    slist = g_slist_prepend (slist, thread);
  G_UNLOCK (g_thread);
  /* walk the list, skipping non-existant threads */
  while (slist)
    {
      GSList *node = slist;
      slist = node->next;
      /* check whether the current thread still exists */
      G_LOCK (g_thread);
      for (thread = g_thread_all_threads; thread; thread = thread->next)
        if (thread == node->data)
          break;
      G_UNLOCK (g_thread);
      if (thread)
        thread_func (thread, user_data);
      g_slist_free_1 (node);
    }
}

/**
 * g_thread_get_initialized
 *
 * Indicates if g_thread_init() has been called.
 *
 * Returns: %TRUE if threads have been initialized.
 *
 * Since: 2.20
 */
gboolean
g_thread_get_initialized ()
{
  return g_thread_supported ();
}
