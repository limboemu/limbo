/*
 * librm: a library for interfacing to real-mode code
 *
 * Michael Brown <mbrown@fensystems.co.uk>
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

/* Drag in local definitions */
#include "librm.h"

/* CR0: protection enabled */
#define CR0_PE ( 1 << 0 )

/* CR0: paging */
#define CR0_PG ( 1 << 31 )

/* CR4: physical address extensions */
#define CR4_PAE ( 1 << 5 )

/* Extended feature enable MSR (EFER) */
#define MSR_EFER 0xc0000080

/* EFER: long mode enable */
#define EFER_LME ( 1 << 8 )

/* Page: present */
#define PG_P 0x01

/* Page: read/write */
#define PG_RW 0x02

/* Page: user/supervisor */
#define PG_US 0x04

/* Page: page size */
#define PG_PS 0x80

/* Size of various paging-related data structures */
#define SIZEOF_PTE_LOG2 3
#define SIZEOF_PTE ( 1 << SIZEOF_PTE_LOG2 )
#define SIZEOF_PT_LOG2 12
#define SIZEOF_PT ( 1 << SIZEOF_PT_LOG2 )
#define SIZEOF_4KB_PAGE_LOG2 12
#define SIZEOF_4KB_PAGE ( 1 << SIZEOF_4KB_PAGE_LOG2 )
#define SIZEOF_2MB_PAGE_LOG2 21
#define SIZEOF_2MB_PAGE ( 1 << SIZEOF_2MB_PAGE_LOG2 )
#define SIZEOF_LOW_4GB_LOG2 32
#define SIZEOF_LOW_4GB ( 1 << SIZEOF_LOW_4GB_LOG2 )

/* Size of various C data structures */
#define SIZEOF_I386_SEG_REGS	12
#define SIZEOF_I386_REGS	32
#define SIZEOF_REAL_MODE_REGS	( SIZEOF_I386_SEG_REGS + SIZEOF_I386_REGS )
#define SIZEOF_I386_FLAGS	4
#define SIZEOF_I386_ALL_REGS	( SIZEOF_REAL_MODE_REGS + SIZEOF_I386_FLAGS )
#define SIZEOF_X86_64_REGS	128

/* Size of an address */
#ifdef __x86_64__
#define SIZEOF_ADDR 8
#else
#define SIZEOF_ADDR 4
#endif

/* Default code size */
#ifdef __x86_64__
#define CODE_DEFAULT code64
#else
#define CODE_DEFAULT code32
#endif

/* Selectively assemble code for 32-bit/64-bit builds */
#ifdef __x86_64__
#define if32 if 0
#define if64 if 1
#else
#define if32 if 1
#define if64 if 0
#endif

/****************************************************************************
 * Global descriptor table
 *
 * Call init_librm to set up the GDT before attempting to use any
 * protected-mode code.
 *
 * NOTE: This must be located before prot_to_real, otherwise gas
 * throws a "can't handle non absolute segment in `ljmp'" error due to
 * not knowing the value of REAL_CS when the ljmp is encountered.
 *
 * Note also that putting ".word gdt_end - gdt - 1" directly into
 * gdt_limit, rather than going via gdt_length, will also produce the
 * "non absolute segment" error.  This is most probably a bug in gas.
 ****************************************************************************
 */
	.section ".data16.gdt", "aw", @progbits
	.align 16
gdt:
gdtr:		/* The first GDT entry is unused, the GDTR can fit here. */
gdt_limit:		.word gdt_length - 1
gdt_base:		.long 0
			.word 0 /* padding */

	.org	gdt + VIRTUAL_CS, 0
virtual_cs:	/* 32 bit protected mode code segment, virtual addresses */
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	.org	gdt + VIRTUAL_DS, 0
virtual_ds:	/* 32 bit protected mode data segment, virtual addresses */
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0
	
	.org	gdt + PHYSICAL_CS, 0
physical_cs:	/* 32 bit protected mode code segment, physical addresses */
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	.org	gdt + PHYSICAL_DS, 0
physical_ds:	/* 32 bit protected mode data segment, physical addresses */
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0	

	.org	gdt + REAL_CS, 0
real_cs: 	/* 16 bit real mode code segment */
	.word	0xffff, 0
	.byte	0, 0x9b, 0x00, 0

	.org	gdt + REAL_DS, 0
real_ds:	/* 16 bit real mode data segment */
	.word	0xffff, 0
	.byte	0, 0x93, 0x00, 0

	.org	gdt + P2R_DS, 0
p2r_ds:		/* 16 bit real mode data segment for prot_to_real transition */
	.word	0xffff, ( P2R_DS << 4 )
	.byte	0, 0x93, 0x00, 0

	.org	gdt + LONG_CS, 0
long_cs:	/* 64 bit long mode code segment */
	.word	0, 0
	.byte	0, 0x9a, 0x20, 0

gdt_end:
	.equ	gdt_length, gdt_end - gdt

/****************************************************************************
 * Stored real-mode and protected-mode stack pointers
 *
 * The real-mode stack pointer is stored here whenever real_to_prot
 * is called and restored whenever prot_to_real is called.  The
 * converse happens for the protected-mode stack pointer.
 *
 * Despite initial appearances this scheme is, in fact re-entrant,
 * because program flow dictates that we always return via the point
 * we left by.  For example:
 *    PXE API call entry
 *  1   real => prot
 *        ...
 *        Print a text string
 *	    ...
 *  2       prot => real
 *            INT 10
 *  3       real => prot
 *	    ...
 *        ...
 *  4   prot => real
 *    PXE API call exit
 *
 * At point 1, the RM mode stack value, say RPXE, is stored in
 * rm_ss,sp.  We want this value to still be present in rm_ss,sp when
 * we reach point 4.
 *
 * At point 2, the RM stack value is restored from RPXE.  At point 3,
 * the RM stack value is again stored in rm_ss,sp.  This *does*
 * overwrite the RPXE that we have stored there, but it's the same
 * value, since the code between points 2 and 3 has managed to return
 * to us.
 ****************************************************************************
 */
	.section ".bss.rm_ss_sp", "aw", @nobits
	.globl rm_sp
rm_sp:	.word 0
	.globl rm_ss
rm_ss:	.word 0

	.section ".data.pm_esp", "aw", @progbits
pm_esp:	.long VIRTUAL(_estack)

