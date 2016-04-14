#ifndef _IPXE_RETRY_H
#define _IPXE_RETRY_H

/** @file
 *
 * Retry timers
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <ipxe/list.h>

/** Default timeout value */
#define DEFAULT_MIN_TIMEOUT ( TICKS_PER_SEC / 4 )

/** Limit after which the timeout will be deemed permanent */
#define DEFAULT_MAX_TIMEOUT ( 10 * TICKS_PER_SEC )

/** A retry timer */
struct retry_timer {
	/** List of active timers */
	struct list_head list;
	/** Timer is currently running */
	unsigned int running;
	/** Timeout value (in ticks) */
	unsigned long timeout;
	/** Minimum timeout value (in ticks)
	 *
	 * A value of zero means "use default timeout."
	 */
	unsigned long min_timeout;
	/** Maximum timeout value before failure (in ticks)
	 *
	 * A value of zero means "use default timeout."
	 */
	unsigned long max_timeout;
	/** Start time (in ticks) */
	unsigned long start;
	/** Retry count */
	unsigned int count;
	/** Timer expired callback
	 *
	 * @v timer	Retry timer
	 * @v fail	Failure indicator
	 *
	 * The timer will already be stopped when this method is
	 * called.  The failure indicator will be True if the retry
	 * timeout has already exceeded @c MAX_TIMEOUT.
	 */
	void ( * expired ) ( struct retry_timer *timer, int over );
	/** Reference counter
	 *
	 * If this interface is not part of a reference-counted
	 * object, this field may be NULL.
	 */
	struct refcnt *refcnt;
};

/**
 * Initialise a timer
 *
 * @v timer		Retry timer
 * @v expired		Timer expired callback
 * @v refcnt		Reference counter, or NULL
 */
static inline __attribute__ (( always_inline )) void
timer_init ( struct retry_timer *timer,
	     void ( * expired ) ( struct retry_timer *timer, int over ),
	     struct refcnt *refcnt ) {
	timer->expired = expired;
	timer->refcnt = refcnt;
}

/**
 * Initialise a static timer
 *
 * @v expired_fn	Timer expired callback
 */
#define TIMER_INIT( expired_fn ) {			\
		.expired = (expired_fn),		\
	}

extern void start_timer ( struct retry_timer *timer );
extern void start_timer_fixed ( struct retry_timer *timer,
				unsigned long timeout );
extern void stop_timer ( struct retry_timer *timer );
extern void retry_poll ( void );

/**
 * Start timer with no delay
 *
 * @v timer		Retry timer
 *
 * This starts the timer running with a zero timeout value.
 */
static inline void start_timer_nodelay ( struct retry_timer *timer ) {
	start_timer_fixed ( timer, 0 );
}

/**
 * Test to see if timer is currently running
 *
 * @v timer		Retry timer
 * @ret running		Non-zero if timer is running
 */
static inline __attribute__ (( always_inline )) unsigned long
timer_running ( struct retry_timer *timer ) {
	return ( timer->running );
}

#endif /* _IPXE_RETRY_H */
