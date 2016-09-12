/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#define FILE_LICENCE(...) extern void __file_licence ( void )
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>
#include <ipxe/efi/Uefi.h>
#include <ipxe/efi/IndustryStandard/PeImage.h>
#include <libgen.h>

#define eprintf(...) fprintf ( stderr, __VA_ARGS__ )

#ifdef EFI_TARGET32

#define EFI_IMAGE_NT_HEADERS		EFI_IMAGE_NT_HEADERS32
#define EFI_IMAGE_NT_OPTIONAL_HDR_MAGIC	EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
#define EFI_IMAGE_FILE_MACHINE		EFI_IMAGE_FILE_32BIT_MACHINE
#define ELFCLASS   ELFCLASS32
#define Elf_Ehdr   Elf32_Ehdr
#define Elf_Shdr   Elf32_Shdr
#define Elf_Sym    Elf32_Sym
#define Elf_Addr   Elf32_Addr
#define Elf_Rel    Elf32_Rel
#define Elf_Rela   Elf32_Rela
#define ELF_R_TYPE ELF32_R_TYPE
#define ELF_R_SYM  ELF32_R_SYM

#elif defined(EFI_TARGET64)

#define EFI_IMAGE_NT_HEADERS		EFI_IMAGE_NT_HEADERS64
#define EFI_IMAGE_NT_OPTIONAL_HDR_MAGIC	EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
#define EFI_IMAGE_FILE_MACHINE		0
#define ELFCLASS   ELFCLASS64
#define Elf_Ehdr   Elf64_Ehdr
#define Elf_Shdr   Elf64_Shdr
#define Elf_Sym    Elf64_Sym
#define Elf_Addr   Elf64_Addr
#define Elf_Rel    Elf64_Rel
#define Elf_Rela   Elf64_Rela
#define ELF_R_TYPE ELF64_R_TYPE
#define ELF_R_SYM  ELF64_R_SYM

#endif

#define ELF_MREL( mach, type ) ( (mach) | ( (type) << 16 ) )

/* Allow for building with older versions of elf.h */
#ifndef EM_AARCH64
#define EM_AARCH64 183
#define R_AARCH64_NONE 0
#define R_AARCH64_ABS64 257
#define R_AARCH64_CALL26 283
#define R_AARCH64_JUMP26 282
#define R_AARCH64_ADR_PREL_LO21 274
#define R_AARCH64_ADR_PREL_PG_HI21 275
#define R_AARCH64_ADD_ABS_LO12_NC 277
#define R_AARCH64_LDST8_ABS_LO12_NC 278
#define R_AARCH64_LDST16_ABS_LO12_NC 284
#define R_AARCH64_LDST32_ABS_LO12_NC 285
#define R_AARCH64_LDST64_ABS_LO12_NC 286
#endif /* EM_AARCH64 */
#ifndef R_ARM_CALL
#define R_ARM_CALL 28
#endif
#ifndef R_ARM_THM_JUMP24
#define R_ARM_THM_JUMP24 30
#endif

/* Seems to be missing from elf.h */
#ifndef R_AARCH64_NULL
#define R_AARCH64_NULL 256
#endif

#define EFI_FILE_ALIGN 0x20

struct elf_file {
	void *data;
	size_t len;
	const Elf_Ehdr *ehdr;
};

struct pe_section {
	struct pe_section *next;
	EFI_IMAGE_SECTION_HEADER hdr;
	void ( * fixup ) ( struct pe_section *section );
	uint8_t contents[0];
};

struct pe_relocs {
	struct pe_relocs *next;
	unsigned long start_rva;
	unsigned int used_relocs;
	unsigned int total_relocs;
	uint16_t *relocs;
};

struct pe_header {
	EFI_IMAGE_DOS_HEADER dos;
	uint8_t padding[128];
	EFI_IMAGE_NT_HEADERS nt;
};

static struct pe_header efi_pe_header = {
	.dos = {
		.e_magic = EFI_IMAGE_DOS_SIGNATURE,
		.e_lfanew = offsetof ( typeof ( efi_pe_header ), nt ),
	},
	.nt = {
		.Signature = EFI_IMAGE_NT_SIGNATURE,
		.FileHeader = {
			.TimeDateStamp = 0x10d1a884,
			.SizeOfOptionalHeader =
				sizeof ( efi_pe_header.nt.OptionalHeader ),
			.Characteristics = ( EFI_IMAGE_FILE_DLL |
					     EFI_IMAGE_FILE_MACHINE |
					     EFI_IMAGE_FILE_EXECUTABLE_IMAGE ),
		},
		.OptionalHeader = {
			.Magic = EFI_IMAGE_NT_OPTIONAL_HDR_MAGIC,
			.MajorLinkerVersion = 42,
			.MinorLinkerVersion = 42,
			.SectionAlignment = EFI_FILE_ALIGN,
			.FileAlignment = EFI_FILE_ALIGN,
			.SizeOfImage = sizeof ( efi_pe_header ),
			.SizeOfHeaders = sizeof ( efi_pe_header ),
			.NumberOfRvaAndSizes =
				EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES,
		},
	},
};

