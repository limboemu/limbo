#ifndef _IPXE_ARM_IO_H
#define _IPXE_ARM_IO_H

/** @file
 *
 * iPXE I/O API for ARM
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#ifdef IOAPI_ARM
#define IOAPI_PREFIX_arm
#else
#define IOAPI_PREFIX_arm __arm_
#endif

/*
 * Memory space mappings
 *
 */

/** Page shift */
#define PAGE_SHIFT 12

/*
 * Physical<->Bus address mappings
 *
 */

static inline __always_inline unsigned long
IOAPI_INLINE ( arm, phys_to_bus ) ( unsigned long phys_addr ) {
	return phys_addr;
}

static inline __always_inline unsigned long
IOAPI_INLINE ( arm, bus_to_phys ) ( unsigned long bus_addr ) {
	return bus_addr;
}

/*
 * MMIO reads and writes up to native word size
 *
 */

#define ARM_READX( _api_func, _type, _insn_suffix, _reg_prefix )	      \
static inline __always_inline _type					      \
IOAPI_INLINE ( arm, _api_func ) ( volatile _type *io_addr ) {		      \
	_type data;							      \
	__asm__ __volatile__ ( "ldr" _insn_suffix " %" _reg_prefix "0, %1"    \
			       : "=r" ( data ) : "Qo" ( *io_addr ) );	      \
	return data;							      \
}
#ifdef __aarch64__
ARM_READX ( readb, uint8_t, "b", "w" );
ARM_READX ( readw, uint16_t, "h", "w" );
ARM_READX ( readl, uint32_t, "", "w" );
ARM_READX ( readq, uint64_t, "", "" );
#else
ARM_READX ( readb, uint8_t, "b", "" );
ARM_READX ( readw, uint16_t, "h", "" );
ARM_READX ( readl, uint32_t, "", "" );
#endif

#define ARM_WRITEX( _api_func, _type, _insn_suffix, _reg_prefix )			\
static inline __always_inline void					      \
IOAPI_INLINE ( arm, _api_func ) ( _type data, volatile _type *io_addr ) {     \
	__asm__ __volatile__ ( "str" _insn_suffix " %" _reg_prefix "0, %1"    \
			       : : "r" ( data ), "Qo" ( *io_addr ) );	      \
}
#ifdef __aarch64__
ARM_WRITEX ( writeb, uint8_t, "b", "w" );
ARM_WRITEX ( writew, uint16_t, "h", "w" );
ARM_WRITEX ( writel, uint32_t, "", "w" );
ARM_WRITEX ( writeq, uint64_t, "", "" );
#else
ARM_WRITEX ( writeb, uint8_t, "b", "" );
ARM_WRITEX ( writew, uint16_t, "h", "" );
ARM_WRITEX ( writel, uint32_t, "", "" );
#endif

/*
 * Slow down I/O
 *
 */
static inline __always_inline void
IOAPI_INLINE ( arm, iodelay ) ( void ) {
	/* Nothing to do */
}

/*
 * Memory barrier
 *
 */
static inline __always_inline void
IOAPI_INLINE ( arm, mb ) ( void ) {

#ifdef __aarch64__
	__asm__ __volatile__ ( "dmb sy" );
#else
	__asm__ __volatile__ ( "dmb" );
#endif
}

#endif /* _IPXE_ARM_IO_H */
