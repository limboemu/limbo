/*
 * (c) Copyright 2010 Stefan Hajnoczi <stefanha@gmail.com>
 *
 * based on the Etherboot virtio-net driver
 *
 *  (c) Copyright 2008 Bull S.A.S.
 *
 *  Author: Laurent Vivier <Laurent.Vivier@bull.net>
 *
 * some parts from Linux Virtio PCI driver
 *
 *  Copyright IBM Corp. 2007
 *  Authors: Anthony Liguori  <aliguori@us.ibm.com>
 *
 *  some parts from Linux Virtio Ring
 *
 *  Copyright Rusty Russell IBM Corporation 2007
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ipxe/list.h>
#include <ipxe/iobuf.h>
#include <ipxe/netdevice.h>
#include <ipxe/pci.h>
#include <ipxe/if_ether.h>
#include <ipxe/ethernet.h>
#include <ipxe/virtio-pci.h>
#include <ipxe/virtio-ring.h>
#include "virtio-net.h"

/*
 * Virtio network device driver
 *
 * Specification:
 * http://ozlabs.org/~rusty/virtio-spec/
 *
 * The virtio network device is supported by Linux virtualization software
 * including QEMU/KVM and lguest.  This driver supports the virtio over PCI
 * transport; virtual machines have one virtio-net PCI adapter per NIC.
 *
 * Virtio-net is different from hardware NICs because virtio devices
 * communicate with the hypervisor via virtqueues, not traditional descriptor
 * rings.  Virtqueues are unordered queues, they support add_buf() and
 * get_buf() operations.  To transmit a packet, the driver has to add the
 * packet buffer onto the virtqueue.  To receive a packet, the driver must
 * first add an empty buffer to the virtqueue and then get the filled packet
 * buffer on completion.
 *
 * Virtqueues are an abstraction that is commonly implemented using the vring
 * descriptor ring layout.  The vring is the actual shared memory structure
 * that allows the virtual machine to communicate buffers with the hypervisor.
 * Because the vring layout is optimized for flexibility and performance rather
 * than space, it is heavy-weight and allocated like traditional descriptor
 * rings in the open() function of the driver and not in probe().
 *
 * There is no true interrupt enable/disable.  Virtqueues have callback
 * enable/disable flags but these are only hints.  The hypervisor may still
 * raise an interrupt.  Nevertheless, this driver disables callbacks in the
 * hopes of avoiding interrupts.
 */

/* Driver types are declared here so virtio-net.h can be easily synced with its
 * Linux source.
 */

/* Virtqueue indices */
enum {
	RX_INDEX = 0,
	TX_INDEX,
	QUEUE_NB
};

enum {
	/** Max number of pending rx packets */
	NUM_RX_BUF = 8,

	/** Max Ethernet frame length, including FCS and VLAN tag */
	RX_BUF_SIZE = 1522,
};

struct virtnet_nic {
	/** Base pio register address */
	unsigned long ioaddr;

	/** 0 for legacy, 1 for virtio 1.0 */
	int virtio_version;

	/** Virtio 1.0 device data */
	struct virtio_pci_modern_device vdev;

	/** RX/TX virtqueues */
	struct vring_virtqueue *virtqueue;

	/** RX packets handed to the NIC waiting to be filled in */
	struct list_head rx_iobufs;

	/** Pending rx packet count */
	unsigned int rx_num_iobufs;

	/** Virtio net packet header, we only need one */
	struct virtio_net_hdr_modern empty_header;
};

/** Add an iobuf to a virtqueue
 *
 * @v netdev		Network device
 * @v vq_idx		Virtqueue index (RX_INDEX or TX_INDEX)
 * @v iobuf		I/O buffer
 *
 * The virtqueue is kicked after the iobuf has been added.
 */
