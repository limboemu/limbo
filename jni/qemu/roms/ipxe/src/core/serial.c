/*
 * The serial port interface routines implement a simple polled i/o
 * interface to a standard serial port.  Due to the space restrictions
 * for the boot blocks, no BIOS support is used (since BIOS requires
 * expensive real/protected mode switches), instead the rudimentary
 * BIOS support is duplicated here.
 *
 * The base address and speed for the i/o port are passed from the
 * Makefile in the COMCONSOLE and CONSPEED preprocessor macros.  The
 * line control parameters are currently hard-coded to 8 bits, no
 * parity, 1 stop bit (8N1).  This can be changed in init_serial().
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include "stddef.h"
#include <ipxe/init.h>
#include <ipxe/io.h>
#include <unistd.h>
#include <ipxe/serial.h>
#include "config/serial.h"

/* Set default values if none specified */

#ifndef COMCONSOLE
#define COMCONSOLE	0x3f8
#endif

#ifndef COMSPEED
#define COMSPEED	9600
#endif

#ifndef COMDATA
#define COMDATA		8
#endif

#ifndef COMPARITY
#define COMPARITY	0
#endif

#ifndef COMSTOP
#define COMSTOP		1
#endif

#undef UART_BASE
#define UART_BASE ( COMCONSOLE )

#undef UART_BAUD
#define UART_BAUD ( COMSPEED )

#if ((115200%UART_BAUD) != 0)
#error Bad ttys0 baud rate
#endif

#define COMBRD (115200/UART_BAUD)

/* Line Control Settings */
#define UART_LCS ( ( ( (COMDATA) - 5 )	<< 0 ) | \
		   ( ( (COMPARITY) )	<< 3 ) | \
		   ( ( (COMSTOP) - 1 )	<< 2 ) )

/* Data */
#define UART_RBR 0x00
#define UART_TBR 0x00

/* Control */
#define UART_IER 0x01
#define UART_IIR 0x02
#define UART_FCR 0x02
#define UART_LCR 0x03
#define UART_MCR 0x04
#define UART_DLL 0x00
#define UART_DLM 0x01

/* Status */
#define UART_LSR 0x05
#define  UART_LSR_TEMPT 0x40	/* Transmitter empty */
#define  UART_LSR_THRE  0x20	/* Transmit-hold-register empty */
#define  UART_LSR_BI	0x10	/* Break interrupt indicator */
#define  UART_LSR_FE	0x08	/* Frame error indicator */
#define  UART_LSR_PE	0x04	/* Parity error indicator */
#define  UART_LSR_OE	0x02	/* Overrun error indicator */
#define  UART_LSR_DR	0x01	/* Receiver data ready */

#define UART_MSR 0x06
#define UART_SCR 0x07

#if defined(UART_MEM)
#define uart_readb(addr) readb((addr))
#define uart_writeb(val,addr) writeb((val),(addr))
#else
#define uart_readb(addr) inb((addr))
#define uart_writeb(val,addr) outb((val),(addr))
#endif

/* Boolean for the state of serial driver initialization */
int serial_initialized = 0;

/*
 * void serial_putc(int ch);
 *	Write character `ch' to port UART_BASE.
 */
void serial_putc ( int ch ) {
	int i;
	int status;
	i = 1000; /* timeout */
	while(--i > 0) {
		status = uart_readb(UART_BASE + UART_LSR);
		if (status & UART_LSR_THRE) { 
			/* TX buffer emtpy */
			uart_writeb(ch, UART_BASE + UART_TBR);
			break;
		}
		mdelay(2);
	}
}

/*
 * int serial_getc(void);
 *	Read a character from port UART_BASE.
 */
int serial_getc ( void ) {
	int status;
	int ch;
	do {
		status = uart_readb(UART_BASE + UART_LSR);
	} while((status & 1) == 0);
	ch = uart_readb(UART_BASE + UART_RBR);	/* fetch (first) character */
	ch &= 0x7f;				/* remove any parity bits we get */
	if (ch == 0x7f) {			/* Make DEL... look like BS */
		ch = 0x08;
	}
	return ch;
}

/*
 * int serial_ischar(void);
 *       If there is a character in the input buffer of port UART_BASE,
 *       return nonzero; otherwise return 0.
 */
int serial_ischar ( void ) {
	int status;
	status = uart_readb(UART_BASE + UART_LSR);	/* line status reg; */
	return status & 1;		/* rx char available */
}

