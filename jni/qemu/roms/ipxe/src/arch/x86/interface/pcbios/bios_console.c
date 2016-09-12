/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <assert.h>
#include <realmode.h>
#include <bios.h>
#include <biosint.h>
#include <ipxe/console.h>
#include <ipxe/ansiesc.h>
#include <ipxe/keys.h>
#include <ipxe/keymap.h>
#include <ipxe/init.h>
#include <config/console.h>

#define ATTR_BOLD		0x08

#define ATTR_FCOL_MASK		0x07
#define ATTR_FCOL_BLACK		0x00
#define ATTR_FCOL_BLUE		0x01
#define ATTR_FCOL_GREEN		0x02
#define ATTR_FCOL_CYAN		0x03
#define ATTR_FCOL_RED		0x04
#define ATTR_FCOL_MAGENTA	0x05
#define ATTR_FCOL_YELLOW	0x06
#define ATTR_FCOL_WHITE		0x07

#define ATTR_BLINK		0x80

#define ATTR_BCOL_MASK		0x70
#define ATTR_BCOL_BLACK		0x00
#define ATTR_BCOL_BLUE		0x10
#define ATTR_BCOL_GREEN		0x20
#define ATTR_BCOL_CYAN		0x30
#define ATTR_BCOL_RED		0x40
#define ATTR_BCOL_MAGENTA	0x50
#define ATTR_BCOL_YELLOW	0x60
#define ATTR_BCOL_WHITE		0x70

#define ATTR_DEFAULT		ATTR_FCOL_WHITE

/* Set default console usage if applicable */
#if ! ( defined ( CONSOLE_PCBIOS ) && CONSOLE_EXPLICIT ( CONSOLE_PCBIOS ) )
#undef CONSOLE_PCBIOS
#define CONSOLE_PCBIOS ( CONSOLE_USAGE_ALL & ~CONSOLE_USAGE_LOG )
#endif

/** Current character attribute */
static unsigned int bios_attr = ATTR_DEFAULT;

/** Keypress injection lock */
static uint8_t __text16 ( bios_inject_lock );
#define bios_inject_lock __use_text16 ( bios_inject_lock )

/** Vector for chaining to other INT 16 handlers */
static struct segoff __text16 ( int16_vector );
#define int16_vector __use_text16 ( int16_vector )

/** Assembly wrapper */
extern void int16_wrapper ( void );

/**
 * Handle ANSI CUP (cursor position)
 *
 * @v ctx		ANSI escape sequence context
 * @v count		Parameter count
 * @v params[0]		Row (1 is top)
 * @v params[1]		Column (1 is left)
 */
static void bios_handle_cup ( struct ansiesc_context *ctx __unused,
			      unsigned int count __unused, int params[] ) {
	int cx = ( params[1] - 1 );
	int cy = ( params[0] - 1 );

	if ( cx < 0 )
		cx = 0;
	if ( cy < 0 )
		cy = 0;

	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x10\n\t"
					   "cli\n\t" )
			       : : "a" ( 0x0200 ), "b" ( 1 ),
			           "d" ( ( cy << 8 ) | cx ) );
}

/**
 * Handle ANSI ED (erase in page)
 *
 * @v ctx		ANSI escape sequence context
 * @v count		Parameter count
 * @v params[0]		Region to erase
 */
static void bios_handle_ed ( struct ansiesc_context *ctx __unused,
			     unsigned int count __unused,
			     int params[] __unused ) {
	/* We assume that we always clear the whole screen */
	assert ( params[0] == ANSIESC_ED_ALL );

	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x10\n\t"
					   "cli\n\t" )
			       : : "a" ( 0x0600 ), "b" ( bios_attr << 8 ),
				   "c" ( 0 ),
				   "d" ( ( ( console_height - 1 ) << 8 ) |
					 ( console_width - 1 ) ) );
}

/**
 * Handle ANSI SGR (set graphics rendition)
 *
 * @v ctx		ANSI escape sequence context
 * @v count		Parameter count
 * @v params		List of graphic rendition aspects
 */
