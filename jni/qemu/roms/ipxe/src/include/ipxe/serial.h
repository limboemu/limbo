#ifndef _IPXE_SERIAL_H
#define _IPXE_SERIAL_H

/** @file
 *
 * Serial driver functions
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

extern void serial_putc ( int ch );
extern int serial_getc ( void );
extern int serial_ischar ( void );
extern int serial_initialized;

#endif /* _IPXE_SERIAL_H */
