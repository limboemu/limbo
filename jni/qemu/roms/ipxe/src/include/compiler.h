#ifndef COMPILER_H
#define COMPILER_H

/*
 * Doxygen can't cope with some of the more esoteric areas of C, so we
 * make its life simpler.
 *
 */
#ifdef DOXYGEN
#define __attribute__(x)
#endif

/** @file
 *
 * Global compiler definitions.
 *
 * This file is implicitly included by every @c .c file in Etherboot.
 * It defines global macros such as DBG().
 *
 * We arrange for each object to export the symbol @c obj_OBJECT
 * (where @c OBJECT is the object name, e.g. @c rtl8139) as a global
 * symbol, so that the linker can drag in selected object files from
 * the library using <tt> -u obj_OBJECT </tt>.
 *
 */

/* Force visibility of all symbols to "hidden", i.e. inform gcc that
 * all symbol references resolve strictly within our final binary.
 * This avoids unnecessary PLT/GOT entries on x86_64.
 *
 * This is a stronger claim than specifying "-fvisibility=hidden",
 * since it also affects symbols marked with "extern".
 */
#ifndef ASSEMBLY
#if __GNUC__ >= 4
#pragma GCC visibility push(hidden)
#endif
#endif /* ASSEMBLY */

#undef _S1
#undef _S2
#undef _C1
#undef _C2

/** Concatenate non-expanded arguments */
#define _C1( x, y ) x ## y
/** Concatenate expanded arguments */
#define _C2( x, y ) _C1 ( x, y )

/** Stringify non-expanded argument */
#define _S1( x ) #x
/** Stringify expanded argument */
#define _S2( x ) _S1 ( x )

/**
 * @defgroup symmacros Macros to provide or require explicit symbols
 * @{
 */

/** Provide a symbol within this object file */
#ifdef ASSEMBLY
#define PROVIDE_SYMBOL( _sym )				\
	.section ".provided", "a", @nobits ;		\
	.hidden _sym ;					\
	.globl	_sym ;					\
	_sym: ;						\
	.previous
#else /* ASSEMBLY */
#define PROVIDE_SYMBOL( _sym )				\
	char _sym[0]					\
	  __attribute__ (( section ( ".provided" ) ))
#endif /* ASSEMBLY */

/** Require a symbol within this object file
 *
 * The symbol is referenced by a relocation in a discarded section, so
 * if it is not available at link time the link will fail.
 */
#ifdef ASSEMBLY
#define REQUIRE_SYMBOL( _sym )				\
	.section ".discard", "a", @progbits ;		\
	.extern	_sym ;					\
	.long	_sym ;					\
	.previous
#else /* ASSEMBLY */
#define REQUIRE_SYMBOL( _sym )				\
	extern char _sym;				\
	static char * _C2 ( _C2 ( __require_, _sym ), _C2 ( _, __LINE__ ) ) \
		__attribute__ (( section ( ".discard" ), used )) \
		= &_sym
#endif

/** Request that a symbol be available at runtime
 *
 * The requested symbol is entered as undefined into the symbol table
 * for this object, so the linker will pull in other object files as
 * necessary to satisfy the reference. However, the undefined symbol
 * is not referenced in any relocations, so the link can still succeed
 * if no file contains it.
 *
 * A symbol passed to this macro may not be referenced anywhere
 * else in the file. If you want to do that, see IMPORT_SYMBOL().
 */
#ifdef ASSEMBLY
#define REQUEST_SYMBOL( _sym )				\
	.equ	__need_ ## _sym, _sym
