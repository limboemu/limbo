/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stddef.h>
#include <ipxe/timer.h>
#include <ipxe/list.h>
#include <ipxe/process.h>
#include <ipxe/init.h>
#include <ipxe/retry.h>

/** @file
 *
 * Retry timers
 *
 * A retry timer is a binary exponential backoff timer.  It can be
 * used to build automatic retransmission into network protocols.
 *
 * This implementation of the timer is designed to satisfy RFC 2988
 * and therefore be usable as a TCP retransmission timer.
 *
 * 
 */

/* The theoretical minimum that the algorithm in stop_timer() can
 * adjust the timeout back down to is seven ticks, so set the minimum
 * timeout to at least that value for the sake of consistency.
 */
#define MIN_TIMEOUT 7

/** List of running timers */
static LIST_HEAD ( timers );

/**
 * Start timer
 *
 * @v timer		Retry timer
 *
 * This starts the timer running with the current timeout value.  If
 * stop_timer() is not called before the timer expires, the timer will
 * be stopped and the timer's callback function will be called.
 */
void start_timer ( struct retry_timer *timer ) {
	if ( ! timer->running ) {
		list_add ( &timer->list, &timers );
		ref_get ( timer->refcnt );
	}
	timer->start = currticks();
	timer->running = 1;

	/* 0 means "use default timeout" */
	if ( timer->min_timeout == 0 )
		timer->min_timeout = DEFAULT_MIN_TIMEOUT;
	/* We must never be less than MIN_TIMEOUT under any circumstances */
	if ( timer->min_timeout < MIN_TIMEOUT )
		timer->min_timeout = MIN_TIMEOUT;
	/* Honor user-specified minimum timeout */
	if ( timer->timeout < timer->min_timeout )
		timer->timeout = timer->min_timeout;

	DBG2 ( "Timer %p started at time %ld (expires at %ld)\n",
	       timer, timer->start, ( timer->start + timer->timeout ) );
}

/**
 * Start timer with a specified fixed timeout
 *
 * @v timer		Retry timer
 * @v timeout		Timeout, in ticks
 */
void start_timer_fixed ( struct retry_timer *timer, unsigned long timeout ) {
	start_timer ( timer );
	timer->timeout = timeout;
	DBG2 ( "Timer %p expiry time changed to %ld\n",
	       timer, ( timer->start + timer->timeout ) );
}

/**
 * Stop timer
 *
 * @v timer		Retry timer
 *
 * This stops the timer and updates the timer's timeout value.
 */
void stop_timer ( struct retry_timer *timer ) {
	unsigned long old_timeout = timer->timeout;
	unsigned long now = currticks();
	unsigned long runtime;

	/* If timer was already stopped, do nothing */
	if ( ! timer->running )
		return;

	list_del ( &timer->list );
	runtime = ( now - timer->start );
	timer->running = 0;
	DBG2 ( "Timer %p stopped at time %ld (ran for %ld)\n",
	       timer, now, runtime );

	/* Update timer.  Variables are:
	 *
	 *   r = round-trip time estimate (i.e. runtime)
	 *   t = timeout value (i.e. timer->timeout)
	 *   s = smoothed round-trip time
	 *
	 * By choice, we set t = 4s, i.e. allow for four times the
	 * normal round-trip time to pass before retransmitting.
	 *
	 * We want to smooth according to s := ( 7 s + r ) / 8
	 *
	 * Since we don't actually store s, this reduces to
	 * t := ( 7 t / 8 ) + ( r / 2 )
	 *
	 */
	if ( timer->count ) {
		timer->count--;
	} else {
		timer->timeout -= ( timer->timeout >> 3 );
		timer->timeout += ( runtime >> 1 );
		if ( timer->timeout != old_timeout ) {
			DBG ( "Timer %p timeout updated to %ld\n",
			      timer, timer->timeout );
		}
	}

	ref_put ( timer->refcnt );
}

/**
 * Handle expired timer
 *
 * @v timer		Retry timer
 */
static void timer_expired ( struct retry_timer *timer ) {
	struct refcnt *refcnt = timer->refcnt;
	int fail;

	/* Stop timer without performing RTT calculations */
	DBG2 ( "Timer %p stopped at time %ld on expiry\n",
	       timer, currticks() );
	assert ( timer->running );
	list_del ( &timer->list );
	timer->running = 0;
	timer->count++;

	/* Back off the timeout value */
	timer->timeout <<= 1;
	if ( timer->max_timeout == 0 ) /* 0 means "use default timeout" */
		timer->max_timeout = DEFAULT_MAX_TIMEOUT;
	if ( ( fail = ( timer->timeout > timer->max_timeout ) ) )
		timer->timeout = timer->max_timeout;
	DBG ( "Timer %p timeout backed off to %ld\n",
	      timer, timer->timeout );

	/* Call expiry callback */
	timer->expired ( timer, fail );
	/* If refcnt is NULL, then timer may already have been freed */

	ref_put ( refcnt );
}

/**
 * Poll the retry timer list
 *
 */
void retry_poll ( void ) {
	struct retry_timer *timer;
	unsigned long now = currticks();
	unsigned long used;

	/* Process at most one timer expiry.  We cannot process
	 * multiple expiries in one pass, because one timer expiring
	 * may end up triggering another timer's deletion from the
	 * list.
	 */
	list_for_each_entry ( timer, &timers, list ) {
		used = ( now - timer->start );
		if ( used >= timer->timeout ) {
			timer_expired ( timer );
			break;
		}
	}
}

/**
 * Single-step the retry timer list
 *
 * @v process		Retry timer process
 */
static void retry_step ( struct process *process __unused ) {
	retry_poll();
}

/** Retry timer process */
PERMANENT_PROCESS ( retry_process, retry_step );
