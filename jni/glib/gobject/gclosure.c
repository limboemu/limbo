/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
 * Copyright (C) 2005 Imendio AB
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

/*
 * MT safe with regards to reference counting.
 */

#include "config.h"

#include <string.h>

#include "gclosure.h"
#include "gvalue.h"


/**
 * SECTION:gclosure
 * @short_description: Functions as first-class objects
 * @title: Closures
 *
 * A #GClosure represents a callback supplied by the programmer. It
 * will generally comprise a function of some kind and a marshaller
 * used to call it. It is the reponsibility of the marshaller to
 * convert the arguments for the invocation from #GValue<!-- -->s into
 * a suitable form, perform the callback on the converted arguments,
 * and transform the return value back into a #GValue.
 *
 * In the case of C programs, a closure usually just holds a pointer
 * to a function and maybe a data argument, and the marshaller
 * converts between #GValue<!-- --> and native C types. The GObject
 * library provides the #GCClosure type for this purpose. Bindings for
 * other languages need marshallers which convert between #GValue<!--
 * -->s and suitable representations in the runtime of the language in
 * order to use functions written in that languages as callbacks.
 *
 * Within GObject, closures play an important role in the
 * implementation of signals. When a signal is registered, the
 * @c_marshaller argument to g_signal_new() specifies the default C
 * marshaller for any closure which is connected to this
 * signal. GObject provides a number of C marshallers for this
 * purpose, see the g_cclosure_marshal_*() functions. Additional C
 * marshallers can be generated with the <link
 * linkend="glib-genmarshal">glib-genmarshal</link> utility.  Closures
 * can be explicitly connected to signals with
 * g_signal_connect_closure(), but it usually more convenient to let
 * GObject create a closure automatically by using one of the
 * g_signal_connect_*() functions which take a callback function/user
 * data pair.
 *
 * Using closures has a number of important advantages over a simple
 * callback function/data pointer combination:
 * <itemizedlist>
 * <listitem><para>
 * Closures allow the callee to get the types of the callback parameters,
 * which means that language bindings don't have to write individual glue
 * for each callback type.
 * </para></listitem>
 * <listitem><para>
 * The reference counting of #GClosure makes it easy to handle reentrancy
 * right; if a callback is removed while it is being invoked, the closure
 * and its parameters won't be freed until the invocation finishes.
 * </para></listitem>
 * <listitem><para>
 * g_closure_invalidate() and invalidation notifiers allow callbacks to be
 * automatically removed when the objects they point to go away.
 * </para></listitem>
 * </itemizedlist>
 */


#define	CLOSURE_MAX_REF_COUNT		((1 << 15) - 1)
#define	CLOSURE_MAX_N_GUARDS		((1 << 1) - 1)
#define	CLOSURE_MAX_N_FNOTIFIERS	((1 << 2) - 1)
#define	CLOSURE_MAX_N_INOTIFIERS	((1 << 8) - 1)
#define	CLOSURE_N_MFUNCS(cl)		((cl)->meta_marshal + \
                                         ((cl)->n_guards << 1L))
/* same as G_CLOSURE_N_NOTIFIERS() (keep in sync) */
#define	CLOSURE_N_NOTIFIERS(cl)		(CLOSURE_N_MFUNCS (cl) + \
                                         (cl)->n_fnotifiers + \
                                         (cl)->n_inotifiers)

typedef union {
  GClosure closure;
  volatile gint vint;
} ClosureInt;

#define CHANGE_FIELD(_closure, _field, _OP, _value, _must_set, _SET_OLD, _SET_NEW)      \
G_STMT_START {                                                                          \
  ClosureInt *cunion = (ClosureInt*) _closure;                 		                \
  gint new_int, old_int, success;                              		                \
  do                                                    		                \
    {                                                   		                \
      ClosureInt tmp;                                   		                \
      tmp.vint = old_int = cunion->vint;                		                \
      _SET_OLD tmp.closure._field;                                                      \
      tmp.closure._field _OP _value;                      		                \
      _SET_NEW tmp.closure._field;                                                      \
      new_int = tmp.vint;                               		                \
      success = g_atomic_int_compare_and_exchange (&cunion->vint, old_int, new_int);    \
    }                                                   		                \
  while (!success && _must_set);                                                        \
} G_STMT_END