static void bios_handle_sgr ( struct ansiesc_context *ctx __unused,
			      unsigned int count, int params[] ) {
	static const uint8_t bios_attr_fcols[10] = {
		ATTR_FCOL_BLACK, ATTR_FCOL_RED, ATTR_FCOL_GREEN,
		ATTR_FCOL_YELLOW, ATTR_FCOL_BLUE, ATTR_FCOL_MAGENTA,
		ATTR_FCOL_CYAN, ATTR_FCOL_WHITE,
		ATTR_FCOL_WHITE, ATTR_FCOL_WHITE /* defaults */
	};
	static const uint8_t bios_attr_bcols[10] = {
		ATTR_BCOL_BLACK, ATTR_BCOL_RED, ATTR_BCOL_GREEN,
		ATTR_BCOL_YELLOW, ATTR_BCOL_BLUE, ATTR_BCOL_MAGENTA,
		ATTR_BCOL_CYAN, ATTR_BCOL_WHITE,
		ATTR_BCOL_BLACK, ATTR_BCOL_BLACK /* defaults */
	};
	unsigned int i;
	int aspect;

	for ( i = 0 ; i < count ; i++ ) {
		aspect = params[i];
		if ( aspect == 0 ) {
			bios_attr = ATTR_DEFAULT;
		} else if ( aspect == 1 ) {
			bios_attr |= ATTR_BOLD;
		} else if ( aspect == 5 ) {
			bios_attr |= ATTR_BLINK;
		} else if ( aspect == 22 ) {
			bios_attr &= ~ATTR_BOLD;
		} else if ( aspect == 25 ) {
			bios_attr &= ~ATTR_BLINK;
		} else if ( ( aspect >= 30 ) && ( aspect <= 39 ) ) {
			bios_attr &= ~ATTR_FCOL_MASK;
			bios_attr |= bios_attr_fcols[ aspect - 30 ];
		} else if ( ( aspect >= 40 ) && ( aspect <= 49 ) ) {
			bios_attr &= ~ATTR_BCOL_MASK;
			bios_attr |= bios_attr_bcols[ aspect - 40 ];
		}
	}
}

/**
 * Handle ANSI DECTCEM set (show cursor)
 *
 * @v ctx		ANSI escape sequence context
 * @v count		Parameter count
 * @v params		List of graphic rendition aspects
 */
static void bios_handle_dectcem_set ( struct ansiesc_context *ctx __unused,
				      unsigned int count __unused,
				      int params[] __unused ) {
	uint8_t height;

	/* Get character height */
	get_real ( height, BDA_SEG, BDA_CHAR_HEIGHT );

	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x10\n\t"
					   "cli\n\t" )
			       : : "a" ( 0x0100 ),
				   "c" ( ( ( height - 2 ) << 8 ) |
					 ( height - 1 ) ) );
}

/**
 * Handle ANSI DECTCEM reset (hide cursor)
 *
 * @v ctx		ANSI escape sequence context
 * @v count		Parameter count
 * @v params		List of graphic rendition aspects
 */
static void bios_handle_dectcem_reset ( struct ansiesc_context *ctx __unused,
					unsigned int count __unused,
					int params[] __unused ) {

	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x10\n\t"
					   "cli\n\t" )
			       : : "a" ( 0x0100 ), "c" ( 0x2000 ) );
}

/** BIOS console ANSI escape sequence handlers */
static struct ansiesc_handler bios_ansiesc_handlers[] = {
	{ ANSIESC_CUP, bios_handle_cup },
	{ ANSIESC_ED, bios_handle_ed },
	{ ANSIESC_SGR, bios_handle_sgr },
	{ ANSIESC_DECTCEM_SET, bios_handle_dectcem_set },
	{ ANSIESC_DECTCEM_RESET, bios_handle_dectcem_reset },
	{ 0, NULL }
};

/** BIOS console ANSI escape sequence context */
static struct ansiesc_context bios_ansiesc_ctx = {
	.handlers = bios_ansiesc_handlers,
};

/**
 * Print a character to BIOS console
 *
 * @v character		Character to be printed
 */
static void bios_putchar ( int character ) {
	int discard_a, discard_b, discard_c;

	/* Intercept ANSI escape sequences */
	character = ansiesc_process ( &bios_ansiesc_ctx, character );
	if ( character < 0 )
		return;

	/* Print character with attribute */
	__asm__ __volatile__ ( REAL_CODE ( "pushl %%ebp\n\t" /* gcc bug */
					   "sti\n\t"
					   /* Skip non-printable characters */
					   "cmpb $0x20, %%al\n\t"
					   "jb 1f\n\t"
					   /* Read attribute */
					   "movb %%al, %%cl\n\t"
					   "movb $0x08, %%ah\n\t"
					   "int $0x10\n\t"
					   "xchgb %%al, %%cl\n\t"
					   /* Skip if attribute matches */
					   "cmpb %%ah, %%bl\n\t"
					   "je 1f\n\t"
					   /* Set attribute */
					   "movw $0x0001, %%cx\n\t"
					   "movb $0x09, %%ah\n\t"
					   "int $0x10\n\t"
					   "\n1:\n\t"
					   /* Print character */
					   "xorw %%bx, %%bx\n\t"
					   "movb $0x0e, %%ah\n\t"
					   "int $0x10\n\t"
					   "cli\n\t"
					   "popl %%ebp\n\t" /* gcc bug */ )
			       : "=a" ( discard_a ), "=b" ( discard_b ),
			         "=c" ( discard_c )
			       : "a" ( character ), "b" ( bios_attr ) );
}

