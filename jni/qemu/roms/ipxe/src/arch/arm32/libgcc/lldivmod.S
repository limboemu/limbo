FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

	.text
	.thumb

/**
 * Unsigned long long division
 *
 * @v r1:r0		Dividend
 * @v r3:r2		Divisor
 * @ret r1:r0		Quotient
 * @ret r3:r2		Remainder
 */
	.section ".text.__aeabi_uldivmod", "ax", %progbits
	.globl	__aeabi_uldivmod
	.type	__aeabi_uldivmod, %function
__aeabi_uldivmod:
	/* Allocate stack space for remainder and pointer to remainder */
	push	{r0, r1, r2, r3, r4, lr}
	/* Call __udivmoddi4() */
	add	r4, sp, #8
	str	r4, [sp]
	bl	__udivmoddi4
	/* Retrieve remainder and return */
	add	sp, sp, #8
	pop	{r2, r3, r4, pc}
	.size	__aeabi_uldivmod, . - __aeabi_uldivmod

/**
 * Signed long long division
 *
 * @v r1:r0		Dividend
 * @v r3:r2		Divisor
 * @ret r1:r0		Quotient
 * @ret r3:r2		Remainder
 */
	.section ".text.__aeabi_ldivmod", "ax", %progbits
	.globl	__aeabi_ldivmod
	.type	__aeabi_ldivmod, %function
__aeabi_ldivmod:
	/* Allocate stack space for remainder and pointer to remainder */
	push	{r0, r1, r2, r3, r4, lr}
	/* Call __divmoddi4() */
	add	r4, sp, #8
	str	r4, [sp]
	bl	__divmoddi4
	/* Retrieve remainder and return */
	add	sp, sp, #8
	pop	{r2, r3, r4, pc}
	.size	__aeabi_ldivmod, . - __aeabi_ldivmod
