/*
 *   Save/restore CPU state macros
 *
 *   Copyright (C) 2015 Mark Cave-Ayland (mark.cave-ayland@ilande.co.uk>)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

/* State size for context (see below) */
#define CONTEXT_STATE_SIZE 0x510

/* Stack size for context (allocated inline of the context stack) */
#define CONTEXT_STACK_SIZE 0x2000

/*
 * SAVE_CPU_STATE and RESTORE_CPU_STATE are macros used to enable a context switch
 * to C to occur within the MMU I/D TLB miss handlers.
 *
 * Because these handlers are called on a TLB miss, we cannot use flushw to store
 * processor window state on the stack, as the memory areas used by each window's
 * stack pointer may not be in the TLB, causing recursive TLB miss traps.
 *
 * For this reason, we save window state by manually rotating the window registers
 * and saving their contents (along with other vital registers) into a special
 * tlb_handler_stack defined above which is guaranteed to be locked in the TLB, and
 * so won't cause issues with trap recursion.
 *
 * Once this process is complete, we remain in a TL=0, CWP=0 state (with IE=1 to allow
 * window fill/spill traps if required), switch to our safe tlb_handler_stack and 
 * invoke the miss handler.
 */

#define SAVE_CPU_WINDOW_STATE(type) \
	/* Save window state into context at %g1 */ \
	rdpr	%cwp, %g7; \
	stx	%g7, [%g1]; \
	rdpr	%cansave, %g7; \
	stx	%g7, [%g1 + 0x8]; \
	rdpr	%canrestore, %g7; \
	stx	%g7, [%g1 + 0x10]; \
	rdpr	%otherwin, %g7; \
	stx	%g7, [%g1 + 0x18]; \
	rdpr	%wstate, %g7; \
	stx	%g7, [%g1 + 0x20]; \
	rdpr	%cleanwin, %g7; \
	stx	%g7, [%g1 + 0x28]; \
	\
	stx	%o0, [%g1 + 0x30]; \
	stx	%o1, [%g1 + 0x38]; \
	stx	%o2, [%g1 + 0x40]; \
	stx	%o3, [%g1 + 0x48]; \
	stx	%o4, [%g1 + 0x50]; \
	stx	%o5, [%g1 + 0x58]; \
	stx	%o6, [%g1 + 0x60]; \
	stx	%o7, [%g1 + 0x68]; \
	\
	rdpr	%pstate, %g7; \
	stx	%g7, [%g1 + 0x70]; \
	rd	%y, %g7; \
	stx	%g7, [%g1 + 0x78]; \
	rd	%fprs, %g7; \
	stx	%g7, [%g1 + 0x80]; \
	rdpr    %tl, %g7; \
	stx     %g7, [%g1 + 0x88]; \
	\
	/* Now iterate through all of the windows saving all l and i registers */ \
	add	%g1, 0x90, %g5; \
	\
	/* Get the number of windows in %g6 */ \
	rdpr	%ver, %g6; \
	and	%g6, 0xf, %g6; \
	\
	mov     %g6, %g4; \
	inc     %g4; \
	\
	/* Starting cwp in g7 */ \
	rdpr    %cwp, %g7; \
	\
save_cpu_window_##type: \
	wrpr    %g7, %cwp; \
	stx	%l0, [%g5]; \
	stx	%l1, [%g5 + 0x8]; \
	stx	%l2, [%g5 + 0x10]; \
	stx	%l3, [%g5 + 0x18]; \
	stx	%l4, [%g5 + 0x20]; \
	stx	%l5, [%g5 + 0x28]; \
	stx	%l6, [%g5 + 0x30]; \
	stx	%l7, [%g5 + 0x38]; \
	stx	%i0, [%g5 + 0x40]; \
	stx	%i1, [%g5 + 0x48]; \
	stx	%i2, [%g5 + 0x50]; \
	stx	%i3, [%g5 + 0x58]; \
	stx	%i4, [%g5 + 0x60]; \
	stx	%i5, [%g5 + 0x68]; \
	stx	%i6, [%g5 + 0x70]; \
	stx	%i7, [%g5 + 0x78]; \
	dec	%g7; \
	and	%g7, %g6, %g7; \
	subcc	%g4, 1, %g4; \
	bne	save_cpu_window_##type; \
	 add	%g5, 0x80, %g5; \
	\
	/* For 8 windows with 16 registers to save in the window, memory required \
	is 16*8*8 = 0x400 bytes */ \
	\
	/* Now we should be in window 0 so update the other window registers */ \
	rdpr	%ver, %g6; \
	and	%g6, 0xf, %g6; \
	dec	%g6; \
	wrpr	%g6, %cansave; \
	\
	wrpr	%g0, %cleanwin; \
	wrpr	%g0, %canrestore; \
	wrpr	%g0, %otherwin; \

	
#define SAVE_CPU_TRAP_STATE(type) \
	/* Save trap state into context at %g1 */ \
	add	%g1, 0x490, %g5; \
	mov	4, %g6; \
	\
