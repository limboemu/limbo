/*
 * Copyright (C) 2015 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/**
 * @file
 *
 * EFI frame buffer console
 *
 */

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/Protocol/GraphicsOutput.h>
#include <ipxe/efi/Protocol/HiiFont.h>
#include <ipxe/ansicol.h>
#include <ipxe/fbcon.h>
#include <ipxe/console.h>
#include <ipxe/umalloc.h>
#include <ipxe/rotate.h>
#include <config/console.h>

/* Avoid dragging in EFI console if not otherwise used */
extern struct console_driver efi_console;
struct console_driver efi_console __attribute__ (( weak ));

/* Set default console usage if applicable
 *
 * We accept either CONSOLE_FRAMEBUFFER or CONSOLE_EFIFB.
 */
#if ( defined ( CONSOLE_FRAMEBUFFER ) && ! defined ( CONSOLE_EFIFB ) )
#define CONSOLE_EFIFB CONSOLE_FRAMEBUFFER
#endif
#if ! ( defined ( CONSOLE_EFIFB ) && CONSOLE_EXPLICIT ( CONSOLE_EFIFB ) )
#undef CONSOLE_EFIFB
#define CONSOLE_EFIFB ( CONSOLE_USAGE_ALL & ~CONSOLE_USAGE_LOG )
#endif

/* Forward declaration */
struct console_driver efifb_console __console_driver;

/** An EFI frame buffer */
struct efifb {
	/** EFI graphics output protocol */
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	/** EFI HII font protocol */
	EFI_HII_FONT_PROTOCOL *hiifont;
	/** Saved mode */
	UINT32 saved_mode;

	/** Frame buffer console */
	struct fbcon fbcon;
	/** Physical start address */
	physaddr_t start;
	/** Pixel geometry */
	struct fbcon_geometry pixel;
	/** Colour mapping */
	struct fbcon_colour_map map;
	/** Font definition */
	struct fbcon_font font;
	/** Character glyphs */
	userptr_t glyphs;
};

/** The EFI frame buffer */
static struct efifb efifb;

/**
 * Get character glyph
 *
 * @v character		Character
 * @v glyph		Character glyph to fill in
 */
static void efifb_glyph ( unsigned int character, uint8_t *glyph ) {
	size_t offset = ( character * efifb.font.height );

	copy_from_user ( glyph, efifb.glyphs, offset, efifb.font.height );
}

/**
 * Get character glyphs
 *
 * @ret rc		Return status code
 */