#else /* ASSEMBLY */
#define REQUEST_SYMBOL( _sym )				\
	__asm__ ( ".equ\t__need_" #_sym ", " #_sym )
#endif /* ASSEMBLY */

/** Set up a symbol to be usable in another file by IMPORT_SYMBOL()
 *
 * The symbol must already be marked as global.
 */
#define EXPORT_SYMBOL( _sym )	PROVIDE_SYMBOL ( __export_ ## _sym )

/** Make a symbol usable to this file if available at link time
 *
 * If no file passed to the linker contains the symbol, it will have
 * @c NULL value to future uses. Keep in mind that the symbol value is
 * really the @e address of a variable or function; see the code
 * snippet below.
 *
 * In C using IMPORT_SYMBOL, you must specify the declaration as the
 * second argument, for instance
 *
 * @code
 *   IMPORT_SYMBOL ( my_func, int my_func ( int arg ) );
 *   IMPORT_SYMBOL ( my_var, int my_var );
 *
 *   void use_imports ( void ) {
 * 	if ( my_func && &my_var )
 * 	   my_var = my_func ( my_var );
 *   }
 * @endcode
 *
 * GCC considers a weak declaration to override a strong one no matter
 * which comes first, so it is safe to include a header file declaring
 * the imported symbol normally, but providing the declaration to
 * IMPORT_SYMBOL is still required.
 *
 * If no EXPORT_SYMBOL declaration exists for the imported symbol in
 * another file, the behavior will be most likely be identical to that
 * for an unavailable symbol.
 */
#ifdef ASSEMBLY
#define IMPORT_SYMBOL( _sym )				\
	REQUEST_SYMBOL ( __export_ ## _sym ) ;		\
	.weak	_sym
#else /* ASSEMBLY */
#define IMPORT_SYMBOL( _sym, _decl )			\
	REQUEST_SYMBOL ( __export_ ## _sym ) ;		\
	extern _decl __attribute__ (( weak ))
#endif

/** @} */

/**
 * @defgroup objmacros Macros to provide or require explicit objects
 * @{
 */

#define PREFIX_OBJECT( _prefix ) _C2 ( _prefix, OBJECT )
#define OBJECT_SYMBOL PREFIX_OBJECT ( obj_ )
#define REQUEST_EXPANDED( _sym ) REQUEST_SYMBOL ( _sym )
#define CONFIG_SYMBOL PREFIX_OBJECT ( obj_config_ )

/** Always provide the symbol for the current object (defined by -DOBJECT) */
PROVIDE_SYMBOL ( OBJECT_SYMBOL );

/** Pull in an object-specific configuration file if available */
REQUEST_EXPANDED ( CONFIG_SYMBOL );

/** Explicitly require another object */
#define REQUIRE_OBJECT( _obj ) REQUIRE_SYMBOL ( obj_ ## _obj )

/** Pull in another object if it exists */
#define REQUEST_OBJECT( _obj ) REQUEST_SYMBOL ( obj_ ## _obj )

/** @} */

/** Select file identifier for errno.h (if used) */
#define ERRFILE PREFIX_OBJECT ( ERRFILE_ )

#ifndef ASSEMBLY

/** Declare a function as weak (use *before* the definition)
 *
 * Due to a bug in at least GCC 4.4.4 and earlier, weak symbols may be
 * inlined if they have hidden visibility (see above for why hidden
 * visibility is used).  This results in the non-weak symbol never
 * being used, so explicitly mark the function as noinline to prevent
 * inlining.
 */
#define __weak		__attribute__ (( weak, noinline ))

/** Prevent a function from being optimized away without inlining
 *
 * Calls to functions with void return type that contain no code in their body
 * may be removed by gcc's optimizer even when inlining is inhibited. Placing
 * this macro in the body of the function prevents that from occurring.
 */
#define __keepme	asm("");

#endif

/** @defgroup dbg Debugging infrastructure
 * @{
 */

/** @def DBG
 *
 * Print a debugging message.
 *
 * The debug level is set at build time by specifying the @c DEBUG=
 * parameter on the @c make command line.  For example, to enable
 * debugging for the PCI bus functions (in pci.c) in a @c .dsk image
 * for the @c rtl8139 card, you could use the command line
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci
 *
 * @endcode
 *
 * This will enable the debugging statements (DBG()) in pci.c.  If
 * debugging is not enabled, DBG() statements will be ignored.
 *
 * You can enable debugging in several objects simultaneously by
 * separating them with commas, as in
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci,buffer,heap
 *
 * @endcode
 *
 * You can increase the debugging level for an object by specifying it
 * with @c :N, where @c N is the level, as in
 *
 * @code
 *
 *   make bin/rtl8139.dsk DEBUG=pci,buffer:2,heap
 *
 * @endcode
 *
 * which would enable debugging for the PCI, buffer-handling and
 * heap-allocation code, with the buffer-handling code at level 2.
 *
 */

#ifndef DBGLVL_MAX
#define NDEBUG
#define DBGLVL_MAX 0
#endif

#ifndef ASSEMBLY

/** printf() for debugging */
extern void __attribute__ (( format ( printf, 1, 2 ) ))
dbg_printf ( const char *fmt, ... );
extern void dbg_autocolourise ( unsigned long id );
extern void dbg_decolourise ( void );
extern void dbg_hex_dump_da ( unsigned long dispaddr,
			      const void *data, unsigned long len );
extern void dbg_md5_da ( unsigned long dispaddr,
			 const void *data, unsigned long len );
extern void dbg_pause ( void );
extern void dbg_more ( void );

/* Allow for selective disabling of enabled debug levels */
#if DBGLVL_MAX
int __debug_disable;
#define DBGLVL ( DBGLVL_MAX & ~__debug_disable )
#define DBG_DISABLE( level ) do {				\
	__debug_disable |= (level);				\
	} while ( 0 )
#define DBG_ENABLE( level ) do {				\
	__debug_disable &= ~(level);				\
	} while ( 0 )
#else
#define DBGLVL 0
#define DBG_DISABLE( level ) do { } while ( 0 )
#define DBG_ENABLE( level ) do { } while ( 0 )
#endif

#define DBGLVL_LOG	1
#define DBG_LOG		( DBGLVL & DBGLVL_LOG )
#define DBGLVL_EXTRA	2
#define DBG_EXTRA	( DBGLVL & DBGLVL_EXTRA )
#define DBGLVL_PROFILE	4
#define DBG_PROFILE	( DBGLVL & DBGLVL_PROFILE )
#define DBGLVL_IO	8
#define DBG_IO		( DBGLVL & DBGLVL_IO )

/**
 * Print debugging message if we are at a certain debug level
 *
 * @v level		Debug level
 * @v ...		printf() argument list
 */
#define DBG_IF( level, ... ) do {				\
		if ( DBG_ ## level ) {				\
			dbg_printf ( __VA_ARGS__ );		\
		}						\
	} while ( 0 )