#define SWAP(_closure, _field, _value, _oldv)   CHANGE_FIELD (_closure, _field, =, _value, TRUE, *(_oldv) =,     (void) )
#define SET(_closure, _field, _value)           CHANGE_FIELD (_closure, _field, =, _value, TRUE,     (void),     (void) )
#define INC(_closure, _field)                   CHANGE_FIELD (_closure, _field, +=,     1, TRUE,     (void),     (void) )
#define INC_ASSIGN(_closure, _field, _newv)     CHANGE_FIELD (_closure, _field, +=,     1, TRUE,     (void), *(_newv) = )
#define DEC(_closure, _field)                   CHANGE_FIELD (_closure, _field, -=,     1, TRUE,     (void),     (void) )
#define DEC_ASSIGN(_closure, _field, _newv)     CHANGE_FIELD (_closure, _field, -=,     1, TRUE,     (void), *(_newv) = )

#if 0   /* for non-thread-safe closures */
#define SWAP(cl,f,v,o)     (void) (*(o) = cl->f, cl->f = v)
#define SET(cl,f,v)        (void) (cl->f = v)
#define INC(cl,f)          (void) (cl->f += 1)
#define INC_ASSIGN(cl,f,n) (void) (cl->f += 1, *(n) = cl->f)
#define DEC(cl,f)          (void) (cl->f -= 1)
#define DEC_ASSIGN(cl,f,n) (void) (cl->f -= 1, *(n) = cl->f)
#endif

enum {
  FNOTIFY,
  INOTIFY,
  PRE_NOTIFY,
  POST_NOTIFY
};


/* --- functions --- */
/**
 * g_closure_new_simple:
 * @sizeof_closure: the size of the structure to allocate, must be at least
 *                  <literal>sizeof (GClosure)</literal>
 * @data: data to store in the @data field of the newly allocated #GClosure
 *
 * Allocates a struct of the given size and initializes the initial
 * part as a #GClosure. This function is mainly useful when
 * implementing new types of closures.
 *
 * |[
 * typedef struct _MyClosure MyClosure;
 * struct _MyClosure
 * {
 *   GClosure closure;
 *   // extra data goes here
 * };
 *
 * static void
 * my_closure_finalize (gpointer  notify_data,
 *                      GClosure *closure)
 * {
 *   MyClosure *my_closure = (MyClosure *)closure;
 *
 *   // free extra data here
 * }
 *
 * MyClosure *my_closure_new (gpointer data)
 * {
 *   GClosure *closure;
 *   MyClosure *my_closure;
 *
 *   closure = g_closure_new_simple (sizeof (MyClosure), data);
 *   my_closure = (MyClosure *) closure;
 *
 *   // initialize extra data here
 *
 *   g_closure_add_finalize_notifier (closure, notify_data,
 *                                    my_closure_finalize);
 *   return my_closure;
 * }
 * ]|
 *
 * Returns: a newly allocated #GClosure
 */
GClosure*
g_closure_new_simple (guint           sizeof_closure,
		      gpointer        data)
{
  GClosure *closure;

  g_return_val_if_fail (sizeof_closure >= sizeof (GClosure), NULL);

  closure = g_malloc0 (sizeof_closure);
  SET (closure, ref_count, 1);
  SET (closure, meta_marshal, 0);
  SET (closure, n_guards, 0);
  SET (closure, n_fnotifiers, 0);
  SET (closure, n_inotifiers, 0);
  SET (closure, in_inotify, FALSE);
  SET (closure, floating, TRUE);
  SET (closure, derivative_flag, 0);
  SET (closure, in_marshal, FALSE);
  SET (closure, is_invalid, FALSE);
  closure->marshal = NULL;
  closure->data = data;
  closure->notifiers = NULL;
  memset (G_STRUCT_MEMBER_P (closure, sizeof (*closure)), 0, sizeof_closure - sizeof (*closure));

  return closure;
}

