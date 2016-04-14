#ifndef _DIGEST_TEST_H
#define _DIGEST_TEST_H

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <ipxe/crypto.h>
#include <ipxe/test.h>

/** Maximum number of digest test fragments */
#define NUM_DIGEST_TEST_FRAG 8

/** A digest test fragment list */
struct digest_test_fragments {
	/** Fragment lengths */
	size_t len[NUM_DIGEST_TEST_FRAG];
};

extern int digest_test ( struct digest_algorithm *digest,
			 struct digest_test_fragments *fragments,
			 void *data, size_t len, void *expected );
extern unsigned long digest_cost ( struct digest_algorithm *digest );

/**
 * Report digest test result
 *
 * @v digest		Digest algorithm
 * @v fragments		Digest test fragment list, or NULL
 * @v data		Test data
 * @v len		Length of test data
 * @v expected		Expected digest value
 */
#define digest_ok( digest, fragments, data, len, expected ) do {	\
	ok ( digest_test ( digest, fragments, data, len, expected ) );	\
	} while ( 0 )

#endif /* _DIGEST_TEST_H */