/*
 * int serial_init(void);
 *	Initialize port UART_BASE to speed COMSPEED, line settings 8N1.
 */
static void serial_init ( void ) {
	int status;
	int divisor, lcs;

	DBG ( "Serial port %#x initialising\n", UART_BASE );

	divisor = COMBRD;
	lcs = UART_LCS;


#ifdef COMPRESERVE
	lcs = uart_readb(UART_BASE + UART_LCR) & 0x7f;
	uart_writeb(0x80 | lcs, UART_BASE + UART_LCR);
	divisor = (uart_readb(UART_BASE + UART_DLM) << 8) | uart_readb(UART_BASE + UART_DLL);
	uart_writeb(lcs, UART_BASE + UART_LCR);
#endif

	/* Set Baud Rate Divisor to COMSPEED, and test to see if the
	 * serial port appears to be present.
	 */
	uart_writeb(0x80 | lcs, UART_BASE + UART_LCR);
	uart_writeb(0xaa, UART_BASE + UART_DLL);
	if (uart_readb(UART_BASE + UART_DLL) != 0xaa) {
		DBG ( "Serial port %#x UART_DLL failed\n", UART_BASE );
		goto out;
	}
	uart_writeb(0x55, UART_BASE + UART_DLL);
	if (uart_readb(UART_BASE + UART_DLL) != 0x55) {
		DBG ( "Serial port %#x UART_DLL failed\n", UART_BASE );
		goto out;
	}
	uart_writeb(divisor & 0xff, UART_BASE + UART_DLL);
	if (uart_readb(UART_BASE + UART_DLL) != (divisor & 0xff)) {
		DBG ( "Serial port %#x UART_DLL failed\n", UART_BASE );
		goto out;
	}
	uart_writeb(0xaa, UART_BASE + UART_DLM);
	if (uart_readb(UART_BASE + UART_DLM) != 0xaa) {
		DBG ( "Serial port %#x UART_DLM failed\n", UART_BASE );
		goto out;
	}
	uart_writeb(0x55, UART_BASE + UART_DLM);
	if (uart_readb(UART_BASE + UART_DLM) != 0x55) {
		DBG ( "Serial port %#x UART_DLM failed\n", UART_BASE );
		goto out;
	}
	uart_writeb((divisor >> 8) & 0xff, UART_BASE + UART_DLM);
	if (uart_readb(UART_BASE + UART_DLM) != ((divisor >> 8) & 0xff)) {
		DBG ( "Serial port %#x UART_DLM failed\n", UART_BASE );
		goto out;
	}
	uart_writeb(lcs, UART_BASE + UART_LCR);
	
	/* disable interrupts */
	uart_writeb(0x0, UART_BASE + UART_IER);

	/* enable fifos */
	uart_writeb(0x01, UART_BASE + UART_FCR);

	/* Set clear to send, so flow control works... */
	uart_writeb((1<<1), UART_BASE + UART_MCR);

	/* Flush the input buffer. */
	do {
		/* rx buffer reg
		 * throw away (unconditionally the first time)
		 */
	        (void) uart_readb(UART_BASE + UART_RBR);
		/* line status reg */
		status = uart_readb(UART_BASE + UART_LSR);
	} while(status & UART_LSR_DR);

	/* Note that serial support has been initialized */
	serial_initialized = 1;
 out:
	return;
}

/*
 * void serial_fini(void);
 *	Cleanup our use of the serial port, in particular flush the
 *	output buffer so we don't accidentially lose characters.
 */
static void serial_fini ( int flags __unused ) {
	int i, status;
	/* Flush the output buffer to avoid dropping characters,
	 * if we are reinitializing the serial port.
	 */
	i = 10000; /* timeout */
	do {
		status = uart_readb(UART_BASE + UART_LSR);
	} while((--i > 0) && !(status & UART_LSR_TEMPT));
	/* Don't mark it as disabled; it's still usable */
}

/**
 * Serial driver initialisation function
 *
 * Initialise serial port early on so that it is available to capture
 * early debug messages.
 */
struct init_fn serial_init_fn __init_fn ( INIT_SERIAL ) = {
	.initialise = serial_init,
};

/** Serial driver startup function */
struct startup_fn serial_startup_fn __startup_fn ( STARTUP_EARLY ) = {
	.shutdown = serial_fini,
};
