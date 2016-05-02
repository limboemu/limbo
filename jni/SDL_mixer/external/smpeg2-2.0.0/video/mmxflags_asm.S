
#if defined(i386) && defined(USE_MMX)

.text
	.align 4
.globl cpu_flags
	.type	 cpu_flags,@function
cpu_flags: 
        pushfl
        popl %eax

        movl %eax,%ecx

        xorl $0x040000,%eax
        pushl %eax

        popfl
        pushfl

        popl %eax
        xorl %ecx,%eax
        jz cpu_flags.L1   # Processor is 386

        pushl %ecx
        popfl

        movl %ecx,%eax
        xorl $0x200000,%eax

        pushl %eax
        popfl
        pushfl

        popl %eax
        xorl %ecx,%eax
        je cpu_flags.L1

        pushl %ebx

        movl $1,%eax
        cpuid
        movl %edx,%eax

        popl %ebx

cpu_flags.L1: 
        ret
.Lfe1:
	.size	 cpu_flags,.Lfe1-cpu_flags

#endif /* i386 && USE_MMX */

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