static void virtnet_enqueue_iob ( struct net_device *netdev,
				  int vq_idx, struct io_buffer *iobuf ) {
	struct virtnet_nic *virtnet = netdev->priv;
	struct vring_virtqueue *vq = &virtnet->virtqueue[vq_idx];
	unsigned int out = ( vq_idx == TX_INDEX ) ? 2 : 0;
	unsigned int in = ( vq_idx == TX_INDEX ) ? 0 : 2;
	size_t header_len = virtnet->virtio_version
		? sizeof ( virtnet->empty_header )
		: sizeof ( virtnet->empty_header.legacy );
	struct vring_list list[] = {
		{
			/* Share a single zeroed virtio net header between all
			 * rx and tx packets.  This works because this driver
			 * does not use any advanced features so none of the
			 * header fields get used.
			 */
			.addr = ( char* ) &virtnet->empty_header,
			.length = header_len,
		},
		{
			.addr = ( char* ) iobuf->data,
			.length = iob_len ( iobuf ),
		},
	};

	DBGC2 ( virtnet, "VIRTIO-NET %p enqueuing iobuf %p on vq %d\n",
		virtnet, iobuf, vq_idx );

	vring_add_buf ( vq, list, out, in, iobuf, 0 );
	vring_kick ( virtnet->virtio_version ? &virtnet->vdev : NULL,
		     virtnet->ioaddr, vq, 1 );
}

/** Try to keep rx virtqueue filled with iobufs
 *
 * @v netdev		Network device
 */
static void virtnet_refill_rx_virtqueue ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;

	while ( virtnet->rx_num_iobufs < NUM_RX_BUF ) {
		struct io_buffer *iobuf;

		/* Try to allocate a buffer, stop for now if out of memory */
		iobuf = alloc_iob ( RX_BUF_SIZE );
		if ( ! iobuf )
			break;

		/* Keep track of iobuf so close() can free it */
		list_add ( &iobuf->list, &virtnet->rx_iobufs );

		/* Mark packet length until we know the actual size */
		iob_put ( iobuf, RX_BUF_SIZE );

		virtnet_enqueue_iob ( netdev, RX_INDEX, iobuf );
		virtnet->rx_num_iobufs++;
	}
}

/** Open network device, legacy virtio 0.9.5
 *
 * @v netdev	Network device
 * @ret rc	Return status code
 */
static int virtnet_open_legacy ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;
	unsigned long ioaddr = virtnet->ioaddr;
	u32 features;
	int i;

	/* Reset for sanity */
	vp_reset ( ioaddr );

	/* Allocate virtqueues */
	virtnet->virtqueue = zalloc ( QUEUE_NB *
				      sizeof ( *virtnet->virtqueue ) );
	if ( ! virtnet->virtqueue )
		return -ENOMEM;

	/* Initialize rx/tx virtqueues */
	for ( i = 0; i < QUEUE_NB; i++ ) {
		if ( vp_find_vq ( ioaddr, i, &virtnet->virtqueue[i] ) == -1 ) {
			DBGC ( virtnet, "VIRTIO-NET %p cannot register queue %d\n",
			       virtnet, i );
			free ( virtnet->virtqueue );
			virtnet->virtqueue = NULL;
			return -ENOENT;
		}
	}

	/* Initialize rx packets */
	INIT_LIST_HEAD ( &virtnet->rx_iobufs );
	virtnet->rx_num_iobufs = 0;
	virtnet_refill_rx_virtqueue ( netdev );

	/* Disable interrupts before starting */
	netdev_irq ( netdev, 0 );

	/* Driver is ready */
	features = vp_get_features ( ioaddr );
	vp_set_features ( ioaddr, features & ( 1 << VIRTIO_NET_F_MAC ) );
	vp_set_status ( ioaddr, VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK );
	return 0;
}

/** Open network device, modern virtio 1.0
 *
 * @v netdev	Network device
 * @ret rc	Return status code
 */
static int virtnet_open_modern ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;
	u64 features;
	u8 status;

	/* Negotiate features */
	features = vpm_get_features ( &virtnet->vdev );
	if ( ! ( features & VIRTIO_F_VERSION_1 ) ) {
		vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_FAILED );
		return -EINVAL;
	}
	vpm_set_features ( &virtnet->vdev, features & (
		( 1ULL << VIRTIO_NET_F_MAC ) |
		( 1ULL << VIRTIO_F_VERSION_1 ) |
		( 1ULL << VIRTIO_F_ANY_LAYOUT ) ) );
	vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_FEATURES_OK );

	status = vpm_get_status ( &virtnet->vdev );
	if ( ! ( status & VIRTIO_CONFIG_S_FEATURES_OK ) ) {
		DBGC ( virtnet, "VIRTIO-NET %p device didn't accept features\n",
		       virtnet );
		vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_FAILED );
		return -EINVAL;
	}

	/* Allocate virtqueues */
	virtnet->virtqueue = zalloc ( QUEUE_NB *
				      sizeof ( *virtnet->virtqueue ) );
	if ( ! virtnet->virtqueue ) {
		vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_FAILED );
		return -ENOMEM;
	}

	/* Initialize rx/tx virtqueues */
	if ( vpm_find_vqs ( &virtnet->vdev, QUEUE_NB, virtnet->virtqueue ) ) {
		DBGC ( virtnet, "VIRTIO-NET %p cannot register queues\n",
		       virtnet );
		free ( virtnet->virtqueue );
		virtnet->virtqueue = NULL;
		vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_FAILED );
		return -ENOENT;
	}

	/* Disable interrupts before starting */
	netdev_irq ( netdev, 0 );

	vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_DRIVER_OK );

	/* Initialize rx packets */
	INIT_LIST_HEAD ( &virtnet->rx_iobufs );
	virtnet->rx_num_iobufs = 0;
	virtnet_refill_rx_virtqueue ( netdev );
	return 0;
}