/****************************************************************************
 * Temporary static data buffer
 *
 * This is used to reduce the amount of real-mode stack space consumed
 * during mode transitions, since we are sometimes called with very
 * little real-mode stack space available.
 ****************************************************************************
 */
	/* Temporary static buffer usage by virt_call */
	.struct	0
VC_TMP_GDT:		.space	6
VC_TMP_IDT:		.space	6
VC_TMP_PAD:		.space	4 /* for alignment */
.if64
VC_TMP_CR3:		.space	4
VC_TMP_CR4:		.space	4
VC_TMP_EMER:		.space	8
.endif
VC_TMP_FXSAVE:		.space	512
VC_TMP_END:
	.previous

	/* Temporary static buffer usage by real_call */
	.struct 0
RC_TMP_FUNCTION:	.space	4
RC_TMP_END:
	.previous

	/* Shared temporary static buffer */
	.section ".bss16.rm_tmpbuf", "aw", @nobits
	.align 16
rm_tmpbuf:
	.space	VC_TMP_END
	.size	rm_tmpbuf, . - rm_tmpbuf

/****************************************************************************
 * Virtual address offsets
 *
 * These are used by the protected-mode code to map between virtual
 * and physical addresses, and to access variables in the .text16 or
 * .data16 segments.
 ****************************************************************************
 */
	.struct 0
VA_VIRT_OFFSET:	.space	SIZEOF_ADDR
VA_TEXT16:	.space	SIZEOF_ADDR
VA_DATA16:	.space	SIZEOF_ADDR
VA_SIZE:
	.previous

	/* Internal copies, used only by librm itself */
	.section ".bss16.rm_virt_addrs", "aw", @nobits
rm_virt_addrs:	.space	VA_SIZE
	.equ	rm_virt_offset, ( rm_virt_addrs + VA_VIRT_OFFSET )
	.equ	rm_text16, ( rm_virt_addrs + VA_TEXT16 )
	.equ	rm_data16, ( rm_virt_addrs + VA_DATA16 )

	/* Externally visible variables, used by C code */
	.section ".bss.virt_addrs", "aw", @nobits
virt_addrs:	.space	VA_SIZE
	.globl	virt_offset
	.equ	virt_offset, ( virt_addrs + VA_VIRT_OFFSET )
	.globl	text16
	.equ	text16, ( virt_addrs + VA_TEXT16 )
	.globl	data16
	.equ	data16, ( virt_addrs + VA_DATA16 )

/****************************************************************************
 * init_librm (real-mode far call, 16-bit real-mode far return address)
 *
 * Initialise the GDT ready for transitions to protected mode.
 *
 * Parameters:
 *   %cs : .text16 segment
 *   %ds : .data16 segment
 *   %edi : Physical base of protected-mode code
 ****************************************************************************
 */
	.section ".text16.init_librm", "ax", @progbits
	.code16
	.globl init_librm
init_librm:
	/* Preserve registers */
	pushl	%eax
	pushl	%ebx
	pushl	%edi

	/* Store rm_virt_offset and set up virtual_cs and virtual_ds segments */
	subl	$VIRTUAL(_textdata), %edi
	movl	%edi, rm_virt_offset
.if64 ;	setae	(rm_virt_offset+4) ; .endif
	movl	%edi, %eax
	movw	$virtual_cs, %bx
	call	set_seg_base
	movw	$virtual_ds, %bx
	call	set_seg_base

	/* Store rm_cs and rm_text16, set up real_cs segment */
	xorl	%eax, %eax
	movw	%cs, %ax
	movw	%ax, %cs:rm_cs
	shll	$4, %eax
	movw	$real_cs, %bx
	call	set_seg_base
.if32 ;	subl	%edi, %eax ; .endif
	movl	%eax, rm_text16

	/* Store rm_ds and rm_data16, set up real_ds segment and GDT base */
	xorl	%eax, %eax
	movw	%ds, %ax
	movw	%ax, %cs:rm_ds
	shll	$4, %eax
	movw	$real_ds, %bx
	call	set_seg_base
	movl	%eax, gdt_base
	addl	$gdt, gdt_base
.if32 ;	subl	%edi, %eax ; .endif
	movl	%eax, rm_data16

	/* Configure virt_call for protected mode, if applicable */
.if64 ;	movl	$VIRTUAL(vc_pmode), %cs:vc_jmp_offset ; .endif

	/* Switch to protected mode */
	virtcall init_librm_pmode
	.section ".text.init_librm", "ax", @progbits
	.code32
init_librm_pmode:

	/* Store virt_offset, text16, and data16 */
	pushw	%ds
	movw	$REAL_DS, %ax
	movw	%ax, %ds
	movl	$rm_virt_addrs, %esi
	movl	$VIRTUAL(virt_addrs), %edi
	movl	$( VA_SIZE / 4 ), %ecx
	rep movsl
	popw	%ds

.if64 ;	/* Initialise long mode, if applicable */
	movl	VIRTUAL(virt_offset), %edi
	leal	VIRTUAL(p2l_ljmp_target)(%edi), %eax
	movl	%eax, VIRTUAL(p2l_ljmp_offset)
	call	init_pages
.endif
	/* Return to real mode */
	ret
	.section ".text16.init_librm", "ax", @progbits
	.code16
init_librm_rmode:

	/* Configure virt_call for long mode, if applicable */
.if64 ;	movl	$VIRTUAL(vc_lmode), %cs:vc_jmp_offset ; .endif

	/* Initialise IDT */
	virtcall init_idt

	/* Restore registers */
	popl	%edi
	popl	%ebx
	popl	%eax
	lret

	.section ".text16.set_seg_base", "ax", @progbits
	.code16
set_seg_base:
1:	movw	%ax, 2(%bx)
	rorl	$16, %eax
	movb	%al, 4(%bx)
	movb	%ah, 7(%bx)
	roll	$16, %eax
	ret

/****************************************************************************
 * real_to_prot (real-mode near call, 32-bit virtual return address)
 *
 * Switch from 16-bit real-mode to 32-bit protected mode with virtual
 * addresses.  The real-mode %ss:sp is stored in rm_ss and rm_sp, and
 * the protected-mode %esp is restored from the saved pm_esp.
 * Interrupts are disabled.  All other registers may be destroyed.
 *
 * The return address for this function should be a 32-bit virtual
 * address.
 *
 * Parameters: 
 *   %ecx : number of bytes to move from RM stack to PM stack
 *   %edx : number of bytes to copy from RM temporary buffer to PM stack
 *
 ****************************************************************************
 */
	.section ".text16.real_to_prot", "ax", @progbits
	.code16