/**
 * Print a hex dump if we are at a certain debug level
 *
 * @v level		Debug level
 * @v dispaddr		Display address
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_HDA_IF( level, dispaddr, data, len )  do {		\
		if ( DBG_ ## level ) {				\
			union {					\
				unsigned long ul;		\
				typeof ( dispaddr ) raw;	\
			} da;					\
			da.ul = 0;				\
			da.raw = dispaddr;			\
			dbg_hex_dump_da ( da.ul, data, len );	\
		}						\
	} while ( 0 )

/**
 * Print a hex dump if we are at a certain debug level
 *
 * @v level		Debug level
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_HD_IF( level, data, len ) do {			\
		const void *_data = data;			\
		DBG_HDA_IF ( level, _data, _data, len );	\
	} while ( 0 )

/**
 * Print an MD5 checksum if we are at a certain debug level
 *
 * @v level		Debug level
 * @v dispaddr		Display address
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_MD5A_IF( level, dispaddr, data, len )  do {		\
		if ( DBG_ ## level ) {				\
			union {					\
				unsigned long ul;		\
				typeof ( dispaddr ) raw;	\
			} da;					\
			da.ul = 0;				\
			da.raw = dispaddr;			\
			dbg_md5_da ( da.ul, data, len );	\
		}						\
	} while ( 0 )

/**
 * Print an MD5 checksum if we are at a certain debug level
 *
 * @v level		Debug level
 * @v data		Data to print
 * @v len		Length of data
 */
#define DBG_MD5_IF( level, data, len ) do {			\
		const void *_data = data;			\
		DBG_MD5A_IF ( level, _data, _data, len );	\
	} while ( 0 )

/**
 * Prompt for key press if we are at a certain debug level
 *
 * @v level		Debug level
 */
#define DBG_PAUSE_IF( level ) do {				\
		if ( DBG_ ## level ) {				\
			dbg_pause();				\
		}						\
	} while ( 0 )

/**
 * Prompt for more output data if we are at a certain debug level
 *
 * @v level		Debug level
 */
#define DBG_MORE_IF( level ) do {				\
		if ( DBG_ ## level ) {				\
			dbg_more();				\
		}						\
	} while ( 0 )

/**
 * Select colour for debug messages if we are at a certain debug level
 *
 * @v level		Debug level
 * @v id		Message stream ID
 */
