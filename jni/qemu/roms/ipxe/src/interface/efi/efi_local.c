/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/refcnt.h>
#include <ipxe/malloc.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/uri.h>
#include <ipxe/iobuf.h>
#include <ipxe/process.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_strings.h>
#include <ipxe/efi/efi_utils.h>
#include <ipxe/efi/Protocol/SimpleFileSystem.h>
#include <ipxe/efi/Guid/FileInfo.h>
#include <ipxe/efi/Guid/FileSystemInfo.h>

/** @file
 *
 * EFI local file access
 *
 */

/** Download blocksize */
#define EFI_LOCAL_BLKSIZE 4096

/** An EFI local file */
struct efi_local {
	/** Reference count */
	struct refcnt refcnt;
	/** Data transfer interface */
	struct interface xfer;
	/** Download process */
	struct process process;

	/** EFI root directory */
	EFI_FILE_PROTOCOL *root;
	/** EFI file */
	EFI_FILE_PROTOCOL *file;
	/** Length of file */
	size_t len;
};

/**
 * Close local file
 *
 * @v local		Local file
 * @v rc		Reason for close
 */
static void efi_local_close ( struct efi_local *local, int rc ) {

	/* Stop process */
	process_del ( &local->process );

	/* Shut down data transfer interface */
	intf_shutdown ( &local->xfer, rc );

	/* Close EFI file */
	if ( local->file ) {
		local->file->Close ( local->file );
		local->file = NULL;
	}

	/* Close EFI root directory */
	if ( local->root ) {
		local->root->Close ( local->root );
		local->root = NULL;
	}
}

/**
 * Local file process
 *
 * @v local		Local file
 */
static void efi_local_step ( struct efi_local *local ) {
	EFI_FILE_PROTOCOL *file = local->file;
	struct io_buffer *iobuf = NULL;
	size_t remaining;
	size_t frag_len;
	UINTN size;
	EFI_STATUS efirc;
	int rc;

	/* Wait until data transfer interface is ready */
	if ( ! xfer_window ( &local->xfer ) )
		return;

	/* Presize receive buffer */
	remaining = local->len;
	xfer_seek ( &local->xfer, remaining );
	xfer_seek ( &local->xfer, 0 );

	/* Get file contents */
	while ( remaining ) {

		/* Calculate length for this fragment */
		frag_len = remaining;
		if ( frag_len > EFI_LOCAL_BLKSIZE )
			frag_len = EFI_LOCAL_BLKSIZE;

		/* Allocate I/O buffer */
		iobuf = xfer_alloc_iob ( &local->xfer, frag_len );
		if ( ! iobuf ) {
			rc = -ENOMEM;
			goto err;
		}

		/* Read block */
		size = frag_len;
		if ( ( efirc = file->Read ( file, &size, iobuf->data ) ) != 0 ){
			rc = -EEFI ( efirc );
			DBGC ( local, "LOCAL %p could not read from file: %s\n",
			       local, strerror ( rc ) );
			goto err;
		}
		assert ( size <= frag_len );
		iob_put ( iobuf, size );

		/* Deliver data */
		if ( ( rc = xfer_deliver_iob ( &local->xfer,
					       iob_disown ( iobuf ) ) ) != 0 ) {
			DBGC ( local, "LOCAL %p could not deliver data: %s\n",
			       local, strerror ( rc ) );
			goto err;
		}

		/* Move to next block */
		remaining -= frag_len;
	}

	/* Close download */
	efi_local_close ( local, 0 );

	return;

 err:
	free_iob ( iobuf );
	efi_local_close ( local, rc );
}

/** Data transfer interface operations */
static struct interface_operation efi_local_operations[] = {
	INTF_OP ( xfer_window_changed, struct efi_local *, efi_local_step ),
	INTF_OP ( intf_close, struct efi_local *, efi_local_close ),
};

/** Data transfer interface descriptor */
static struct interface_descriptor efi_local_xfer_desc =
	INTF_DESC ( struct efi_local, xfer, efi_local_operations );

/** Process descriptor */
static struct process_descriptor efi_local_process_desc =
	PROC_DESC_ONCE ( struct efi_local, process, efi_local_step );

/**
 * Check for matching volume name
 *
 * @v local		Local file
 * @v device		Device handle
 * @v root		Root filesystem handle
 * @v volume		Volume name
 * @ret rc		Return status code
 */
