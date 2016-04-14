#ifndef _IPXE_SHA256_H
#define _IPXE_SHA256_H

/** @file
 *
 * SHA-256 algorithm
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <ipxe/crypto.h>

/** An SHA-256 digest */
struct sha256_digest {
	/** Hash output */
	uint32_t h[8];
};

/** An SHA-256 data block */
union sha256_block {
	/** Raw bytes */
	uint8_t byte[64];
	/** Raw dwords */
	uint32_t dword[16];
	/** Final block structure */
	struct {
		/** Padding */
		uint8_t pad[56];
		/** Length in bits */
		uint64_t len;
	} final;
};

/** SHA-256 digest and data block
 *
 * The order of fields within this structure is designed to minimise
 * code size.
 */
struct sha256_digest_data {
	/** Digest of data already processed */
	struct sha256_digest digest;
	/** Accumulated data */
	union sha256_block data;
} __attribute__ (( packed ));

/** SHA-256 digest and data block */
union sha256_digest_data_dwords {
	/** Digest and data block */
	struct sha256_digest_data dd;
	/** Raw dwords */
	uint32_t dword[ sizeof ( struct sha256_digest_data ) /
			sizeof ( uint32_t ) ];
};

/** An SHA-256 context */
struct sha256_context {
	/** Amount of accumulated data */
	size_t len;
	/** Digest and accumulated data */
	union sha256_digest_data_dwords ddd;
} __attribute__ (( packed ));

/** SHA-256 context size */
#define SHA256_CTX_SIZE sizeof ( struct sha256_context )

/** SHA-256 digest size */
#define SHA256_DIGEST_SIZE sizeof ( struct sha256_digest )

extern struct digest_algorithm sha256_algorithm;

#endif /* _IPXE_SHA256_H */