/** Command-line options */
struct options {
	unsigned int subsystem;
};

/**
 * Allocate memory
 *
 * @v len		Length of memory to allocate
 * @ret ptr		Pointer to allocated memory
 */
static void * xmalloc ( size_t len ) {
	void *ptr;

	ptr = malloc ( len );
	if ( ! ptr ) {
		eprintf ( "Could not allocate %zd bytes\n", len );
		exit ( 1 );
	}

	return ptr;
}

/**
 * Align section within PE file
 *
 * @v offset		Unaligned offset
 * @ret aligned_offset	Aligned offset
 */
static unsigned long efi_file_align ( unsigned long offset ) {
	return ( ( offset + EFI_FILE_ALIGN - 1 ) & ~( EFI_FILE_ALIGN - 1 ) );
}

/**
 * Generate entry in PE relocation table
 *
 * @v pe_reltab		PE relocation table
 * @v rva		RVA
 * @v size		Size of relocation entry
 */
static void generate_pe_reloc ( struct pe_relocs **pe_reltab,
				unsigned long rva, size_t size ) {
	unsigned long start_rva;
	uint16_t reloc;
	struct pe_relocs *pe_rel;
	uint16_t *relocs;

	/* Construct */
	start_rva = ( rva & ~0xfff );
	reloc = ( rva & 0xfff );
	switch ( size ) {
	case 8:
		reloc |= 0xa000;
		break;
	case 4:
		reloc |= 0x3000;
		break;
	case 2:
		reloc |= 0x2000;
		break;
	default:
		eprintf ( "Unsupported relocation size %zd\n", size );
		exit ( 1 );
	}

	/* Locate or create PE relocation table */
	for ( pe_rel = *pe_reltab ; pe_rel ; pe_rel = pe_rel->next ) {
		if ( pe_rel->start_rva == start_rva )
			break;
	}
	if ( ! pe_rel ) {
		pe_rel = xmalloc ( sizeof ( *pe_rel ) );
		memset ( pe_rel, 0, sizeof ( *pe_rel ) );
		pe_rel->next = *pe_reltab;
		*pe_reltab = pe_rel;
		pe_rel->start_rva = start_rva;
	}

	/* Expand relocation list if necessary */
	if ( pe_rel->used_relocs < pe_rel->total_relocs ) {
		relocs = pe_rel->relocs;
	} else {
		pe_rel->total_relocs = ( pe_rel->total_relocs ?
					 ( pe_rel->total_relocs * 2 ) : 256 );
		relocs = xmalloc ( pe_rel->total_relocs *
				   sizeof ( pe_rel->relocs[0] ) );
		memset ( relocs, 0,
			 pe_rel->total_relocs * sizeof ( pe_rel->relocs[0] ) );
		memcpy ( relocs, pe_rel->relocs,
			 pe_rel->used_relocs * sizeof ( pe_rel->relocs[0] ) );
		free ( pe_rel->relocs );
		pe_rel->relocs = relocs;
	}

	/* Store relocation */
	pe_rel->relocs[ pe_rel->used_relocs++ ] = reloc;
}

/**
 * Calculate size of binary PE relocation table
 *
 * @v pe_reltab		PE relocation table
 * @v buffer		Buffer to contain binary table, or NULL
 * @ret size		Size of binary table
 */
static size_t output_pe_reltab ( struct pe_relocs *pe_reltab,
				 void *buffer ) {
	struct pe_relocs *pe_rel;
	unsigned int num_relocs;
	size_t size;
	size_t total_size = 0;

	for ( pe_rel = pe_reltab ; pe_rel ; pe_rel = pe_rel->next ) {
		num_relocs = ( ( pe_rel->used_relocs + 1 ) & ~1 );
		size = ( sizeof ( uint32_t ) /* VirtualAddress */ +
			 sizeof ( uint32_t ) /* SizeOfBlock */ +
			 ( num_relocs * sizeof ( uint16_t ) ) );
		if ( buffer ) {
			*( (uint32_t *) ( buffer + total_size + 0 ) )
				= pe_rel->start_rva;
			*( (uint32_t *) ( buffer + total_size + 4 ) ) = size;
			memcpy ( ( buffer + total_size + 8 ), pe_rel->relocs,
				 ( num_relocs * sizeof ( uint16_t ) ) );
		}
		total_size += size;
	}

	return total_size;
}

