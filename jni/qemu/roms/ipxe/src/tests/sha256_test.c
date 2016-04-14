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
 * SHA-256 tests
 *
 */

#include <stdint.h>
#include <ipxe/sha256.h>
#include <ipxe/test.h>
#include "digest_test.h"

/** An SHA-256 test vector */
struct sha256_test_vector {
	/** Test data */
	void *data;
	/** Test data length */
	size_t len;
	/** Expected digest */
	uint8_t digest[SHA256_DIGEST_SIZE];
};

/** SHA-256 test vectors */
static struct sha256_test_vector sha256_test_vectors[] = {
	/* Empty test data
	 *
	 * Expected digest value obtained from "sha256sum /dev/null"
	 */
	{ NULL, 0,
	  { 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
	    0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
	    0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55 } },
	/* Test data and expected digests taken from the NIST
	 * Cryptographic Toolkit Algorithm Examples at
	 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA256.pdf
	 */
	{ "abc", 3,
	  { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
	    0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
	    0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad } },
	{ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
	  { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5, 0xc0, 0x26,
	    0x93, 0x0c, 0x3e, 0x60, 0x39, 0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff,
	    0x21, 0x67, 0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 } },
};

/** SHA-256 test fragment lists */
static struct digest_test_fragments sha256_test_fragments[] = {
	{ { 0, -1UL, } },
	{ { 1, 1, 1, 1, 1, 1, 1, 1 } },
	{ { 2, 0, 23, 4, 6, 1, 0 } },
};

/**
 * Perform SHA-256 self-test
 *
 */
static void sha256_test_exec ( void ) {
	struct digest_algorithm *digest = &sha256_algorithm;
	struct sha256_test_vector *test;
	unsigned long cost;
	unsigned int i;
	unsigned int j;

	/* Correctness test */
	for ( i = 0 ; i < ( sizeof ( sha256_test_vectors ) /
			    sizeof ( sha256_test_vectors[0] ) ) ; i++ ) {
		test = &sha256_test_vectors[i];
		/* Test with a single pass */
		digest_ok ( digest, NULL, test->data, test->len, test->digest );
		/* Test with fragment lists */
		for ( j = 0 ; j < ( sizeof ( sha256_test_fragments ) /
				    sizeof ( sha256_test_fragments[0] )); j++ ){
			digest_ok ( digest, &sha256_test_fragments[j],
				    test->data, test->len, test->digest );
		}
	}

	/* Speed test */
	cost = digest_cost ( digest );
	DBG ( "SHA256 required %ld cycles per byte\n", cost );
}

/** SHA-256 self-test */
struct self_test sha256_test __self_test = {
	.name = "sha256",
	.exec = sha256_test_exec,
};
