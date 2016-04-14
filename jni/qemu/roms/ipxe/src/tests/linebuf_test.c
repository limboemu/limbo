#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ipxe/linebuf.h>

static const char data1[] = 
"Hello world\r\n"
"This is a reasonably nice set of lines\n"
"with not many different terminators\r\n\r\n"
"There should be exactly one blank line above\n"
"and this line should never appear at all since it has no terminator";

void linebuf_test ( void ) {
	struct line_buffer linebuf;
	const char *data = data1;
	size_t len = ( sizeof ( data1 ) - 1 /* be mean; strip the NUL */ );
	ssize_t frag_len;
	char *line;

	memset ( &linebuf, 0, sizeof ( linebuf ) );
	while ( len ) {
		frag_len = line_buffer ( &linebuf, data, len );
		if ( frag_len < 0 ) {
			printf ( "line_buffer() failed: %s\n",
				 strerror ( frag_len ) );
			return;
		}
		data += frag_len;
		len -= frag_len;
		if ( ( line = buffered_line ( &linebuf ) ) )
			printf ( "\"%s\"\n", line );
	}

	empty_line_buffer ( &linebuf );
}
