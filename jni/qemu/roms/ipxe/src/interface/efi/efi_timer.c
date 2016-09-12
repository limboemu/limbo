/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ipxe/timer.h>
#include <ipxe/init.h>
#include <ipxe/efi/efi.h>

/** @file
 *
 * iPXE timer API for EFI
 *
 */

/** Current tick count */
static unsigned long efi_jiffies;

/** Timer tick event */
static EFI_EVENT efi_tick_event;

/** Colour for debug messages */
#define colour &efi_jiffies

/**
 * Delay for a fixed number of microseconds
 *
 * @v usecs		Number of microseconds for which to delay
 */
static void efi_udelay ( unsigned long usecs ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_STATUS efirc;
	int rc;

	if ( ( efirc = bs->Stall ( usecs ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( colour, "EFI could not delay for %ldus: %s\n",
		       usecs, strerror ( rc ) );
		/* Probably screwed */
	}
}

/**
 * Get current system time in ticks
 *
 * @ret ticks		Current time, in ticks
 */
static unsigned long efi_currticks ( void ) {

	return efi_jiffies;
}

/**
 * Timer tick
 *
 * @v event		Timer tick event
 * @v context		Event context
 */
static EFIAPI void efi_tick ( EFI_EVENT event __unused,
			      void *context __unused ) {

	/* Increment tick count */
	efi_jiffies++;
}

/**
 * Start timer tick
 *
 */
static void efi_tick_startup ( void ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_STATUS efirc;
	int rc;

	/* Create timer tick event */
	if ( ( efirc = bs->CreateEvent ( ( EVT_TIMER | EVT_NOTIFY_SIGNAL ),
					 TPL_CALLBACK, efi_tick, NULL,
					 &efi_tick_event ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( colour, "EFI could not create timer tick: %s\n",
		       strerror ( rc ) );
		/* Nothing we can do about it */
		return;
	}

	/* Start timer tick */
	if ( ( efirc = bs->SetTimer ( efi_tick_event, TimerPeriodic,
				      ( 10000000 / EFI_TICKS_PER_SEC ) ) ) !=0){
		rc = -EEFI ( efirc );
		DBGC ( colour, "EFI could not start timer tick: %s\n",
		       strerror ( rc ) );
		/* Nothing we can do about it */
		return;
	}
	DBGC ( colour, "EFI timer started at %d ticks per second\n",
	       EFI_TICKS_PER_SEC );
}

/**
 * Stop timer tick
 *
 * @v booting		System is shutting down in order to boot
 */
static void efi_tick_shutdown ( int booting __unused ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_STATUS efirc;
	int rc;

	/* Stop timer tick */
	if ( ( efirc = bs->SetTimer ( efi_tick_event, TimerCancel, 0 ) ) != 0 ){
		rc = -EEFI ( efirc );
		DBGC ( colour, "EFI could not stop timer tick: %s\n",
		       strerror ( rc ) );
		/* Self-destruct initiated */
		return;
	}
	DBGC ( colour, "EFI timer stopped\n" );

	/* Destroy timer tick event */
	if ( ( efirc = bs->CloseEvent ( efi_tick_event ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( colour, "EFI could not destroy timer tick: %s\n",
		       strerror ( rc ) );
		/* Probably non-fatal */
		return;
	}
}

/** Timer tick startup function */
struct startup_fn efi_tick_startup_fn __startup_fn ( STARTUP_EARLY ) = {
	.startup = efi_tick_startup,
	.shutdown = efi_tick_shutdown,
};

PROVIDE_TIMER ( efi, udelay, efi_udelay );
PROVIDE_TIMER ( efi, currticks, efi_currticks );
PROVIDE_TIMER_INLINE ( efi, ticks_per_sec );