#define DBG_AC_IF( level, id ) do {				\
		if ( DBG_ ## level ) {				\
			union {					\
				unsigned long ul;		\
				typeof ( id ) raw;		\
			} dbg_stream;				\
			dbg_stream.ul = 0;			\
			dbg_stream.raw = id;			\
			dbg_autocolourise ( dbg_stream.ul );	\
		}						\
	} while ( 0 )

/**
 * Revert colour for debug messages if we are at a certain debug level
 *
 * @v level		Debug level
 */
#define DBG_DC_IF( level ) do {					\
		if ( DBG_ ## level ) {				\
			dbg_decolourise();			\
		}						\
	} while ( 0 )

/* Autocolourising versions of the DBGxxx_IF() macros */

#define DBGC_IF( level, id, ... ) do {				\
		DBG_AC_IF ( level, id );			\
		DBG_IF ( level, __VA_ARGS__ );			\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_HDA_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_HDA_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_HD_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_HD_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_MD5A_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_MD5A_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_MD5_IF( level, id, ... ) do {			\
		DBG_AC_IF ( level, id );			\
		DBG_MD5_IF ( level, __VA_ARGS__ );		\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_PAUSE_IF( level, id ) do {				\
		DBG_AC_IF ( level, id );			\
		DBG_PAUSE_IF ( level );				\
		DBG_DC_IF ( level );				\
	} while ( 0 )

#define DBGC_MORE_IF( level, id ) do {				\
		DBG_AC_IF ( level, id );			\
		DBG_MORE_IF ( level );				\
		DBG_DC_IF ( level );				\
	} while ( 0 )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( LOG, ... )*/

#define DBG( ... )		DBG_IF		( LOG, ##__VA_ARGS__ )
#define DBG_HDA( ... )		DBG_HDA_IF	( LOG, ##__VA_ARGS__ )
#define DBG_HD( ... )		DBG_HD_IF	( LOG, ##__VA_ARGS__ )
#define DBG_MD5A( ... )		DBG_MD5A_IF	( LOG, ##__VA_ARGS__ )
#define DBG_MD5( ... )		DBG_MD5_IF	( LOG, ##__VA_ARGS__ )
#define DBG_PAUSE( ... )	DBG_PAUSE_IF	( LOG, ##__VA_ARGS__ )
#define DBG_MORE( ... )		DBG_MORE_IF	( LOG, ##__VA_ARGS__ )
#define DBGC( ... )		DBGC_IF		( LOG, ##__VA_ARGS__ )
#define DBGC_HDA( ... )		DBGC_HDA_IF	( LOG, ##__VA_ARGS__ )
#define DBGC_HD( ... )		DBGC_HD_IF	( LOG, ##__VA_ARGS__ )
#define DBGC_MD5A( ... )	DBGC_MD5A_IF	( LOG, ##__VA_ARGS__ )
#define DBGC_MD5( ... )		DBGC_MD5_IF	( LOG, ##__VA_ARGS__ )
#define DBGC_PAUSE( ... )	DBGC_PAUSE_IF	( LOG, ##__VA_ARGS__ )
#define DBGC_MORE( ... )	DBGC_MORE_IF	( LOG, ##__VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( EXTRA, ... )*/

#define DBG2( ... )		DBG_IF		( EXTRA, ##__VA_ARGS__ )
#define DBG2_HDA( ... )		DBG_HDA_IF	( EXTRA, ##__VA_ARGS__ )
#define DBG2_HD( ... )		DBG_HD_IF	( EXTRA, ##__VA_ARGS__ )
#define DBG2_MD5A( ... )	DBG_MD5A_IF	( EXTRA, ##__VA_ARGS__ )
#define DBG2_MD5( ... )		DBG_MD5_IF	( EXTRA, ##__VA_ARGS__ )
#define DBG2_PAUSE( ... )	DBG_PAUSE_IF	( EXTRA, ##__VA_ARGS__ )
#define DBG2_MORE( ... )	DBG_MORE_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2( ... )		DBGC_IF		( EXTRA, ##__VA_ARGS__ )
#define DBGC2_HDA( ... )	DBGC_HDA_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2_HD( ... )		DBGC_HD_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2_MD5A( ... )	DBGC_MD5A_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2_MD5( ... )	DBGC_MD5_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2_PAUSE( ... )	DBGC_PAUSE_IF	( EXTRA, ##__VA_ARGS__ )
#define DBGC2_MORE( ... )	DBGC_MORE_IF	( EXTRA, ##__VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( PROFILE, ... )*/

