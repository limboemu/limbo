#ifndef AXTLS_OS_PORT_H
#define AXTLS_OS_PORT_H

/**
 * @file os_port.h
 *
 * Trick the axtls code into building within our build environment.
 */

#include <stdint.h>
#include <byteswap.h>

/** All imported axTLS files are licensed using the three-clause BSD licence */
FILE_LICENCE ( BSD3 );

/** We can't actually abort, since we are effectively a kernel... */
#define abort() assert ( 0 )

/** rsa.c uses alloca() */
#define alloca( size ) __builtin_alloca ( size )

#include <ipxe/random_nz.h>
static inline void get_random_NZ ( int num_rand_bytes, uint8_t *rand_data ) {
	/* AXTLS does not check for failures when generating random
	 * data.  Rely on the fact that get_random_nz() does not
	 * request prediction resistance (and so cannot introduce new
	 * failures) and therefore any potential failure must already
	 * have been encountered by e.g. tls_generate_random(), which
	 * does check for failures.
	 */
	get_random_nz ( rand_data, num_rand_bytes );
}

/* Expose AES_encrypt() and AES_decrypt() in aes.o */
#define aes 1
#if OBJECT

struct aes_key_st;

static void AES_encrypt ( const struct aes_key_st *ctx, uint32_t *data );
static void AES_decrypt ( const struct aes_key_st *ctx, uint32_t *data );

void axtls_aes_encrypt ( void *ctx, uint32_t *data ) {
	AES_encrypt ( ctx, data );
}

void axtls_aes_decrypt ( void *ctx, uint32_t *data ) {
	AES_decrypt ( ctx, data );
}

#endif
#undef aes

#endif 
