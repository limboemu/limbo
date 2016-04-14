/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>
 * Copyright (C) 2010 Piotr Jaroszyński <p.jaroszynski@gmail.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdlib.h>
#include <ctype.h>

/*
 * Despite being exactly the same as strtoul() except the long long instead of
 * long it ends up being much bigger so provide a separate implementation in a
 * separate object so that it won't be linked in if not used.
 */
unsigned long long strtoull ( const char *p, char **endp, int base ) {
	unsigned long long ret = 0;
	int negative = 0;
	unsigned int charval;

	while ( isspace ( *p ) )
		p++;

	if ( *p == '-' ) {
		negative = 1;
		p++;
	}

	base = strtoul_base ( &p, base );

	while ( 1 ) {
		charval = strtoul_charval ( *p );
		if ( charval >= ( unsigned int ) base )
			break;
		ret = ( ( ret * base ) + charval );
		p++;
	}

	if ( negative )
		ret = -ret;

	if ( endp )
		*endp = ( char * ) p;

	return ( ret );
}
