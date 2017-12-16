/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <gioerror.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>
#endif
#include "gcancellable.h"
#include "glibintl.h"


/**
 * SECTION:gcancellable
 * @short_description: Thread-safe Operation Cancellation Stack
 * @include: gio/gio.h
 *
 * GCancellable is a thread-safe operation cancellation stack used 
 * throughout GIO to allow for cancellation of synchronous and
 * asynchronous operations.
 */

enum {
  CANCELLED,
  LAST_SIGNAL
};

struct _GCancellablePrivate
{
  guint cancelled : 1;
  guint cancelled_running : 1;
  guint cancelled_running_waiting : 1;

  guint fd_refcount;
  int cancel_pipe[2];

#ifdef G_OS_WIN32
  HANDLE event;
#endif
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GCancellable, g_cancellable, G_TYPE_OBJECT);

static GStaticPrivate current_cancellable = G_STATIC_PRIVATE_INIT;
G_LOCK_DEFINE_STATIC(cancellable);
static GCond *cancellable_cond = NULL;
  
static void
g_cancellable_close_pipe (GCancellable *cancellable)
{
  GCancellablePrivate *priv;
  
  priv = cancellable->priv;

  if (priv->cancel_pipe[0] != -1)
    {
      close (priv->cancel_pipe[0]);
      priv->cancel_pipe[0] = -1;
    }
  
  if (priv->cancel_pipe[1] != -1)
    {
      close (priv->cancel_pipe[1]);
      priv->cancel_pipe[1] = -1;
    }

#ifdef G_OS_WIN32
  if (priv->event)
    {
      CloseHandle (priv->event);
      priv->event = NULL;
    }
#endif
}

static void
g_cancellable_finalize (GObject *object)
{
  GCancellable *cancellable = G_CANCELLABLE (object);

  g_cancellable_close_pipe (cancellable);

  G_OBJECT_CLASS (g_cancellable_parent_class)->finalize (object);
}

static void
g_cancellable_class_init (GCancellableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GCancellablePrivate));

  if (cancellable_cond == NULL && g_thread_supported ())
    cancellable_cond = g_cond_new ();
  
  gobject_class->finalize = g_cancellable_finalize;

  /**
   * GCancellable::cancelled:
   * @cancellable: a #GCancellable.
   * 
   * Emitted when the operation has been cancelled.
   * 
   * Can be used by implementations of cancellable operations. If the
   * operation is cancelled from another thread, the signal will be
   * emitted in the thread that cancelled the operation, not the
   * thread that is running the operation.
   *
   * Note that disconnecting from this signal (or any signal) in a
   * multi-threaded program is prone to race conditions. For instance
   * it is possible that a signal handler may be invoked even
   * <emphasis>after</emphasis> a call to
   * g_signal_handler_disconnect() for that handler has already
   * returned.
   * 
   * There is also a problem when cancellation happen
   * right before connecting to the signal. If this happens the
   * signal will unexpectedly not be emitted, and checking before
   * connecting to the signal leaves a race condition where this is
   * still happening.
   *
   * In order to make it safe and easy to connect handlers there
   * are two helper functions: g_cancellable_connect() and
   * g_cancellable_disconnect() which protect against problems
   * like this.
   *
   * An example of how to us this:
   * |[
   *     /<!-- -->* Make sure we don't do any unnecessary work if already cancelled *<!-- -->/
   *     if (g_cancellable_set_error_if_cancelled (cancellable))
   *       return;
   *
   *     /<!-- -->* Set up all the data needed to be able to
   *      * handle cancellation of the operation *<!-- -->/
   *     my_data = my_data_new (...);
   *
   *     id = 0;
   *     if (cancellable)
   *       id = g_cancellable_connect (cancellable,
   *     			      G_CALLBACK (cancelled_handler)
   *     			      data, NULL);
   *
   *     /<!-- -->* cancellable operation here... *<!-- -->/
   *
   *     g_cancellable_disconnect (cancellable, id);
   *
   *     /<!-- -->* cancelled_handler is never called after this, it
   *      * is now safe to free the data *<!-- -->/
   *     my_data_free (my_data);  
   * ]|
   *
   * Note that the cancelled signal is emitted in the thread that
   * the user cancelled from, which may be the main thread. So, the
   * cancellable signal should not do something that can block.
   */
  signals[CANCELLED] =
    g_signal_new (I_("cancelled"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GCancellableClass, cancelled),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
}