real_to_prot:
	/* Enable A20 line */
	call	enable_a20
	/* A failure at this point is fatal, and there's nothing we
	 * can do about it other than lock the machine to make the
	 * problem immediately visible.
	 */
1:	jc	1b

	/* Make sure we have our data segment available */
	movw	%cs:rm_ds, %ds

	/* Add protected-mode return address to length of data to be copied */
	addw	$4, %cx /* %ecx must be less than 64kB anyway */

	/* Real-mode %ss:%sp => %ebp and virtual address => %esi */
	xorl	%eax, %eax
	movw	%ss, %ax
	shll	$4, %eax
	movzwl	%sp, %ebp
	addr32 leal (%eax,%ebp), %esi
	subl	rm_virt_offset, %esi
	shll	$12, %eax
	orl	%eax, %ebp

	/* Real-mode data segment virtual address => %ebx */
	movl	rm_data16, %ebx
.if64 ; subl	rm_virt_offset, %ebx ; .endif

	/* Load protected-mode global descriptor table */
	data32 lgdt gdtr

	/* Zero segment registers.  This wastes around 12 cycles on
	 * real hardware, but saves a substantial number of emulated
	 * instructions under KVM.
	 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Switch to protected mode (with paging disabled if applicable) */
	cli
	movl	%cr0, %eax
.if64 ;	andl	$~CR0_PG, %eax ; .endif
	orb	$CR0_PE, %al
	movl	%eax, %cr0
	data32 ljmp	$VIRTUAL_CS, $VIRTUAL(r2p_pmode)
	.section ".text.real_to_prot", "ax", @progbits
	.code32
r2p_pmode:
	/* Set up protected-mode data segments and stack pointer */
	movw	$VIRTUAL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movl	VIRTUAL(pm_esp), %esp

	/* Load protected-mode interrupt descriptor table */
	lidt	VIRTUAL(idtr32)

	/* Record real-mode %ss:sp (after removal of data) */
	addl	%ecx, %ebp
	movl	%ebp, VIRTUAL(rm_sp)

	/* Move data from RM stack to PM stack */
	subl	%edx, %esp
	subl	%ecx, %esp
	movl	%esp, %edi
	rep movsb

	/* Copy data from RM temporary buffer to PM stack */
	leal	rm_tmpbuf(%ebx), %esi
	movl	%edx, %ecx
	rep movsb

	/* Return to virtual address */
	ret

/****************************************************************************
 * prot_to_real (protected-mode near call, 32-bit real-mode return address)
 *
 * Switch from 32-bit protected mode with virtual addresses to 16-bit
 * real mode.  The protected-mode %esp is stored in pm_esp and the
 * real-mode %ss:sp is restored from the saved rm_ss and rm_sp.  The
 * high word of the real-mode %esp is set to zero.  All real-mode data
 * segment registers are loaded from the saved rm_ds.  Interrupts are
 * *not* enabled, since we want to be able to use prot_to_real in an
 * ISR.  All other registers may be destroyed.
 *
 * The return address for this function should be a 32-bit (sic)
 * real-mode offset within .code16.
 *
 * Parameters: 
 *   %ecx : number of bytes to move from PM stack to RM stack
 *   %edx : number of bytes to move from PM stack to RM temporary buffer
 *   %esi : real-mode global and interrupt descriptor table registers
 *
 ****************************************************************************
 */
	.section ".text.prot_to_real", "ax", @progbits
	.code32
prot_to_real:
	/* Copy real-mode global descriptor table register to RM code segment */
	movl	VIRTUAL(text16), %edi
.if64 ;	subl	VIRTUAL(virt_offset), %edi ; .endif
	leal	rm_gdtr(%edi), %edi
	movsw
	movsl

	/* Load real-mode interrupt descriptor table register */
	lidt	(%esi)

	/* Add return address to data to be moved to RM stack */
	addl	$4, %ecx

	/* Real-mode %ss:sp => %ebp and virtual address => %edi */
	movl	VIRTUAL(rm_sp), %ebp
	subl	%ecx, %ebp
	movzwl	VIRTUAL(rm_ss), %eax
	shll	$4, %eax
	movzwl	%bp, %edi
	addl	%eax, %edi
	subl	VIRTUAL(virt_offset), %edi

	/* Move data from PM stack to RM stack */
	movl	%esp, %esi
	rep movsb

	/* Move data from PM stack to RM temporary buffer */
	movl	VIRTUAL(data16), %edi
.if64 ;	subl	VIRTUAL(virt_offset), %edi ; .endif
	addl	$rm_tmpbuf, %edi
	movl	%edx, %ecx
	rep movsb

	/* Record protected-mode %esp (after removal of data) */
	movl	%esi, VIRTUAL(pm_esp)

	/* Load real-mode segment limits */
	movw	$P2R_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	ljmp	$REAL_CS, $p2r_rmode
	.section ".text16.prot_to_real", "ax", @progbits
	.code16
p2r_rmode:
	/* Load real-mode GDT */
	data32 lgdt %cs:rm_gdtr
	/* Switch to real mode */
	movl	%cr0, %eax
	andb	$0!CR0_PE, %al
	movl	%eax, %cr0
p2r_ljmp_rm_cs:
	ljmp	$0, $1f
1:
	/* Set up real-mode data segments and stack pointer */
	movw	%cs:rm_ds, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movl	%ebp, %eax
	shrl	$16, %eax
	movw	%ax, %ss
	movzwl	%bp, %esp

	/* Return to real-mode address */
	data32 ret


	/* Real-mode code and data segments.  Assigned by the call to
	 * init_librm.  rm_cs doubles as the segment part of the jump
	 * instruction used by prot_to_real.  Both are located in
	 * .text16 rather than .data16: rm_cs since it forms part of
	 * the jump instruction within the code segment, and rm_ds
	 * since real-mode code needs to be able to locate the data
	 * segment with no other reference available.
	 */
	.globl rm_cs
	.equ	rm_cs, ( p2r_ljmp_rm_cs + 3 )

	.section ".text16.data.rm_ds", "aw", @progbits
	.globl rm_ds
