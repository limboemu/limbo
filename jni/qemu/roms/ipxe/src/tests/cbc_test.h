#ifndef _CBC_TEST_H
#define _CBC_TEST_H

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <ipxe/crypto.h>
#include <ipxe/test.h>

extern int cbc_test_encrypt ( struct cipher_algorithm *cipher, const void *key,
			      size_t key_len, const void *iv,
			      const void *plaintext,
			      const void *expected_ciphertext, size_t len );
extern int cbc_test_decrypt ( struct cipher_algorithm *cipher, const void *key,
			      size_t key_len, const void *iv,
			      const void *ciphertext,
			      const void *expected_plaintext, size_t len );
extern unsigned long cbc_cost_encrypt ( struct cipher_algorithm *cipher,
					size_t key_len );
extern unsigned long cbc_cost_decrypt ( struct cipher_algorithm *cipher,
					size_t key_len );

/**
 * Report CBC encryption test result
 *
 * @v cipher			Cipher algorithm
 * @v key			Key
 * @v key_len			Length of key
 * @v iv			Initialisation vector
 * @v plaintext			Plaintext data
 * @v expected_ciphertext	Expected ciphertext data
 * @v len			Length of data
 */
#define cbc_encrypt_ok( cipher, key, key_len, iv, plaintext,		\
			expected_ciphertext, len ) do {			\
	ok ( cbc_test_encrypt ( cipher, key, key_len, iv, plaintext,	\
				expected_ciphertext, len ) );		\
	} while ( 0 )

/**
 * Report CBC decryption test result
 *
 * @v cipher			Cipher algorithm
 * @v key			Key
 * @v key_len			Length of key
 * @v iv			Initialisation vector
 * @v ciphertext		Ciphertext data
 * @v expected_plaintext	Expected plaintext data
 * @v len			Length of data
 */
#define cbc_decrypt_ok( cipher, key, key_len, iv, ciphertext,		\
			expected_plaintext, len ) do {			\
	ok ( cbc_test_decrypt ( cipher, key, key_len, iv, ciphertext,	\
				expected_plaintext, len ) );		\
	} while ( 0 )

#endif /* _CBC_TEST_H */
