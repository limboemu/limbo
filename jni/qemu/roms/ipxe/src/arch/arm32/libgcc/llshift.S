FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

	.text
	.arm

/**
 * Logical shift left
 *
 * @v r1:r0		Value to shift
 * @v r2		Shift amount
 * @ret r1:r0		Shifted value
 */
	.section ".text.__aeabi_llsl", "ax", %progbits
	.globl	__aeabi_llsl
	.type	__aeabi_llsl, %function
__aeabi_llsl:
	/* r3 = ( shift - 32 ) */
	subs	r3, r2, #32
	/* If shift >= 32, then
	 *   high = ( low << ( shift - 32 ) )
	 */
	movpl	r1, r0, lsl r3
	/* If shift < 32, then
	 *   high = ( ( high << shift ) | ( low >> ( 32 - shift ) ) )
	 */
	movmi	r1, r1, lsl r2
	rsbmi	r3, r2, #32
	orrmi	r1, r1, r0, lsr r3
	/* low = ( low << shift ) */
	mov	r0, r0, lsl r2
	bx	lr
	.size	__aeabi_llsl, . - __aeabi_llsl

/**
 * Logical shift right
 *
 * @v r1:r0		Value to shift
 * @v r2		Shift amount
 * @ret r1:r0		Shifted value
 */
	.section ".text.__aeabi_llsr", "ax", %progbits
	.globl	__aeabi_llsr
	.type	__aeabi_llsr, %function
__aeabi_llsr:
	/* r3 = ( shift - 32 ) */
	subs	r3, r2, #32
	/* If shift >= 32, then
	 *   low = ( high >> ( shift - 32 ) )
	 */
	movpl	r0, r1, lsr r3
	/* If shift < 32, then
	 *   low = ( ( low >> shift ) | ( high << ( 32 - shift ) ) )
	 */
	movmi	r0, r0, lsr r2
	rsbmi	r3, r2, #32
	orrmi	r0, r0, r1, lsl r3
	/* high = ( high >> shift ) */
	mov	r1, r1, lsr r2
	bx	lr
	.size	__aeabi_llsr, . - __aeabi_llsr

/**
 * Arithmetic shift right
 *
 * @v r1:r0		Value to shift
 * @v r2		Shift amount
 * @ret r1:r0		Shifted value
 */
	.section ".text.__aeabi_lasr", "ax", %progbits
	.globl	__aeabi_lasr
	.type	__aeabi_lasr, %function
__aeabi_lasr:
	/* r3 = ( shift - 32 ) */
	subs	r3, r2, #32
	/* If shift >= 32, then
	 *   low = ( high >> ( shift - 32 ) )
	 */
	movpl	r0, r1, asr r3
	/* If shift < 32, then
	 *   low = ( ( low >> shift ) | ( high << ( 32 - shift ) ) )
	 */
	movmi	r0, r0, lsr r2
	rsbmi	r3, r2, #32
	orrmi	r0, r0, r1, lsl r3
	/* high = ( high >> shift ) */
	mov	r1, r1, asr r2
	bx	lr
	.size	__aeabi_lasr, . - __aeabi_lasr