/** Open network device
 *
 * @v netdev	Network device
 * @ret rc	Return status code
 */
static int virtnet_open ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;

	if ( virtnet->virtio_version ) {
		return virtnet_open_modern ( netdev );
	} else {
		return virtnet_open_legacy ( netdev );
	}
}

/** Close network device
 *
 * @v netdev	Network device
 */
static void virtnet_close ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;
	struct io_buffer *iobuf;
	struct io_buffer *next_iobuf;
	int i;

	if ( virtnet->virtio_version ) {
		vpm_reset ( &virtnet->vdev );
	} else {
		vp_reset ( virtnet->ioaddr );
	}

	/* Virtqueues can be freed now that NIC is reset */
	for ( i = 0 ; i < QUEUE_NB ; i++ ) {
		virtio_pci_unmap_capability ( &virtnet->virtqueue[i].notification );
	}

	free ( virtnet->virtqueue );
	virtnet->virtqueue = NULL;

	/* Free rx iobufs */
	list_for_each_entry_safe ( iobuf, next_iobuf, &virtnet->rx_iobufs, list ) {
		free_iob ( iobuf );
	}
	INIT_LIST_HEAD ( &virtnet->rx_iobufs );
	virtnet->rx_num_iobufs = 0;
}

/** Transmit packet
 *
 * @v netdev	Network device
 * @v iobuf	I/O buffer
 * @ret rc	Return status code
 */
static int virtnet_transmit ( struct net_device *netdev,
			      struct io_buffer *iobuf ) {
	virtnet_enqueue_iob ( netdev, TX_INDEX, iobuf );
	return 0;
}

/** Complete packet transmission
 *
 * @v netdev	Network device
 */
static void virtnet_process_tx_packets ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;
	struct vring_virtqueue *tx_vq = &virtnet->virtqueue[TX_INDEX];

	while ( vring_more_used ( tx_vq ) ) {
		struct io_buffer *iobuf = vring_get_buf ( tx_vq, NULL );

		DBGC2 ( virtnet, "VIRTIO-NET %p tx complete iobuf %p\n",
			virtnet, iobuf );

		netdev_tx_complete ( netdev, iobuf );
	}
}

/** Complete packet reception
 *
 * @v netdev	Network device
 */
static void virtnet_process_rx_packets ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;
	struct vring_virtqueue *rx_vq = &virtnet->virtqueue[RX_INDEX];

	while ( vring_more_used ( rx_vq ) ) {
		unsigned int len;
		struct io_buffer *iobuf = vring_get_buf ( rx_vq, &len );

		/* Release ownership of iobuf */
		list_del ( &iobuf->list );
		virtnet->rx_num_iobufs--;

		/* Update iobuf length */
		iob_unput ( iobuf, RX_BUF_SIZE );
		iob_put ( iobuf, len - sizeof ( struct virtio_net_hdr ) );

		DBGC2 ( virtnet, "VIRTIO-NET %p rx complete iobuf %p len %zd\n",
			virtnet, iobuf, iob_len ( iobuf ) );

		/* Pass completed packet to the network stack */
		netdev_rx ( netdev, iobuf );
	}

	virtnet_refill_rx_virtqueue ( netdev );
}

/** Poll for completed and received packets
 *
 * @v netdev	Network device
 */