static inline void
closure_invoke_notifiers (GClosure *closure,
			  guint     notify_type)
{
  /* notifier layout:
   *     meta_marshal  n_guards    n_guards     n_fnotif.  n_inotifiers
   * ->[[meta_marshal][pre_guards][post_guards][fnotifiers][inotifiers]]
   *
   * CLOSURE_N_MFUNCS(cl)    = meta_marshal + n_guards + n_guards;
   * CLOSURE_N_NOTIFIERS(cl) = CLOSURE_N_MFUNCS(cl) + n_fnotifiers + n_inotifiers
   *
   * constrains/catches:
   * - closure->notifiers may be reloacted during callback
   * - closure->n_fnotifiers and closure->n_inotifiers may change during callback
   * - i.e. callbacks can be removed/added during invocation
   * - must prepare for callback removal during FNOTIFY and INOTIFY (done via ->marshal= & ->data=)
   * - must distinguish (->marshal= & ->data=) for INOTIFY vs. FNOTIFY (via ->in_inotify)
   * + closure->n_guards is const during PRE_NOTIFY & POST_NOTIFY
   * + closure->meta_marshal is const for all cases
   * + none of the callbacks can cause recursion
   * + closure->n_inotifiers is const 0 during FNOTIFY
   */
  switch (notify_type)
    {
      GClosureNotifyData *ndata;
      guint i, offs;
    case FNOTIFY:
      while (closure->n_fnotifiers)
	{
          guint n;
	  DEC_ASSIGN (closure, n_fnotifiers, &n);

	  ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + n;
	  closure->marshal = (GClosureMarshal) ndata->notify;
	  closure->data = ndata->data;
	  ndata->notify (ndata->data, closure);
	}
      closure->marshal = NULL;
      closure->data = NULL;
      break;
    case INOTIFY:
      SET (closure, in_inotify, TRUE);
      while (closure->n_inotifiers)
	{
          guint n;
          DEC_ASSIGN (closure, n_inotifiers, &n);

	  ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers + n;
	  closure->marshal = (GClosureMarshal) ndata->notify;
	  closure->data = ndata->data;
	  ndata->notify (ndata->data, closure);
	}
      closure->marshal = NULL;
      closure->data = NULL;
      SET (closure, in_inotify, FALSE);
      break;
    case PRE_NOTIFY:
      i = closure->n_guards;
      offs = closure->meta_marshal;
      while (i--)
	{
	  ndata = closure->notifiers + offs + i;
	  ndata->notify (ndata->data, closure);
	}
      break;
    case POST_NOTIFY:
      i = closure->n_guards;
      offs = closure->meta_marshal + i;
      while (i--)
	{
	  ndata = closure->notifiers + offs + i;
	  ndata->notify (ndata->data, closure);
	}
      break;
    }
}

/**
 * g_closure_set_meta_marshal:
 * @closure: a #GClosure
 * @marshal_data: context-dependent data to pass to @meta_marshal
 * @meta_marshal: a #GClosureMarshal function
 *
 * Sets the meta marshaller of @closure.  A meta marshaller wraps
 * @closure->marshal and modifies the way it is called in some
 * fashion. The most common use of this facility is for C callbacks.
 * The same marshallers (generated by <link
 * linkend="glib-genmarshal">glib-genmarshal</link>) are used
 * everywhere, but the way that we get the callback function
 * differs. In most cases we want to use @closure->callback, but in
 * other cases we want to use some different technique to retrieve the
 * callback function.
 *
 * For example, class closures for signals (see
 * g_signal_type_cclosure_new()) retrieve the callback function from a
 * fixed offset in the class structure.  The meta marshaller retrieves
 * the right callback and passes it to the marshaller as the
 * @marshal_data argument.
 */
