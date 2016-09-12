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

#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include <netdriver.h>

#define RX_QUEUE_SIZE		128
#define BUFFER_ENTRY_SIZE	1514

enum {
	VQ_RX = 0,	/* Receive Queue */
	VQ_TX = 1,	/* Transmit Queue */
};

/* VIRTIO_NET Feature bits */
#define VIRTIO_NET_F_MAC       (1 << 5)

extern net_driver_t *virtionet_open(struct virtio_device *dev);
extern void virtionet_close(net_driver_t *driver);
extern int virtionet_read(char *buf, int len);
extern int virtionet_write(char *buf, int len);

#endif
