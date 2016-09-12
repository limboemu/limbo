#ifndef _BITS_COMPILER_H
#define _BITS_COMPILER_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** Dummy relocation type */
#define RELOC_TYPE_NONE R_AARCH64_NULL

#ifndef ASSEMBLY

#define __asmcall
#define __libgcc

#endif /* ASSEMBLY */

#endif /*_BITS_COMPILER_H */