#define DBGP( ... )		DBG_IF		( PROFILE, ##__VA_ARGS__ )
#define DBGP_HDA( ... )		DBG_HDA_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGP_HD( ... )		DBG_HD_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGP_MD5A( ... )	DBG_MD5A_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGP_MD5( ... )		DBG_MD5_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGP_PAUSE( ... )	DBG_PAUSE_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGP_MORE( ... )	DBG_MORE_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP( ... )		DBGC_IF		( PROFILE, ##__VA_ARGS__ )
#define DBGCP_HDA( ... )	DBGC_HDA_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP_HD( ... )		DBGC_HD_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP_MD5A( ... )	DBGC_MD5A_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP_MD5( ... )	DBGC_MD5_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP_PAUSE( ... )	DBGC_PAUSE_IF	( PROFILE, ##__VA_ARGS__ )
#define DBGCP_MORE( ... )	DBGC_MORE_IF	( PROFILE, ##__VA_ARGS__ )

/* Versions of the DBGxxx_IF() macros that imply DBGxxx_IF( IO, ... )*/

#define DBGIO( ... )		DBG_IF		( IO, ##__VA_ARGS__ )
#define DBGIO_HDA( ... )	DBG_HDA_IF	( IO, ##__VA_ARGS__ )
#define DBGIO_HD( ... )		DBG_HD_IF	( IO, ##__VA_ARGS__ )
#define DBGIO_MD5A( ... )	DBG_MD5A_IF	( IO, ##__VA_ARGS__ )
#define DBGIO_MD5( ... )	DBG_MD5_IF	( IO, ##__VA_ARGS__ )
#define DBGIO_PAUSE( ... )	DBG_PAUSE_IF	( IO, ##__VA_ARGS__ )
#define DBGIO_MORE( ... )	DBG_MORE_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO( ... )		DBGC_IF		( IO, ##__VA_ARGS__ )
#define DBGCIO_HDA( ... )	DBGC_HDA_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO_HD( ... )	DBGC_HD_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO_MD5A( ... )	DBGC_MD5A_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO_MD5( ... )	DBGC_MD5_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO_PAUSE( ... )	DBGC_PAUSE_IF	( IO, ##__VA_ARGS__ )
#define DBGCIO_MORE( ... )	DBGC_MORE_IF	( IO, ##__VA_ARGS__ )

#endif /* ASSEMBLY */
/** @} */

/** @defgroup attrs Miscellaneous attributes
 * @{
 */
#ifndef ASSEMBLY

/** Declare a variable or data structure as unused. */
#define __unused __attribute__ (( unused ))

/**
 * Declare a function as pure - i.e. without side effects
 */
#define __pure __attribute__ (( pure ))

/**
 * Declare a function as const - i.e. it does not access global memory
 * (including dereferencing pointers passed to it) at all.
 * Must also not call any non-const functions.
 */
#define __const __attribute__ (( const ))

/**
 * Declare a function's pointer parameters as non-null - i.e. force
 * compiler to check pointers at compile time and enable possible
 * optimizations based on that fact
 */
#define __nonnull __attribute__ (( nonnull ))

/**
 * Declare a pointer returned by a function as a unique memory address
 * as returned by malloc-type functions.
 */
#define __malloc __attribute__ (( malloc ))

/**
 * Declare a function as used.
 *
 * Necessary only if the function is called only from assembler code.
 */
#define __used __attribute__ (( used ))

/** Declare a data structure to be aligned with 16-byte alignment */
#define __aligned __attribute__ (( aligned ( 16 ) ))

/** Declare a function to be always inline */
#define __always_inline __attribute__ (( always_inline ))

/* Force all inline functions to not be instrumented
 *
 * This is required to cope with what seems to be a long-standing gcc
 * bug, in which -finstrument-functions will cause instances of
 * inlined functions to be reported as further calls to the
 * *containing* function.  This makes instrumentation very difficult
 * to use.
 *
 * Work around this problem by adding the no_instrument_function
 * attribute to all inlined functions.
 */
