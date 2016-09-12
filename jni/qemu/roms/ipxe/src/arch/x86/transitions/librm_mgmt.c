/*
 * librm: a library for interfacing to real-mode code
 *
 * Michael Brown <mbrown@fensystems.co.uk>
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <strings.h>
#include <assert.h>
#include <ipxe/profile.h>
#include <realmode.h>
#include <pic8259.h>

/*
 * This file provides functions for managing librm.
 *
 */

/** The interrupt wrapper */
extern char interrupt_wrapper[];

/** The interrupt vectors */
static struct interrupt_vector intr_vec[NUM_INT];

/** The 32-bit interrupt descriptor table */
static struct interrupt32_descriptor
idt32[NUM_INT] __attribute__ (( aligned ( 16 ) ));

/** The 32-bit interrupt descriptor table register */
struct idtr32 idtr32 = {
	.limit = ( sizeof ( idt32 ) - 1 ),
};

/** The 64-bit interrupt descriptor table */
static struct interrupt64_descriptor
idt64[NUM_INT] __attribute__ (( aligned ( 16 ) ));

/** The interrupt descriptor table register */
struct idtr64 idtr64 = {
	.limit = ( sizeof ( idt64 ) - 1 ),
};

/** Timer interrupt profiler */
static struct profiler timer_irq_profiler __profiler = { .name = "irq.timer" };

/** Other interrupt profiler */
static struct profiler other_irq_profiler __profiler = { .name = "irq.other" };

/**
 * Allocate space on the real-mode stack and copy data there from a
 * user buffer
 *
 * @v data		User buffer
 * @v size		Size of stack data
 * @ret sp		New value of real-mode stack pointer
 */
uint16_t copy_user_to_rm_stack ( userptr_t data, size_t size ) {
	userptr_t rm_stack;
	rm_sp -= size;
	rm_stack = real_to_user ( rm_ss, rm_sp );
	memcpy_user ( rm_stack, 0, data, 0, size );
	return rm_sp;
};

/**
 * Deallocate space on the real-mode stack, optionally copying back
 * data to a user buffer.
 *
 * @v data		User buffer
 * @v size		Size of stack data
 */
void remove_user_from_rm_stack ( userptr_t data, size_t size ) {
	if ( data ) {
		userptr_t rm_stack = real_to_user ( rm_ss, rm_sp );
		memcpy_user ( rm_stack, 0, data, 0, size );
	}
	rm_sp += size;
};

/**
 * Set interrupt vector
 *
 * @v intr		Interrupt number
 * @v vector		Interrupt vector, or NULL to disable
 */
void set_interrupt_vector ( unsigned int intr, void *vector ) {
	struct interrupt32_descriptor *idte32;
	struct interrupt64_descriptor *idte64;
	intptr_t addr = ( ( intptr_t ) vector );

	/* Populate 32-bit interrupt descriptor */
	idte32 = &idt32[intr];
	idte32->segment = VIRTUAL_CS;
	idte32->attr = ( vector ? ( IDTE_PRESENT | IDTE_TYPE_IRQ32 ) : 0 );
	idte32->low = ( addr >> 0 );
	idte32->high = ( addr >> 16 );

	/* Populate 64-bit interrupt descriptor, if applicable */
	if ( sizeof ( physaddr_t ) > sizeof ( uint32_t ) ) {
		idte64 = &idt64[intr];
		idte64->segment = LONG_CS;
		idte64->attr = ( vector ?
				 ( IDTE_PRESENT | IDTE_TYPE_IRQ64 ) : 0 );
		idte64->low = ( addr >> 0 );
		idte64->mid = ( addr >> 16 );
		idte64->high = ( ( ( uint64_t ) addr ) >> 32 );
	}
}

/**
 * Initialise interrupt descriptor table
 *
 */
void init_idt ( void ) {
	struct interrupt_vector *vec;
	unsigned int intr;

	/* Initialise the interrupt descriptor table and interrupt vectors */
	for ( intr = 0 ; intr < NUM_INT ; intr++ ) {
		vec = &intr_vec[intr];
		vec->push = PUSH_INSN;
		vec->movb = MOVB_INSN;
		vec->intr = intr;
		vec->jmp = JMP_INSN;
		vec->offset = ( ( intptr_t ) interrupt_wrapper -
				( intptr_t ) vec->next );
		set_interrupt_vector ( intr, vec );
	}
	DBGC ( &intr_vec[0], "INTn vector at %p+%zxn (phys %#lx+%zxn)\n",
	       intr_vec, sizeof ( intr_vec[0] ),
	       virt_to_phys ( intr_vec ), sizeof ( intr_vec[0] ) );

	/* Initialise the 32-bit interrupt descriptor table register */
	idtr32.base = virt_to_phys ( idt32 );

	/* Initialise the 64-bit interrupt descriptor table register,
	 * if applicable.
	 */
	if ( sizeof ( physaddr_t ) > sizeof ( uint32_t ) )
		idtr64.base = virt_to_phys ( idt64 );
}

/**
 * Determine interrupt profiler (for debugging)
 *
 * @v intr		Interrupt number
 * @ret profiler	Profiler
 */
static struct profiler * interrupt_profiler ( int intr ) {

	switch ( intr ) {
	case IRQ_INT ( 0 ) :
		return &timer_irq_profiler;
	default:
		return &other_irq_profiler;
	}
}