#ifndef G_OS_WIN32
static void
set_fd_nonblocking (int fd)
{
#ifdef F_GETFL
  glong fcntl_flags;
  fcntl_flags = fcntl (fd, F_GETFL);

#ifdef O_NONBLOCK
  fcntl_flags |= O_NONBLOCK;
#else
  fcntl_flags |= O_NDELAY;
#endif

  fcntl (fd, F_SETFL, fcntl_flags);
#endif
}

static void
set_fd_close_exec (int fd)
{
  int flags;

  flags = fcntl (fd, F_GETFD, 0);
  if (flags != -1 && (flags & FD_CLOEXEC) == 0)
    {
      flags |= FD_CLOEXEC;
      fcntl (fd, F_SETFD, flags);
    }
}


static void
g_cancellable_open_pipe (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  priv = cancellable->priv;
  if (pipe (priv->cancel_pipe) == 0)
    {
      /* Make them nonblocking, just to be sure we don't block
       * on errors and stuff
       */
      set_fd_nonblocking (priv->cancel_pipe[0]);
      set_fd_nonblocking (priv->cancel_pipe[1]);
      set_fd_close_exec (priv->cancel_pipe[0]);
      set_fd_close_exec (priv->cancel_pipe[1]);
      
      if (priv->cancelled)
        {
          const char ch = 'x';
          gssize c;

          do
            c = write (priv->cancel_pipe[1], &ch, 1);
          while (c == -1 && errno == EINTR);
        }
    }
}
#endif

static void
g_cancellable_init (GCancellable *cancellable)
{
  cancellable->priv = G_TYPE_INSTANCE_GET_PRIVATE (cancellable,
					           G_TYPE_CANCELLABLE,
					           GCancellablePrivate);
  cancellable->priv->cancel_pipe[0] = -1;
  cancellable->priv->cancel_pipe[1] = -1;
}

/**
 * g_cancellable_new:
 * 
 * Creates a new #GCancellable object.
 *
 * Applications that want to start one or more operations
 * that should be cancellable should create a #GCancellable
 * and pass it to the operations.
 *
 * One #GCancellable can be used in multiple consecutive
 * operations, but not in multiple concurrent operations.
 *  
 * Returns: a #GCancellable.
 **/
GCancellable *
g_cancellable_new (void)
{
  return g_object_new (G_TYPE_CANCELLABLE, NULL);
}

/**
 * g_cancellable_push_current:
 * @cancellable: a #GCancellable object
 *
 * Pushes @cancellable onto the cancellable stack. The current
 * cancllable can then be recieved using g_cancellable_get_current().
 *
 * This is useful when implementing cancellable operations in
 * code that does not allow you to pass down the cancellable object.
 *
 * This is typically called automatically by e.g. #GFile operations,
 * so you rarely have to call this yourself.
 **/
void
g_cancellable_push_current (GCancellable *cancellable)
{
  GSList *l;

  g_return_if_fail (cancellable != NULL);

  l = g_static_private_get (&current_cancellable);
  l = g_slist_prepend (l, cancellable);
  g_static_private_set (&current_cancellable, l, NULL);
}

/**
 * g_cancellable_pop_current:
 * @cancellable: a #GCancellable object
 *
 * Pops @cancellable off the cancellable stack (verifying that @cancellable
 * is on the top of the stack).
 **/
void
g_cancellable_pop_current (GCancellable *cancellable)
{
  GSList *l;

  l = g_static_private_get (&current_cancellable);

  g_return_if_fail (l != NULL);
  g_return_if_fail (l->data == cancellable);

  l = g_slist_delete_link (l, l);
  g_static_private_set (&current_cancellable, l, NULL);
}

