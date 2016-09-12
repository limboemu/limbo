#include "cpustate.h"

	.globl	sparc64_of_client_interface, client_tba


/*
 * SAVE_WINDOW_STATE and RESTORE_WINDOW_STATE are used to ensure
 * that the CPU window state is preserved across CIF calls. This is
 * to workaround a *BSD restriction that window fill/spill traps must
 * be minimised during trap table takeover, and likely emulates the
 * behaviour of OBP.
 */

	.data
	.align	8

client_stack:
	.xword	0
client_tba:
	.xword	0
client_window:
	.skip	2048


	.text
	.align	4
        .register %g2, #scratch
        .register %g3, #scratch
        .register %g6, #scratch
        .register %g7, #scratch
/*
	make some more space on stack since linux kernel only provides 128 bytes
	without memory to spill registers (used by gcc in -O0 mode)
*/

sparc64_of_client_interface:

	/* Save globals on callers stack */
	add	%sp, -248, %sp

	stx	%g1, [%sp + 2047 + 192]
	stx	%g2, [%sp + 2047 + 200]
	stx	%g3, [%sp + 2047 + 208]
	stx	%g4, [%sp + 2047 + 216]
	stx	%g5, [%sp + 2047 + 224]
	stx	%g6, [%sp + 2047 + 232]
	stx	%g7, [%sp + 2047 + 240]

	/* Save client trap table */
	setx	client_tba, %g6, %g7
	rdpr	%tba, %g6
	stx	%g6, [%g7]

	/* Save existing stack */
	setx	client_stack, %g6, %g7
	stx	%sp, [%g7]

	/* Save windows */
	setx	_fcstack_ptr, %g6, %g7
	ldx	[%g7], %g1
	add	%g1, -CONTEXT_STATE_SIZE, %g1
	stx	%g1, [%g7]
	
	SAVE_CPU_WINDOW_STATE(cif)

	/* Move to OpenBIOS context stack */
	setx	_fcstack_ptr, %g6, %g7
	ldx	[%g7], %g6
	setx	CONTEXT_STACK_SIZE, %g4, %g5
	sub	%g6, %g5, %g6
	stx	%g6, [%g7]
	
	setx	- 2047 - 192, %g6, %g7
	add	%g1, %g7, %g7
	mov	%g7, %sp

	/* Call client inteface */
	call of_client_interface
	 ldx	[%g1 + 0x30], %o0

	/* Restore windows */
	setx	_fcstack_ptr, %g6, %g7
	ldx	[%g7], %g1
	setx	CONTEXT_STACK_SIZE, %g4, %g5
	add	%g1, %g5, %g1
	stx	%g1, [%g7]
	
	/* Return value */
	stx	%o0, [%g1 + 0x30]
	
	RESTORE_CPU_WINDOW_STATE(cif)
	
	add	%g1, CONTEXT_STATE_SIZE, %g1
	setx	_fcstack_ptr, %g6, %g7
	stx	%g1, [%g7]
	
	/* Restore stack */
	setx	client_stack, %g6, %g7
	ldx	[%g7], %sp

	/* Restore client trap table */
	setx	client_tba, %g6, %g7
	ldx	[%g7], %g6
	wrpr	%g6, %tba

	/* Restore globals */
	ldx	[%sp + 2047 + 192], %g1
	ldx	[%sp + 2047 + 200], %g2
	ldx	[%sp + 2047 + 208], %g3
	ldx	[%sp + 2047 + 216], %g4
	ldx	[%sp + 2047 + 224], %g5
	ldx	[%sp + 2047 + 232], %g6
	ldx	[%sp + 2047 + 240], %g7

	add	%sp, 248, %sp

	jmp	%o7+8
	 nop