save_trap_state_##type: \
	deccc	%g6; \
	wrpr	%g6, %tl; \
	rdpr	%tpc, %g7; \
	stx	%g7, [%g5]; \
	rdpr	%tnpc, %g7; \
	stx	%g7, [%g5 + 0x8]; \
	rdpr	%tstate, %g7; \
	stx	%g7, [%g5 + 0x10]; \
	rdpr	%tt, %g7; \
	stx	%g7, [%g5 + 0x18]; \
	bne	save_trap_state_##type; \
	 add	%g5, 0x20, %g5; \
	\
	/* For 4 trap levels with 4 registers, memory required is \
	4*8*4 = 0x80 bytes */

/* Save all state into context at %g1 */
#define SAVE_CPU_STATE(type) \
	SAVE_CPU_WINDOW_STATE(type); \
	SAVE_CPU_TRAP_STATE(type);


#define RESTORE_CPU_WINDOW_STATE(type) \
	/* Restore window state from context at %g1 */ \
	\
	/* Get the number of windows in %g6 */ \
	rdpr	%ver, %g6; \
	and	%g6, 0xf, %g6; \
	\
	mov	%g6, %g4; \
	inc	%g4; \
	\
	/* Set starting window */ \
	ldx	[%g1], %g7; \
	\
	/* Now iterate through all of the windows restoring all l and i registers */ \
	add	%g1, 0x90, %g5; \
	\
restore_cpu_window_##type: \
	wrpr	%g7, %cwp; \
	ldx	[%g5], %l0; \
	ldx	[%g5 + 0x8], %l1; \
	ldx	[%g5 + 0x10], %l2; \
	ldx	[%g5 + 0x18], %l3; \
	ldx	[%g5 + 0x20], %l4; \
	ldx	[%g5 + 0x28], %l5; \
	ldx	[%g5 + 0x30], %l6; \
	ldx	[%g5 + 0x38], %l7; \
	ldx	[%g5 + 0x40], %i0; \
	ldx	[%g5 + 0x48], %i1; \
	ldx	[%g5 + 0x50], %i2; \
	ldx	[%g5 + 0x58], %i3; \
	ldx	[%g5 + 0x60], %i4; \
	ldx	[%g5 + 0x68], %i5; \
	ldx	[%g5 + 0x70], %i6; \
	ldx	[%g5 + 0x78], %i7; \
	dec	%g7; \
	and	%g7, %g6, %g7; \
	subcc	%g4, 1, %g4; \
	bne	restore_cpu_window_##type; \
	 add	%g5, 0x80, %g5; \
	\
	/* Restore the window registers to their original value */ \
	ldx	[%g1], %g7; \
	wrpr	%g7, %cwp; \
	ldx	[%g1 + 0x8], %g7; \
	wrpr	%g7, %cansave; \
	ldx	[%g1 + 0x10], %g7; \
	wrpr	%g7, %canrestore; \
	ldx	[%g1 + 0x18], %g7; \
	wrpr	%g7, %otherwin; \
	ldx	[%g1 + 0x20], %g7; \
	wrpr	%g7, %wstate; \
	ldx	[%g1 + 0x28], %g7; \
	wrpr	%g7, %cleanwin; \
	\
	ldx	[%g1 + 0x30], %o0; \
	ldx	[%g1 + 0x38], %o1; \
	ldx	[%g1 + 0x40], %o2; \
	ldx	[%g1 + 0x48], %o3; \
	ldx	[%g1 + 0x50], %o4; \
	ldx	[%g1 + 0x58], %o5; \
	ldx	[%g1 + 0x60], %o6; \
	ldx	[%g1 + 0x68], %o7; \
	\
	ldx	[%g1 + 0x70], %g7; \
	wrpr	%g7, %pstate; \
	ldx	[%g1 + 0x78], %g7; \
	wr	%g7, 0, %y; \
	ldx	[%g1 + 0x80], %g7; \
	wr	%g7, 0, %fprs; \


#define RESTORE_CPU_TRAP_STATE(type) \
	/* Restore trap state from context at %g1 */ \
	add	%g1, 0x490, %g5; \
	mov	4, %g6; \
	\
restore_trap_state_##type: \
	deccc	%g6; \
	wrpr	%g6, %tl; \
	ldx	[%g5], %g7; \
	wrpr	%g7, %tpc; \
	ldx	[%g5 + 0x8], %g7; \
	wrpr	%g7, %tnpc; \
	ldx	[%g5 + 0x10], %g7; \
	wrpr	%g7, %tstate; \
	ldx	[%g5 + 0x18], %g7; \
	wrpr	%g7, %tt; \
	bne	restore_trap_state_##type; \
	 add	%g5, 0x20, %g5; \
	\
	ldx	[%g1 + 0x88], %g7; \
	wrpr	%g7, %tl

/* Restore all state from context at %g1 */
#define RESTORE_CPU_STATE(type) \
	RESTORE_CPU_WINDOW_STATE(type); \
	RESTORE_CPU_TRAP_STATE(type);
