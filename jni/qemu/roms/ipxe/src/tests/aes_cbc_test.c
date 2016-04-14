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
 * AES-in-CBC-mode tests
 *
 * These test vectors are provided by NIST as part of the
 * Cryptographic Toolkit Examples, downloadable from:
 *
 *    http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/AES_CBC.pdf
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <assert.h>
#include <string.h>
#include <ipxe/aes.h>
#include <ipxe/test.h>
#include "cbc_test.h"

/** Define inline key */
#define KEY(...) { __VA_ARGS__ }

/** Define inline initialisation vector */
#define IV(...) { __VA_ARGS__ }

/** Define inline plaintext data */
#define PLAINTEXT(...) { __VA_ARGS__ }

/** Define inline ciphertext data */
#define CIPHERTEXT(...) { __VA_ARGS__ }

/** An AES-in-CBC-mode test */
struct aes_cbc_test {
	/** Key */
	const void *key;
	/** Length of key */
	size_t key_len;
	/** Initialisation vector */
	const void *iv;
	/** Length of initialisation vector */
	size_t iv_len;
	/** Plaintext */
	const void *plaintext;
	/** Length of plaintext */
	size_t plaintext_len;
	/** Ciphertext */
	const void *ciphertext;
	/** Length of ciphertext */
	size_t ciphertext_len;
};

/**
 * Define an AES-in-CBC-mode test
 *
 * @v name		Test name
 * @v key_array		Key
 * @v iv_array		Initialisation vector
 * @v plaintext_array	Plaintext
 * @v ciphertext_array	Ciphertext
 * @ret test		AES-in-CBC-mode test
 */
#define AES_CBC_TEST( name, key_array, iv_array, plaintext_array,	\
		      ciphertext_array )				\
	static const uint8_t name ## _key [] = key_array;		\
	static const uint8_t name ## _iv [] = iv_array;			\
	static const uint8_t name ## _plaintext [] = plaintext_array;	\
	static const uint8_t name ## _ciphertext [] = ciphertext_array;	\
	static struct aes_cbc_test name = {				\
		.key = name ## _key,					\
		.key_len = sizeof ( name ## _key ),			\
		.iv = name ## _iv,					\
		.iv_len = sizeof ( name ## _iv ),			\
		.plaintext = name ## _plaintext,			\
		.plaintext_len = sizeof ( name ## _plaintext ),		\
		.ciphertext = name ## _ciphertext,			\
		.ciphertext_len = sizeof ( name ## _ciphertext ),	\
	}

/**
 * Report AES-in-CBC-mode
 *
 * @v state		HMAC_DRBG internal state
 * @v test		Instantiation test
 */
#define aes_cbc_ok( test ) do {						\
	struct cipher_algorithm *cipher = &aes_cbc_algorithm;		\
									\
	assert ( (test)->iv_len == cipher->blocksize );			\
	assert ( (test)->plaintext_len == (test)->ciphertext_len );	\
	cbc_encrypt_ok ( cipher, (test)->key, (test)->key_len,		\
			 (test)->iv, (test)->plaintext,			\
			 (test)->ciphertext, (test)->plaintext_len );	\
	cbc_decrypt_ok ( cipher, (test)->key, (test)->key_len,		\
			 (test)->iv, (test)->ciphertext,		\
			 (test)->plaintext, (test)->ciphertext_len );	\
	} while ( 0 )

/** CBC_AES128 */
AES_CBC_TEST ( test_128,
	KEY ( 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15,
	      0x88, 0x09, 0xcf, 0x4f, 0x3c ),
	IV ( 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	     0x0b, 0x0c, 0x0d, 0x0e, 0x0f ),
	PLAINTEXT ( 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
		    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
		    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
		    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
		    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
		    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
		    0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 ),
	CIPHERTEXT ( 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
		     0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
		     0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
		     0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
		     0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
		     0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
		     0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
		     0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7 ) );

/** CBC_AES256 */
AES_CBC_TEST ( test_256,
	KEY ( 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
	      0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
	      0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 ),
	IV ( 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	     0x0b, 0x0c, 0x0d, 0x0e, 0x0f ),
	PLAINTEXT ( 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
		    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
		    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
		    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
		    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
		    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
		    0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 ),
	CIPHERTEXT ( 0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
		     0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
		     0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d,
		     0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
		     0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf,
		     0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
		     0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc,
		     0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b ) );

/**
 * Perform AES-in-CBC-mode self-test
 *
 */
static void aes_cbc_test_exec ( void ) {
	struct cipher_algorithm *cipher = &aes_cbc_algorithm;

	/* Correctness tests */
	aes_cbc_ok ( &test_128 );
	aes_cbc_ok ( &test_256 );

	/* Speed tests */
	DBG ( "AES128 encryption required %ld cycles per byte\n",
	      cbc_cost_encrypt ( cipher, test_128.key_len ) );
	DBG ( "AES128 decryption required %ld cycles per byte\n",
	      cbc_cost_decrypt ( cipher, test_128.key_len ) );
	DBG ( "AES256 encryption required %ld cycles per byte\n",
	      cbc_cost_encrypt ( cipher, test_256.key_len ) );
	DBG ( "AES256 decryption required %ld cycles per byte\n",
	      cbc_cost_decrypt ( cipher, test_256.key_len ) );
}

/** AES-in-CBC-mode self-test */
struct self_test aes_cbc_test __self_test = {
	.name = "aes_cbc",
	.exec = aes_cbc_test_exec,
};