/**
 * Read input ELF file
 *
 * @v name		File name
 * @v elf		ELF file
 */
static void read_elf_file ( const char *name, struct elf_file *elf ) {
	static const unsigned char ident[] = {
		ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS, ELFDATA2LSB
	};
	struct stat stat;
	const Elf_Ehdr *ehdr;
	const Elf_Shdr *shdr;
	void *data;
	size_t offset;
	unsigned int i;
	int fd;

	/* Open file */
	fd = open ( name, O_RDONLY );
	if ( fd < 0 ) {
		eprintf ( "Could not open %s: %s\n", name, strerror ( errno ) );
		exit ( 1 );
	}

	/* Get file size */
	if ( fstat ( fd, &stat ) < 0 ) {
		eprintf ( "Could not get size of %s: %s\n",
			  name, strerror ( errno ) );
		exit ( 1 );
	}
	elf->len = stat.st_size;

	/* Map file */
	data = mmap ( NULL, elf->len, PROT_READ, MAP_SHARED, fd, 0 );
	if ( data == MAP_FAILED ) {
		eprintf ( "Could not map %s: %s\n", name, strerror ( errno ) );
		exit ( 1 );
	}
	elf->data = data;

	/* Close file */
	close ( fd );

	/* Check header */
	ehdr = elf->data;
	if ( ( elf->len < sizeof ( *ehdr ) ) ||
	     ( memcmp ( ident, ehdr->e_ident, sizeof ( ident ) ) != 0 ) ) {
		eprintf ( "Invalid ELF header in %s\n", name );
		exit ( 1 );
	}
	elf->ehdr = ehdr;

	/* Check section headers */
	for ( i = 0 ; i < ehdr->e_shnum ; i++ ) {
		offset = ( ehdr->e_shoff + ( i * ehdr->e_shentsize ) );
		if ( elf->len < ( offset + sizeof ( *shdr ) ) ) {
			eprintf ( "ELF section header outside file in %s\n",
				  name );
			exit ( 1 );
		}
		shdr = ( data + offset );
		if ( ( shdr->sh_type != SHT_NOBITS ) &&
		     ( ( elf->len < shdr->sh_offset ) ||
		       ( ( ( elf->len - shdr->sh_offset ) < shdr->sh_size ) ))){
			eprintf ( "ELF section %d outside file in %s\n",
				  i, name );
			exit ( 1 );
		}
		if ( shdr->sh_link >= ehdr->e_shnum ) {
			eprintf ( "ELF section %d link section %d out of "
				  "range\n", i, shdr->sh_link );
			exit ( 1 );
		}
	}
}

/**
 * Get ELF string
 *
 * @v elf		ELF file
 * @v section		String table section number
 * @v offset		String table offset
 * @ret string		ELF string
 */
static const char * elf_string ( struct elf_file *elf, unsigned int section,
				 size_t offset ) {
	const Elf_Ehdr *ehdr = elf->ehdr;
	const Elf_Shdr *shdr;
	char *string;
	char *last;

	/* Locate section header */
	if ( section >= ehdr->e_shnum ) {
		eprintf ( "Invalid ELF string section %d\n", section );
		exit ( 1 );
	}
	shdr = ( elf->data + ehdr->e_shoff + ( section * ehdr->e_shentsize ) );

	/* Sanity check section */
	if ( shdr->sh_type != SHT_STRTAB ) {
		eprintf ( "ELF section %d (type %d) is not a string table\n",
			  section, shdr->sh_type );
		exit ( 1 );
	}
	last = ( elf->data + shdr->sh_offset + shdr->sh_size - 1 );
	if ( *last != '\0' ) {
		eprintf ( "ELF section %d is not NUL-terminated\n", section );
		exit ( 1 );
	}

	/* Locate string */
	if ( offset >= shdr->sh_size ) {
		eprintf ( "Invalid ELF string offset %zd in section %d\n",
			  offset, section );
		exit ( 1 );
	}
	string = ( elf->data + shdr->sh_offset + offset );

	return string;
}

/**
 * Set machine architecture
 *
 * @v elf		ELF file
 * @v pe_header		PE file header
 */
