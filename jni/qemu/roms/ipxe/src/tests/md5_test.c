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
 * MD5 tests
 *
 */

#include <stdint.h>
#include <ipxe/md5.h>
#include <ipxe/test.h>
#include "digest_test.h"

/** An MD5 test vector */
struct md5_test_vector {
	/** Test data */
	void *data;
	/** Test data length */
	size_t len;
	/** Expected digest */
	uint8_t digest[MD5_DIGEST_SIZE];
};

/** MD5 test vectors */
static struct md5_test_vector md5_test_vectors[] = {
	/* Test inputs borrowed from SHA-1 tests, with results
	 * calculated using md5sum.
	 */
	{ NULL, 0,
	  { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
	    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e } },
	{ "abc", 3,
	  { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
	    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 } },
	{ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
	  { 0x82, 0x15, 0xef, 0x07, 0x96, 0xa2, 0x0b, 0xca,
	    0xaa, 0xe1, 0x16, 0xd3, 0x87, 0x6c, 0x66, 0x4a } },
};

/** MD5 test fragment lists */
static struct digest_test_fragments md5_test_fragments[] = {
	{ { 0, -1UL, } },
	{ { 1, 1, 1, 1, 1, 1, 1, 1 } },
	{ { 2, 0, 23, 4, 6, 1, 0 } },
};

/**
 * Perform MD5 self-test
 *
 */
static void md5_test_exec ( void ) {
	struct digest_algorithm *digest = &md5_algorithm;
	struct md5_test_vector *test;
	unsigned long cost;
	unsigned int i;
	unsigned int j;

	/* Correctness test */
	for ( i = 0 ; i < ( sizeof ( md5_test_vectors ) /
			    sizeof ( md5_test_vectors[0] ) ) ; i++ ) {
		test = &md5_test_vectors[i];
		/* Test with a single pass */
		digest_ok ( digest, NULL, test->data, test->len, test->digest );
		/* Test with fragment lists */
		for ( j = 0 ; j < ( sizeof ( md5_test_fragments ) /
				    sizeof ( md5_test_fragments[0] ) ) ; j++ ){
			digest_ok ( digest, &md5_test_fragments[j],
				    test->data, test->len, test->digest );
		}
	}

	/* Speed test */
	cost = digest_cost ( digest );
	DBG ( "MD5 required %ld cycles per byte\n", cost );
}

/** MD5 self-test */
struct self_test md5_test __self_test = {
	.name = "md5",
	.exec = md5_test_exec,
};
