#ifndef GDBMACH_H
#define GDBMACH_H

/** @file
 *
 * GDB architecture specifics
 *
 * This file declares functions for manipulating the machine state and
 * debugging context.
 *
 */

#include <stdint.h>

typedef unsigned long gdbreg_t;

/* Register snapshot */
enum {
	GDBMACH_RAX,
	GDBMACH_RBX,
	GDBMACH_RCX,
	GDBMACH_RDX,
	GDBMACH_RSI,
	GDBMACH_RDI,
	GDBMACH_RBP,
	GDBMACH_RSP,
	GDBMACH_R8,
	GDBMACH_R9,
	GDBMACH_R10,
	GDBMACH_R11,
	GDBMACH_R12,
	GDBMACH_R13,
	GDBMACH_R14,
	GDBMACH_R15,
	GDBMACH_RIP,
	GDBMACH_RFLAGS,
	GDBMACH_CS,
	GDBMACH_SS,
	GDBMACH_DS,
	GDBMACH_ES,
	GDBMACH_FS,
	GDBMACH_GS,
	GDBMACH_NREGS,
};

#define GDBMACH_SIZEOF_REGS ( GDBMACH_NREGS * sizeof ( gdbreg_t ) )

/* Breakpoint types */
enum {
	GDBMACH_BPMEM,
	GDBMACH_BPHW,
	GDBMACH_WATCH,
	GDBMACH_RWATCH,
	GDBMACH_AWATCH,
};

/* Exception vectors */
extern void gdbmach_sigfpe ( void );
extern void gdbmach_sigtrap ( void );
extern void gdbmach_sigstkflt ( void );
extern void gdbmach_sigill ( void );

static inline void gdbmach_set_pc ( gdbreg_t *regs, gdbreg_t pc ) {
	regs[GDBMACH_RIP] = pc;
}

static inline void gdbmach_set_single_step ( gdbreg_t *regs, int step ) {
	regs[GDBMACH_RFLAGS] &= ~( 1 << 8 ); /* Trace Flag (TF) */
	regs[GDBMACH_RFLAGS] |= ( step << 8 );
}

static inline void gdbmach_breakpoint ( void ) {
	__asm__ __volatile__ ( "int $3\n" );
}

extern int gdbmach_set_breakpoint ( int type, unsigned long addr, size_t len,
				    int enable );
extern void gdbmach_init ( void );

#endif /* GDBMACH_H */
