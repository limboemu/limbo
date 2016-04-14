/*
 * arch/i386/core/i386_timer.c
 *
 * Use the "System Timer 2" to implement the udelay callback in
 * the BIOS timer driver. Also used to calibrate the clock rate
 * in the RTDSC timer driver.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stddef.h>
#include <ipxe/timer2.h>
#include <ipxe/io.h>

/* Timers tick over at this rate */
#define TIMER2_TICKS_PER_SEC	1193180U

/* Parallel Peripheral Controller Port B */
#define	PPC_PORTB	0x61

/* Meaning of the port bits */
#define	PPCB_T2OUT	0x20	/* Bit 5 */
#define	PPCB_SPKR	0x02	/* Bit 1 */
#define	PPCB_T2GATE	0x01	/* Bit 0 */

/* Ports for the 8254 timer chip */
#define	TIMER2_PORT	0x42
#define	TIMER_MODE_PORT	0x43

/* Meaning of the mode bits */
#define	TIMER0_SEL	0x00
#define	TIMER1_SEL	0x40
#define	TIMER2_SEL	0x80
#define	READBACK_SEL	0xC0

#define	LATCH_COUNT	0x00
#define	LOBYTE_ACCESS	0x10
#define	HIBYTE_ACCESS	0x20
#define	WORD_ACCESS	0x30

#define	MODE0		0x00
#define	MODE1		0x02
#define	MODE2		0x04
#define	MODE3		0x06
#define	MODE4		0x08
#define	MODE5		0x0A

#define	BINARY_COUNT	0x00
#define	BCD_COUNT	0x01

static void load_timer2 ( unsigned int ticks ) {
	/*
	 * Now let's take care of PPC channel 2
	 *
	 * Set the Gate high, program PPC channel 2 for mode 0,
	 * (interrupt on terminal count mode), binary count,
	 * load 5 * LATCH count, (LSB and MSB) to begin countdown.
	 *
	 * Note some implementations have a bug where the high bits byte
	 * of channel 2 is ignored.
	 */
	/* Set up the timer gate, turn off the speaker */
	/* Set the Gate high, disable speaker */
	outb((inb(PPC_PORTB) & ~PPCB_SPKR) | PPCB_T2GATE, PPC_PORTB);
	/* binary, mode 0, LSB/MSB, Ch 2 */
	outb(TIMER2_SEL|WORD_ACCESS|MODE0|BINARY_COUNT, TIMER_MODE_PORT);
	/* LSB of ticks */
	outb(ticks & 0xFF, TIMER2_PORT);
	/* MSB of ticks */
	outb(ticks >> 8, TIMER2_PORT);
}

static int timer2_running ( void ) {
	return ((inb(PPC_PORTB) & PPCB_T2OUT) == 0);
}

void timer2_udelay ( unsigned long usecs ) {
	load_timer2 ( ( usecs * TIMER2_TICKS_PER_SEC ) / ( 1000 * 1000 ) );
	while (timer2_running()) {
		/* Do nothing */
	}
}