static void set_machine ( struct elf_file *elf, struct pe_header *pe_header ) {
	const Elf_Ehdr *ehdr = elf->ehdr;
	uint16_t machine;

	/* Identify machine architecture */
	switch ( ehdr->e_machine ) {
	case EM_386:
		machine = EFI_IMAGE_MACHINE_IA32;
		break;
	case EM_X86_64:
		machine = EFI_IMAGE_MACHINE_X64;
		break;
	case EM_ARM:
		machine = EFI_IMAGE_MACHINE_ARMTHUMB_MIXED;
		break;
	case EM_AARCH64:
		machine = EFI_IMAGE_MACHINE_AARCH64;
		break;
	default:
		eprintf ( "Unknown ELF architecture %d\n", ehdr->e_machine );
		exit ( 1 );
	}

	/* Set machine architecture */
	pe_header->nt.FileHeader.Machine = machine;
}

/**
 * Process section
 *
 * @v elf		ELF file
 * @v shdr		ELF section header
 * @v pe_header		PE file header
 * @ret new		New PE section
 */
static struct pe_section * process_section ( struct elf_file *elf,
					     const Elf_Shdr *shdr,
					     struct pe_header *pe_header ) {
	struct pe_section *new;
	const char *name;
	size_t section_memsz;
	size_t section_filesz;
	unsigned long code_start;
	unsigned long code_end;
	unsigned long data_start;
	unsigned long data_mid;
	unsigned long data_end;
	unsigned long start;
	unsigned long end;
	unsigned long *applicable_start;
	unsigned long *applicable_end;

	/* Get section name */
	name = elf_string ( elf, elf->ehdr->e_shstrndx, shdr->sh_name );

	/* Extract current RVA limits from file header */
	code_start = pe_header->nt.OptionalHeader.BaseOfCode;
	code_end = ( code_start + pe_header->nt.OptionalHeader.SizeOfCode );
#if defined(EFI_TARGET32)
	data_start = pe_header->nt.OptionalHeader.BaseOfData;
#elif defined(EFI_TARGET64)
	data_start = code_end;
#endif
	data_mid = ( data_start +
		     pe_header->nt.OptionalHeader.SizeOfInitializedData );
	data_end = ( data_mid +
		     pe_header->nt.OptionalHeader.SizeOfUninitializedData );

	/* Allocate PE section */
	section_memsz = shdr->sh_size;
	section_filesz = ( ( shdr->sh_type == SHT_PROGBITS ) ?
			   efi_file_align ( section_memsz ) : 0 );
	new = xmalloc ( sizeof ( *new ) + section_filesz );
	memset ( new, 0, sizeof ( *new ) + section_filesz );

	/* Fill in section header details */
	strncpy ( ( char * ) new->hdr.Name, name, sizeof ( new->hdr.Name ) );
	new->hdr.Misc.VirtualSize = section_memsz;
	new->hdr.VirtualAddress = shdr->sh_addr;
	new->hdr.SizeOfRawData = section_filesz;

	/* Fill in section characteristics and update RVA limits */
	if ( ( shdr->sh_type == SHT_PROGBITS ) &&
	     ( shdr->sh_flags & SHF_EXECINSTR ) ) {
		/* .text-type section */
		new->hdr.Characteristics =
			( EFI_IMAGE_SCN_CNT_CODE |
			  EFI_IMAGE_SCN_MEM_NOT_PAGED |
			  EFI_IMAGE_SCN_MEM_EXECUTE |
			  EFI_IMAGE_SCN_MEM_READ );
		applicable_start = &code_start;
		applicable_end = &code_end;
	} else if ( ( shdr->sh_type == SHT_PROGBITS ) &&
		    ( shdr->sh_flags & SHF_WRITE ) ) {
		/* .data-type section */
		new->hdr.Characteristics =
			( EFI_IMAGE_SCN_CNT_INITIALIZED_DATA |
			  EFI_IMAGE_SCN_MEM_NOT_PAGED |
			  EFI_IMAGE_SCN_MEM_READ |
			  EFI_IMAGE_SCN_MEM_WRITE );
		applicable_start = &data_start;
		applicable_end = &data_mid;
	} else if ( shdr->sh_type == SHT_PROGBITS ) {
		/* .rodata-type section */
		new->hdr.Characteristics =
			( EFI_IMAGE_SCN_CNT_INITIALIZED_DATA |
			  EFI_IMAGE_SCN_MEM_NOT_PAGED |
			  EFI_IMAGE_SCN_MEM_READ );
		applicable_start = &data_start;
		applicable_end = &data_mid;
	} else if ( shdr->sh_type == SHT_NOBITS ) {
		/* .bss-type section */
		new->hdr.Characteristics =
			( EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA |
			  EFI_IMAGE_SCN_MEM_NOT_PAGED |
			  EFI_IMAGE_SCN_MEM_READ |
			  EFI_IMAGE_SCN_MEM_WRITE );
		applicable_start = &data_mid;
		applicable_end = &data_end;
	} else {
		eprintf ( "Unrecognised characteristics for section %s\n",
			  name );
		exit ( 1 );
	}

	/* Copy in section contents */
	if ( shdr->sh_type == SHT_PROGBITS ) {
		memcpy ( new->contents, ( elf->data + shdr->sh_offset ),
			 shdr->sh_size );
	}

	/* Update RVA limits */
	start = new->hdr.VirtualAddress;
	end = ( start + new->hdr.Misc.VirtualSize );
	if ( ( ! *applicable_start ) || ( *applicable_start >= start ) )
		*applicable_start = start;
	if ( *applicable_end < end )
		*applicable_end = end;
	if ( data_start < code_end )
		data_start = code_end;
	if ( data_mid < data_start )
		data_mid = data_start;
	if ( data_end < data_mid )
		data_end = data_mid;

	/* Write RVA limits back to file header */
	pe_header->nt.OptionalHeader.BaseOfCode = code_start;
	pe_header->nt.OptionalHeader.SizeOfCode = ( code_end - code_start );
#if defined(EFI_TARGET32)
	pe_header->nt.OptionalHeader.BaseOfData = data_start;
#endif
	pe_header->nt.OptionalHeader.SizeOfInitializedData =
		( data_mid - data_start );
	pe_header->nt.OptionalHeader.SizeOfUninitializedData =
		( data_end - data_mid );

	/* Update remaining file header fields */
	pe_header->nt.FileHeader.NumberOfSections++;
	pe_header->nt.OptionalHeader.SizeOfHeaders += sizeof ( new->hdr );
	pe_header->nt.OptionalHeader.SizeOfImage =
		efi_file_align ( data_end );

	return new;
}

