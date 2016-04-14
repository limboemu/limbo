/* 
 * Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Originally this code was part of ucl the data compression library
 * for upx the ``Ultimate Packer of eXecutables''.
 *
 * - Converted to gas assembly, and refitted to work with etherboot.
 *   Eric Biederman 20 Aug 2002
 *
 * - Structure modified to be a subroutine call rather than an
 *   executable prefix.
 *   Michael Brown 30 Mar 2004
 *
 * - Modified to be compilable as either 16-bit or 32-bit code.
 *   Michael Brown 9 Mar 2005
 */

FILE_LICENCE ( GPL2_OR_LATER )

/****************************************************************************
 * This file provides the decompress() and decompress16() functions
 * which can be called in order to decompress an image compressed with
 * the nrv2b utility in src/util.
 *
 * These functions are designed to be called by the prefix.  They are
 * position-independent code.
 *
 * The same basic assembly code is used to compile both
 * decompress() and decompress16().
 ****************************************************************************
 */

	.text
	.arch i386
	.section ".prefix.lib", "ax", @progbits

#ifdef CODE16
/****************************************************************************
 * decompress16 (real-mode near call, position independent)
 *
 * Decompress data in 16-bit mode
 *
 * Parameters (passed via registers):
 *   %ds:%esi - Start of compressed input data
 *   %es:%edi - Start of output buffer
 * Returns:
 *   %ds:%esi - End of compressed input data
 *   %es:%edi - End of decompressed output data
 *   All other registers are preserved
 *
 * NOTE: It would be possible to build a smaller version of the
 * decompression code for -DKEEP_IT_REAL by using
 *    #define REG(x) x
 * to use 16-bit registers where possible.  This would impose limits
 * that the compressed data size must be in the range [1,65533-%si]
 * and the uncompressed data size must be in the range [1,65536-%di]
 * (where %si and %di are the input values for those registers).  Note
 * particularly that the lower limit is 1, not 0, and that the upper
 * limit on the input (compressed) data really is 65533, since the
 * algorithm may read up to three bytes beyond the end of the input
 * data, since it reads dwords.
 ****************************************************************************
 */

#define REG(x) e ## x
#define ADDR32 addr32

	.code16
	.globl	decompress16
decompress16:
	
#else /* CODE16 */

/****************************************************************************
 * decompress (32-bit protected-mode near call, position independent)
 *
 * Parameters (passed via registers):
 *   %ds:%esi - Start of compressed input data
 *   %es:%edi - Start of output buffer
 * Returns:
 *   %ds:%esi - End of compressed input data
 *   %es:%edi - End of decompressed output data
 *   All other registers are preserved
 ****************************************************************************
 */

#define REG(x) e ## x
#define ADDR32
	
	.code32
	.globl	decompress
decompress:

#endif /* CODE16 */

#define xAX	REG(ax)
#define xCX	REG(cx)
#define xBP	REG(bp)
#define xSI	REG(si)
#define xDI	REG(di)

	/* Save registers */
	push	%xAX
	pushl	%ebx
	push	%xCX
	push	%xBP
	/* Do the decompression */
	cld
	xor	%xBP, %xBP
	dec	%xBP		/* last_m_off = -1 */
	jmp	dcl1_n2b
	
decompr_literals_n2b:
	ADDR32 movsb
decompr_loop_n2b:
	addl	%ebx, %ebx
	jnz	dcl2_n2b
dcl1_n2b:
	call	getbit32
dcl2_n2b:
	jc	decompr_literals_n2b
	xor	%xAX, %xAX
	inc	%xAX		/* m_off = 1 */
loop1_n2b:
	call	getbit1
	adc	%xAX, %xAX	/* m_off = m_off*2 + getbit() */
	call	getbit1
	jnc	loop1_n2b	/* while(!getbit()) */
	sub	$3, %xAX
	jb	decompr_ebpeax_n2b	/* if (m_off == 2) goto decompr_ebpeax_n2b ? */
	shl	$8, %xAX	
	ADDR32 movb (%xSI), %al	/* m_off = (m_off - 3)*256 + src[ilen++] */
	inc	%xSI
	xor	$-1, %xAX
	jz	decompr_end_n2b	/* if (m_off == 0xffffffff) goto decomp_end_n2b */
	mov	%xAX, %xBP	/* last_m_off = m_off ?*/
decompr_ebpeax_n2b:
	xor	%xCX, %xCX
	call	getbit1
	adc	%xCX, %xCX	/* m_len = getbit() */
	call	getbit1
	adc	%xCX, %xCX	/* m_len = m_len*2 + getbit()) */
	jnz	decompr_got_mlen_n2b	/* if (m_len == 0) goto decompr_got_mlen_n2b */
	inc	%xCX		/* m_len++ */
loop2_n2b:
	call	getbit1	
	adc	%xCX, %xCX	/* m_len = m_len*2 + getbit() */
	call	getbit1
	jnc	loop2_n2b	/* while(!getbit()) */
	inc	%xCX
	inc	%xCX		/* m_len += 2 */
decompr_got_mlen_n2b:
	cmp	$-0xd00, %xBP
	adc	$1, %xCX	/* m_len = m_len + 1 + (last_m_off > 0xd00) */
	push	%xSI
	ADDR32 lea (%xBP,%xDI), %xSI	/* m_pos = dst + olen + -m_off  */
	rep
	es ADDR32 movsb		/* dst[olen++] = *m_pos++ while(m_len > 0) */
	pop	%xSI
	jmp	decompr_loop_n2b


getbit1:
	addl	%ebx, %ebx
	jnz	1f
getbit32:
	ADDR32 movl (%xSI), %ebx
	sub	$-4, %xSI	/* sets carry flag */
	adcl	%ebx, %ebx
1:
	ret

decompr_end_n2b:
	/* Restore registers and return */
	pop	%xBP
	pop	%xCX
	popl	%ebx
	pop	%xAX
	ret
