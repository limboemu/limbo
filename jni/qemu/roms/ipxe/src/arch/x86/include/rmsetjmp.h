#ifndef _RMSETJMP_H
#define _RMSETJMP_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <setjmp.h>
#include <realmode.h>

/** A real-mode-extended jump buffer */
typedef struct {
	/** Jump buffer */
	jmp_buf env;
	/** Real-mode stack pointer */
	segoff_t rm_stack;
} rmjmp_buf[1];

#define rmsetjmp( _env ) ( {					\
	(_env)->rm_stack.segment = rm_ss;			\
	(_env)->rm_stack.offset = rm_sp;			\
	setjmp ( (_env)->env ); } )				\

#define rmlongjmp( _env, _val ) do {				\
	rm_ss = (_env)->rm_stack.segment;			\
	rm_sp = (_env)->rm_stack.offset;			\
	longjmp ( (_env)->env, (_val) );			\
	} while ( 0 )

#endif /* _RMSETJMP_H */