void
g_closure_set_meta_marshal (GClosure       *closure,
			    gpointer        marshal_data,
			    GClosureMarshal meta_marshal)
{
  GClosureNotifyData *notifiers;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (meta_marshal != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (closure->meta_marshal == 0);

  notifiers = closure->notifiers;
  closure->notifiers = g_renew (GClosureNotifyData, NULL, CLOSURE_N_NOTIFIERS (closure) + 1);
  if (notifiers)
    {
      /* usually the meta marshal will be setup right after creation, so the
       * g_memmove() should be rare-case scenario
       */
      g_memmove (closure->notifiers + 1, notifiers, CLOSURE_N_NOTIFIERS (closure) * sizeof (notifiers[0]));
      g_free (notifiers);
    }
  closure->notifiers[0].data = marshal_data;
  closure->notifiers[0].notify = (GClosureNotify) meta_marshal;
  SET (closure, meta_marshal, 1);
}

/**
 * g_closure_add_marshal_guards:
 * @closure: a #GClosure
 * @pre_marshal_data: data to pass to @pre_marshal_notify
 * @pre_marshal_notify: a function to call before the closure callback
 * @post_marshal_data: data to pass to @post_marshal_notify
 * @post_marshal_notify: a function to call after the closure callback
 *
 * Adds a pair of notifiers which get invoked before and after the
 * closure callback, respectively. This is typically used to protect
 * the extra arguments for the duration of the callback. See
 * g_object_watch_closure() for an example of marshal guards.
 */
void
g_closure_add_marshal_guards (GClosure      *closure,
			      gpointer       pre_marshal_data,
			      GClosureNotify pre_marshal_notify,
			      gpointer       post_marshal_data,
			      GClosureNotify post_marshal_notify)
{
  guint i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (pre_marshal_notify != NULL);
  g_return_if_fail (post_marshal_notify != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (closure->n_guards < CLOSURE_MAX_N_GUARDS);

  closure->notifiers = g_renew (GClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 2);
  if (closure->n_inotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers + 1)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
									  closure->n_fnotifiers + 0)];
  if (closure->n_inotifiers > 1)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
								      closure->n_fnotifiers + 1)];
  if (closure->n_fnotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers + 1)] = closure->notifiers[CLOSURE_N_MFUNCS (closure) + 0];
  if (closure->n_fnotifiers > 1)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers)] = closure->notifiers[CLOSURE_N_MFUNCS (closure) + 1];
  if (closure->n_guards)
    closure->notifiers[(closure->meta_marshal +
			closure->n_guards +
			closure->n_guards + 1)] = closure->notifiers[closure->meta_marshal + closure->n_guards];
  i = closure->n_guards;
  closure->notifiers[closure->meta_marshal + i].data = pre_marshal_data;
  closure->notifiers[closure->meta_marshal + i].notify = pre_marshal_notify;
  closure->notifiers[closure->meta_marshal + i + 1].data = post_marshal_data;
  closure->notifiers[closure->meta_marshal + i + 1].notify = post_marshal_notify;
  INC (closure, n_guards);
}

/**
 * g_closure_add_finalize_notifier:
 * @closure: a #GClosure
 * @notify_data: data to pass to @notify_func
 * @notify_func: the callback function to register
 *
 * Registers a finalization notifier which will be called when the
 * reference count of @closure goes down to 0. Multiple finalization
 * notifiers on a single closure are invoked in unspecified order. If
 * a single call to g_closure_unref() results in the closure being
 * both invalidated and finalized, then the invalidate notifiers will
 * be run before the finalize notifiers.
 */
void
g_closure_add_finalize_notifier (GClosure      *closure,
				 gpointer       notify_data,
				 GClosureNotify notify_func)
{
  guint i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);
  g_return_if_fail (closure->n_fnotifiers < CLOSURE_MAX_N_FNOTIFIERS);

  closure->notifiers = g_renew (GClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 1);
  if (closure->n_inotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
								      closure->n_fnotifiers + 0)];
  i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers;
  closure->notifiers[i].data = notify_data;
  closure->notifiers[i].notify = notify_func;
  INC (closure, n_fnotifiers);
}

/**
 * g_closure_add_invalidate_notifier:
 * @closure: a #GClosure
 * @notify_data: data to pass to @notify_func
 * @notify_func: the callback function to register
 *
 * Registers an invalidation notifier which will be called when the
 * @closure is invalidated with g_closure_invalidate(). Invalidation
 * notifiers are invoked before finalization notifiers, in an
 * unspecified order.
 */
void
g_closure_add_invalidate_notifier (GClosure      *closure,
				   gpointer       notify_data,
				   GClosureNotify notify_func)
{
  guint i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->n_inotifiers < CLOSURE_MAX_N_INOTIFIERS);

  closure->notifiers = g_renew (GClosureNotifyData, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 1);
  i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers + closure->n_inotifiers;
  closure->notifiers[i].data = notify_data;
  closure->notifiers[i].notify = notify_func;
  INC (closure, n_inotifiers);
}

static inline gboolean
closure_try_remove_inotify (GClosure       *closure,
			    gpointer       notify_data,
			    GClosureNotify notify_func)
{
  GClosureNotifyData *ndata, *nlast;

  nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure) - 1;
  for (ndata = nlast + 1 - closure->n_inotifiers; ndata <= nlast; ndata++)
    if (ndata->notify == notify_func && ndata->data == notify_data)
      {
	DEC (closure, n_inotifiers);
	if (ndata < nlast)
	  *ndata = *nlast;

	return TRUE;
      }
  return FALSE;
}