#define inline inline __attribute__ (( no_instrument_function ))

/**
 * Shared data.
 *
 * To save space in the binary when multiple-driver images are
 * compiled, uninitialised data areas can be shared between drivers.
 * This will typically be used to share statically-allocated receive
 * and transmit buffers between drivers.
 *
 * Use as e.g.
 *
 * @code
 *
 *   struct {
 *	char	rx_buf[NUM_RX_BUF][RX_BUF_SIZE];
 *	char	tx_buf[TX_BUF_SIZE];
 *   } my_static_data __shared;
 *
 * @endcode
 *
 */
#define __shared __asm__ ( "_shared_bss" ) __aligned

#endif /* ASSEMBLY */
/** @} */

/**
 * Optimisation barrier
 */
#ifndef ASSEMBLY
#define barrier() __asm__ __volatile__ ( "" : : : "memory" )
#endif /* ASSEMBLY */

/**
 * @defgroup licences Licence declarations
 *
 * For reasons that are partly historical, various different files
 * within the iPXE codebase have differing licences.
 *
 * @{
 */

/** Declare a file as being in the public domain
 *
 * This licence declaration is applicable when a file states itself to
 * be in the public domain.
 */
#define FILE_LICENCE_PUBLIC_DOMAIN \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__public_domain__ ) )

/** Declare a file as being under version 2 (or later) of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under the GNU GPL; "either version 2 of the License, or
 * (at your option) any later version".
 */
#define FILE_LICENCE_GPL2_OR_LATER \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__gpl2_or_later__ ) )

/** Declare a file as being under version 2 of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under version 2 of the GPL, and does not include the
 * "or, at your option, any later version" clause.
 */
#define FILE_LICENCE_GPL2_ONLY \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__gpl2_only__ ) )

/** Declare a file as being under any version of the GNU GPL
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under the GPL, but does not specify a version.
 *
 * According to section 9 of the GPLv2, "If the Program does not
 * specify a version number of this License, you may choose any
 * version ever published by the Free Software Foundation".
 */
#define FILE_LICENCE_GPL_ANY \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__gpl_any__ ) )

/** Declare a file as being under the three-clause BSD licence
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under terms allowing redistribution in source and
 * binary forms (with or without modification) provided that:
 *
 *     redistributions of source code retain the copyright notice,
 *     list of conditions and any attached disclaimers
 *
 *     redistributions in binary form reproduce the copyright notice,
 *     list of conditions and any attached disclaimers in the
 *     documentation and/or other materials provided with the
 *     distribution
 *
 *     the name of the author is not used to endorse or promote
 *     products derived from the software without specific prior
 *     written permission
 *
 * It is not necessary for the file to explicitly state that it is
 * under a "BSD" licence; only that the licensing terms be
 * functionally equivalent to the standard three-clause BSD licence.
 */
#define FILE_LICENCE_BSD3 \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__bsd3__ ) )

/** Declare a file as being under the two-clause BSD licence
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under terms allowing redistribution in source and
 * binary forms (with or without modification) provided that:
 *
 *     redistributions of source code retain the copyright notice,
 *     list of conditions and any attached disclaimers
 *
 *     redistributions in binary form reproduce the copyright notice,
 *     list of conditions and any attached disclaimers in the
 *     documentation and/or other materials provided with the
 *     distribution
 *
 * It is not necessary for the file to explicitly state that it is
 * under a "BSD" licence; only that the licensing terms be
 * functionally equivalent to the standard two-clause BSD licence.
 */
#define FILE_LICENCE_BSD2 \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__bsd2__ ) )

/** Declare a file as being under the one-clause MIT-style licence
 *
 * This licence declaration is applicable when a file states itself to
 * be licensed under terms allowing redistribution for any purpose
 * with or without fee, provided that the copyright notice and
 * permission notice appear in all copies.
 */
#define FILE_LICENCE_MIT \
	PROVIDE_SYMBOL ( PREFIX_OBJECT ( __licence__mit__ ) )

/** Declare a particular licence as applying to a file */
#define FILE_LICENCE( _licence ) FILE_LICENCE_ ## _licence

/** @} */

/* This file itself is under GPLv2-or-later */
FILE_LICENCE ( GPL2_OR_LATER );

#include <bits/compiler.h>

#endif /* COMPILER_H */