/**
 * Process relocation record
 *
 * @v elf		ELF file
 * @v shdr		ELF section header
 * @v syms		Symbol table
 * @v nsyms		Number of symbol table entries
 * @v rel		Relocation record
 * @v pe_reltab		PE relocation table to fill in
 */
static void process_reloc ( struct elf_file *elf, const Elf_Shdr *shdr,
			    const Elf_Sym *syms, unsigned int nsyms,
			    const Elf_Rel *rel, struct pe_relocs **pe_reltab ) {
	unsigned int type = ELF_R_TYPE ( rel->r_info );
	unsigned int sym = ELF_R_SYM ( rel->r_info );
	unsigned int mrel = ELF_MREL ( elf->ehdr->e_machine, type );
	size_t offset = ( shdr->sh_addr + rel->r_offset );

	/* Look up symbol and process relocation */
	if ( sym >= nsyms ) {
		eprintf ( "Symbol out of range\n" );
		exit ( 1 );
	}
	if ( syms[sym].st_shndx == SHN_ABS ) {
		/* Skip absolute symbols; the symbol value won't
		 * change when the object is loaded.
		 */
	} else {
		switch ( mrel ) {
		case ELF_MREL ( EM_386, R_386_NONE ) :
		case ELF_MREL ( EM_ARM, R_ARM_NONE ) :
		case ELF_MREL ( EM_X86_64, R_X86_64_NONE ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_NONE ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_NULL ) :
			/* Ignore dummy relocations used by REQUIRE_SYMBOL() */
			break;
		case ELF_MREL ( EM_386, R_386_32 ) :
		case ELF_MREL ( EM_ARM, R_ARM_ABS32 ) :
			/* Generate a 4-byte PE relocation */
			generate_pe_reloc ( pe_reltab, offset, 4 );
			break;
		case ELF_MREL ( EM_X86_64, R_X86_64_64 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_ABS64 ) :
			/* Generate an 8-byte PE relocation */
			generate_pe_reloc ( pe_reltab, offset, 8 );
			break;
		case ELF_MREL ( EM_386, R_386_PC32 ) :
		case ELF_MREL ( EM_ARM, R_ARM_CALL ) :
		case ELF_MREL ( EM_ARM, R_ARM_THM_PC22 ) :
		case ELF_MREL ( EM_ARM, R_ARM_THM_JUMP24 ) :
		case ELF_MREL ( EM_X86_64, R_X86_64_PC32 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_CALL26 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_JUMP26 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_ADR_PREL_LO21 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_ADR_PREL_PG_HI21 ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_ADD_ABS_LO12_NC ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_LDST8_ABS_LO12_NC ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_LDST16_ABS_LO12_NC ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_LDST32_ABS_LO12_NC ) :
		case ELF_MREL ( EM_AARCH64, R_AARCH64_LDST64_ABS_LO12_NC ) :
			/* Skip PC-relative relocations; all relative
			 * offsets remain unaltered when the object is
			 * loaded.
			 */
			break;
		default:
			eprintf ( "Unrecognised relocation type %d\n", type );
			exit ( 1 );
		}
	}
}