static inline gboolean
closure_try_remove_fnotify (GClosure       *closure,
			    gpointer       notify_data,
			    GClosureNotify notify_func)
{
  GClosureNotifyData *ndata, *nlast;

  nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure) - closure->n_inotifiers - 1;
  for (ndata = nlast + 1 - closure->n_fnotifiers; ndata <= nlast; ndata++)
    if (ndata->notify == notify_func && ndata->data == notify_data)
      {
	DEC (closure, n_fnotifiers);
	if (ndata < nlast)
	  *ndata = *nlast;
	if (closure->n_inotifiers)
	  closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			      closure->n_fnotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
									    closure->n_fnotifiers +
									    closure->n_inotifiers)];
	return TRUE;
      }
  return FALSE;
}

/**
 * g_closure_ref:
 * @closure: #GClosure to increment the reference count on
 *
 * Increments the reference count on a closure to force it staying
 * alive while the caller holds a pointer to it.
 *
 * Returns: The @closure passed in, for convenience
 */
GClosure*
g_closure_ref (GClosure *closure)
{
  guint new_ref_count;
  g_return_val_if_fail (closure != NULL, NULL);
  g_return_val_if_fail (closure->ref_count > 0, NULL);
  g_return_val_if_fail (closure->ref_count < CLOSURE_MAX_REF_COUNT, NULL);

  INC_ASSIGN (closure, ref_count, &new_ref_count);
  g_return_val_if_fail (new_ref_count > 1, NULL);

  return closure;
}

/**
 * g_closure_invalidate:
 * @closure: GClosure to invalidate
 *
 * Sets a flag on the closure to indicate that its calling
 * environment has become invalid, and thus causes any future
 * invocations of g_closure_invoke() on this @closure to be
 * ignored. Also, invalidation notifiers installed on the closure will
 * be called at this point. Note that unless you are holding a
 * reference to the closure yourself, the invalidation notifiers may
 * unref the closure and cause it to be destroyed, so if you need to
 * access the closure after calling g_closure_invalidate(), make sure
 * that you've previously called g_closure_ref().
 *
 * Note that g_closure_invalidate() will also be called when the
 * reference count of a closure drops to zero (unless it has already
 * been invalidated before).
 */
void
g_closure_invalidate (GClosure *closure)
{
  g_return_if_fail (closure != NULL);

  if (!closure->is_invalid)
    {
      gboolean was_invalid;
      g_closure_ref (closure);           /* preserve floating flag */
      SWAP (closure, is_invalid, TRUE, &was_invalid);
      /* invalidate only once */
      if (!was_invalid)
        closure_invoke_notifiers (closure, INOTIFY);
      g_closure_unref (closure);
    }
}

/**
 * g_closure_unref:
 * @closure: #GClosure to decrement the reference count on
 *
 * Decrements the reference count of a closure after it was previously
 * incremented by the same caller. If no other callers are using the
 * closure, then the closure will be destroyed and freed.
 */
void
g_closure_unref (GClosure *closure)
{
  guint new_ref_count;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->ref_count > 0);

  if (closure->ref_count == 1)	/* last unref, invalidate first */
    g_closure_invalidate (closure);

  DEC_ASSIGN (closure, ref_count, &new_ref_count);

  if (new_ref_count == 0)
    {
      closure_invoke_notifiers (closure, FNOTIFY);
      g_free (closure->notifiers);
      g_free (closure);
    }
}

/**
 * g_closure_sink:
 * @closure: #GClosure to decrement the initial reference count on, if it's
 *           still being held
 *
 * Takes over the initial ownership of a closure.  Each closure is
 * initially created in a <firstterm>floating</firstterm> state, which
 * means that the initial reference count is not owned by any caller.
 * g_closure_sink() checks to see if the object is still floating, and
 * if so, unsets the floating state and decreases the reference
 * count. If the closure is not floating, g_closure_sink() does
 * nothing. The reason for the existance of the floating state is to
 * prevent cumbersome code sequences like:
 * |[
 * closure = g_cclosure_new (cb_func, cb_data);
 * g_source_set_closure (source, closure);
 * g_closure_unref (closure); // XXX GObject doesn't really need this
 * ]|
 * Because g_source_set_closure() (and similar functions) take ownership of the
 * initial reference count, if it is unowned, we instead can write:
 * |[
 * g_source_set_closure (source, g_cclosure_new (cb_func, cb_data));
 * ]|
 *
 * Generally, this function is used together with g_closure_ref(). Ane example
 * of storing a closure for later notification looks like:
 * |[
 * static GClosure *notify_closure = NULL;
 * void
 * foo_notify_set_closure (GClosure *closure)
 * {
 *   if (notify_closure)
 *     g_closure_unref (notify_closure);
 *   notify_closure = closure;
 *   if (notify_closure)
 *     {
 *       g_closure_ref (notify_closure);
 *       g_closure_sink (notify_closure);
 *     }
 * }
 * ]|
 *
 * Because g_closure_sink() may decrement the reference count of a closure
 * (if it hasn't been called on @closure yet) just like g_closure_unref(),
 * g_closure_ref() should be called prior to this function.
 */
