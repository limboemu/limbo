#include <ipxe/io.h>
#include <registers.h>

/*
 * Originally by Eric Biederman
 *
 * Heavily modified by Michael Brown 
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/* Linker symbols */
extern char _textdata[];
extern char _etextdata[];

/* within 1MB of 4GB is too close. 
 * MAX_ADDR is the maximum address we can easily do DMA to.
 *
 * Not sure where this constraint comes from, but kept it from Eric's
 * old code - mcb30
 */
#define MAX_ADDR (0xfff00000UL)

/* Preserve alignment to a 4kB page
 *
 * Required for x86_64, and doesn't hurt for i386.
 */
#define ALIGN 4096

/**
 * Relocate iPXE
 *
 * @v ebp		Maximum address to use for relocation
 * @ret esi		Current physical address
 * @ret edi		New physical address
 * @ret ecx		Length to copy
 *
 * This finds a suitable location for iPXE near the top of 32-bit
 * address space, and returns the physical address of the new location
 * to the prefix in %edi.
 */
__asmcall void relocate ( struct i386_all_regs *ix86 ) {
	struct memory_map memmap;
	uint32_t start, end, size, padded_size, max;
	uint32_t new_start, new_end;
	unsigned i;

	/* Get memory map and current location */
	get_memmap ( &memmap );
	start = virt_to_phys ( _textdata );
	end = virt_to_phys ( _etextdata );
	size = ( end - start );
	padded_size = ( size + ALIGN - 1 );

	DBG ( "Relocate: currently at [%x,%x)\n"
	      "...need %x bytes for %d-byte alignment\n",
	      start, end, padded_size, ALIGN );

	/* Determine maximum usable address */
	max = MAX_ADDR;
	if ( ix86->regs.ebp < max ) {
		max = ix86->regs.ebp;
		DBG ( "Limiting relocation to [0,%x)\n", max );
	}

	/* Walk through the memory map and find the highest address
	 * below 4GB that iPXE will fit into.
	 */
	new_end = end;
	for ( i = 0 ; i < memmap.count ; i++ ) {
		struct memory_region *region = &memmap.regions[i];
		uint32_t r_start, r_end;

		DBG ( "Considering [%llx,%llx)\n", region->start, region->end);
		
		/* Truncate block to maximum address.  This will be
		 * less than 4GB, which means that we can get away
		 * with using just 32-bit arithmetic after this stage.
		 */
		if ( region->start > max ) {
			DBG ( "...starts after max=%x\n", max );
			continue;
		}
		r_start = region->start;
		if ( region->end > max ) {
			DBG ( "...end truncated to max=%x\n", max );
			r_end = max;
		} else {
			r_end = region->end;
		}
		DBG ( "...usable portion is [%x,%x)\n", r_start, r_end );

		/* If we have rounded down r_end below r_ start, skip
		 * this block.
		 */
		if ( r_end < r_start ) {
			DBG ( "...truncated to negative size\n" );
			continue;
		}

		/* Check that there is enough space to fit in iPXE */
		if ( ( r_end - r_start ) < size ) {
			DBG ( "...too small (need %x bytes)\n", size );
			continue;
		}

		/* If the start address of the iPXE we would
		 * place in this block is higher than the end address
		 * of the current highest block, use this block.
		 *
		 * Note that this avoids overlaps with the current
		 * iPXE, as well as choosing the highest of all viable
		 * blocks.
		 */
		if ( ( r_end - size ) > new_end ) {
			new_end = r_end;
			DBG ( "...new best block found.\n" );
		}
	}

	/* Calculate new location of iPXE, and align it to the
	 * required alignemnt.
	 */
	new_start = new_end - padded_size;
	new_start += ( ( start - new_start ) & ( ALIGN - 1 ) );
	new_end = new_start + size;

	DBG ( "Relocating from [%x,%x) to [%x,%x)\n",
	      start, end, new_start, new_end );
	
	/* Let prefix know what to copy */
	ix86->regs.esi = start;
	ix86->regs.edi = new_start;
	ix86->regs.ecx = size;
}