static void virtnet_poll ( struct net_device *netdev ) {
	struct virtnet_nic *virtnet = netdev->priv;

	/* Acknowledge interrupt.  This is necessary for UNDI operation and
	 * interrupts that are raised despite VRING_AVAIL_F_NO_INTERRUPT being
	 * set (that flag is just a hint and the hypervisor does not have to
	 * honor it).
	 */
	if ( virtnet->virtio_version ) {
		vpm_get_isr ( &virtnet->vdev );
	} else {
		vp_get_isr ( virtnet->ioaddr );
	}

	virtnet_process_tx_packets ( netdev );
	virtnet_process_rx_packets ( netdev );
}

/** Enable or disable interrupts
 *
 * @v netdev	Network device
 * @v enable	Interrupts should be enabled
 */
static void virtnet_irq ( struct net_device *netdev, int enable ) {
	struct virtnet_nic *virtnet = netdev->priv;
	int i;

	for ( i = 0; i < QUEUE_NB; i++ ) {
		if ( enable )
			vring_enable_cb ( &virtnet->virtqueue[i] );
		else
			vring_disable_cb ( &virtnet->virtqueue[i] );
	}
}

/** virtio-net device operations */
static struct net_device_operations virtnet_operations = {
	.open = virtnet_open,
	.close = virtnet_close,
	.transmit = virtnet_transmit,
	.poll = virtnet_poll,
	.irq = virtnet_irq,
};

/**
 * Probe PCI device, legacy virtio 0.9.5
 *
 * @v pci	PCI device
 * @ret rc	Return status code
 */
static int virtnet_probe_legacy ( struct pci_device *pci ) {
	unsigned long ioaddr = pci->ioaddr;
	struct net_device *netdev;
	struct virtnet_nic *virtnet;
	u32 features;
	int rc;

	/* Allocate and hook up net device */
	netdev = alloc_etherdev ( sizeof ( *virtnet ) );
	if ( ! netdev )
		return -ENOMEM;
	netdev_init ( netdev, &virtnet_operations );
	virtnet = netdev->priv;
	virtnet->ioaddr = ioaddr;
	pci_set_drvdata ( pci, netdev );
	netdev->dev = &pci->dev;

	DBGC ( virtnet, "VIRTIO-NET %p busaddr=%s ioaddr=%#lx irq=%d\n",
	       virtnet, pci->dev.name, ioaddr, pci->irq );

	/* Enable PCI bus master and reset NIC */
	adjust_pci_device ( pci );
	vp_reset ( ioaddr );

	/* Load MAC address */
	features = vp_get_features ( ioaddr );
	if ( features & ( 1 << VIRTIO_NET_F_MAC ) ) {
		vp_get ( ioaddr, offsetof ( struct virtio_net_config, mac ),
			 netdev->hw_addr, ETH_ALEN );
		DBGC ( virtnet, "VIRTIO-NET %p mac=%s\n", virtnet,
		       eth_ntoa ( netdev->hw_addr ) );
	}

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register_netdev;

	/* Mark link as up, control virtqueue is not used */
	netdev_link_up ( netdev );

	return 0;

	unregister_netdev ( netdev );
 err_register_netdev:
	vp_reset ( ioaddr );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
	return rc;
}

/**
 * Probe PCI device, modern virtio 1.0
 *
 * @v pci	PCI device
 * @v found_dev	Set to non-zero if modern device was found (probe may still fail)
 * @ret rc	Return status code
 */
