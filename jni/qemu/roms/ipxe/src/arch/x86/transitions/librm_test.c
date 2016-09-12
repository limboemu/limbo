/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * Real mode transition self-tests
 *
 * This file allows for easy measurement of the time taken to perform
 * real mode transitions, which may have a substantial overhead when
 * running under a hypervisor.
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <ipxe/test.h>
#include <ipxe/profile.h>
#include <realmode.h>

/** Number of sample iterations for profiling */
#define PROFILE_COUNT 4096

/** Protected-to-real mode transition profiler */
static struct profiler p2r_profiler __profiler = { .name = "p2r" };

/** Real-to-protected mode transition profiler */
static struct profiler r2p_profiler __profiler = { .name = "r2p" };

/** Real-mode call profiler */
static struct profiler real_call_profiler __profiler = { .name = "real_call" };

/** Virtual call profiler */
static struct profiler virt_call_profiler __profiler = { .name = "virt_call" };

/**
 * Dummy function for profiling tests
 */
static __asmcall void librm_test_call ( struct i386_all_regs *ix86 __unused ) {
	/* Do nothing */
}

/**
 * Perform real mode transition self-tests
 *
 */
static void librm_test_exec ( void ) {
	unsigned int i;
	unsigned long timestamp;
	uint32_t timestamp_lo;
	uint32_t timestamp_hi;
	uint32_t started;
	uint32_t stopped;
	uint32_t discard_d;

	/* Profile mode transitions.  We want to profile each
	 * direction of the transition separately, so perform an RDTSC
	 * while in real mode and tweak the profilers' start/stop
	 * times appropriately.
	 */
	for ( i = 0 ; i < PROFILE_COUNT ; i++ ) {
		profile_start ( &p2r_profiler );
		__asm__ __volatile__ ( REAL_CODE ( "rdtsc\n\t" )
				       : "=a" ( timestamp_lo ),
					 "=d" ( timestamp_hi )
				       : );
		timestamp = timestamp_lo;
		if ( sizeof ( timestamp ) > sizeof ( timestamp_lo ) )
			timestamp |= ( ( ( uint64_t ) timestamp_hi ) << 32 );
		profile_start_at ( &r2p_profiler, timestamp );
		profile_stop ( &r2p_profiler );
		profile_stop_at ( &p2r_profiler, timestamp );
	}

	/* Profile complete real-mode call cycle */
	for ( i = 0 ; i < PROFILE_COUNT ; i++ ) {
		profile_start ( &real_call_profiler );
		__asm__ __volatile__ ( REAL_CODE ( "" ) );
		profile_stop ( &real_call_profiler );
	}

	/* Profile complete virtual call cycle */
	for ( i = 0 ; i < PROFILE_COUNT ; i++ ) {
		__asm__ __volatile__ ( REAL_CODE ( "rdtsc\n\t"
						   "movl %k0, %k2\n\t"
						   VIRT_CALL ( librm_test_call )
						   "rdtsc\n\t" )
				       : "=a" ( stopped ), "=d" ( discard_d ),
					 "=R" ( started ) : );
		profile_start_at ( &virt_call_profiler, started );
		profile_stop_at ( &virt_call_profiler, stopped );
	}
}

/** Real mode transition self-test */
struct self_test librm_test __self_test = {
	.name = "librm",
	.exec = librm_test_exec,
};

REQUIRING_SYMBOL ( librm_test );
REQUIRE_OBJECT ( test );
