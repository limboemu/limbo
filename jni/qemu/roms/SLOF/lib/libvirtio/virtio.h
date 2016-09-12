/******************************************************************************
 * Copyright (c) 2011 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#ifndef _LIBVIRTIO_H
#define _LIBVIRTIO_H

#include <stdint.h>
#include <stdbool.h>

/* Device status bits */
#define VIRTIO_STAT_ACKNOWLEDGE		1
#define VIRTIO_STAT_DRIVER		2
#define VIRTIO_STAT_DRIVER_OK		4
#define VIRTIO_STAT_FEATURES_OK		8
#define VIRTIO_STAT_NEEDS_RESET		64
#define VIRTIO_STAT_FAILED		128

#define BIT(x) (1UL << (x))

/* VIRTIO 1.0 Device independent feature bits */
#define VIRTIO_F_RING_INDIRECT_DESC	BIT(28)
#define VIRTIO_F_RING_EVENT_IDX		BIT(29)
#define VIRTIO_F_VERSION_1		BIT(32)

#define VIRTIO_TIMEOUT		        5000 /* 5 sec timeout */

/* Definitions for vring_desc.flags */
#define VRING_DESC_F_NEXT	1	/* buffer continues via the next field */
#define VRING_DESC_F_WRITE	2	/* buffer is write-only (otherwise read-only) */
#define VRING_DESC_F_INDIRECT	4	/* buffer contains a list of buffer descriptors */

/* Descriptor table entry - see Virtio Spec chapter 2.3.2 */
struct vring_desc {
	uint64_t addr;		/* Address (guest-physical) */
	uint32_t len;		/* Length */
	uint16_t flags;		/* The flags as indicated above */
	uint16_t next;		/* Next field if flags & NEXT */
};

/* Definitions for vring_avail.flags */
#define VRING_AVAIL_F_NO_INTERRUPT	1

/* Available ring - see Virtio Spec chapter 2.3.4 */
struct vring_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
};

/* Definitions for vring_used.flags */
#define VRING_USED_F_NO_NOTIFY		1

struct vring_used_elem {
	uint32_t id;		/* Index of start of used descriptor chain */
	uint32_t len;		/* Total length of the descriptor chain which was used */
};

struct vring_used {
	uint16_t flags;
	uint16_t idx;
	struct vring_used_elem ring[];
};

/* Structure shared with SLOF and is 16bytes */
struct virtio_cap {
	void *addr;
	uint8_t bar;
	uint8_t is_io;
	uint8_t cap_id;
};

struct virtio_device {
	uint32_t is_modern;     /* Indicates whether to use virtio 1.0 */
	struct virtio_cap legacy;
	struct virtio_cap common;
	struct virtio_cap notify;
	struct virtio_cap isr;
	struct virtio_cap device;
	struct virtio_cap pci;
	uint32_t notify_off_mul;
};

struct vqs {
	uint64_t id;	/* Queue ID */
	uint32_t size;
	void *buf_mem;
	struct vring_desc *desc;
	struct vring_avail *avail;
	struct vring_used *used;
};

/* Parts of the virtqueue are aligned on a 4096 byte page boundary */
#define VQ_ALIGN(addr)	(((addr) + 0xfff) & ~0xfff)

extern unsigned long virtio_vring_size(unsigned int qsize);
extern unsigned int virtio_get_qsize(struct virtio_device *dev, int queue);
extern struct vring_desc *virtio_get_vring_desc(struct virtio_device *dev, int queue);
extern struct vring_avail *virtio_get_vring_avail(struct virtio_device *dev, int queue);
extern struct vring_used *virtio_get_vring_used(struct virtio_device *dev, int queue);
extern void virtio_fill_desc(struct vring_desc *desc, bool is_modern,
                             uint64_t addr, uint32_t len,
                             uint16_t flags, uint16_t next);
extern int virtio_queue_init_vq(struct virtio_device *dev, struct vqs *vq, unsigned int id);

extern struct virtio_device *virtio_setup_vd(void);
extern void virtio_reset_device(struct virtio_device *dev);
extern void virtio_queue_notify(struct virtio_device *dev, int queue);
extern void virtio_set_status(struct virtio_device *dev, int status);
extern void virtio_get_status(struct virtio_device *dev, int *status);
extern void virtio_set_qaddr(struct virtio_device *dev, int queue, unsigned long qaddr);
extern void virtio_set_guest_features(struct virtio_device *dev, uint64_t features);
extern uint64_t virtio_get_host_features(struct virtio_device *dev);
extern int virtio_negotiate_guest_features(struct virtio_device *dev, uint64_t features);
extern uint64_t virtio_get_config(struct virtio_device *dev, int offset, int size);
extern int __virtio_read_config(struct virtio_device *dev, void *dst,
				int offset, int len);


#endif /* _LIBVIRTIO_H */
