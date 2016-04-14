/*
 * Copyright (C) 2010 Piotr Jaroszyński <p.jaroszynski@gmail.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _IPXE_LINUX_UACCESS_H
#define _IPXE_LINUX_UACCESS_H

FILE_LICENCE(GPL2_OR_LATER);

/** @file
 *
 * iPXE user access API for linux
 *
 * In linux userspace virtual == user == phys addresses.
 * Physical addresses also being the same is wrong, but there is no general way
 * of converting userspace addresses to physical as what appears to be
 * contiguous in userspace is physically fragmented.
 * Currently only the DMA memory is special-cased, but its conversion to bus
 * addresses is done in phys_to_bus.
 * This is known to break virtio as it is passing phys addresses to the virtual
 * device.
 */

#ifdef UACCESS_LINUX
#define UACCESS_PREFIX_linux
#else
#define UACCESS_PREFIX_linux __linux_
#endif

static inline __always_inline userptr_t
UACCESS_INLINE(linux, phys_to_user)(unsigned long phys_addr)
{
	return phys_addr;
}

static inline __always_inline unsigned long
UACCESS_INLINE(linux, user_to_phys)(userptr_t userptr, off_t offset)
{
	return userptr + offset;
}

static inline __always_inline userptr_t
UACCESS_INLINE(linux, virt_to_user)(volatile const void *addr)
{
	return trivial_virt_to_user(addr);
}

static inline __always_inline void *
UACCESS_INLINE(linux, user_to_virt)(userptr_t userptr, off_t offset)
{
	return trivial_user_to_virt(userptr, offset);
}

static inline __always_inline userptr_t
UACCESS_INLINE(linux, userptr_add)(userptr_t userptr, off_t offset)
{
	return trivial_userptr_add(userptr, offset);
}

static inline __always_inline off_t
UACCESS_INLINE(linux, userptr_sub)(userptr_t userptr, userptr_t subtrahend)
{
	return trivial_userptr_sub ( userptr, subtrahend );
}

static inline __always_inline void
UACCESS_INLINE(linux, memcpy_user)(userptr_t dest, off_t dest_off, userptr_t src, off_t src_off, size_t len)
{
	trivial_memcpy_user(dest, dest_off, src, src_off, len);
}

static inline __always_inline void
UACCESS_INLINE(linux, memmove_user)(userptr_t dest, off_t dest_off, userptr_t src, off_t src_off, size_t len)
{
	trivial_memmove_user(dest, dest_off, src, src_off, len);
}

static inline __always_inline int
UACCESS_INLINE(linux, memcmp_user)(userptr_t first, off_t first_off, userptr_t second, off_t second_off, size_t len)
{
	return trivial_memcmp_user(first, first_off, second, second_off, len);
}

static inline __always_inline void
UACCESS_INLINE(linux, memset_user)(userptr_t buffer, off_t offset, int c, size_t len)
{
	trivial_memset_user(buffer, offset, c, len);
}

static inline __always_inline size_t
UACCESS_INLINE(linux, strlen_user)(userptr_t buffer, off_t offset)
{
	return trivial_strlen_user(buffer, offset);
}

static inline __always_inline off_t
UACCESS_INLINE(linux, memchr_user)(userptr_t buffer, off_t offset, int c, size_t len)
{
	return trivial_memchr_user(buffer, offset, c, len);
}

#endif /* _IPXE_LINUX_UACCESS_H */
