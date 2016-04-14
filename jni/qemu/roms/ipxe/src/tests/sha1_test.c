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
 * SHA-1 tests
 *
 */

#include <stdint.h>
#include <ipxe/sha1.h>
#include <ipxe/test.h>
#include "digest_test.h"

/** An SHA-1 test vector */
struct sha1_test_vector {
	/** Test data */
	void *data;
	/** Test data length */
	size_t len;
	/** Expected digest */
	uint8_t digest[SHA1_DIGEST_SIZE];
};

/** SHA-1 test vectors */
static struct sha1_test_vector sha1_test_vectors[] = {
	/* Empty test data
	 *
	 * Expected digest value obtained from "sha1sum /dev/null"
	 */
	{ NULL, 0,
	  { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
	    0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09 } },
	/* Test data and expected digests taken from the NIST
	 * Cryptographic Toolkit Algorithm Examples at
	 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA1.pdf
	 */
	{ "abc", 3,
	  { 0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
	    0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d } },
	{ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
	  { 0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae,
	    0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1 } },
};

/** SHA-1 test fragment lists */
static struct digest_test_fragments sha1_test_fragments[] = {
	{ { 0, -1UL, } },
	{ { 1, 1, 1, 1, 1, 1, 1, 1 } },
	{ { 2, 0, 23, 4, 6, 1, 0 } },
};

/**
 * Perform SHA-1 self-test
 *
 */
static void sha1_test_exec ( void ) {
	struct digest_algorithm *digest = &sha1_algorithm;
	struct sha1_test_vector *test;
	unsigned long cost;
	unsigned int i;
	unsigned int j;

	/* Correctness test */
	for ( i = 0 ; i < ( sizeof ( sha1_test_vectors ) /
			    sizeof ( sha1_test_vectors[0] ) ) ; i++ ) {
		test = &sha1_test_vectors[i];
		/* Test with a single pass */
		digest_ok ( digest, NULL, test->data, test->len, test->digest );
		/* Test with fragment lists */
		for ( j = 0 ; j < ( sizeof ( sha1_test_fragments ) /
				    sizeof ( sha1_test_fragments[0] ) ) ; j++ ){
			digest_ok ( digest, &sha1_test_fragments[j],
				    test->data, test->len, test->digest );
		}
	}

	/* Speed test */
	cost = digest_cost ( digest );
	DBG ( "SHA1 required %ld cycles per byte\n", cost );
}

/** SHA-1 self-test */
struct self_test sha1_test __self_test = {
	.name = "sha1",
	.exec = sha1_test_exec,
};
