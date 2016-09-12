#ifndef _SETJMP_H
#define _SETJMP_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>

/** A jump buffer */
typedef struct {
	/** Saved x19 */
	uint64_t x19;
	/** Saved x20 */
	uint64_t x20;
	/** Saved x21 */
	uint64_t x21;
	/** Saved x22 */
	uint64_t x22;
	/** Saved x23 */
	uint64_t x23;
	/** Saved x24 */
	uint64_t x24;
	/** Saved x25 */
	uint64_t x25;
	/** Saved x26 */
	uint64_t x26;
	/** Saved x27 */
	uint64_t x27;
	/** Saved x28 */
	uint64_t x28;
	/** Saved frame pointer (x29) */
	uint64_t x29;
	/** Saved link register (x30) */
	uint64_t x30;
	/** Saved stack pointer (x31) */
	uint64_t sp;
} jmp_buf[1];

extern int __asmcall __attribute__ (( returns_twice ))
setjmp ( jmp_buf env );

extern void __asmcall __attribute__ (( noreturn ))
longjmp ( jmp_buf env, int val );

#endif /* _SETJMP_H */
