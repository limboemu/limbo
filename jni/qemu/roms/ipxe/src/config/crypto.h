#ifndef CONFIG_CRYPTO_H
#define CONFIG_CRYPTO_H

/** @file
 *
 * Cryptographic configuration
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

/** Margin of error (in seconds) allowed in signed timestamps
 *
 * We default to allowing a reasonable margin of error: 12 hours to
 * allow for the local time zone being non-GMT, plus 30 minutes to
 * allow for general clock drift.
 */
#define TIMESTAMP_ERROR_MARGIN ( ( 12 * 60 + 30 ) * 60 )

#include <config/named.h>
#include NAMED_CONFIG(crypto.h)
#include <config/local/crypto.h>
#include LOCAL_NAMED_CONFIG(crypto.h)

#endif /* CONFIG_CRYPTO_H */