void
g_closure_sink (GClosure *closure)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->ref_count > 0);

  /* floating is basically a kludge to avoid creating closures
   * with a ref_count of 0. so the intial ref_count a closure has
   * is unowned. with invoking g_closure_sink() code may
   * indicate that it takes over that intiial ref_count.
   */
  if (closure->floating)
    {
      gboolean was_floating;
      SWAP (closure, floating, FALSE, &was_floating);
      /* unref floating flag only once */
      if (was_floating)
        g_closure_unref (closure);
    }
}

/**
 * g_closure_remove_invalidate_notifier:
 * @closure: a #GClosure
 * @notify_data: data which was passed to g_closure_add_invalidate_notifier()
 *               when registering @notify_func
 * @notify_func: the callback function to remove
 *
 * Removes an invalidation notifier.
 *
 * Notice that notifiers are automatically removed after they are run.
 */
void
g_closure_remove_invalidate_notifier (GClosure      *closure,
				      gpointer       notify_data,
				      GClosureNotify notify_func)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);

  if (closure->is_invalid && closure->in_inotify && /* account removal of notify_func() while it's called */
      ((gpointer) closure->marshal) == ((gpointer) notify_func) &&
      closure->data == notify_data)
    closure->marshal = NULL;
  else if (!closure_try_remove_inotify (closure, notify_data, notify_func))
    g_warning (G_STRLOC ": unable to remove uninstalled invalidation notifier: %p (%p)",
	       notify_func, notify_data);
}

/**
 * g_closure_remove_finalize_notifier:
 * @closure: a #GClosure
 * @notify_data: data which was passed to g_closure_add_finalize_notifier()
 *  when registering @notify_func
 * @notify_func: the callback function to remove
 *
 * Removes a finalization notifier.
 *
 * Notice that notifiers are automatically removed after they are run.
 */
void
g_closure_remove_finalize_notifier (GClosure      *closure,
				    gpointer       notify_data,
				    GClosureNotify notify_func)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);

  if (closure->is_invalid && !closure->in_inotify && /* account removal of notify_func() while it's called */
      ((gpointer) closure->marshal) == ((gpointer) notify_func) &&
      closure->data == notify_data)
    closure->marshal = NULL;
  else if (!closure_try_remove_fnotify (closure, notify_data, notify_func))
    g_warning (G_STRLOC ": unable to remove uninstalled finalization notifier: %p (%p)",
               notify_func, notify_data);
}

/**
 * g_closure_invoke:
 * @closure: a #GClosure
 * @return_value: a #GValue to store the return value. May be %NULL if the
 *                callback of @closure doesn't return a value.
 * @n_param_values: the length of the @param_values array
 * @param_values: an array of #GValue<!-- -->s holding the arguments on
 *                which to invoke the callback of @closure
 * @invocation_hint: a context-dependent invocation hint
 *
 * Invokes the closure, i.e. executes the callback represented by the @closure.
 */
void
g_closure_invoke (GClosure       *closure,
		  GValue /*out*/ *return_value,
		  guint           n_param_values,
		  const GValue   *param_values,
		  gpointer        invocation_hint)
{
  g_return_if_fail (closure != NULL);

  g_closure_ref (closure);      /* preserve floating flag */
  if (!closure->is_invalid)
    {
      GClosureMarshal marshal;
      gpointer marshal_data;
      gboolean in_marshal = closure->in_marshal;

      g_return_if_fail (closure->marshal || closure->meta_marshal);

      SET (closure, in_marshal, TRUE);
      if (closure->meta_marshal)
	{
	  marshal_data = closure->notifiers[0].data;
	  marshal = (GClosureMarshal) closure->notifiers[0].notify;
	}
      else
	{
	  marshal_data = NULL;
	  marshal = closure->marshal;
	}
      if (!in_marshal)
	closure_invoke_notifiers (closure, PRE_NOTIFY);
      marshal (closure,
	       return_value,
	       n_param_values, param_values,
	       invocation_hint,
	       marshal_data);
      if (!in_marshal)
	closure_invoke_notifiers (closure, POST_NOTIFY);
      SET (closure, in_marshal, in_marshal);
    }
  g_closure_unref (closure);
}

