/*****************************************************************************
 * Fast function for filling cache-inhibited memory regions via h-call.
 *
 * Copyright 2015 Red Hat, Inc.
 *
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     Thomas Huth, Red Hat Inc. - initial implementation
 *****************************************************************************/

#include <cache.h>
#include <string.h>

typedef unsigned long type_u;

/**
 * fast_rfill is the implementation of the FAST_RFILL macro with h-calls.
 * This is defined here instead of cache.h since we need a temporary
 * local buffer - and that caused stack size problems in engine() when
 * we used it directly in the FAST_RFILL macro.
 */
void fast_rfill(char *dst, long size, char pat)
{
	type_u buf[64];

	memset(buf, pat, size < sizeof(buf) ? size : sizeof(buf));

	while (size > sizeof(buf)) {
		FAST_MRMOVE(buf, dst, sizeof(buf));
		dst += sizeof(buf);
		size -= sizeof(buf);
	}
	FAST_MRMOVE(buf, dst, size);
}