/**
 * Process relocation records
 *
 * @v elf		ELF file
 * @v shdr		ELF section header
 * @v stride		Relocation record size
 * @v pe_reltab		PE relocation table to fill in
 */
static void process_relocs ( struct elf_file *elf, const Elf_Shdr *shdr,
			     size_t stride, struct pe_relocs **pe_reltab ) {
	const Elf_Shdr *symtab;
	const Elf_Sym *syms;
	const Elf_Rel *rel;
	unsigned int nsyms;
	unsigned int nrels;
	unsigned int i;

	/* Identify symbol table */
	symtab = ( elf->data + elf->ehdr->e_shoff +
		   ( shdr->sh_link * elf->ehdr->e_shentsize ) );
	syms = ( elf->data + symtab->sh_offset );
	nsyms = ( symtab->sh_size / sizeof ( syms[0] ) );

	/* Process each relocation */
	rel = ( elf->data + shdr->sh_offset );
	nrels = ( shdr->sh_size / stride );
	for ( i = 0 ; i < nrels ; i++ ) {
		process_reloc ( elf, shdr, syms, nsyms, rel, pe_reltab );
		rel = ( ( ( const void * ) rel ) + stride );
	}
}

/**
 * Create relocations section
 *
 * @v pe_header		PE file header
 * @v pe_reltab		PE relocation table
 * @ret section		Relocation section
 */
static struct pe_section *
create_reloc_section ( struct pe_header *pe_header,
		       struct pe_relocs *pe_reltab ) {
	struct pe_section *reloc;
	size_t section_memsz;
	size_t section_filesz;
	EFI_IMAGE_DATA_DIRECTORY *relocdir;

	/* Allocate PE section */
	section_memsz = output_pe_reltab ( pe_reltab, NULL );
	section_filesz = efi_file_align ( section_memsz );
	reloc = xmalloc ( sizeof ( *reloc ) + section_filesz );
	memset ( reloc, 0, sizeof ( *reloc ) + section_filesz );

	/* Fill in section header details */
	strncpy ( ( char * ) reloc->hdr.Name, ".reloc",
		  sizeof ( reloc->hdr.Name ) );
	reloc->hdr.Misc.VirtualSize = section_memsz;
	reloc->hdr.VirtualAddress = pe_header->nt.OptionalHeader.SizeOfImage;
	reloc->hdr.SizeOfRawData = section_filesz;
	reloc->hdr.Characteristics = ( EFI_IMAGE_SCN_CNT_INITIALIZED_DATA |
				       EFI_IMAGE_SCN_MEM_NOT_PAGED |
				       EFI_IMAGE_SCN_MEM_READ );

	/* Copy in section contents */
	output_pe_reltab ( pe_reltab, reloc->contents );

	/* Update file header details */
	pe_header->nt.FileHeader.NumberOfSections++;
	pe_header->nt.OptionalHeader.SizeOfHeaders += sizeof ( reloc->hdr );
	pe_header->nt.OptionalHeader.SizeOfImage += section_filesz;
	relocdir = &(pe_header->nt.OptionalHeader.DataDirectory
		     [EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC]);
	relocdir->VirtualAddress = reloc->hdr.VirtualAddress;
	relocdir->Size = reloc->hdr.Misc.VirtualSize;

	return reloc;
}

/**
 * Fix up debug section
 *
 * @v debug		Debug section
 */
static void fixup_debug_section ( struct pe_section *debug ) {
	EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *contents;

	/* Fix up FileOffset */
	contents = ( ( void * ) debug->contents );
	contents->FileOffset += ( debug->hdr.PointerToRawData -
				  debug->hdr.VirtualAddress );
}

/**
 * Create debug section
 *
 * @v pe_header		PE file header
 * @ret section		Debug section
 */
