/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <byteswap.h>
#include <ipxe/crypto.h>
#include <ipxe/cbc.h>
#include <ipxe/aes.h>
#include "crypto/axtls/crypto.h"

/** @file
 *
 * AES algorithm
 *
 */

/**
 * Set key
 *
 * @v ctx		Context
 * @v key		Key
 * @v keylen		Key length
 * @ret rc		Return status code
 */
static int aes_setkey ( void *ctx, const void *key, size_t keylen ) {
	struct aes_context *aes_ctx = ctx;
	AES_MODE mode;
	void *iv;

	switch ( keylen ) {
	case ( 128 / 8 ):
		mode = AES_MODE_128;
		break;
	case ( 256 / 8 ):
		mode = AES_MODE_256;
		break;
	default:
		return -EINVAL;
	}

	/* IV is not a relevant concept at this stage; use a dummy
	 * value that will have no side-effects.
	 */
	iv = &aes_ctx->axtls_ctx.iv;

	AES_set_key ( &aes_ctx->axtls_ctx, key, iv, mode );

	aes_ctx->decrypting = 0;

	return 0;
}

/**
 * Set initialisation vector
 *
 * @v ctx		Context
 * @v iv		Initialisation vector
 */
static void aes_setiv ( void *ctx __unused, const void *iv __unused ) {
	/* Nothing to do */
}

/**
 * Call AXTLS' AES_encrypt() or AES_decrypt() functions
 *
 * @v axtls_ctx		AXTLS AES context
 * @v src		Data to process
 * @v dst		Buffer for output
 * @v func		AXTLS AES function to call
 */
static void aes_call_axtls ( AES_CTX *axtls_ctx, const void *src, void *dst,
			     void ( * func ) ( const AES_CTX *axtls_ctx,
					       uint32_t *data ) ){
	const uint32_t *srcl = src;
	uint32_t *dstl = dst;
	unsigned int i;

	/* AXTLS' AES_encrypt() and AES_decrypt() functions both
	 * expect to deal with an array of four dwords in host-endian
	 * order.
	 */
	for ( i = 0 ; i < 4 ; i++ )
		dstl[i] = ntohl ( srcl[i] );
	func ( axtls_ctx, dstl );
	for ( i = 0 ; i < 4 ; i++ )
		dstl[i] = htonl ( dstl[i] );
}

/**
 * Encrypt data
 *
 * @v ctx		Context
 * @v src		Data to encrypt
 * @v dst		Buffer for encrypted data
 * @v len		Length of data
 */
static void aes_encrypt ( void *ctx, const void *src, void *dst,
			  size_t len ) {
	struct aes_context *aes_ctx = ctx;

	assert ( len == AES_BLOCKSIZE );
	if ( aes_ctx->decrypting )
		assert ( 0 );
	aes_call_axtls ( &aes_ctx->axtls_ctx, src, dst, axtls_aes_encrypt );
}

/**
 * Decrypt data
 *
 * @v ctx		Context
 * @v src		Data to decrypt
 * @v dst		Buffer for decrypted data
 * @v len		Length of data
 */
static void aes_decrypt ( void *ctx, const void *src, void *dst,
			  size_t len ) {
	struct aes_context *aes_ctx = ctx;

	assert ( len == AES_BLOCKSIZE );
	if ( ! aes_ctx->decrypting ) {
		AES_convert_key ( &aes_ctx->axtls_ctx );
		aes_ctx->decrypting = 1;
	}
	aes_call_axtls ( &aes_ctx->axtls_ctx, src, dst, axtls_aes_decrypt );
}

/** Basic AES algorithm */
struct cipher_algorithm aes_algorithm = {
	.name = "aes",
	.ctxsize = sizeof ( struct aes_context ),
	.blocksize = AES_BLOCKSIZE,
	.setkey = aes_setkey,
	.setiv = aes_setiv,
	.encrypt = aes_encrypt,
	.decrypt = aes_decrypt,
};

/* AES with cipher-block chaining */
CBC_CIPHER ( aes_cbc, aes_cbc_algorithm,
	     aes_algorithm, struct aes_context, AES_BLOCKSIZE );
