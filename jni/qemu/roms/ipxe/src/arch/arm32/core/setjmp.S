FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

	.text
	.arm

/*
 * Save stack context for non-local goto
 */
	.globl	setjmp
	.type	setjmp, %function
setjmp:
	/* Store registers */
	stmia	r0, { r4, r5, r6, r7, r8, r9, r10, fp, sp, lr }
	/* Return 0 when returning as setjmp() */
	mov	r0, #0
	bx	lr
	.size	setjmp, . - setjmp

/*
 * Non-local jump to a saved stack context
 */
	.globl	longjmp
	.type	longjmp, %function
longjmp:
	/* Restore registers */
	ldmia	r0, { r4, r5, r6, r7, r8, r9, r10, fp, sp, lr }
	/* Force result to non-zero */
	movs	r0, r1
	moveq	r0, #1
	/* Return to setjmp() caller */
	bx	lr
	.size	longjmp, . - longjmp