/**
 * Interrupt handler
 *
 * @v intr		Interrupt number
 */
void __attribute__ (( regparm ( 1 ) )) interrupt ( int intr ) {
	struct profiler *profiler = interrupt_profiler ( intr );
	uint32_t discard_eax;

	/* Reissue interrupt in real mode */
	profile_start ( profiler );
	__asm__ __volatile__ ( REAL_CODE ( "movb %%al, %%cs:(1f + 1)\n\t"
					   "\n1:\n\t"
					   "int $0x00\n\t" )
			       : "=a" ( discard_eax ) : "0" ( intr ) );
	profile_stop ( profiler );
	profile_exclude ( profiler );
}

/**
 * Map pages for I/O
 *
 * @v bus_addr		Bus address
 * @v len		Length of region
 * @ret io_addr		I/O address
 */
static void * ioremap_pages ( unsigned long bus_addr, size_t len ) {
	unsigned long start;
	unsigned int count;
	unsigned int stride;
	unsigned int first;
	unsigned int i;
	size_t offset;
	void *io_addr;

	DBGC ( &io_pages, "IO mapping %08lx+%zx\n", bus_addr, len );

	/* Sanity check */
	assert ( len != 0 );

	/* Round down start address to a page boundary */
	start = ( bus_addr & ~( IO_PAGE_SIZE - 1 ) );
	offset = ( bus_addr - start );
	assert ( offset < IO_PAGE_SIZE );

	/* Calculate number of pages required */
	count = ( ( offset + len + IO_PAGE_SIZE - 1 ) / IO_PAGE_SIZE );
	assert ( count != 0 );
	assert ( count < ( sizeof ( io_pages.page ) /
			   sizeof ( io_pages.page[0] ) ) );

	/* Round up number of pages to a power of two */
	stride = ( 1 << ( fls ( count ) - 1 ) );
	assert ( count <= stride );

	/* Allocate pages */
	for ( first = 0 ; first < ( sizeof ( io_pages.page ) /
				    sizeof ( io_pages.page[0] ) ) ;
	      first += stride ) {

		/* Calculate I/O address */
		io_addr = ( IO_BASE + ( first * IO_PAGE_SIZE ) + offset );

		/* Check that page table entries are available */
		for ( i = first ; i < ( first + count ) ; i++ ) {
			if ( io_pages.page[i] & PAGE_P ) {
				io_addr = NULL;
				break;
			}
		}
		if ( ! io_addr )
			continue;

		/* Create page table entries */
		for ( i = first ; i < ( first + count ) ; i++ ) {
			io_pages.page[i] = ( start | PAGE_P | PAGE_RW |
					     PAGE_US | PAGE_PWT | PAGE_PCD |
					     PAGE_PS );
			start += IO_PAGE_SIZE;
		}

		/* Mark last page as being the last in this allocation */
		io_pages.page[ i - 1 ] |= PAGE_LAST;

		/* Return I/O address */
		DBGC ( &io_pages, "IO mapped %08lx+%zx to %p using PTEs "
		       "[%d-%d]\n", bus_addr, len, io_addr, first,
		       ( first + count - 1 ) );
		return io_addr;
	}

	DBGC ( &io_pages, "IO could not map %08lx+%zx\n", bus_addr, len );
	return NULL;
}

/**
 * Unmap pages for I/O
 *
 * @v io_addr		I/O address
 */
static void iounmap_pages ( volatile const void *io_addr ) {
	volatile const void *invalidate = io_addr;
	unsigned int first;
	unsigned int i;
	int is_last;

	DBGC ( &io_pages, "IO unmapping %p\n", io_addr );

	/* Calculate first page table entry */
	first = ( ( io_addr - IO_BASE ) / IO_PAGE_SIZE );

	/* Clear page table entries */
	for ( i = first ; ; i++ ) {

		/* Sanity check */
		assert ( io_pages.page[i] & PAGE_P );

		/* Check if this is the last page in this allocation */
		is_last = ( io_pages.page[i] & PAGE_LAST );

		/* Clear page table entry */
		io_pages.page[i] = 0;

		/* Invalidate TLB for this page */
		__asm__ __volatile__ ( "invlpg (%0)" : : "r" ( invalidate ) );
		invalidate += IO_PAGE_SIZE;

		/* Terminate if this was the last page */
		if ( is_last )
			break;
	}

	DBGC ( &io_pages, "IO unmapped %p using PTEs [%d-%d]\n",
	       io_addr, first, i );
}

PROVIDE_UACCESS_INLINE ( librm, phys_to_user );
PROVIDE_UACCESS_INLINE ( librm, user_to_phys );
PROVIDE_UACCESS_INLINE ( librm, virt_to_user );
PROVIDE_UACCESS_INLINE ( librm, user_to_virt );
PROVIDE_UACCESS_INLINE ( librm, userptr_add );
PROVIDE_UACCESS_INLINE ( librm, memcpy_user );
PROVIDE_UACCESS_INLINE ( librm, memmove_user );
PROVIDE_UACCESS_INLINE ( librm, memset_user );
PROVIDE_UACCESS_INLINE ( librm, strlen_user );
PROVIDE_UACCESS_INLINE ( librm, memchr_user );
PROVIDE_IOMAP_INLINE ( pages, io_to_bus );
PROVIDE_IOMAP ( pages, ioremap, ioremap_pages );
PROVIDE_IOMAP ( pages, iounmap, iounmap_pages );
