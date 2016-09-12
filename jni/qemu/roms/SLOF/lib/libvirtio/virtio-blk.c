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

#include <stdio.h>
#include <cpu.h>
#include <helpers.h>
#include <byteorder.h>
#include "virtio.h"
#include "virtio-blk.h"
#include "virtio-internal.h"

#define DEFAULT_SECTOR_SIZE 512
#define DRIVER_FEATURE_SUPPORT  (VIRTIO_BLK_F_BLK_SIZE | VIRTIO_F_VERSION_1)

static struct vqs vq;

/**
 * Initialize virtio-block device.
 * @param  dev  pointer to virtio device information
 */
int
virtioblk_init(struct virtio_device *dev)
{
	struct vring_avail *vq_avail;
	int blk_size = DEFAULT_SECTOR_SIZE;
	uint64_t features;
	int status = VIRTIO_STAT_ACKNOWLEDGE;

	/* Reset device */
	virtio_reset_device(dev);

	/* Acknowledge device. */
	virtio_set_status(dev, status);

	/* Tell HV that we know how to drive the device. */
	status |= VIRTIO_STAT_DRIVER;
	virtio_set_status(dev, status);

	if (dev->is_modern) {
		/* Negotiate features and sets FEATURES_OK if successful */
		if (virtio_negotiate_guest_features(dev, DRIVER_FEATURE_SUPPORT))
			goto dev_error;

		virtio_get_status(dev, &status);
	} else {
		/* Device specific setup - we support F_BLK_SIZE */
		virtio_set_guest_features(dev,  VIRTIO_BLK_F_BLK_SIZE);
	}

	if (virtio_queue_init_vq(dev, &vq, 0))
		goto dev_error;

	vq_avail = virtio_get_vring_avail(dev, 0);
	vq_avail->flags = virtio_cpu_to_modern16(dev, VRING_AVAIL_F_NO_INTERRUPT);
	vq_avail->idx = 0;

	/* Tell HV that setup succeeded */
	status |= VIRTIO_STAT_DRIVER_OK;
	virtio_set_status(dev, status);

	features = virtio_get_host_features(dev);
	if (features & VIRTIO_BLK_F_BLK_SIZE) {
		blk_size = virtio_get_config(dev,
					     offset_of(struct virtio_blk_cfg, blk_size),
					     sizeof(blk_size));
	}

	return blk_size;
dev_error:
	printf("%s: failed\n", __func__);
	status |= VIRTIO_STAT_FAILED;
	virtio_set_status(dev, status);
	return 0;
}


/**
 * Shutdown the virtio-block device.
 * @param  dev  pointer to virtio device information
 */
void
virtioblk_shutdown(struct virtio_device *dev)
{
	/* Quiesce device */
	virtio_set_status(dev, VIRTIO_STAT_FAILED);

	/* Reset device */
	virtio_reset_device(dev);
}

static void fill_blk_hdr(struct virtio_blk_req *blkhdr, bool is_modern,
                         uint32_t type, uint32_t ioprio, uint32_t sector)
{
	if (is_modern) {
		blkhdr->type = cpu_to_le32(type);
		blkhdr->ioprio = cpu_to_le32(ioprio);
		blkhdr->sector = cpu_to_le64(sector);
	} else {
		blkhdr->type = type;
		blkhdr->ioprio = ioprio;
		blkhdr->sector = sector;
	}
}

/**
 * Read blocks
 * @param  reg  pointer to "reg" property
 * @param  buf  pointer to destination buffer
 * @param  blocknum  block number of the first block that should be read
 * @param  cnt  amount of blocks that should be read
 * @return number of blocks that have been read successfully
 */
int
virtioblk_read(struct virtio_device *dev, char *buf, uint64_t blocknum, long cnt)
{
	struct vring_desc *desc;
	int id;
	static struct virtio_blk_req blkhdr;
	//struct virtio_blk_config *blkconf;
	uint64_t capacity;
	uint32_t vq_size, time;
	struct vring_desc *vq_desc;		/* Descriptor vring */
	struct vring_avail *vq_avail;		/* "Available" vring */
	struct vring_used *vq_used;		/* "Used" vring */
	volatile uint8_t status = -1;
	volatile uint16_t *current_used_idx;
	uint16_t last_used_idx, avail_idx;
	int blk_size = DEFAULT_SECTOR_SIZE;

	//printf("virtioblk_read: dev=%p buf=%p blocknum=%li count=%li\n",
	//	dev, buf, blocknum, cnt);

	/* Check whether request is within disk capacity */
	capacity = virtio_get_config(dev,
			offset_of(struct virtio_blk_cfg, capacity),
			sizeof(capacity));
	if (blocknum + cnt - 1 > capacity) {
		puts("virtioblk_read: Access beyond end of device!");
		return 0;
	}

	blk_size = virtio_get_config(dev,
			offset_of(struct virtio_blk_cfg, blk_size),
			sizeof(blk_size));
	if (blk_size % DEFAULT_SECTOR_SIZE) {
		fprintf(stderr, "virtio-blk: Unaligned sector read %d\n", blk_size);
		return 0;
	}

	vq_size = virtio_get_qsize(dev, 0);
	vq_desc = virtio_get_vring_desc(dev, 0);
	vq_avail = virtio_get_vring_avail(dev, 0);
	vq_used = virtio_get_vring_used(dev, 0);

	avail_idx = virtio_modern16_to_cpu(dev, vq_avail->idx);

	last_used_idx = vq_used->idx;
	current_used_idx = &vq_used->idx;

	/* Set up header */
	fill_blk_hdr(&blkhdr, dev->is_modern, VIRTIO_BLK_T_IN | VIRTIO_BLK_T_BARRIER,
		     1, blocknum * blk_size / DEFAULT_SECTOR_SIZE);

	/* Determine descriptor index */
	id = (avail_idx * 3) % vq_size;

	/* Set up virtqueue descriptor for header */
	desc = &vq_desc[id];
	virtio_fill_desc(desc, dev->is_modern, (uint64_t)&blkhdr,
			 sizeof(struct virtio_blk_req),
			 VRING_DESC_F_NEXT, (id + 1) % vq_size);

	/* Set up virtqueue descriptor for data */
	desc = &vq_desc[(id + 1) % vq_size];
	virtio_fill_desc(desc, dev->is_modern, (uint64_t)buf, cnt * blk_size,
			 VRING_DESC_F_NEXT | VRING_DESC_F_WRITE,
			 (id + 2) % vq_size);

	/* Set up virtqueue descriptor for status */
	desc = &vq_desc[(id + 2) % vq_size];
	virtio_fill_desc(desc, dev->is_modern, (uint64_t)&status, 1,
			 VRING_DESC_F_WRITE, 0);

	vq_avail->ring[avail_idx % vq_size] = virtio_cpu_to_modern16 (dev, id);
	mb();
	vq_avail->idx = virtio_cpu_to_modern16(dev, avail_idx + 1);

	/* Tell HV that the queue is ready */
	virtio_queue_notify(dev, 0);

	/* Wait for host to consume the descriptor */
	time = SLOF_GetTimer() + VIRTIO_TIMEOUT;
	while (*current_used_idx == last_used_idx) {
		// do something better
		mb();
		if (time < SLOF_GetTimer())
			break;
	}

	if (status == 0)
		return cnt;

	printf("virtioblk_read failed! status = %i\n", status);

	return 0;
}