rm_ds:	.word 0

	/* Real-mode global and interrupt descriptor table registers */
	.section ".text16.data.rm_gdtr", "aw", @progbits
rm_gdtr:
	.word 0 /* Limit */
	.long 0 /* Base */

/****************************************************************************
 * phys_to_prot (protected-mode near call, 32-bit physical return address)
 *
 * Switch from 32-bit protected mode with physical addresses to 32-bit
 * protected mode with virtual addresses.  %esp is adjusted to a
 * virtual address.  All other registers are preserved.
 *
 * The return address for this function should be a 32-bit physical
 * (sic) address.
 *
 ****************************************************************************
 */
	.section ".text.phys_to_prot", "ax", @progbits
	.code32
	.globl phys_to_prot
phys_to_prot:
	/* Preserve registers */
	pushl	%eax
	pushl	%ebp

	/* Switch to virtual code segment */
	cli
	ljmp	$VIRTUAL_CS, $VIRTUAL(1f)
1:
	/* Switch to virtual data segment and adjust %esp */
	movw	$VIRTUAL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movl	VIRTUAL(virt_offset), %ebp
	subl	%ebp, %esp

	/* Adjust return address to a virtual address */
	subl	%ebp, 8(%esp)

	/* Restore registers and return */
	popl	%ebp
	popl	%eax
	ret

.if32	/* Expose as _phys_to_virt for use by COMBOOT, if applicable */
	.globl	_phys_to_virt
	.equ	_phys_to_virt, phys_to_prot
.endif

/****************************************************************************
 * prot_to_phys (protected-mode near call, 32-bit virtual return address)
 *
 * Switch from 32-bit protected mode with virtual addresses to 32-bit
 * protected mode with physical addresses.  %esp is adjusted to a
 * physical address.  All other registers are preserved.
 *
 * The return address for this function should be a 32-bit virtual
 * (sic) address.
 *
 ****************************************************************************
 */
	.section ".text.prot_to_phys", "ax", @progbits
	.code32
prot_to_phys:
	/* Preserve registers */
	pushl	%eax
	pushl	%ebp

	/* Adjust return address to a physical address */
	movl	VIRTUAL(virt_offset), %ebp
	addl	%ebp, 8(%esp)

	/* Switch to physical code segment */
	cli
	pushl	$PHYSICAL_CS
	leal	VIRTUAL(1f)(%ebp), %eax
	pushl	%eax
	lret
1:
	/* Switch to physical data segment and adjust %esp */
	movw	$PHYSICAL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	addl	%ebp, %esp

	/* Restore registers and return */
	popl	%ebp
	popl	%eax
	ret

.if32	/* Expose as _virt_to_phys for use by COMBOOT, if applicable */
	.globl	_virt_to_phys
	.equ	_virt_to_phys, prot_to_phys
.endif

/****************************************************************************
 * intr_to_prot (protected-mode near call, 32-bit virtual return address)
 *
 * Switch from 32-bit protected mode with a virtual code segment and
 * either a physical or virtual stack segment to 32-bit protected mode
 * with normal virtual addresses.  %esp is adjusted if necessary to a
 * virtual address.  All other registers are preserved.
 *
 * The return address for this function should be a 32-bit virtual
 * address.
 *
 ****************************************************************************
 */
	.section ".text.intr_to_prot", "ax", @progbits
	.code32
	.globl intr_to_prot
intr_to_prot:
	/* Preserve registers */
	pushl	%eax

	/* Check whether stack segment is physical or virtual */
	movw	%ss, %ax
	cmpw	$VIRTUAL_DS, %ax
	movw	$VIRTUAL_DS, %ax

	/* Reload data segment registers */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	/* Reload stack segment and adjust %esp if necessary */
	je	1f
	movw	%ax, %ss
	subl	VIRTUAL(virt_offset), %esp
1:
	/* Restore registers and return */
	popl	%eax
	ret

	/* Expose as _intr_to_virt for use by GDB */
	.globl	_intr_to_virt
	.equ	_intr_to_virt, intr_to_prot

/****************************************************************************
 * prot_to_long (protected-mode near call, 32-bit virtual return address)
 *
 * Switch from 32-bit protected mode with virtual addresses to 64-bit
 * long mode.  The protected-mode %esp is adjusted to a physical
 * address.  All other registers are preserved.
 *
 * The return address for this function should be a 32-bit (sic)
 * virtual address.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.prot_to_long", "ax", @progbits
	.code32
prot_to_long:
	/* Preserve registers */
	pushl	%eax
	pushl	%ecx
	pushl	%edx

	/* Set up PML4 */
	movl	VIRTUAL(pml4), %eax
	movl	%eax, %cr3

	/* Enable PAE */
	movl	%cr4, %eax
	orb	$CR4_PAE, %al
	movl	%eax, %cr4

	/* Enable long mode */
	movl	$MSR_EFER, %ecx
	rdmsr
	orw	$EFER_LME, %ax
	wrmsr

	/* Enable paging */
	movl	%cr0, %eax
	orl	$CR0_PG, %eax
	movl	%eax, %cr0

	/* Restore registers */
	popl	%edx
	popl	%ecx
	popl	%eax

	/* Construct 64-bit return address */
	pushl	(%esp)
	movl	$0xffffffff, 4(%esp)
p2l_ljmp:
	/* Switch to long mode (using a physical %rip) */
	ljmp	$LONG_CS, $0
	.code64
p2l_lmode:
	/* Adjust and zero-extend %esp to a physical address */
	addl	virt_offset, %esp

	/* Use long-mode IDT */
	lidt	idtr64

	/* Return to virtual address */
	ret

	/* Long mode jump offset and target.  Required since an ljmp
	 * in protected mode will zero-extend the offset, and so
	 * cannot reach an address within the negative 2GB as used by
	 * -mcmodel=kernel.  Assigned by the call to init_librm.
	 */
	.equ	p2l_ljmp_offset, ( p2l_ljmp + 1 )
	.equ	p2l_ljmp_target, p2l_lmode

	.endif

/****************************************************************************
 * long_to_prot (long-mode near call, 64-bit virtual return address)
 *
 * Switch from 64-bit long mode to 32-bit protected mode with virtual
 * addresses.  The long-mode %rsp is adjusted to a virtual address.
 * All other registers are preserved.
 *
 * The return address for this function should be a 64-bit (sic)
 * virtual address.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.long_to_prot", "ax", @progbits
	.code64
long_to_prot:
	/* Switch to protected mode */
	ljmp	*l2p_vector
	.code32