/**
 * g_closure_set_marshal:
 * @closure: a #GClosure
 * @marshal: a #GClosureMarshal function
 *
 * Sets the marshaller of @closure. The <literal>marshal_data</literal>
 * of @marshal provides a way for a meta marshaller to provide additional
 * information to the marshaller. (See g_closure_set_meta_marshal().) For
 * GObject's C predefined marshallers (the g_cclosure_marshal_*()
 * functions), what it provides is a callback function to use instead of
 * @closure->callback.
 */
void
g_closure_set_marshal (GClosure       *closure,
		       GClosureMarshal marshal)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (marshal != NULL);

  if (closure->marshal && closure->marshal != marshal)
    g_warning ("attempt to override closure->marshal (%p) with new marshal (%p)",
	       closure->marshal, marshal);
  else
    closure->marshal = marshal;
}

/**
 * g_cclosure_new:
 * @callback_func: the function to invoke
 * @user_data: user data to pass to @callback_func
 * @destroy_data: destroy notify to be called when @user_data is no longer used
 *
 * Creates a new closure which invokes @callback_func with @user_data as
 * the last parameter.
 *
 * Returns: a new #GCClosure
 */
GClosure*
g_cclosure_new (GCallback      callback_func,
		gpointer       user_data,
		GClosureNotify destroy_data)
{
  GClosure *closure;
  
  g_return_val_if_fail (callback_func != NULL, NULL);
  
  closure = g_closure_new_simple (sizeof (GCClosure), user_data);
  if (destroy_data)
    g_closure_add_finalize_notifier (closure, user_data, destroy_data);
  ((GCClosure*) closure)->callback = (gpointer) callback_func;
  
  return closure;
}

/**
 * g_cclosure_new_swap:
 * @callback_func: the function to invoke
 * @user_data: user data to pass to @callback_func
 * @destroy_data: destroy notify to be called when @user_data is no longer used
 *
 * Creates a new closure which invokes @callback_func with @user_data as
 * the first parameter.
 *
 * Returns: a new #GCClosure
 */
GClosure*
g_cclosure_new_swap (GCallback      callback_func,
		     gpointer       user_data,
		     GClosureNotify destroy_data)
{
  GClosure *closure;
  
  g_return_val_if_fail (callback_func != NULL, NULL);
  
  closure = g_closure_new_simple (sizeof (GCClosure), user_data);
  if (destroy_data)
    g_closure_add_finalize_notifier (closure, user_data, destroy_data);
  ((GCClosure*) closure)->callback = (gpointer) callback_func;
  SET (closure, derivative_flag, TRUE);
  
  return closure;
}

static void
g_type_class_meta_marshal (GClosure       *closure,
			   GValue /*out*/ *return_value,
			   guint           n_param_values,
			   const GValue   *param_values,
			   gpointer        invocation_hint,
			   gpointer        marshal_data)
{
  GTypeClass *class;
  gpointer callback;
  /* GType itype = (GType) closure->data; */
  guint offset = GPOINTER_TO_UINT (marshal_data);
  
  class = G_TYPE_INSTANCE_GET_CLASS (g_value_peek_pointer (param_values + 0), itype, GTypeClass);
  callback = G_STRUCT_MEMBER (gpointer, class, offset);
  if (callback)
    closure->marshal (closure,
		      return_value,
		      n_param_values, param_values,
		      invocation_hint,
		      callback);
}

static void
g_type_iface_meta_marshal (GClosure       *closure,
			   GValue /*out*/ *return_value,
			   guint           n_param_values,
			   const GValue   *param_values,
			   gpointer        invocation_hint,
			   gpointer        marshal_data)
{
  GTypeClass *class;
  gpointer callback;
  GType itype = (GType) closure->data;
  guint offset = GPOINTER_TO_UINT (marshal_data);
  
  class = G_TYPE_INSTANCE_GET_INTERFACE (g_value_peek_pointer (param_values + 0), itype, GTypeClass);
  callback = G_STRUCT_MEMBER (gpointer, class, offset);
  if (callback)
    closure->marshal (closure,
		      return_value,
		      n_param_values, param_values,
		      invocation_hint,
		      callback);
}