/**
 * g_cancellable_get_current:
 *
 * Gets the top cancellable from the stack.
 *
 * Returns: a #GCancellable from the top of the stack, or %NULL
 * if the stack is empty.
 **/
GCancellable *
g_cancellable_get_current  (void)
{
  GSList *l;

  l = g_static_private_get (&current_cancellable);
  if (l == NULL)
    return NULL;

  return G_CANCELLABLE (l->data);
}

/**
 * g_cancellable_reset:
 * @cancellable: a #GCancellable object.
 * 
 * Resets @cancellable to its uncancelled state. 
 **/
void 
g_cancellable_reset (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  g_return_if_fail (G_IS_CANCELLABLE (cancellable));

  G_LOCK(cancellable);

  priv = cancellable->priv;
  
  while (priv->cancelled_running)
    {
      priv->cancelled_running_waiting = TRUE;
      g_cond_wait (cancellable_cond,
                   g_static_mutex_get_mutex (& G_LOCK_NAME (cancellable)));
    }
  
  if (priv->cancelled)
    {
    /* Make sure we're not leaving old cancel state around */
      
#ifdef G_OS_WIN32
      if (priv->event)
	ResetEvent (priv->event);
#endif
      if (priv->cancel_pipe[0] != -1)
        {
          gssize c;
          char ch;

          do
            c = read (priv->cancel_pipe[0], &ch, 1);
          while (c == -1 && errno == EINTR);
        }

      priv->cancelled = FALSE;
    }
  G_UNLOCK(cancellable);
}

/**
 * g_cancellable_is_cancelled:
 * @cancellable: a #GCancellable or NULL.
 * 
 * Checks if a cancellable job has been cancelled.
 * 
 * Returns: %TRUE if @cancellable is cancelled, 
 * FALSE if called with %NULL or if item is not cancelled. 
 **/
gboolean
g_cancellable_is_cancelled (GCancellable *cancellable)
{
  return cancellable != NULL && cancellable->priv->cancelled;
}

/**
 * g_cancellable_set_error_if_cancelled:
 * @cancellable: a #GCancellable object.
 * @error: #GError to append error state to.
 * 
 * If the @cancellable is cancelled, sets the error to notify
 * that the operation was cancelled.
 * 
 * Returns: %TRUE if @cancellable was cancelled, %FALSE if it was not.
 **/
gboolean
g_cancellable_set_error_if_cancelled (GCancellable  *cancellable,
				      GError       **error)
{
  if (g_cancellable_is_cancelled (cancellable))
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_CANCELLED,
                           _("Operation was cancelled"));
      return TRUE;
    }
  
  return FALSE;
}

/**
 * g_cancellable_get_fd:
 * @cancellable: a #GCancellable.
 * 
 * Gets the file descriptor for a cancellable job. This can be used to
 * implement cancellable operations on Unix systems. The returned fd will
 * turn readable when @cancellable is cancelled.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 * 
 * After a successful return from this function, you should use 
 * g_cancellable_release_fd() to free up resources allocated for 
 * the returned file descriptor.
 *
 * See also g_cancellable_make_pollfd().
 *
 * Returns: A valid file descriptor. %-1 if the file descriptor 
 * is not supported, or on errors. 
 **/
int
g_cancellable_get_fd (GCancellable *cancellable)
{
  GCancellablePrivate *priv;
  int fd;

  if (cancellable == NULL)
    return -1;

  priv = cancellable->priv;

#ifdef G_OS_WIN32
  return -1;
#else
  G_LOCK(cancellable);
  if (priv->cancel_pipe[0] == -1)
    g_cancellable_open_pipe (cancellable);
  fd = priv->cancel_pipe[0];
  if (fd != -1)
    priv->fd_refcount++;
  G_UNLOCK(cancellable);
#endif

  return fd;
}

