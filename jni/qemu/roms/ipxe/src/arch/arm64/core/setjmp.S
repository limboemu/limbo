FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

	.text

	/* Must match jmp_buf structure layout */
	.struct	0
env_x19_x20:	.quad	0, 0
env_x21_x22:	.quad	0, 0
env_x23_x24:	.quad	0, 0
env_x25_x26:	.quad	0, 0
env_x27_x28:	.quad	0, 0
env_x29_x30:	.quad	0, 0
env_sp:		.quad	0
	.previous

/*
 * Save stack context for non-local goto
 */
	.globl	setjmp
	.type	setjmp, %function
setjmp:
	/* Store registers */
	stp	x19, x20, [x0, #env_x19_x20]
	stp	x21, x22, [x0, #env_x21_x22]
	stp	x23, x24, [x0, #env_x23_x24]
	stp	x25, x26, [x0, #env_x25_x26]
	stp	x27, x28, [x0, #env_x27_x28]
	stp	x29, x30, [x0, #env_x29_x30]
	mov	x16, sp
	str	x16, [x0, #env_sp]
	/* Return 0 when returning as setjmp() */
	mov	x0, #0
	ret
	.size	setjmp, . - setjmp

/*
 * Non-local jump to a saved stack context
 */
	.globl	longjmp
	.type	longjmp, %function
longjmp:
	/* Restore registers */
	ldp	x19, x20, [x0, #env_x19_x20]
	ldp	x21, x22, [x0, #env_x21_x22]
	ldp	x23, x24, [x0, #env_x23_x24]
	ldp	x25, x26, [x0, #env_x25_x26]
	ldp	x27, x28, [x0, #env_x27_x28]
	ldp	x29, x30, [x0, #env_x29_x30]
	ldr	x16, [x0, #env_sp]
	mov	sp, x16
	/* Force result to non-zero */
	cmp	w1, #0
	csinc	w0, w1, w1, ne
	/* Return to setjmp() caller */
	br	x30
	.size	longjmp, . - longjmp
