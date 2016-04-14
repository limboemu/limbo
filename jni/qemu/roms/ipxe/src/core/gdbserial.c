/*
 * Copyright (C) 2008 Stefan Hajnoczi <stefanha@gmail.com>.
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

#include <assert.h>
#include <ipxe/serial.h>
#include <ipxe/gdbstub.h>
#include <ipxe/gdbserial.h>

struct gdb_transport serial_gdb_transport __gdb_transport;

static size_t gdbserial_recv ( char *buf, size_t len ) {
	assert ( len > 0 );
	buf [ 0 ] = serial_getc();
	return 1;
}

static void gdbserial_send ( const char *buf, size_t len ) {
	while ( len-- > 0 ) {
		serial_putc ( *buf++ );
	}
}

struct gdb_transport serial_gdb_transport __gdb_transport = {
	.name = "serial",
	.recv = gdbserial_recv,
	.send = gdbserial_send,
};

struct gdb_transport *gdbserial_configure ( void ) {
	return &serial_gdb_transport;
}
