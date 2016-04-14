#include <errno.h>

FILE_LICENCE ( GPL2_OR_LATER );

/** @file
 *
 * Error codes
 *
 * This file provides the global variable #errno.
 *
 */

/**
 * Global "last error" number.
 *
 * This is valid only when a function has just returned indicating a
 * failure.
 *
 */
int errno;