static int virtnet_probe_modern ( struct pci_device *pci, int *found_dev ) {
	struct net_device *netdev;
	struct virtnet_nic *virtnet;
	u64 features;
	int rc, common, isr, notify, config, device;

	common = virtio_pci_find_capability ( pci, VIRTIO_PCI_CAP_COMMON_CFG );
	if ( ! common ) {
		DBG ( "Common virtio capability not found!\n" );
		return -ENODEV;
	}
	*found_dev = 1;

	isr = virtio_pci_find_capability ( pci, VIRTIO_PCI_CAP_ISR_CFG );
	notify = virtio_pci_find_capability ( pci, VIRTIO_PCI_CAP_NOTIFY_CFG );
	config = virtio_pci_find_capability ( pci, VIRTIO_PCI_CAP_PCI_CFG );
	if ( ! isr || ! notify || ! config ) {
		DBG ( "Missing virtio capabilities %i/%i/%i/%i\n",
		      common, isr, notify, config );
		return -EINVAL;
	}
	device = virtio_pci_find_capability ( pci, VIRTIO_PCI_CAP_DEVICE_CFG );

	/* Allocate and hook up net device */
	netdev = alloc_etherdev ( sizeof ( *virtnet ) );
	if ( ! netdev )
		return -ENOMEM;
	netdev_init ( netdev, &virtnet_operations );
	virtnet = netdev->priv;

	pci_set_drvdata ( pci, netdev );
	netdev->dev = &pci->dev;

	DBGC ( virtnet, "VIRTIO-NET modern %p busaddr=%s irq=%d\n",
	       virtnet, pci->dev.name, pci->irq );

	virtnet->vdev.pci = pci;
	rc = virtio_pci_map_capability ( pci, common,
		sizeof ( struct virtio_pci_common_cfg ), 4,
		0, sizeof ( struct virtio_pci_common_cfg ),
		&virtnet->vdev.common );
	if ( rc )
		goto err_map_common;

	rc = virtio_pci_map_capability ( pci, isr, sizeof ( u8 ), 1,
		0, 1,
		&virtnet->vdev.isr );
	if ( rc )
		goto err_map_isr;

	virtnet->vdev.notify_cap_pos = notify;
	virtnet->vdev.cfg_cap_pos = config;

	/* Map the device capability */
	if ( device ) {
		rc = virtio_pci_map_capability ( pci, device,
			0, 4, 0, sizeof ( struct virtio_net_config ),
			&virtnet->vdev.device );
		if ( rc )
			goto err_map_device;
	}

	/* Enable the PCI device */
	adjust_pci_device ( pci );

	/* Reset the device and set initial status bits */
	vpm_reset ( &virtnet->vdev );
	vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE );
	vpm_add_status ( &virtnet->vdev, VIRTIO_CONFIG_S_DRIVER );

	/* Load MAC address */
	if ( device ) {
		features = vpm_get_features ( &virtnet->vdev );
		if ( features & ( 1ULL << VIRTIO_NET_F_MAC ) ) {
			vpm_get ( &virtnet->vdev,
				  offsetof ( struct virtio_net_config, mac ),
				  netdev->hw_addr, ETH_ALEN );
			DBGC ( virtnet, "VIRTIO-NET %p mac=%s\n", virtnet,
			       eth_ntoa ( netdev->hw_addr ) );
		}
	}

	/* We need a valid MAC address */
	if ( ! is_valid_ether_addr ( netdev->hw_addr ) ) {
		rc = -EADDRNOTAVAIL;
		goto err_mac_address;
	}

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register_netdev;

	/* Mark link as up, control virtqueue is not used */
	netdev_link_up ( netdev );

	virtnet->virtio_version = 1;
	return 0;

	unregister_netdev ( netdev );
err_register_netdev:
err_mac_address:
	vpm_reset ( &virtnet->vdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );

	virtio_pci_unmap_capability ( &virtnet->vdev.device );
err_map_device:
	virtio_pci_unmap_capability ( &virtnet->vdev.isr );
err_map_isr:
	virtio_pci_unmap_capability ( &virtnet->vdev.common );
err_map_common:
	return rc;
}

/**
 * Probe PCI device
 *
 * @v pci	PCI device
 * @ret rc	Return status code
 */
static int virtnet_probe ( struct pci_device *pci ) {
	int found_modern = 0;
	int rc = virtnet_probe_modern ( pci, &found_modern );
	if ( ! found_modern && pci->device < 0x1040 ) {
		/* fall back to the legacy probe */
		rc = virtnet_probe_legacy ( pci );
	}
	return rc;
}

/**
 * Remove device
 *
 * @v pci	PCI device
 */
static void virtnet_remove ( struct pci_device *pci ) {
	struct net_device *netdev = pci_get_drvdata ( pci );
	struct virtnet_nic *virtnet = netdev->priv;

	virtio_pci_unmap_capability ( &virtnet->vdev.device );
	virtio_pci_unmap_capability ( &virtnet->vdev.isr );
	virtio_pci_unmap_capability ( &virtnet->vdev.common );

	unregister_netdev ( netdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

static struct pci_device_id virtnet_nics[] = {
PCI_ROM(0x1af4, 0x1000, "virtio-net", "Virtio Network Interface", 0),
PCI_ROM(0x1af4, 0x1041, "virtio-net", "Virtio Network Interface 1.0", 0),
};

struct pci_driver virtnet_driver __pci_driver = {
	.ids = virtnet_nics,
	.id_count = ( sizeof ( virtnet_nics ) / sizeof ( virtnet_nics[0] ) ),
	.probe = virtnet_probe,
	.remove = virtnet_remove,
};