/**
 * Pointer to current ANSI output sequence
 *
 * While we are in the middle of returning an ANSI sequence for a
 * special key, this will point to the next character to return.  When
 * not in the middle of such a sequence, this will point to a NUL
 * (note: not "will be NULL").
 */
static const char *bios_ansi_input = "";

/** A BIOS key */
struct bios_key {
	/** Scancode */
	uint8_t scancode;
	/** Key code */
	uint16_t key;
} __attribute__ (( packed ));

/** Mapping from BIOS scan codes to iPXE key codes */
static const struct bios_key bios_keys[] = {
	{ 0x53, KEY_DC },
	{ 0x48, KEY_UP },
	{ 0x50, KEY_DOWN },
	{ 0x4b, KEY_LEFT },
	{ 0x4d, KEY_RIGHT },
	{ 0x47, KEY_HOME },
	{ 0x4f, KEY_END },
	{ 0x49, KEY_PPAGE },
	{ 0x51, KEY_NPAGE },
	{ 0x3f, KEY_F5 },
	{ 0x40, KEY_F6 },
	{ 0x41, KEY_F7 },
	{ 0x42, KEY_F8 },
	{ 0x43, KEY_F9 },
	{ 0x44, KEY_F10 },
	{ 0x85, KEY_F11 },
	{ 0x86, KEY_F12 },
};

/**
 * Get ANSI escape sequence corresponding to BIOS scancode
 *
 * @v scancode		BIOS scancode
 * @ret ansi_seq	ANSI escape sequence, if any, otherwise NULL
 */
static const char * bios_ansi_seq ( unsigned int scancode ) {
	static char buf[ 5 /* "[" + two digits + terminator + NUL */ ];
	unsigned int key;
	unsigned int terminator;
	unsigned int n;
	unsigned int i;
	char *tmp = buf;

	/* Construct ANSI escape sequence for scancode, if known */
	for ( i = 0 ; i < ( sizeof ( bios_keys ) /
			    sizeof ( bios_keys[0] ) ) ; i++ ) {

		/* Look for matching scancode */
		if ( bios_keys[i].scancode != scancode )
			continue;

		/* Construct escape sequence */
		key = bios_keys[i].key;
		n = KEY_ANSI_N ( key );
		terminator = KEY_ANSI_TERMINATOR ( key );
		*(tmp++) = '[';
		if ( n )
			tmp += sprintf ( tmp, "%d", n );
		*(tmp++) = terminator;
		*(tmp++) = '\0';
		assert ( tmp <= &buf[ sizeof ( buf ) ] );
		return buf;
	}

	DBG ( "Unrecognised BIOS scancode %02x\n", scancode );
	return NULL;
}

/**
 * Map a key
 *
 * @v character		Character read from console
 * @ret character	Mapped character
 */
static int bios_keymap ( unsigned int character ) {
	struct key_mapping *mapping;

	for_each_table_entry ( mapping, KEYMAP ) {
		if ( mapping->from == character )
			return mapping->to;
	}
	return character;
}

/**
 * Get character from BIOS console
 *
 * @ret character	Character read from console
 */
static int bios_getchar ( void ) {
	uint16_t keypress;
	unsigned int character;
	const char *ansi_seq;

	/* If we are mid-sequence, pass out the next byte */
	if ( ( character = *bios_ansi_input ) ) {
		bios_ansi_input++;
		return character;
	}

	/* Do nothing if injection is in progress */
	if ( bios_inject_lock )
		return 0;

	/* Read character from real BIOS console */
	bios_inject_lock++;
	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x16\n\t"
					   "cli\n\t" )
			       : "=a" ( keypress )
			       : "a" ( 0x1000 ), "m" ( bios_inject_lock ) );
	bios_inject_lock--;
	character = ( keypress & 0xff );

	/* If it's a normal character, just map and return it */
	if ( character && ( character < 0x80 ) )
		return bios_keymap ( character );

	/* Otherwise, check for a special key that we know about */
	if ( ( ansi_seq = bios_ansi_seq ( keypress >> 8 ) ) ) {
		/* Start of escape sequence: return ESC (0x1b) */
		bios_ansi_input = ansi_seq;
		return 0x1b;
	}

	return 0;
}

