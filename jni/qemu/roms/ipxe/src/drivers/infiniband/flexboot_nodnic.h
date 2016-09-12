#ifndef SRC_DRIVERS_INFINIBAND_FLEXBOOT_NODNIC_FLEXBOOT_NODNIC_H_
#define SRC_DRIVERS_INFINIBAND_FLEXBOOT_NODNIC_FLEXBOOT_NODNIC_H_

/*
 * Copyright (C) 2015 Mellanox Technologies Ltd.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include "mlx_nodnic/include/mlx_nodnic_data_structures.h"
#include "nodnic_prm.h"
#include <ipxe/io.h>
#include <ipxe/infiniband.h>
#include <ipxe/netdevice.h>

/*
 * If defined, use interrupts in NODNIC driver
 */
#define NODNIC_IRQ_ENABLED

#define FLEXBOOT_NODNIC_MAX_PORTS		2
#define FLEXBOOT_NODNIC_PORT_BASE		1

#define FLEXBOOT_NODNIC_OPCODE_SEND		0xa

/* Port protocol */
enum flexboot_nodnic_protocol {
	FLEXBOOT_NODNIC_PROT_IB_IPV6 = 0,
	FLEXBOOT_NODNIC_PROT_ETH,
	FLEXBOOT_NODNIC_PROT_IB_IPV4,
	FLEXBOOT_NODNIC_PROT_FCOE
};

/** A flexboot nodnic port */
struct flexboot_nodnic_port {
	/** Infiniband device */
	struct ib_device *ibdev;
	/** Network device */
	struct net_device *netdev;
	/** nodic port */
	nodnic_port_priv port_priv;
	/** Port type */
	struct flexboot_nodnic_port_type *type;
	/** Ethernet completion queue */
	struct ib_completion_queue *eth_cq;
	/** Ethernet queue pair */
	struct ib_queue_pair *eth_qp;
};


/** A flexboot nodnic queue pair */
struct flexboot_nodnic_queue_pair {
	nodnic_qp *nodnic_queue_pair;
};

/** A flexboot nodnic cq */
struct flexboot_nodnic_completion_queue {
	nodnic_cq *nodnic_completion_queue;
};

/** A flexboot_nodnic device */
struct flexboot_nodnic {
	/** PCI device */
	struct pci_device *pci;
	/** nic specific data*/
	struct flexboot_nodnic_callbacks *callbacks;
	/**nodnic device*/
	nodnic_device_priv device_priv;
	/**flexboot_nodnic ports*/
	struct flexboot_nodnic_port port[FLEXBOOT_NODNIC_MAX_PORTS];
	/** Device open request counter */
	unsigned int open_count;
	/** Port masking  */
	u16 port_mask;
	/** device private data */
	void *priv_data;
};

/** A flexboot_nodnic port type */
struct flexboot_nodnic_port_type {
	/** Register port
	 *
	 * @v flexboot_nodnic		flexboot_nodnic device
	 * @v port		flexboot_nodnic port
	 * @ret mlx_status		Return status code
	 */
	mlx_status ( * register_dev ) (
			struct flexboot_nodnic *flexboot_nodnic,
			struct flexboot_nodnic_port *port
			);
	/** Port state changed
	 *
	 * @v flexboot_nodnic		flexboot_nodnic device
	 * @v port		flexboot_nodnic port
	 * @v link_up		Link is up
	 */
	void ( * state_change ) (
			struct flexboot_nodnic *flexboot_nodnic,
				  struct flexboot_nodnic_port *port,
				  int link_up
				  );
	/** Unregister port
	 *
	 * @v flexboot_nodnic		flexboot_nodnic device
	 * @v port		flexboot_nodnic port
	 */
	void ( * unregister_dev ) (
			struct flexboot_nodnic *flexboot_nodnic,
			struct flexboot_nodnic_port *port
			);
};

struct cqe_data{
	mlx_boolean owner;
	mlx_uint32 qpn;
	mlx_uint32 is_send;
	mlx_uint32 is_error;
	mlx_uint32 syndrome;
	mlx_uint32 vendor_err_syndrome;
	mlx_uint32 wqe_counter;
	mlx_uint32 byte_cnt;
};

struct flexboot_nodnic_callbacks {
	mlx_status ( * fill_completion ) ( void *cqe, struct cqe_data *cqe_data );
	mlx_status ( * cqe_set_owner ) ( void *cq, unsigned int num_cqes );
	mlx_size ( * get_cqe_size ) ();
	mlx_status ( * fill_send_wqe[5] ) (
				struct ib_device *ibdev,
				struct ib_queue_pair *qp,
				struct ib_address_vector *av,
				struct io_buffer *iobuf,
				struct nodnic_send_wqbb *wqbb,
				unsigned long wqe_idx
				);
	void ( * irq ) ( struct net_device *netdev, int enable );
};

int flexboot_nodnic_probe ( struct pci_device *pci,
		struct flexboot_nodnic_callbacks *callbacks,
		void *drv_priv );
void flexboot_nodnic_remove ( struct pci_device *pci );
void flexboot_nodnic_eth_irq ( struct net_device *netdev, int enable );
int flexboot_nodnic_is_supported ( struct pci_device *pci );
void flexboot_nodnic_copy_mac ( uint8_t mac_addr[], uint32_t low_byte,
		uint16_t high_byte );

#endif /* SRC_DRIVERS_INFINIBAND_FLEXBOOT_NODNIC_FLEXBOOT_NODNIC_H_ */
