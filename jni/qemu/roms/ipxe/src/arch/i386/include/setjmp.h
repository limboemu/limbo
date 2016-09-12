#ifndef _SETJMP_H
#define _SETJMP_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>

/** A jump buffer */
typedef struct {
	/** Saved return address */
	uint32_t retaddr;
	/** Saved stack pointer */
	uint32_t stack;
	/** Saved %ebx */
	uint32_t ebx;
	/** Saved %esi */
	uint32_t esi;
	/** Saved %edi */
	uint32_t edi;
	/** Saved %ebp */
	uint32_t ebp;
} jmp_buf[1];

extern int __asmcall __attribute__ (( returns_twice ))
setjmp ( jmp_buf env );

extern void __asmcall __attribute__ (( noreturn ))
longjmp ( jmp_buf env, int val );

#endif /* _SETJMP_H */