static int efifb_glyphs ( void ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_IMAGE_OUTPUT *blt;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pixel;
	size_t offset;
	size_t len;
	uint8_t bitmask;
	unsigned int character;
	unsigned int x;
	unsigned int y;
	EFI_STATUS efirc;
	int rc;

	/* Get font height.  The GetFontInfo() call nominally returns
	 * this information in an EFI_FONT_DISPLAY_INFO structure, but
	 * is known to fail on many UEFI implementations.  Instead, we
	 * iterate over all printable characters to find the maximum
	 * height.
	 */
	efifb.font.height = 0;
	for ( character = 0 ; character < 256 ; character++ ) {

		/* Skip non-printable characters */
		if ( ! isprint ( character ) )
			continue;

		/* Get glyph */
		blt = NULL;
		if ( ( efirc = efifb.hiifont->GetGlyph ( efifb.hiifont,
							 character, NULL, &blt,
							 NULL ) ) != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( &efifb, "EFIFB could not get glyph %d: %s\n",
			       character, strerror ( rc ) );
			continue;
		}
		assert ( blt != NULL );

		/* Calculate maximum height */
		if ( efifb.font.height < blt->Height )
			efifb.font.height = blt->Height;

		/* Free glyph */
		bs->FreePool ( blt );
	}
	if ( ! efifb.font.height ) {
		DBGC ( &efifb, "EFIFB could not get font height\n" );
		return -ENOENT;
	}

	/* Allocate glyph data */
	len = ( 256 * efifb.font.height * sizeof ( bitmask ) );
	efifb.glyphs = umalloc ( len );
	if ( ! efifb.glyphs ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	memset_user ( efifb.glyphs, 0, 0, len );

	/* Get font data */
	for ( character = 0 ; character < 256 ; character++ ) {

		/* Skip non-printable characters */
		if ( ! isprint ( character ) )
			continue;

		/* Get glyph */
		blt = NULL;
		if ( ( efirc = efifb.hiifont->GetGlyph ( efifb.hiifont,
							 character, NULL, &blt,
							 NULL ) ) != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( &efifb, "EFIFB could not get glyph %d: %s\n",
			       character, strerror ( rc ) );
			continue;
		}
		assert ( blt != NULL );

		/* Sanity check */
		if ( blt->Width > 8 ) {
			DBGC ( &efifb, "EFIFB glyph %d invalid width %d\n",
			       character, blt->Width );
			continue;
		}
		if ( blt->Height > efifb.font.height ) {
			DBGC ( &efifb, "EFIFB glyph %d invalid height %d\n",
			       character, blt->Height );
			continue;
		}

		/* Convert glyph to bitmap */
		pixel = blt->Image.Bitmap;
		offset = ( character * efifb.font.height );
		for ( y = 0 ; y < blt->Height ; y++ ) {
			bitmask = 0;
			for ( x = 0 ; x < blt->Width ; x++ ) {
				bitmask = rol8 ( bitmask, 1 );
				if ( pixel->Blue || pixel->Green || pixel->Red )
					bitmask |= 0x01;
				pixel++;
			}
			copy_to_user ( efifb.glyphs, offset++, &bitmask,
				       sizeof ( bitmask ) );
		}

		/* Free glyph */
		bs->FreePool ( blt );
	}

	efifb.font.glyph = efifb_glyph;
	return 0;

	ufree ( efifb.glyphs );
 err_alloc:
	return rc;
}

/**
 * Generate colour mapping for a single colour component
 *
 * @v mask		Mask value
 * @v scale		Scale value to fill in
 * @v lsb		LSB value to fill in
 * @ret rc		Return status code
 */
static int efifb_colour_map_mask ( uint32_t mask, uint8_t *scale,
				   uint8_t *lsb ) {
	uint32_t check;

	/* Fill in LSB and scale */
	*lsb = ( mask ? ( ffs ( mask ) - 1 ) : 0 );
	*scale = ( mask ? ( 8 - ( fls ( mask ) - *lsb ) ) : 8 );

	/* Check that original mask was contiguous */
	check = ( ( 0xff >> *scale ) << *lsb );
	if ( check != mask )
		return -ENOTSUP;

	return 0;
}

/**
 * Generate colour mapping
 *
 * @v info		EFI mode information
 * @v map		Colour mapping to fill in
 * @ret bpp		Number of bits per pixel, or negative error
 */
static int efifb_colour_map ( EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info,
			      struct fbcon_colour_map *map ) {
	static EFI_PIXEL_BITMASK rgb_mask = {
		0x000000ffUL, 0x0000ff00UL, 0x00ff0000UL, 0xff000000UL
	};
	static EFI_PIXEL_BITMASK bgr_mask = {
		0x00ff0000UL, 0x0000ff00UL, 0x000000ffUL, 0xff000000UL
	};
	EFI_PIXEL_BITMASK *mask;
	uint8_t reserved_scale;
	uint8_t reserved_lsb;
	int rc;

	/* Determine applicable mask */
	switch ( info->PixelFormat ) {
	case PixelRedGreenBlueReserved8BitPerColor:
		mask = &rgb_mask;
		break;
	case PixelBlueGreenRedReserved8BitPerColor:
		mask = &bgr_mask;
		break;
	case PixelBitMask:
		mask = &info->PixelInformation;
		break;
	default:
		DBGC ( &efifb, "EFIFB unrecognised pixel format %d\n",
		       info->PixelFormat );
		return -ENOTSUP;
	}

	/* Map each colour component */
	if ( ( rc = efifb_colour_map_mask ( mask->RedMask, &map->red_scale,
					    &map->red_lsb ) ) != 0 )
		return rc;
	if ( ( rc = efifb_colour_map_mask ( mask->GreenMask, &map->green_scale,
					    &map->green_lsb ) ) != 0 )
		return rc;
	if ( ( rc = efifb_colour_map_mask ( mask->BlueMask, &map->blue_scale,
					    &map->blue_lsb ) ) != 0 )
		return rc;
	if ( ( rc = efifb_colour_map_mask ( mask->ReservedMask, &reserved_scale,
					    &reserved_lsb ) ) != 0 )
		return rc;

	/* Calculate total number of bits per pixel */
	return ( 32 - ( reserved_scale + map->red_scale + map->green_scale +
			map->blue_scale ) );
}

/**
 * Select video mode
 *
 * @v min_width		Minimum required width (in pixels)
 * @v min_height	Minimum required height (in pixels)
 * @v min_bpp		Minimum required colour depth (in bits per pixel)
 * @ret mode_number	Mode number, or negative error
 */
static int efifb_select_mode ( unsigned int min_width, unsigned int min_height,
			       unsigned int min_bpp ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct fbcon_colour_map map;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	int best_mode_number = -ENOENT;
	unsigned int best_score = INT_MAX;
	unsigned int score;
	unsigned int mode;
	int bpp;
	UINTN size;
	EFI_STATUS efirc;
	int rc;

	/* Find the best mode */
	for ( mode = 0 ; mode < efifb.gop->Mode->MaxMode ; mode++ ) {

		/* Get mode information */
		if ( ( efirc = efifb.gop->QueryMode ( efifb.gop, mode, &size,
						      &info ) ) != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( &efifb, "EFIFB could not get mode %d "
			       "information: %s\n", mode, strerror ( rc ) );
			goto err_query;
		}

		/* Skip unusable modes */
		bpp = efifb_colour_map ( info, &map );
		if ( bpp < 0 ) {
			rc = bpp;
			DBGC ( &efifb, "EFIFB could not build colour map for "
			       "mode %d: %s\n", mode, strerror ( rc ) );
			goto err_map;
		}

		/* Skip modes not meeting the requirements */
		if ( ( info->HorizontalResolution < min_width ) ||
		     ( info->VerticalResolution < min_height ) ||
		     ( ( ( unsigned int ) bpp ) < min_bpp ) ) {
			goto err_requirements;
		}

		/* Select this mode if it has the best (i.e. lowest)
		 * score.  We choose the scoring system to favour
		 * modes close to the specified width and height;
		 * within modes of the same width and height we prefer
		 * a higher colour depth.
		 */
		score = ( ( info->HorizontalResolution *
			    info->VerticalResolution ) - bpp );
		if ( score < best_score ) {
			best_mode_number = mode;
			best_score = score;
		}

	err_requirements:
	err_map:
		bs->FreePool ( info );
	err_query:
		continue;
	}

	if ( best_mode_number < 0 )
		DBGC ( &efifb, "EFIFB found no suitable mode\n" );
	return best_mode_number;
}

/**
 * Restore video mode
 *
 * @v rc		Return status code
 */
static int efifb_restore ( void ) {
	EFI_STATUS efirc;
	int rc;

	/* Restore original mode */
	if ( ( efirc = efifb.gop->SetMode ( efifb.gop,
					    efifb.saved_mode ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( &efifb, "EFIFB could not restore mode %d: %s\n",
		       efifb.saved_mode, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Initialise EFI frame buffer
 *
 * @v config		Console configuration, or NULL to reset
 * @ret rc		Return status code
 */
static int efifb_init ( struct console_configuration *config ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	void *interface;
	int mode;
	int bpp;
	EFI_STATUS efirc;
	int rc;

	/* Locate graphics output protocol */
	if ( ( efirc = bs->LocateProtocol ( &efi_graphics_output_protocol_guid,
					    NULL, &interface ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( &efifb, "EFIFB could not locate graphics output "
		       "protocol: %s\n", strerror ( rc ) );
		goto err_locate_gop;
	}
	efifb.gop = interface;

	/* Locate HII font protocol */
	if ( ( efirc = bs->LocateProtocol ( &efi_hii_font_protocol_guid,
					    NULL, &interface ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( &efifb, "EFIFB could not locate HII font protocol: %s\n",
		       strerror ( rc ) );
		goto err_locate_hiifont;
	}
	efifb.hiifont = interface;

	/* Locate glyphs */
	if ( ( rc = efifb_glyphs() ) != 0 )
		goto err_glyphs;

	/* Save original mode */
	efifb.saved_mode = efifb.gop->Mode->Mode;

	/* Select mode */
	if ( ( mode = efifb_select_mode ( config->width, config->height,
					  config->depth ) ) < 0 ) {
		rc = mode;
		goto err_select_mode;
	}

	/* Set mode */
	if ( ( efirc = efifb.gop->SetMode ( efifb.gop, mode ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( &efifb, "EFIFB could not set mode %d: %s\n",
		       mode, strerror ( rc ) );
		goto err_set_mode;
	}
	info = efifb.gop->Mode->Info;

	/* Populate colour map */
	bpp = efifb_colour_map ( info, &efifb.map );
	if ( bpp < 0 ) {
		rc = bpp;
		DBGC ( &efifb, "EFIFB could not build colour map for "
		       "mode %d: %s\n", mode, strerror ( rc ) );
		goto err_map;
	}

	/* Populate pixel geometry */
	efifb.pixel.width = info->HorizontalResolution;
	efifb.pixel.height = info->VerticalResolution;
	efifb.pixel.len = ( ( bpp + 7 ) / 8 );
	efifb.pixel.stride = ( efifb.pixel.len * info->PixelsPerScanLine );

	/* Populate frame buffer address */
	efifb.start = efifb.gop->Mode->FrameBufferBase;
	DBGC ( &efifb, "EFIFB using mode %d (%dx%d %dbpp at %#08lx)\n",
	       mode, efifb.pixel.width, efifb.pixel.height, bpp, efifb.start );

	/* Initialise frame buffer console */
	if ( ( rc = fbcon_init ( &efifb.fbcon, phys_to_user ( efifb.start ),
				 &efifb.pixel, &efifb.map, &efifb.font,
				 config ) ) != 0 )
		goto err_fbcon_init;

	return 0;

	fbcon_fini ( &efifb.fbcon );
 err_fbcon_init:
 err_map:
	efifb_restore();
 err_set_mode:
 err_select_mode:
	ufree ( efifb.glyphs );
 err_glyphs:
 err_locate_hiifont:
 err_locate_gop:
	return rc;
}

/**
 * Finalise EFI frame buffer
 *
 */
static void efifb_fini ( void ) {

	/* Finalise frame buffer console */
	fbcon_fini ( &efifb.fbcon );

	/* Restore saved mode */
	efifb_restore();

	/* Free glyphs */
	ufree ( efifb.glyphs );
}

/**
 * Print a character to current cursor position
 *
 * @v character		Character
 */
static void efifb_putchar ( int character ) {

	fbcon_putchar ( &efifb.fbcon, character );
}

/**
 * Configure console
 *
 * @v config		Console configuration, or NULL to reset
 * @ret rc		Return status code
 */
static int efifb_configure ( struct console_configuration *config ) {
	int rc;

	/* Reset console, if applicable */
	if ( ! efifb_console.disabled ) {
		efifb_fini();
		efi_console.disabled &= ~CONSOLE_DISABLED_OUTPUT;
		ansicol_reset_magic();
	}
	efifb_console.disabled = CONSOLE_DISABLED;

	/* Do nothing more unless we have a usable configuration */
	if ( ( config == NULL ) ||
	     ( config->width == 0 ) || ( config->height == 0 ) ) {
		return 0;
	}

	/* Initialise EFI frame buffer */
	if ( ( rc = efifb_init ( config ) ) != 0 )
		return rc;

	/* Mark console as enabled */
	efifb_console.disabled = 0;
	efi_console.disabled |= CONSOLE_DISABLED_OUTPUT;

	/* Set magic colour to transparent if we have a background picture */
	if ( config->pixbuf )
		ansicol_set_magic_transparent();

	return 0;
}

/** EFI graphics output protocol console driver */
struct console_driver efifb_console __console_driver = {
	.usage = CONSOLE_EFIFB,
	.putchar = efifb_putchar,
	.configure = efifb_configure,
	.disabled = CONSOLE_DISABLED,
};