l2p_pmode:
	/* Adjust %esp to a virtual address */
	subl	VIRTUAL(virt_offset), %esp

	/* Preserve registers */
	pushl	%eax
	pushl	%ecx
	pushl	%edx

	/* Disable paging */
	movl	%cr0, %eax
	andl	$~CR0_PG, %eax
	movl	%eax, %cr0

	/* Disable PAE (in case external non-PAE-aware code enables paging) */
	movl	%cr4, %eax
	andb	$~CR4_PAE, %al
	movl	%eax, %cr4

	/* Disable long mode */
	movl	$MSR_EFER, %ecx
	rdmsr
	andw	$~EFER_LME, %ax
	wrmsr

	/* Restore registers */
	popl	%edx
	popl	%ecx
	popl	%eax

	/* Use protected-mode IDT */
	lidt	VIRTUAL(idtr32)

	/* Return */
	ret	$4

	/* Long mode jump vector.  Required since there is no "ljmp
	 * immediate" instruction in long mode.
	 */
	.section ".data.l2p_vector", "aw", @progbits
l2p_vector:
	.long	VIRTUAL(l2p_pmode), VIRTUAL_CS

	.endif

/****************************************************************************
 * long_save_regs (long-mode near call, 64-bit virtual return address)
 *
 * Preserve registers that are accessible only in long mode.  This
 * includes %r8-%r15 and the upper halves of %rax, %rbx, %rcx, %rdx,
 * %rsi, %rdi, and %rbp.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.long_preserve_regs", "ax", @progbits
	.code64
long_preserve_regs:
	/* Preserve registers */
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	pushq	%rbx
	pushq	%rsp
	pushq	%rbp
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* Return */
	jmp	*SIZEOF_X86_64_REGS(%rsp)

	.endif

/****************************************************************************
 * long_restore_regs (long-mode near call, 64-bit virtual return address)
 *
 * Restore registers that are accessible only in long mode.  This
 * includes %r8-%r15 and the upper halves of %rax, %rbx, %rcx, %rdx,
 * %rsi, %rdi, and %rbp.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.long_restore_regs", "ax", @progbits
	.code64
long_restore_regs:
	/* Move return address above register dump */
	popq	SIZEOF_X86_64_REGS(%rsp)

	/* Restore registers */
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	movl	%edi, (%rsp)
	popq	%rdi
	movl	%esi, (%rsp)
	popq	%rsi
	movl	%ebp, (%rsp)
	popq	%rbp
	leaq	8(%rsp), %rsp /* discard */
	movl	%ebx, (%rsp)
	popq	%rbx
	movl	%edx, (%rsp)
	popq	%rdx
	movl	%ecx, (%rsp)
	popq	%rcx
	movl	%eax, (%rsp)
	popq	%rax

	/* Return */
	ret

	.endif

/****************************************************************************
 * virt_call (real-mode near call, 16-bit real-mode near return address)
 *
 * Call a specific C function in 32-bit protected mode or 64-bit long
 * mode (as applicable).  The prototype of the C function must be
 *   void function ( struct i386_all_regs *ix86 ); 
 * ix86 will point to a struct containing the real-mode registers
 * at entry to virt_call().
 *
 * All registers will be preserved across virt_call(), unless the C
 * function explicitly overwrites values in ix86.  Interrupt status
 * and GDT will also be preserved.  Gate A20 will be enabled.
 *
 * Note that virt_call() does not rely on the real-mode stack
 * remaining intact in order to return, since everything relevant is
 * copied to the protected-mode stack for the duration of the call.
 * In particular, this means that a real-mode prefix can make a call
 * to main() which will return correctly even if the prefix's stack
 * gets vapourised during the Etherboot run.  (The prefix cannot rely
 * on anything else on the stack being preserved, so should move any
 * critical data to registers before calling main()).
 *
 * Parameters:
 *   function : 32-bit virtual address of function to call
 *
 * Example usage:
 *	pushl	$pxe_api_call
 *	call	virt_call
 * to call in to the C function
 *      void pxe_api_call ( struct i386_all_regs *ix86 );
 ****************************************************************************
 */
	.struct	0
VC_OFFSET_IX86:		.space	SIZEOF_I386_ALL_REGS
VC_OFFSET_PADDING:	.space	2 /* for alignment */
VC_OFFSET_RETADDR:	.space	2
VC_OFFSET_PARAMS:
VC_OFFSET_FUNCTION:	.space	4
VC_OFFSET_END:
	.previous

	.section ".text16.virt_call", "ax", @progbits
	.code16
	.globl virt_call
virt_call:
	/* Preserve registers and flags on external RM stack */
	pushw	%ss /* padding */
	pushfl
	pushal
	pushw	%gs
	pushw	%fs
	pushw	%es
	pushw	%ds
	pushw	%ss
	pushw	%cs

	/* Claim ownership of temporary static buffer */
	cli

	/* Preserve FPU, MMX and SSE state in temporary static buffer */
	movw	%cs:rm_ds, %ds
	fxsave	( rm_tmpbuf + VC_TMP_FXSAVE )

	/* Preserve GDT and IDT in temporary static buffer */
	sidt	( rm_tmpbuf + VC_TMP_IDT )
	sgdt	( rm_tmpbuf + VC_TMP_GDT )

.if64 ;	/* Preserve control registers, if applicable */
	movl	$MSR_EFER, %ecx
	rdmsr
	movl	%eax, ( rm_tmpbuf + VC_TMP_EMER + 0 )
	movl	%edx, ( rm_tmpbuf + VC_TMP_EMER + 4 )
	movl	%cr4, %eax
	movl	%eax, ( rm_tmpbuf + VC_TMP_CR4 )
	movl	%cr3, %eax
	movl	%eax, ( rm_tmpbuf + VC_TMP_CR3 )
.endif
	/* For sanity's sake, clear the direction flag as soon as possible */
	cld

	/* Switch to protected mode and move register dump to PM stack */
	movl	$VC_OFFSET_END, %ecx
	movl	$VC_TMP_END, %edx
	pushl	$VIRTUAL(vc_pmode)
