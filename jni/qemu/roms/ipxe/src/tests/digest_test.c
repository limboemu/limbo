/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
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

/** @file
 *
 * Digest self-tests
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <stdlib.h>
#include <string.h>
#include <ipxe/crypto.h>
#include <ipxe/profile.h>
#include "digest_test.h"

/** Number of sample iterations for profiling */
#define PROFILE_COUNT 16

/**
 * Test digest algorithm
 *
 * @v digest		Digest algorithm
 * @v fragments		Digest test fragment list, or NULL
 * @v data		Test data
 * @v len		Length of test data
 * @v expected		Expected digest value
 * @ret ok		Digest value is as expected
 */
int digest_test ( struct digest_algorithm *digest,
		  struct digest_test_fragments *fragments,
		  void *data, size_t len, void *expected ) {
	uint8_t ctx[digest->ctxsize];
	uint8_t out[digest->digestsize];
	size_t frag_len = 0;
	unsigned int i;

	/* Initialise digest */
	digest_init ( digest, ctx );

	/* Update digest fragment-by-fragment */
	for ( i = 0 ; len && ( i < ( sizeof ( fragments->len ) /
				     sizeof ( fragments->len[0] ) ) ) ; i++ ) {
		if ( fragments )
			frag_len = fragments->len[i];
		if ( ( frag_len == 0 ) || ( frag_len < len ) )
			frag_len = len;
		digest_update ( digest, ctx, data, frag_len );
		data += frag_len;
		len -= frag_len;
	}

	/* Finalise digest */
	digest_final ( digest, ctx, out );

	/* Compare against expected output */
	return ( memcmp ( expected, out, sizeof ( out ) ) == 0 );
}

/**
 * Calculate digest algorithm cost
 *
 * @v digest		Digest algorithm
 * @ret cost		Cost (in cycles per byte)
 */
unsigned long digest_cost ( struct digest_algorithm *digest ) {
	static uint8_t random[8192]; /* Too large for stack */
	uint8_t ctx[digest->ctxsize];
	uint8_t out[digest->digestsize];
	struct profiler profiler;
	unsigned long cost;
	unsigned int i;

	/* Fill buffer with pseudo-random data */
	srand ( 0x1234568 );
	for ( i = 0 ; i < sizeof ( random ) ; i++ )
		random[i] = rand();

	/* Profile digest calculation */
	memset ( &profiler, 0, sizeof ( profiler ) );
	for ( i = 0 ; i < PROFILE_COUNT ; i++ ) {
		profile_start ( &profiler );
		digest_init ( digest, ctx );
		digest_update ( digest, ctx, random, sizeof ( random ) );
		digest_final ( digest, ctx, out );
		profile_stop ( &profiler );
	}

	/* Round to nearest whole number of cycles per byte */
	cost = ( ( profile_mean ( &profiler ) + ( sizeof ( random ) / 2 ) ) /
		 sizeof ( random ) );

	return cost;
}
