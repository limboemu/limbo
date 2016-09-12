#ifndef __EFI_APPLE_NET_BOOT_PROTOCOL_H__
#define __EFI_APPLE_NET_BOOT_PROTOCOL_H__

/** @file
 *
 * Apple Net Boot Protocol
 *
 */

FILE_LICENCE ( BSD3 );

#define EFI_APPLE_NET_BOOT_PROTOCOL_GUID				\
	{ 0x78ee99fb, 0x6a5e, 0x4186,					\
	{ 0x97, 0xde, 0xcd, 0x0a, 0xba, 0x34, 0x5a, 0x74 } }

typedef struct _EFI_APPLE_NET_BOOT_PROTOCOL EFI_APPLE_NET_BOOT_PROTOCOL;

/**
  Get a DHCP packet obtained by the firmware during NetBoot.

  @param  This		A pointer to the APPLE_NET_BOOT_PROTOCOL instance.
  @param  BufferSize	A pointer to the size of the buffer in bytes.
  @param  DataBuffer	The memory buffer to copy the packet to. If it is
			NULL, then the size of the packet is returned
			in BufferSize.
  @retval EFI_SUCCESS		The packet was copied.
  @retval EFI_BUFFER_TOO_SMALL	The BufferSize is too small to read the
				current packet. BufferSize has been
				updated with the size needed to
				complete the request.
**/
typedef
EFI_STATUS
(EFIAPI *GET_DHCP_RESPONSE) (
  IN EFI_APPLE_NET_BOOT_PROTOCOL	*This,
  IN OUT UINTN				*BufferSize,
  OUT VOID				*DataBuffer
  );

struct _EFI_APPLE_NET_BOOT_PROTOCOL
{
  GET_DHCP_RESPONSE	GetDhcpResponse;
  GET_DHCP_RESPONSE	GetBsdpResponse;
};

#endif /*__EFI_APPLE_NET_BOOT_PROTOCOL_H__ */
