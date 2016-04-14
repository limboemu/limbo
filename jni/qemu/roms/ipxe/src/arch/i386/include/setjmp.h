#ifndef ETHERBOOT_SETJMP_H
#define ETHERBOOT_SETJMP_H

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <realmode.h>

/** A jump buffer */
typedef struct {
	uint32_t retaddr;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
} jmp_buf[1];

/** A real-mode-extended jump buffer */
typedef struct {
	jmp_buf env;
	uint16_t rm_ss;
	uint16_t rm_sp;
} rmjmp_buf[1];

extern int __asmcall setjmp ( jmp_buf env );
extern void __asmcall longjmp ( jmp_buf env, int val );

#define rmsetjmp( _env ) ( {			\
	(_env)->rm_ss = rm_ss;			\
	(_env)->rm_sp = rm_sp;			\
	setjmp ( (_env)->env ); } )		\

#define rmlongjmp( _env, _val ) do {		\
	rm_ss = (_env)->rm_ss;			\
	rm_sp = (_env)->rm_sp;			\
	longjmp ( (_env)->env, (_val) );	\
	} while ( 0 )

#endif /* ETHERBOOT_SETJMP_H */
