#ifndef STDDEF_H
#define STDDEF_H

FILE_LICENCE ( GPL2_ONLY );

/* for size_t */
#include <stdint.h>

#undef NULL
#define NULL ((void *)0)

#undef offsetof
#if ( defined ( __GNUC__ ) && ( __GNUC__ > 3 ) )
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#undef container_of
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

/* __WCHAR_TYPE__ is defined by gcc and will change if -fshort-wchar is used */
#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__ uint16_t
#endif
#ifndef __WINT_TYPE__
#define __WINT_TYPE__ int
#endif
typedef __WCHAR_TYPE__ wchar_t;
typedef __WINT_TYPE__ wint_t;

#endif /* STDDEF_H */