vc_jmp:	jmp	real_to_prot
	.section ".text.virt_call", "ax", @progbits
	.code32
vc_pmode:
	/* Call function (in protected mode) */
	pushl	%esp
	call	*(VC_OFFSET_FUNCTION+4)(%esp)
	popl	%eax /* discard */

.if64 ; /* Switch to long mode */
	jmp	1f
vc_lmode:
	call	prot_to_long
	.code64

	/* Call function (in long mode) */
	movq	%rsp, %rdi
	movslq	VC_OFFSET_FUNCTION(%rsp), %rax
	callq	*%rax

	/* Switch to protected mode */
	call	long_to_prot
1:	.code32
.endif
	/* Switch to real mode and move register dump back to RM stack */
	movl	$VC_OFFSET_END, %ecx
	movl	$VC_TMP_END, %edx
	leal	VC_TMP_GDT(%esp, %ecx), %esi
	pushl	$vc_rmode
	jmp	prot_to_real
	.section ".text16.virt_call", "ax", @progbits
	.code16
vc_rmode:
.if64 ;	/* Restore control registers, if applicable */
	movw	%sp, %bp
	movl	( rm_tmpbuf + VC_TMP_CR3 ), %eax
	movl	%eax, %cr3
	movl	( rm_tmpbuf + VC_TMP_CR4 ), %eax
	movl	%eax, %cr4
	movl	( rm_tmpbuf + VC_TMP_EMER + 0 ), %eax
	movl	( rm_tmpbuf + VC_TMP_EMER + 4 ), %edx
	movl	$MSR_EFER, %ecx
	wrmsr
.endif
	/* Restore FPU, MMX and SSE state from temporary static buffer */
	fxrstor	( rm_tmpbuf + VC_TMP_FXSAVE )

	/* Restore registers and flags and return */
	popl	%eax /* skip %cs and %ss */
	popw	%ds
	popw	%es
	popw	%fs
	popw	%gs
	popal
	/* popal skips %esp.  We therefore want to do "movl -20(%sp),
	 * %esp", but -20(%sp) is not a valid 80386 expression.
	 * Fortunately, prot_to_real() zeroes the high word of %esp, so
	 * we can just use -20(%esp) instead.
	 */
	addr32 movl -20(%esp), %esp
	popfl
	popw	%ss /* padding */

	/* Return and discard function parameters */
	ret	$( VC_OFFSET_END - VC_OFFSET_PARAMS )


	/* Protected-mode jump target */
	.equ	vc_jmp_offset, ( vc_jmp - 4 )

/****************************************************************************
 * real_call (protected-mode near call, 32-bit virtual return address)
 * real_call (long-mode near call, 64-bit virtual return address)
 *
 * Call a real-mode function from protected-mode or long-mode code.
 *
 * The non-segment register values will be passed directly to the
 * real-mode code.  The segment registers will be set as per
 * prot_to_real.  The non-segment register values set by the real-mode
 * function will be passed back to the protected-mode or long-mode
 * caller.  A result of this is that this routine cannot be called
 * directly from C code, since it clobbers registers that the C ABI
 * expects the callee to preserve.
 *
 * librm.h defines a convenient macro REAL_CODE() for using real_call.
 * See librm.h and realmode.h for details and examples.
 *
 * Parameters:
 *   function : offset within .text16 of real-mode function to call
 *
 * Returns: none
 ****************************************************************************
 */
	.struct	0
RC_OFFSET_REGS:		.space	SIZEOF_I386_REGS
RC_OFFSET_REGS_END:
RC_OFFSET_FUNCTION_COPY:.space	4
.if64
RC_OFFSET_LREGS:	.space	SIZEOF_X86_64_REGS
RC_OFFSET_LREG_RETADDR:	.space	SIZEOF_ADDR
.endif
RC_OFFSET_RETADDR:	.space	SIZEOF_ADDR
RC_OFFSET_PARAMS:
RC_OFFSET_FUNCTION:	.space	SIZEOF_ADDR
RC_OFFSET_END:
	.previous

	.section ".text.real_call", "ax", @progbits
	.CODE_DEFAULT
	.globl real_call
real_call:
.if64 ;	/* Preserve registers and switch to protected mode, if applicable */
	call	long_preserve_regs
	call	long_to_prot
	.code32
.endif
	/* Create register dump and function pointer copy on PM stack */
	pushl	( RC_OFFSET_FUNCTION - RC_OFFSET_FUNCTION_COPY - 4 )(%esp)
	pushal

	/* Switch to real mode and move register dump to RM stack  */
	movl	$RC_OFFSET_REGS_END, %ecx
	movl	$RC_TMP_END, %edx
	pushl	$rc_rmode
	movl	$VIRTUAL(rm_default_gdtr_idtr), %esi
	jmp	prot_to_real
	.section ".text16.real_call", "ax", @progbits
	.code16
rc_rmode:
	/* Call real-mode function */
	popal
	call	*( rm_tmpbuf + RC_TMP_FUNCTION )
	pushal

	/* For sanity's sake, clear the direction flag as soon as possible */
	cld

	/* Switch to protected mode and move register dump back to PM stack */
	movl	$RC_OFFSET_REGS_END, %ecx
	xorl	%edx, %edx
	pushl	$VIRTUAL(rc_pmode)
	jmp	real_to_prot
	.section ".text.real_call", "ax", @progbits
	.code32
rc_pmode:
	/* Restore registers */
	popal

.if64 ; /* Switch to long mode and restore registers, if applicable */
	call	prot_to_long
	.code64
	call	long_restore_regs
.endif
	/* Return and discard function parameters */
	ret	$( RC_OFFSET_END - RC_OFFSET_PARAMS )


	/* Default real-mode global and interrupt descriptor table registers */
	.section ".data.rm_default_gdtr_idtr", "aw", @progbits
rm_default_gdtr_idtr:
	.word 0		/* Global descriptor table limit */
	.long 0		/* Global descriptor table base */
	.word 0x03ff	/* Interrupt descriptor table limit */
	.long 0		/* Interrupt descriptor table base */

/****************************************************************************
 * phys_call (protected-mode near call, 32-bit virtual return address)
 * phys_call (long-mode near call, 64-bit virtual return address)
 *
 * Call a function with flat 32-bit physical addressing
 *
 * The non-segment register values will be passed directly to the
 * function.  The segment registers will be set for flat 32-bit
 * physical addressing.  The non-segment register values set by the
 * function will be passed back to the caller.
 *
 * librm.h defines a convenient macro PHYS_CODE() for using phys_call.
 *
 * Parameters:
 *   function : virtual (sic) address of function to call
 *
 ****************************************************************************
 */
	.struct 0