static struct pe_section *
create_debug_section ( struct pe_header *pe_header, const char *filename ) {
	struct pe_section *debug;
	size_t section_memsz;
	size_t section_filesz;
	EFI_IMAGE_DATA_DIRECTORY *debugdir;
	struct {
		EFI_IMAGE_DEBUG_DIRECTORY_ENTRY debug;
		EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY rsds;
		char name[ strlen ( filename ) + 1 ];
	} *contents;

	/* Allocate PE section */
	section_memsz = sizeof ( *contents );
	section_filesz = efi_file_align ( section_memsz );
	debug = xmalloc ( sizeof ( *debug ) + section_filesz );
	memset ( debug, 0, sizeof ( *debug ) + section_filesz );
	contents = ( void * ) debug->contents;

	/* Fill in section header details */
	strncpy ( ( char * ) debug->hdr.Name, ".debug",
		  sizeof ( debug->hdr.Name ) );
	debug->hdr.Misc.VirtualSize = section_memsz;
	debug->hdr.VirtualAddress = pe_header->nt.OptionalHeader.SizeOfImage;
	debug->hdr.SizeOfRawData = section_filesz;
	debug->hdr.Characteristics = ( EFI_IMAGE_SCN_CNT_INITIALIZED_DATA |
				       EFI_IMAGE_SCN_MEM_NOT_PAGED |
				       EFI_IMAGE_SCN_MEM_READ );
	debug->fixup = fixup_debug_section;

	/* Create section contents */
	contents->debug.TimeDateStamp = 0x10d1a884;
	contents->debug.Type = EFI_IMAGE_DEBUG_TYPE_CODEVIEW;
	contents->debug.SizeOfData =
		( sizeof ( *contents ) - sizeof ( contents->debug ) );
	contents->debug.RVA = ( debug->hdr.VirtualAddress +
				offsetof ( typeof ( *contents ), rsds ) );
	contents->debug.FileOffset = contents->debug.RVA;
	contents->rsds.Signature = CODEVIEW_SIGNATURE_RSDS;
	snprintf ( contents->name, sizeof ( contents->name ), "%s",
		   filename );

	/* Update file header details */
	pe_header->nt.FileHeader.NumberOfSections++;
	pe_header->nt.OptionalHeader.SizeOfHeaders += sizeof ( debug->hdr );
	pe_header->nt.OptionalHeader.SizeOfImage += section_filesz;
	debugdir = &(pe_header->nt.OptionalHeader.DataDirectory
		     [EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
	debugdir->VirtualAddress = debug->hdr.VirtualAddress;
	debugdir->Size = sizeof ( contents->debug );

	return debug;
}

/**
 * Write out PE file
 *
 * @v pe_header		PE file header
 * @v pe_sections	List of PE sections
 * @v pe		Output file
 */
static void write_pe_file ( struct pe_header *pe_header,
			    struct pe_section *pe_sections,
			    FILE *pe ) {
	struct pe_section *section;
	unsigned long fpos = 0;

	/* Align length of headers */
	fpos = pe_header->nt.OptionalHeader.SizeOfHeaders =
		efi_file_align ( pe_header->nt.OptionalHeader.SizeOfHeaders );

	/* Assign raw data pointers */
	for ( section = pe_sections ; section ; section = section->next ) {
		if ( section->hdr.SizeOfRawData ) {
			section->hdr.PointerToRawData = fpos;
			fpos += section->hdr.SizeOfRawData;
			fpos = efi_file_align ( fpos );
		}
		if ( section->fixup )
			section->fixup ( section );
	}

	/* Write file header */
	if ( fwrite ( pe_header, sizeof ( *pe_header ), 1, pe ) != 1 ) {
		perror ( "Could not write PE header" );
		exit ( 1 );
	}

	/* Write section headers */
	for ( section = pe_sections ; section ; section = section->next ) {
		if ( fwrite ( &section->hdr, sizeof ( section->hdr ),
			      1, pe ) != 1 ) {
			perror ( "Could not write section header" );
			exit ( 1 );
		}
	}

	/* Write sections */
	for ( section = pe_sections ; section ; section = section->next ) {
		if ( fseek ( pe, section->hdr.PointerToRawData,
			     SEEK_SET ) != 0 ) {
			eprintf ( "Could not seek to %x: %s\n",
				  section->hdr.PointerToRawData,
				  strerror ( errno ) );
			exit ( 1 );
		}
		if ( section->hdr.SizeOfRawData &&
		     ( fwrite ( section->contents, section->hdr.SizeOfRawData,
				1, pe ) != 1 ) ) {
			eprintf ( "Could not write section %.8s: %s\n",
				  section->hdr.Name, strerror ( errno ) );
			exit ( 1 );
		}
	}
}

/**
 * Convert ELF to PE
 *
 * @v elf_name		ELF file name
 * @v pe_name		PE file name
 */
static void elf2pe ( const char *elf_name, const char *pe_name,
		     struct options *opts ) {
	char pe_name_tmp[ strlen ( pe_name ) + 1 ];
	struct pe_relocs *pe_reltab = NULL;
	struct pe_section *pe_sections = NULL;
	struct pe_section **next_pe_section = &pe_sections;
	struct pe_header pe_header;
	struct elf_file elf;
	const Elf_Shdr *shdr;
	size_t offset;
	unsigned int i;
	FILE *pe;

	/* Create a modifiable copy of the PE name */
	memcpy ( pe_name_tmp, pe_name, sizeof ( pe_name_tmp ) );

	/* Read ELF file */
	read_elf_file ( elf_name, &elf );

	/* Initialise the PE header */
	memcpy ( &pe_header, &efi_pe_header, sizeof ( pe_header ) );
	set_machine ( &elf, &pe_header );
	pe_header.nt.OptionalHeader.AddressOfEntryPoint = elf.ehdr->e_entry;
	pe_header.nt.OptionalHeader.Subsystem = opts->subsystem;

	/* Process input sections */
	for ( i = 0 ; i < elf.ehdr->e_shnum ; i++ ) {
		offset = ( elf.ehdr->e_shoff + ( i * elf.ehdr->e_shentsize ) );
		shdr = ( elf.data + offset );

		/* Process section */
		if ( shdr->sh_flags & SHF_ALLOC ) {

			/* Create output section */
			*(next_pe_section) = process_section ( &elf, shdr,
							       &pe_header );
			next_pe_section = &(*next_pe_section)->next;

		} else if ( shdr->sh_type == SHT_REL ) {

			/* Process .rel relocations */
			process_relocs ( &elf, shdr, sizeof ( Elf_Rel ),
					 &pe_reltab );

		} else if ( shdr->sh_type == SHT_RELA ) {

			/* Process .rela relocations */
			process_relocs ( &elf, shdr, sizeof ( Elf_Rela ),
					 &pe_reltab );
		}
	}

	/* Create the .reloc section */
	*(next_pe_section) = create_reloc_section ( &pe_header, pe_reltab );
	next_pe_section = &(*next_pe_section)->next;

	/* Create the .debug section */
	*(next_pe_section) = create_debug_section ( &pe_header,
						    basename ( pe_name_tmp ) );
	next_pe_section = &(*next_pe_section)->next;

	/* Write out PE file */
	pe = fopen ( pe_name, "w" );
	if ( ! pe ) {
		eprintf ( "Could not open %s for writing: %s\n",
			  pe_name, strerror ( errno ) );
		exit ( 1 );
	}
	write_pe_file ( &pe_header, pe_sections, pe );
	fclose ( pe );

	/* Unmap ELF file */
	munmap ( elf.data, elf.len );
}

/**
 * Print help
 *
 * @v program_name	Program name
 */
static void print_help ( const char *program_name ) {
	eprintf ( "Syntax: %s [--subsystem=<number>] infile outfile\n",
		  program_name );
}

/**
 * Parse command-line options
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @v opts		Options structure to populate
 */
static int parse_options ( const int argc, char **argv,
			   struct options *opts ) {
	char *end;
	int c;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "subsystem", required_argument, NULL, 's' },
			{ "help", 0, NULL, 'h' },
			{ 0, 0, 0, 0 }
		};

		if ( ( c = getopt_long ( argc, argv, "s:h",
					 long_options,
					 &option_index ) ) == -1 ) {
			break;
		}

		switch ( c ) {
		case 's':
			opts->subsystem = strtoul ( optarg, &end, 0 );
			if ( *end ) {
				eprintf ( "Invalid subsytem \"%s\"\n",
					  optarg );
				exit ( 2 );
			}
			break;
		case 'h':
			print_help ( argv[0] );
			exit ( 0 );
		case '?':
		default:
			exit ( 2 );
		}
	}
	return optind;
}

int main ( int argc, char **argv ) {
	struct options opts = {
		.subsystem = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION,
	};
	int infile_index;
	const char *infile;
	const char *outfile;

	/* Parse command-line arguments */
	infile_index = parse_options ( argc, argv, &opts );
	if ( argc != ( infile_index + 2 ) ) {
		print_help ( argv[0] );
		exit ( 2 );
	}
	infile = argv[infile_index];
	outfile = argv[infile_index + 1];

	/* Convert file */
	elf2pe ( infile, outfile, &opts );

	return 0;
}
