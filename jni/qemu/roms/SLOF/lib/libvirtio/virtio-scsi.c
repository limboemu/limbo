/******************************************************************************
 * Copyright (c) 2012 IBM Corporation
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
#include <string.h>
#include <cpu.h>
#include <helpers.h>
#include "virtio.h"
#include "virtio-internal.h"
#include "virtio-scsi.h"

int virtioscsi_send(struct virtio_device *dev,
		    struct virtio_scsi_req_cmd *req,
		    struct virtio_scsi_resp_cmd *resp,
		    int is_read, void *buf, uint64_t buf_len)
{
	struct vring_desc *vq_desc;		/* Descriptor vring */
	struct vring_avail *vq_avail;		/* "Available" vring */
	struct vring_used *vq_used;		/* "Used" vring */

	volatile uint16_t *current_used_idx;
	uint16_t last_used_idx, avail_idx;
	int id;
	uint32_t vq_size, time;

	int vq = VIRTIO_SCSI_REQUEST_VQ;

	vq_size = virtio_get_qsize(dev, vq);
	vq_desc = virtio_get_vring_desc(dev, vq);
	vq_avail = virtio_get_vring_avail(dev, vq);
	vq_used = virtio_get_vring_used(dev, vq);

	avail_idx = virtio_modern16_to_cpu(dev, vq_avail->idx);

	last_used_idx = vq_used->idx;
	current_used_idx = &vq_used->idx;

	/* Determine descriptor index */
	id = (avail_idx * 3) % vq_size;
	virtio_fill_desc(&vq_desc[id], dev->is_modern, (uint64_t)req, sizeof(*req), VRING_DESC_F_NEXT,
			 (id + 1) % vq_size);

	/* Set up virtqueue descriptor for data */
	if (buf && buf_len) {
		virtio_fill_desc(&vq_desc[(id + 1) % vq_size], dev->is_modern,
				 (uint64_t)resp, sizeof(*resp),
				 VRING_DESC_F_NEXT | VRING_DESC_F_WRITE,
				 (id + 2) % vq_size);
		/* Set up virtqueue descriptor for status */
		virtio_fill_desc(&vq_desc[(id + 2) % vq_size], dev->is_modern,
				 (uint64_t)buf, buf_len,
				 (is_read ? VRING_DESC_F_WRITE : 0), 0);
	} else {
		virtio_fill_desc(&vq_desc[(id + 1) % vq_size], dev->is_modern,
				 (uint64_t)resp, sizeof(*resp),
				 VRING_DESC_F_WRITE, 0);
	}

	vq_avail->ring[avail_idx % vq_size] = virtio_cpu_to_modern16(dev, id);
	mb();
	vq_avail->idx = virtio_cpu_to_modern16(dev, avail_idx + 1);

	/* Tell HV that the vq is ready */
	virtio_queue_notify(dev, vq);

	/* Wait for host to consume the descriptor */
	time = SLOF_GetTimer() + VIRTIO_TIMEOUT;
	while (*current_used_idx == last_used_idx) {
		// do something better
		mb();
		if (time < SLOF_GetTimer())
			break;
	}

	return 0;
}

/**
 * Initialize virtio-block device.
 * @param  dev  pointer to virtio device information
 */
int virtioscsi_init(struct virtio_device *dev)
{
	struct vring_avail *vq_avail;
	unsigned int idx = 0;
	int qsize = 0;
	int status = VIRTIO_STAT_ACKNOWLEDGE;

	/* Reset device */
	// XXX That will clear the virtq base. We need to move
	//     initializing it to here anyway
	//
	//     virtio_reset_device(dev);

	/* Acknowledge device. */
	virtio_set_status(dev, status);

	/* Tell HV that we know how to drive the device. */
	status |= VIRTIO_STAT_DRIVER;
	virtio_set_status(dev, status);

	/* Device specific setup - we do not support special features right now */
	if (dev->is_modern) {
		if (virtio_negotiate_guest_features(dev, VIRTIO_F_VERSION_1))
			goto dev_error;
		virtio_get_status(dev, &status);
	} else {
		virtio_set_guest_features(dev, 0);
	}

	while(1) {
		qsize = virtio_get_qsize(dev, idx);
		if (!qsize)
			break;
		virtio_vring_size(qsize);

		vq_avail = virtio_get_vring_avail(dev, idx);
		vq_avail->flags = virtio_cpu_to_modern16(dev, VRING_AVAIL_F_NO_INTERRUPT);
		vq_avail->idx = 0;
		idx++;
	}

	/* Tell HV that setup succeeded */
	status |= VIRTIO_STAT_DRIVER_OK;
	virtio_set_status(dev, status);

	return 0;
dev_error:
	printf("%s: failed\n", __func__);
	status |= VIRTIO_STAT_FAILED;
	virtio_set_status(dev, status);
	return -1;
}

/**
 * Shutdown the virtio-block device.
 * @param  dev  pointer to virtio device information
 */
void virtioscsi_shutdown(struct virtio_device *dev)
{
	/* Quiesce device */
	virtio_set_status(dev, VIRTIO_STAT_FAILED);

	/* Reset device */
	virtio_reset_device(dev);
}