.if64
PHC_OFFSET_LREGS:	.space	SIZEOF_X86_64_REGS
PHC_OFFSET_LREG_RETADDR:.space	SIZEOF_ADDR
.endif
PHC_OFFSET_RETADDR:	.space	SIZEOF_ADDR
PHC_OFFSET_PARAMS:
PHC_OFFSET_FUNCTION:	.space	SIZEOF_ADDR
PHC_OFFSET_END:
	.previous

	.section ".text.phys_call", "ax", @progbits
	.CODE_DEFAULT
	.globl phys_call
phys_call:
.if64 ;	/* Preserve registers and switch to protected mode, if applicable */
	call	long_preserve_regs
	call	long_to_prot
	.code32
.endif
	/* Adjust function pointer to a physical address */
	pushl	%ebp
	movl	VIRTUAL(virt_offset), %ebp
	addl	%ebp, ( PHC_OFFSET_FUNCTION + 4 /* saved %ebp */ )(%esp)
	popl	%ebp

	/* Switch to physical addresses */
	call	prot_to_phys

	/* Call function */
	call	*PHC_OFFSET_FUNCTION(%esp)

	/* For sanity's sake, clear the direction flag as soon as possible */
	cld

	/* Switch to virtual addresses */
	call	phys_to_prot

.if64 ; /* Switch to long mode and restore registers, if applicable */
	call	prot_to_long
	.code64
	call	long_restore_regs
.endif
	/* Return and discard function parameters */
	ret	$( PHC_OFFSET_END - PHC_OFFSET_PARAMS )

/****************************************************************************
 * phys_to_long (protected-mode near call, 32-bit physical return address)
 *
 * Used by COMBOOT.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.phys_to_long", "ax", @progbits
	.code32
phys_to_long:

	/* Switch to virtual addresses */
	call	phys_to_prot

	/* Convert to 32-bit virtual return address */
	pushl	%eax
	movl	VIRTUAL(virt_offset), %eax
	subl	%eax, 4(%esp)
	popl	%eax

	/* Switch to long mode and return */
	jmp	prot_to_long

	/* Expose as _phys_to_virt for use by COMBOOT */
	.globl  _phys_to_virt
	.equ    _phys_to_virt, phys_to_long

	.endif

/****************************************************************************
 * long_to_phys (long-mode near call, 64-bit virtual return address)
 *
 * Used by COMBOOT.
 *
 ****************************************************************************
 */
	.if64

	.section ".text.long_to_phys", "ax", @progbits
	.code64
long_to_phys:

	/* Switch to protected mode */
	call	long_to_prot
	.code32

	/* Convert to 32-bit virtual return address */
	popl	(%esp)

	/* Switch to physical addresses and return */
	jmp	prot_to_phys

	/* Expose as _virt_to_phys for use by COMBOOT */
	.globl  _virt_to_phys
	.equ    _virt_to_phys, long_to_phys

	.endif

/****************************************************************************
 * flatten_real_mode (real-mode near call)
 *
 * Switch to flat real mode
 *
 ****************************************************************************
 */
	.section ".text16.flatten_real_mode", "ax", @progbits
	.code16
	.globl flatten_real_mode
flatten_real_mode:
	/* Modify GDT to use flat real mode */
	movb	$0x8f, real_cs + 6
	movb	$0x8f, real_ds + 6
	/* Call dummy protected-mode function */
	virtcall flatten_dummy
	/* Restore GDT */
	movb	$0x00, real_cs + 6
	movb	$0x00, real_ds + 6
	/* Return */
	ret

	.section ".text.flatten_dummy", "ax", @progbits
	.CODE_DEFAULT
flatten_dummy:
	ret

/****************************************************************************
 * Interrupt wrapper
 *
 * Used by the protected-mode and long-mode interrupt vectors to call
 * the interrupt() function.
 *
 * May be entered with either physical or virtual stack segment.
 ****************************************************************************
 */
	.section ".text.interrupt_wrapper", "ax", @progbits
	.code32
	.globl interrupt_wrapper
interrupt_wrapper:
	/* Preserve registers (excluding already-saved %eax and
	 * otherwise unused registers which are callee-save for both
	 * 32-bit and 64-bit ABIs).
	 */
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi

	/* Expand IRQ number to whole %eax register */
	movzbl	%al, %eax

.if64 ; /* Skip transition to long mode, if applicable */
	movw	%cs, %bx
	cmpw	$LONG_CS, %bx
	je	1f
.endif
	/* Preserve segment registers and original %esp */
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	pushl	%ss
	pushl	%esp

	/* Switch to virtual addressing */
	call	intr_to_prot
.if64
	/* Switch to long mode */
	call	prot_to_long
	.code64

1:	/* Preserve long-mode caller-save registers */
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11

	/* Expand IRQ number to whole %rdi register */
	movl	%eax, %edi
.endif
	/* Call interrupt handler */
	call	interrupt
.if64
	/* Restore long-mode caller-save registers */
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8

	/* Skip transition back to protected mode, if applicable */
	cmpw	$LONG_CS, %bx
	je	1f

	/* Switch to protected mode */
	call	long_to_prot
	.code32
	cmpw	$LONG_CS, %bx
.endif
	/* Restore segment registers and original %esp */
	lss	(%esp), %esp
	popl	%ss
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds

1:	/* Restore registers */
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax

	/* Return from interrupt (with REX prefix if required) */
.if64 ; jne 1f ; .byte 0x48 ; .endif
1:	iret

/****************************************************************************
 * Page tables
 *
 ****************************************************************************
 */
	.section ".pages", "aw", @nobits
	.align	SIZEOF_PT

	/* Page map level 4 entries (PML4Es)
	 *
	 * This comprises
	 *
	 * - PML4E[0x000] covering [0x0000000000000000-0x0000007fffffffff]
	 * - PML4E[0x1ff] covering [0xffffff8000000000-0xffffffffffffffff]
	 *
	 * These point to the PDPT.  This creates some aliased
	 * addresses within unused portions of the 64-bit address
	 * space, but allows us to use just a single PDPT.
	 *
	 * - PDE[...] covering arbitrary 2MB portions of I/O space
	 *
	 * These are 2MB pages created by ioremap() to cover I/O
	 * device addresses.
	 */