/**
 * g_cancellable_make_pollfd:
 * @cancellable: a #GCancellable or %NULL
 * @pollfd: a pointer to a #GPollFD
 * 
 * Creates a #GPollFD corresponding to @cancellable; this can be passed
 * to g_poll() and used to poll for cancellation. This is useful both
 * for unix systems without a native poll and for portability to
 * windows.
 *
 * When this function returns %TRUE, you should use 
 * g_cancellable_release_fd() to free up resources allocated for the 
 * @pollfd. After a %FALSE return, do not call g_cancellable_release_fd().
 *
 * If this function returns %FALSE, either no @cancellable was given or
 * resource limits prevent this function from allocating the necessary 
 * structures for polling. (On Linux, you will likely have reached 
 * the maximum number of file descriptors.) The suggested way to handle
 * these cases is to ignore the @cancellable.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 *
 * Returns: %TRUE if @pollfd was successfully initialized, %FALSE on 
 *          failure to prepare the cancellable.
 * 
 * Since: 2.22
 **/
gboolean
g_cancellable_make_pollfd (GCancellable *cancellable, GPollFD *pollfd)
{
  g_return_val_if_fail (pollfd != NULL, FALSE);
  if (cancellable == NULL)
    return FALSE;
  g_return_val_if_fail (G_IS_CANCELLABLE (cancellable), FALSE);

  {
#ifdef G_OS_WIN32
    GCancellablePrivate *priv;

    priv = cancellable->priv;
    G_LOCK(cancellable);
    if (priv->event == NULL)
      {
        /* A manual reset anonymous event, starting unset */
        priv->event = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (priv->event == NULL)
          {
            G_UNLOCK(cancellable);
            return FALSE;
          }
        if (priv->cancelled)
          SetEvent(priv->event);
      }
    priv->fd_refcount++;
    G_UNLOCK(cancellable);

    pollfd->fd = (gintptr)priv->event;
#else /* !G_OS_WIN32 */
    int fd = g_cancellable_get_fd (cancellable);

    if (fd == -1)
      return FALSE;
    pollfd->fd = fd;
#endif /* G_OS_WIN32 */
  }

  pollfd->events = G_IO_IN;
  pollfd->revents = 0;

  return TRUE;
}

/**
 * g_cancellable_release_fd:
 * @cancellable: a #GCancellable
 *
 * Releases a resources previously allocated by g_cancellable_get_fd()
 * or g_cancellable_make_pollfd().
 *
 * For compatibility reasons with older releases, calling this function 
 * is not strictly required, the resources will be automatically freed
 * when the @cancellable is finalized. However, the @cancellable will
 * block scarce file descriptors until it is finalized if this function
 * is not called. This can cause the application to run out of file 
 * descriptors when many #GCancellables are used at the same time.
 * 
 * Since: 2.22
 **/