static int efi_local_check_volume_name ( struct efi_local *local,
					 EFI_HANDLE device,
					 EFI_FILE_PROTOCOL *root,
					 const char *volume ) {
	EFI_FILE_SYSTEM_INFO *info;
	UINTN size;
	char *label;
	EFI_STATUS efirc;
	int rc;

	/* Get length of file system information */
	size = 0;
	root->GetInfo ( root, &efi_file_system_info_id, &size, NULL );

	/* Allocate file system information */
	info = malloc ( size );
	if ( ! info ) {
		rc = -ENOMEM;
		goto err_alloc_info;
	}

	/* Get file system information */
	if ( ( efirc = root->GetInfo ( root, &efi_file_system_info_id, &size,
				       info ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( local, "LOCAL %p could not get file system info on %s: "
		       "%s\n", local, efi_handle_name ( device ),
		       strerror ( rc ) );
		goto err_get_info;
	}
	DBGC2 ( local, "LOCAL %p found %s with label \"%ls\"\n",
		local, efi_handle_name ( device ), info->VolumeLabel );

	/* Construct volume label for comparison */
	if ( asprintf ( &label, "%ls", info->VolumeLabel ) < 0 ) {
		rc = -ENOMEM;
		goto err_alloc_label;
	}

	/* Compare volume label */
	if ( strcasecmp ( volume, label ) != 0 ) {
		rc = -ENOENT;
		goto err_compare;
	}

	/* Success */
	rc = 0;

 err_compare:
	free ( label );
 err_alloc_label:
 err_get_info:
	free ( info );
 err_alloc_info:
	return rc;
}

/**
 * Open root filesystem
 *
 * @v local		Local file
 * @v device		Device handle
 * @v root		Root filesystem handle to fill in
 * @ret rc		Return status code
 */
static int efi_local_open_root ( struct efi_local *local, EFI_HANDLE device,
				 EFI_FILE_PROTOCOL **root ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	union {
		void *interface;
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	} u;
	EFI_STATUS efirc;
	int rc;

	/* Open file system protocol */
	if ( ( efirc = bs->OpenProtocol ( device,
					  &efi_simple_file_system_protocol_guid,
					  &u.interface, efi_image_handle,
					  device,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		rc = -EEFI ( efirc );
		DBGC ( local, "LOCAL %p could not open filesystem on %s: %s\n",
		       local, efi_handle_name ( device ), strerror ( rc ) );
		goto err_filesystem;
	}

	/* Open root directory */
	if ( ( efirc = u.fs->OpenVolume ( u.fs, root ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( local, "LOCAL %p could not open volume on %s: %s\n",
		       local, efi_handle_name ( device ), strerror ( rc ) );
		goto err_volume;
	}

	/* Success */
	rc = 0;

 err_volume:
	bs->CloseProtocol ( device, &efi_simple_file_system_protocol_guid,
			    efi_image_handle, device );
 err_filesystem:
	return rc;
}

/**
 * Open root filesystem of specified volume
 *
 * @v local		Local file
 * @v volume		Volume name, or NULL to use loaded image's device
 * @ret rc		Return status code
 */
static int efi_local_open_volume ( struct efi_local *local,
				   const char *volume ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_GUID *protocol = &efi_simple_file_system_protocol_guid;
	int ( * check ) ( struct efi_local *local, EFI_HANDLE device,
			  EFI_FILE_PROTOCOL *root, const char *volume );
	EFI_FILE_PROTOCOL *root;
	EFI_HANDLE *handles;
	EFI_HANDLE device;
	UINTN num_handles;
	UINTN i;
	EFI_STATUS efirc;
	int rc;

	/* Identify candidate handles */
	if ( volume ) {
		/* Locate all filesystem handles */
		if ( ( efirc = bs->LocateHandleBuffer ( ByProtocol, protocol,
							NULL, &num_handles,
							&handles ) ) != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( local, "LOCAL %p could not enumerate handles: "
			       "%s\n", local, strerror ( rc ) );
			return rc;
		}
		check = efi_local_check_volume_name;
	} else {
		/* Use our loaded image's device handle */
		handles = &efi_loaded_image->DeviceHandle;
		num_handles = 1;
		check = NULL;
	}

	/* Find matching handle */
	for ( i = 0 ; i < num_handles ; i++ ) {

		/* Get this device handle */
		device = handles[i];

		/* Open root directory */
		if ( ( rc = efi_local_open_root ( local, device, &root ) ) != 0)
			continue;

		/* Check volume name, if applicable */
		if ( ( check == NULL ) ||
		     ( ( rc = check ( local, device, root, volume ) ) == 0 ) ) {
			DBGC ( local, "LOCAL %p using %s",
			       local, efi_handle_name ( device ) );
			if ( volume )
				DBGC ( local, " with label \"%s\"", volume );
			DBGC ( local, "\n" );
			local->root = root;
			break;
		}

		/* Close root directory */
		root->Close ( root );
	}

	/* Free handles, if applicable */
	if ( volume )
		bs->FreePool ( handles );

	/* Fail if we found no matching handle */
	if ( ! local->root ) {
		DBGC ( local, "LOCAL %p found no matching handle\n", local );
		return -ENOENT;
	}

	return 0;
}

/**
 * Open fully-resolved path
 *
 * @v local		Local file
 * @v resolved		Resolved path
 * @ret rc		Return status code
 */
static int efi_local_open_resolved ( struct efi_local *local,
				     const char *resolved ) {
	size_t name_len = strlen ( resolved );
	CHAR16 name[ name_len + 1 /* wNUL */ ];
	EFI_FILE_PROTOCOL *file;
	EFI_STATUS efirc;
	int rc;

	/* Construct filename */
	efi_snprintf ( name, ( name_len + 1 /* wNUL */ ), "%s", resolved );

	/* Open file */
	if ( ( efirc = local->root->Open ( local->root, &file, name,
					   EFI_FILE_MODE_READ, 0 ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( local, "LOCAL %p could not open \"%s\": %s\n",
		       local, resolved, strerror ( rc ) );
		return rc;
	}
	local->file = file;

	return 0;
}

/**
 * Open specified path
 *
 * @v local		Local file
 * @v path		Path to file
 * @ret rc		Return status code
 */
static int efi_local_open_path ( struct efi_local *local, const char *path ) {
	FILEPATH_DEVICE_PATH *fp = container_of ( efi_loaded_image->FilePath,
						  FILEPATH_DEVICE_PATH, Header);
	size_t fp_len = ( fp ? efi_devpath_len ( &fp->Header ) : 0 );
	char base[ fp_len / 2 /* Cannot exceed this length */ ];
	size_t remaining = sizeof ( base );
	size_t len;
	char *resolved;
	char *tmp;
	int rc;

	/* Construct base path to our own image, if possible */
	memset ( base, 0, sizeof ( base ) );
	tmp = base;
	while ( fp && ( fp->Header.Type != END_DEVICE_PATH_TYPE ) ) {
		len = snprintf ( tmp, remaining, "%ls", fp->PathName );
		assert ( len < remaining );
		tmp += len;
		remaining -= len;
		fp = ( ( ( void * ) fp ) + ( ( fp->Header.Length[1] << 8 ) |
					     fp->Header.Length[0] ) );
	}
	DBGC2 ( local, "LOCAL %p base path \"%s\"\n",
		local, base );

	/* Convert to sane path separators */
	for ( tmp = base ; *tmp ; tmp++ ) {
		if ( *tmp == '\\' )
			*tmp = '/';
	}

	/* Resolve path */
	resolved = resolve_path ( base, path );
	if ( ! resolved ) {
		rc = -ENOMEM;
		goto err_resolve;
	}

	/* Convert to insane path separators */
	for ( tmp = resolved ; *tmp ; tmp++ ) {
		if ( *tmp == '/' )
			*tmp = '\\';
	}
	DBGC ( local, "LOCAL %p using \"%s\"\n",
	       local, resolved );

	/* Open resolved path */
	if ( ( rc = efi_local_open_resolved ( local, resolved ) ) != 0 )
		goto err_open;

 err_open:
	free ( resolved );
 err_resolve:
	return rc;
}

/**
 * Get file length
 *
 * @v local		Local file
 * @ret rc		Return status code
 */
static int efi_local_len ( struct efi_local *local ) {
	EFI_FILE_PROTOCOL *file = local->file;
	EFI_FILE_INFO *info;
	EFI_STATUS efirc;
	UINTN size;
	int rc;

	/* Get size of file information */
	size = 0;
	file->GetInfo ( file, &efi_file_info_id, &size, NULL );

	/* Allocate file information */
	info = malloc ( size );
	if ( ! info ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* Get file information */
	if ( ( efirc = file->GetInfo ( file, &efi_file_info_id, &size,
				       info ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( local, "LOCAL %p could not get file info: %s\n",
		       local, strerror ( rc ) );
		goto err_info;
	}

	/* Record file length */
	local->len = info->FileSize;

	/* Success */
	rc = 0;

 err_info:
	free ( info );
 err_alloc:
	return rc;
}

/**
 * Open local file
 *
 * @v xfer		Data transfer interface
 * @v uri		Request URI
 * @ret rc		Return status code
 */
static int efi_local_open ( struct interface *xfer, struct uri *uri ) {
	struct efi_local *local;
	const char *volume;
	const char *path;
	int rc;

	/* Parse URI */
	volume = ( ( uri->host && uri->host[0] ) ? uri->host : NULL );
	path = ( uri->opaque ? uri->opaque : uri->path );

	/* Allocate and initialise structure */
	local = zalloc ( sizeof ( *local ) );
	if ( ! local ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	ref_init ( &local->refcnt, NULL );
	intf_init ( &local->xfer, &efi_local_xfer_desc, &local->refcnt );
	process_init ( &local->process, &efi_local_process_desc,
		       &local->refcnt );

	/* Open specified volume */
	if ( ( rc = efi_local_open_volume ( local, volume ) ) != 0 )
		goto err_open_root;

	/* Open specified path */
	if ( ( rc = efi_local_open_path ( local, path ) ) != 0 )
		goto err_open_file;

	/* Get length of file */
	if ( ( rc = efi_local_len ( local ) ) != 0 )
		goto err_len;

	/* Attach to parent interface, mortalise self, and return */
	intf_plug_plug ( &local->xfer, xfer );
	ref_put ( &local->refcnt );
	return 0;

 err_len:
 err_open_file:
 err_open_root:
	efi_local_close ( local, 0 );
	ref_put ( &local->refcnt );
 err_alloc:
	return rc;
}

/** EFI local file URI opener */
struct uri_opener efi_local_uri_opener __uri_opener = {
	.scheme	= "file",
	.open	= efi_local_open,
};