pml4e:
	.space	SIZEOF_PT
	.size	pml4e, . - pml4e

	.globl	io_pages
	.equ	io_pages, pml4e

	/* Page directory pointer table entries (PDPTEs)
	 *
	 * This comprises:
	 *
	 * - PDPTE[0x000] covering [0x0000000000000000-0x000000003fffffff]
	 * - PDPTE[0x001] covering [0x0000000040000000-0x000000007fffffff]
	 * - PDPTE[0x002] covering [0x0000000080000000-0x00000000bfffffff]
	 * - PDPTE[0x003] covering [0x00000000c0000000-0x00000000ffffffff]
	 *
	 * These point to the appropriate page directories (in pde_low)
	 * used to identity-map the whole of the 32-bit address space.
	 *
	 * - PDPTE[0x004] covering [0x0000000100000000-0x000000013fffffff]
	 *
	 * This points back to the PML4, allowing the PML4 to be
	 * (ab)used to hold 2MB pages used for I/O device addresses.
	 *
	 * - PDPTE[0x1ff] covering [0xffffffffc0000000-0xffffffffffffffff]
	 *
	 * This points back to the PDPT itself, allowing the PDPT to be
	 * (ab)used to hold PDEs covering .textdata.
	 *
	 * - PDE[N-M] covering [_textdata,_end)
	 *
	 * These are used to point to the page tables (in pte_textdata)
	 * used to map our .textdata section.  Note that each PDE
	 * covers 2MB, so we are likely to use only a single PDE in
	 * practice.
	 */
pdpte:
	.space	SIZEOF_PT
	.size	pdpte, . - pdpte
	.equ	pde_textdata, pdpte /* (ab)use */

	/* Page directory entries (PDEs) for the low 4GB
	 *
	 * This comprises 2048 2MB pages to identity-map the whole of
	 * the 32-bit address space.
	 */
pde_low:
	.equ	PDE_LOW_PTES, ( SIZEOF_LOW_4GB / SIZEOF_2MB_PAGE )
	.equ	PDE_LOW_PTS, ( ( PDE_LOW_PTES * SIZEOF_PTE ) / SIZEOF_PT )
	.space	( PDE_LOW_PTS * SIZEOF_PT )
	.size	pde_low, . - pde_low

	/* Page table entries (PTEs) for .textdata
	 *
	 * This comprises enough 4kB pages to map the whole of
	 * .textdata.  The required number of PTEs is calculated by
	 * the linker script.
	 *
	 * Note that these mappings do not cover the PTEs themselves.
	 * This does not matter, since code running with paging
	 * enabled never needs to access these PTEs.
	 */
pte_textdata:
	/* Allocated by linker script; must be at the end of .textdata */

	.section ".bss.pml4", "aw", @nobits
pml4:	.long	0

/****************************************************************************
 * init_pages (protected-mode near call)
 *
 * Initialise the page tables ready for long mode.
 *
 * Parameters:
 *   %edi : virt_offset
 ****************************************************************************
 */
	.section ".text.init_pages", "ax", @progbits
	.code32
init_pages:
	/* Initialise PML4Es for low 4GB and negative 2GB */
	leal	( VIRTUAL(pdpte) + ( PG_P | PG_RW | PG_US ) )(%edi), %eax
	movl	%eax, VIRTUAL(pml4e)
	movl	%eax, ( VIRTUAL(pml4e) + SIZEOF_PT - SIZEOF_PTE )

	/* Initialise PDPTE for negative 1GB */
	movl	%eax, ( VIRTUAL(pdpte) + SIZEOF_PT - SIZEOF_PTE )

	/* Initialise PDPTE for I/O space */
	leal	( VIRTUAL(pml4e) + ( PG_P | PG_RW | PG_US ) )(%edi), %eax
	movl	%eax, ( VIRTUAL(pdpte) + ( PDE_LOW_PTS * SIZEOF_PTE ) )

	/* Initialise PDPTEs for low 4GB */
	movl	$PDE_LOW_PTS, %ecx
	leal	( VIRTUAL(pde_low) + ( PDE_LOW_PTS * SIZEOF_PT ) + \
		  ( PG_P | PG_RW | PG_US ) )(%edi), %eax
1:	subl	$SIZEOF_PT, %eax
	movl	%eax, ( VIRTUAL(pdpte) - SIZEOF_PTE )(,%ecx,SIZEOF_PTE)
	loop	1b

	/* Initialise PDEs for low 4GB */
	movl	$PDE_LOW_PTES, %ecx
	leal	( 0 + ( PG_P | PG_RW | PG_US | PG_PS ) ), %eax
1:	subl	$SIZEOF_2MB_PAGE, %eax
	movl	%eax, ( VIRTUAL(pde_low) - SIZEOF_PTE )(,%ecx,SIZEOF_PTE)
	loop	1b

	/* Initialise PDEs for .textdata */
	movl	$_textdata_pdes, %ecx
	leal	( VIRTUAL(_etextdata) + ( PG_P | PG_RW | PG_US ) )(%edi), %eax
	movl	$VIRTUAL(_textdata), %ebx
	shrl	$( SIZEOF_2MB_PAGE_LOG2 - SIZEOF_PTE_LOG2 ), %ebx
	andl	$( SIZEOF_PT - 1 ), %ebx
1:	subl	$SIZEOF_PT, %eax
	movl	%eax, (VIRTUAL(pde_textdata) - SIZEOF_PTE)(%ebx,%ecx,SIZEOF_PTE)
	loop	1b

	/* Initialise PTEs for .textdata */
	movl	$_textdata_ptes, %ecx
	leal	( VIRTUAL(_textdata) + ( PG_P | PG_RW | PG_US ) )(%edi), %eax
	addl	$_textdata_paged_len, %eax
1:	subl	$SIZEOF_4KB_PAGE, %eax
	movl	%eax, ( VIRTUAL(pte_textdata) - SIZEOF_PTE )(,%ecx,SIZEOF_PTE)
	loop	1b

	/* Record PML4 physical address */
	leal	VIRTUAL(pml4e)(%edi), %eax
	movl	%eax, VIRTUAL(pml4)

	/* Return */
	ret
