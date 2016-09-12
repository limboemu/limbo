/******************************************************************************
 * Copyright (c) 2016 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#ifndef _LIBVIRTIO_INTERNAL_H
#define _LIBVIRTIO_INTERNAL_H

#include <byteorder.h>

static inline uint16_t virtio_cpu_to_modern16(struct virtio_device *dev, uint16_t val)
{
	return dev->is_modern ? cpu_to_le16(val) : val;
}

static inline uint32_t virtio_cpu_to_modern32(struct virtio_device *dev, uint32_t val)
{
	return dev->is_modern ? cpu_to_le32(val) : val;
}

static inline uint64_t virtio_cpu_to_modern64(struct virtio_device *dev, uint64_t val)
{
	return dev->is_modern ? cpu_to_le64(val) : val;
}

static inline uint16_t virtio_modern16_to_cpu(struct virtio_device *dev, uint16_t val)
{
	return dev->is_modern ? le16_to_cpu(val) : val;
}

static inline uint32_t virtio_modern32_to_cpu(struct virtio_device *dev, uint32_t val)
{
	return dev->is_modern ? le32_to_cpu(val) : val;
}

static inline uint64_t virtio_modern64_to_cpu(struct virtio_device *dev, uint64_t val)
{
	return dev->is_modern ? le64_to_cpu(val) : val;
}

#endif /* _LIBVIRTIO_INTERNAL_H */