/**
 * g_signal_type_cclosure_new:
 * @itype: the #GType identifier of an interface or classed type
 * @struct_offset: the offset of the member function of @itype's class
 *  structure which is to be invoked by the new closure
 *
 * Creates a new closure which invokes the function found at the offset
 * @struct_offset in the class structure of the interface or classed type
 * identified by @itype.
 *
 * Returns: a new #GCClosure
 */
GClosure*
g_signal_type_cclosure_new (GType    itype,
			    guint    struct_offset)
{
  GClosure *closure;
  
  g_return_val_if_fail (G_TYPE_IS_CLASSED (itype) || G_TYPE_IS_INTERFACE (itype), NULL);
  g_return_val_if_fail (struct_offset >= sizeof (GTypeClass), NULL);
  
  closure = g_closure_new_simple (sizeof (GClosure), (gpointer) itype);
  if (G_TYPE_IS_INTERFACE (itype))
    g_closure_set_meta_marshal (closure, GUINT_TO_POINTER (struct_offset), g_type_iface_meta_marshal);
  else
    g_closure_set_meta_marshal (closure, GUINT_TO_POINTER (struct_offset), g_type_class_meta_marshal);
  
  return closure;
}


/**
 * g_cclosure_marshal_VOID__VOID:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 1
 * @param_values: a #GValue array holding only the instance
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__BOOLEAN:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gboolean parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gboolean arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__CHAR:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gchar parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gchar arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__UCHAR:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #guchar parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, guchar arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__INT:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gint parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gint arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__UINT:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #guint parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, guint arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__LONG:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #glong parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, glong arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__ULONG:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gulong parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gulong arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__ENUM:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the enumeration parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gint arg1, gpointer user_data)</literal> where the #gint parameter denotes an enumeration type..
 */

/**
 * g_cclosure_marshal_VOID__FLAGS:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the flags parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gint arg1, gpointer user_data)</literal> where the #gint parameter denotes a flags type.
 */

/**
 * g_cclosure_marshal_VOID__FLOAT:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gfloat parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gfloat arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__DOUBLE:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gdouble parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gdouble arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__STRING:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gchar* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, const gchar *arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__PARAM:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #GParamSpec* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, GParamSpec *arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__BOXED:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #GBoxed* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, GBoxed *arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__POINTER:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #gpointer parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, gpointer arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__OBJECT:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #GObject* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, GObject *arg1, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_VOID__VARIANT:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #GValue array holding the instance and the #GVariant* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, GVariant *arg1, gpointer user_data)</literal>.
 *
 * Since: 2.26
 */

/**
 * g_cclosure_marshal_VOID__UINT_POINTER:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 3
 * @param_values: a #GValue array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>void (*callback) (gpointer instance, guint arg1, gpointer arg2, gpointer user_data)</literal>.
 */

/**
 * g_cclosure_marshal_BOOLEAN__FLAGS:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: a #GValue which can store the returned #gboolean
 * @n_param_values: 2
 * @param_values: a #GValue array holding instance and arg1
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>gboolean (*callback) (gpointer instance, gint arg1, gpointer user_data)</literal> where the #gint parameter
 * denotes a flags type.
 */

/**
 * g_cclosure_marshal_BOOL__FLAGS:
 *
 * Another name for g_cclosure_marshal_BOOLEAN__FLAGS().
 */
/**
 * g_cclosure_marshal_STRING__OBJECT_POINTER:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: a #GValue, which can store the returned string
 * @n_param_values: 3
 * @param_values: a #GValue array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>gchar* (*callback) (gpointer instance, GObject *arg1, gpointer arg2, gpointer user_data)</literal>.
 */
/**
 * g_cclosure_marshal_BOOLEAN__OBJECT_BOXED_BOXED:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: a #GValue, which can store the returned string
 * @n_param_values: 3
 * @param_values: a #GValue array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to g_closure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * <literal>gboolean (*callback) (gpointer instance, GBoxed *arg1, GBoxed *arg2, gpointer user_data)</literal>.
 *
 * Since: 2.26
 */