void
g_cancellable_release_fd (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  if (cancellable == NULL)
    return;

  g_return_if_fail (G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (cancellable->priv->fd_refcount > 0);

  priv = cancellable->priv;

  G_LOCK (cancellable);
  priv->fd_refcount--;
  if (priv->fd_refcount == 0)
    g_cancellable_close_pipe (cancellable);
  G_UNLOCK (cancellable);
}

/**
 * g_cancellable_cancel:
 * @cancellable: a #GCancellable object.
 * 
 * Will set @cancellable to cancelled, and will emit the
 * #GCancellable::cancelled signal. (However, see the warning about
 * race conditions in the documentation for that signal if you are
 * planning to connect to it.)
 *
 * This function is thread-safe. In other words, you can safely call
 * it from a thread other than the one running the operation that was
 * passed the @cancellable.
 *
 * The convention within gio is that cancelling an asynchronous
 * operation causes it to complete asynchronously. That is, if you
 * cancel the operation from the same thread in which it is running,
 * then the operation's #GAsyncReadyCallback will not be invoked until
 * the application returns to the main loop.
 **/
void
g_cancellable_cancel (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  if (cancellable == NULL ||
      cancellable->priv->cancelled)
    return;

  priv = cancellable->priv;

  G_LOCK(cancellable);
  if (priv->cancelled)
    {
      G_UNLOCK (cancellable);
      return;
    }

  priv->cancelled = TRUE;
  priv->cancelled_running = TRUE;
#ifdef G_OS_WIN32
  if (priv->event)
    SetEvent (priv->event);
#endif
  if (priv->cancel_pipe[1] != -1)
    {
      const char ch = 'x';
      gssize c;

      do
        c = write (priv->cancel_pipe[1], &ch, 1);
      while (c == -1 && errno == EINTR);
    }
  G_UNLOCK(cancellable);

  g_object_ref (cancellable);
  g_signal_emit (cancellable, signals[CANCELLED], 0);

  G_LOCK(cancellable);

  priv->cancelled_running = FALSE;
  if (priv->cancelled_running_waiting)
    g_cond_broadcast (cancellable_cond);
  priv->cancelled_running_waiting = FALSE;

  G_UNLOCK(cancellable);

  g_object_unref (cancellable);
}

/**
 * g_cancellable_connect:
 * @cancellable: A #GCancellable.
 * @callback: The #GCallback to connect.
 * @data: Data to pass to @callback.
 * @data_destroy_func: Free function for @data or %NULL.
 *
 * Convenience function to connect to the #GCancellable::cancelled
 * signal. Also handles the race condition that may happen
 * if the cancellable is cancelled right before connecting.
 *
 * @callback is called at most once, either directly at the
 * time of the connect if @cancellable is already cancelled,
 * or when @cancellable is cancelled in some thread.
 *
 * @data_destroy_func will be called when the handler is
 * disconnected, or immediately if the cancellable is already
 * cancelled.
 *
 * See #GCancellable::cancelled for details on how to use this.
 *
 * Returns: The id of the signal handler or 0 if @cancellable has already
 *          been cancelled.
 *
 * Since: 2.22
 */
gulong
g_cancellable_connect (GCancellable   *cancellable,
		       GCallback       callback,
		       gpointer        data,
		       GDestroyNotify  data_destroy_func)
{
  gulong id;

  g_return_val_if_fail (G_IS_CANCELLABLE (cancellable), 0);

  G_LOCK (cancellable);

  if (cancellable->priv->cancelled)
    {
      void (*_callback) (GCancellable *cancellable,
                         gpointer      user_data);

      _callback = (void *)callback;
      id = 0;

      _callback (cancellable, data);

      if (data_destroy_func)
        data_destroy_func (data);
    }
  else
    {
      id = g_signal_connect_data (cancellable, "cancelled",
                                  callback, data,
                                  (GClosureNotify) data_destroy_func,
                                  0);
    }
  G_UNLOCK (cancellable);

  return id;
}

/**
 * g_cancellable_disconnect:
 * @cancellable: A #GCancellable or %NULL.
 * @handler_id: Handler id of the handler to be disconnected, or %0.
 *
 * Disconnects a handler from a cancellable instance similar to
 * g_signal_handler_disconnect().  Additionally, in the event that a
 * signal handler is currently running, this call will block until the
 * handler has finished.  Calling this function from a
 * #GCancellable::cancelled signal handler will therefore result in a
 * deadlock.
 *
 * This avoids a race condition where a thread cancels at the
 * same time as the cancellable operation is finished and the
 * signal handler is removed. See #GCancellable::cancelled for
 * details on how to use this.
 *
 * If @cancellable is %NULL or @handler_id is %0 this function does
 * nothing.
 *
 * Since: 2.22
 */
void
g_cancellable_disconnect (GCancellable  *cancellable,
			  gulong         handler_id)
{
  GCancellablePrivate *priv;

  if (handler_id == 0 ||  cancellable == NULL)
    return;

  G_LOCK (cancellable);

  priv = cancellable->priv;

  while (priv->cancelled_running)
    {
      priv->cancelled_running_waiting = TRUE;
      g_cond_wait (cancellable_cond,
                   g_static_mutex_get_mutex (& G_LOCK_NAME (cancellable)));
    }

  g_signal_handler_disconnect (cancellable, handler_id);
  G_UNLOCK (cancellable);
}
