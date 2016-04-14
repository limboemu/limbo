#ifndef _IPXE_AES_H
#define _IPXE_AES_H

FILE_LICENCE ( GPL2_OR_LATER );

struct cipher_algorithm;

/** Basic AES blocksize */
#define AES_BLOCKSIZE 16

#include "crypto/axtls/crypto.h"

/** AES context */
struct aes_context {
	/** AES context for AXTLS */
	AES_CTX axtls_ctx;
	/** Cipher is being used for decrypting */
	int decrypting;
};

/** AES context size */
#define AES_CTX_SIZE sizeof ( struct aes_context )

/* AXTLS functions */
extern void axtls_aes_encrypt ( const AES_CTX *ctx, uint32_t *data );
extern void axtls_aes_decrypt ( const AES_CTX *ctx, uint32_t *data );

extern struct cipher_algorithm aes_algorithm;
extern struct cipher_algorithm aes_cbc_algorithm;

int aes_wrap ( const void *kek, const void *src, void *dest, int nblk );
int aes_unwrap ( const void *kek, const void *src, void *dest, int nblk );

#endif /* _IPXE_AES_H */
