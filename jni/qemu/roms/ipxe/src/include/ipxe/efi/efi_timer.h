#ifndef _IPXE_EFI_TIMER_H
#define _IPXE_EFI_TIMER_H

/** @file
 *
 * iPXE timer API for EFI
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#ifdef TIMER_EFI
#define TIMER_PREFIX_efi
#else
#define TIMER_PREFIX_efi __efi_
#endif

/**
 * Number of ticks per second
 *
 * This is a policy decision.
 */
#define EFI_TICKS_PER_SEC 20

/**
 * Get number of ticks per second
 *
 * @ret ticks_per_sec	Number of ticks per second
 */
static inline __attribute__ (( always_inline )) unsigned long
TIMER_INLINE ( efi, ticks_per_sec ) ( void ) {

	return EFI_TICKS_PER_SEC;
}

#endif /* _IPXE_EFI_TIMER_H */