/**
 * Check for character ready to read from BIOS console
 *
 * @ret True		Character available to read
 * @ret False		No character available to read
 */
static int bios_iskey ( void ) {
	unsigned int discard_a;
	unsigned int flags;

	/* If we are mid-sequence, we are always ready */
	if ( *bios_ansi_input )
		return 1;

	/* Do nothing if injection is in progress */
	if ( bios_inject_lock )
		return 0;

	/* Otherwise check the real BIOS console */
	bios_inject_lock++;
	__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
					   "int $0x16\n\t"
					   "pushfw\n\t"
					   "popw %w0\n\t"
					   "cli\n\t" )
			       : "=R" ( flags ), "=a" ( discard_a )
			       : "a" ( 0x1100 ), "m" ( bios_inject_lock ) );
	bios_inject_lock--;
	return ( ! ( flags & ZF ) );
}

/** BIOS console */
struct console_driver bios_console __console_driver = {
	.putchar = bios_putchar,
	.getchar = bios_getchar,
	.iskey = bios_iskey,
	.usage = CONSOLE_PCBIOS,
};

/**
 * Inject keypresses
 *
 * @v ix86		Registers as passed to INT 16
 */
static __asmcall void bios_inject ( struct i386_all_regs *ix86 ) {
	unsigned int discard_a;
	unsigned int scancode;
	unsigned int i;
	uint16_t keypress;
	int key;

	/* If this is a blocking call, then loop until the
	 * non-blocking variant of the call indicates that a keypress
	 * is available.  Do this without acquiring the injection
	 * lock, so that injection may take place.
	 */
	if ( ( ix86->regs.ah & ~0x10 ) == 0x00 ) {
		__asm__ __volatile__ ( REAL_CODE ( "sti\n\t"
						   "\n1:\n\t"
						   "pushw %%ax\n\t"
						   "int $0x16\n\t"
						   "popw %%ax\n\t"
						   "jc 2f\n\t"
						   "jz 1b\n\t"
						   "\n2:\n\t"
						   "cli\n\t" )
				       : "=a" ( discard_a )
				       : "a" ( ix86->regs.eax | 0x0100 ),
					 "m" ( bios_inject_lock ) );
	}

	/* Acquire injection lock */
	bios_inject_lock++;

	/* Check for keypresses */
	if ( iskey() ) {

		/* Get key */
		key = getkey ( 0 );

		/* Reverse internal CR->LF mapping */
		if ( key == '\n' )
			key = '\r';

		/* Convert to keypress */
		keypress = ( ( key << 8 ) | key );

		/* Handle special keys */
		if ( key >= KEY_MIN ) {
			for ( i = 0 ; i < ( sizeof ( bios_keys ) /
					    sizeof ( bios_keys[0] ) ) ; i++ ) {
				if ( bios_keys[i].key == key ) {
					scancode = bios_keys[i].scancode;
					keypress = ( scancode << 8 );
					break;
				}
			}
		}

		/* Inject keypress */
		DBGC ( &bios_console, "BIOS injecting keypress %04x\n",
		       keypress );
		__asm__ __volatile__ ( REAL_CODE ( "int $0x16\n\t" )
				       : "=a" ( discard_a )
				       : "a" ( 0x0500 ), "c" ( keypress ),
					 "m" ( bios_inject_lock ) );
	}

	/* Release injection lock */
	bios_inject_lock--;
}

/**
 * Start up keypress injection
 *
 */
static void bios_inject_startup ( void ) {

	/* Assembly wrapper to call bios_inject() */
	__asm__ __volatile__ (
		TEXT16_CODE ( "\nint16_wrapper:\n\t"
			      "pushfw\n\t"
			      "cmpb $0, %cs:bios_inject_lock\n\t"
			      "jnz 1f\n\t"
			      VIRT_CALL ( bios_inject )
			      "\n1:\n\t"
			      "popfw\n\t"
			      "ljmp *%cs:int16_vector\n\t" ) );

	/* Hook INT 16 */
	hook_bios_interrupt ( 0x16, ( ( intptr_t ) int16_wrapper ),
			      &int16_vector );
}

/**
 * Shut down keypress injection
 *
 * @v booting		System is shutting down for OS boot
 */
static void bios_inject_shutdown ( int booting __unused ) {

	/* Unhook INT 16 */
	unhook_bios_interrupt ( 0x16, ( ( intptr_t ) int16_wrapper ),
				&int16_vector );
}

/** Keypress injection startup function */
struct startup_fn bios_inject_startup_fn __startup_fn ( STARTUP_NORMAL ) = {
	.startup = bios_inject_startup,
	.shutdown = bios_inject_shutdown,
};
