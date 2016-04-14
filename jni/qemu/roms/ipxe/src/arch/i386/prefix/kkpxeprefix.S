/*****************************************************************************
 * PXE prefix that keeps the whole PXE stack present
 *****************************************************************************
 */

FILE_LICENCE ( GPL2_OR_LATER )

/* Since we have the whole stack, we can use cached DHCP information */
REQUEST_OBJECT ( pxeparent_dhcp )

#define PXELOADER_KEEP_UNDI
#define PXELOADER_KEEP_PXE
#define _pxe_start _kkpxe_start
#include "pxeprefix.S"
