#ifndef NODNIC_PORT_H_
#define NODNIC_PORT_H_

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

#include "mlx_nodnic_data_structures.h"

#define NODNIC_PORT_MAC_FILTERS_OFFSET 0x10

typedef enum {
	nodnic_port_option_link_type = 0,
	nodnic_port_option_mac_low,
	nodnic_port_option_mac_high,
	nodnic_port_option_log_cq_size,
	nodnic_port_option_reset_needed,
	nodnic_port_option_mac_filters_en,
	nodnic_port_option_port_state,
	nodnic_port_option_network_en,
	nodnic_port_option_dma_en,
	nodnic_port_option_eq_addr_low,
	nodnic_port_option_eq_addr_high,
	nodnic_port_option_cq_addr_low,
	nodnic_port_option_cq_addr_high,
	nodnic_port_option_port_management_change_event,
	nodnic_port_option_port_promisc_en,
	nodnic_port_option_arm_cq,
	nodnic_port_option_port_promisc_multicast_en,
#ifdef DEVICE_CX3
	nodnic_port_option_crspace_en,
#endif
}nodnic_port_option;

struct nodnic_port_data_entry{
	nodnic_port_option	option;
	mlx_uint32			offset;
	mlx_uint8			align;
	mlx_uint32			mask;
};

struct nodnic_qp_data_entry{
	nodnic_queue_pair_type	type;
	mlx_uint32			send_offset;
	mlx_uint32			recv_offset;
};


typedef enum {
	nodnic_port_state_down = 0,
	nodnic_port_state_initialize,
	nodnic_port_state_armed,
	nodnic_port_state_active,
}nodnic_port_state;

mlx_status
nodnic_port_get_state(
					IN  nodnic_port_priv	*port_priv,
					OUT nodnic_port_state			*state
					);

mlx_status
nodnic_port_get_type(
					IN  nodnic_port_priv	*port_priv,
					OUT nodnic_port_type	*type
					);

mlx_status
nodnic_port_query(
					IN  nodnic_port_priv	*port_priv,
					IN  nodnic_port_option		option,
					OUT	mlx_uint32				*out
					);

mlx_status
nodnic_port_set(
					IN  nodnic_port_priv	*port_priv,
					IN  nodnic_port_option		option,
					IN	mlx_uint32				in
					);

mlx_status
nodnic_port_create_cq(
					IN nodnic_port_priv	*port_priv,
					IN mlx_size	cq_size,
					OUT nodnic_cq	**cq
					);

mlx_status
nodnic_port_destroy_cq(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_cq	*cq
					);

mlx_status
nodnic_port_create_qp(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_queue_pair_type	type,
					IN mlx_size	send_wq_size,
					IN mlx_uint32 send_wqe_num,
					IN mlx_size	receive_wq_size,
					IN mlx_uint32 recv_wqe_num,
					OUT nodnic_qp	**qp
					);

mlx_status
nodnic_port_destroy_qp(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_queue_pair_type	type,
					IN nodnic_qp	*qp
					);
mlx_status
nodnic_port_get_qpn(
			IN nodnic_port_priv	*port_priv,
			IN struct nodnic_ring *ring,
			OUT mlx_uint32 *qpn
			);
mlx_status
nodnic_port_update_ring_doorbell(
					IN nodnic_port_priv	*port_priv,
					IN struct nodnic_ring *ring,
					IN mlx_uint16 index
					);
mlx_status
nodnic_port_get_cq_size(
		IN nodnic_port_priv	*port_priv,
		OUT mlx_uint64 *cq_size
		);

mlx_status
nodnic_port_allocate_eq(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_uint8			log_eq_size
					);
mlx_status
nodnic_port_free_eq(
					IN  nodnic_port_priv	*port_priv
					);

mlx_status
nodnic_port_add_mac_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					);

mlx_status
nodnic_port_remove_mac_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					);
mlx_status
nodnic_port_add_mgid_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					);

mlx_status
nodnic_port_remove_mgid_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					);
mlx_status
nodnic_port_thin_init(
		IN nodnic_device_priv	*device_priv,
		IN nodnic_port_priv		*port_priv,
		IN mlx_uint8			port_index
		);

mlx_status
nodnic_port_set_promisc(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		);

mlx_status
nodnic_port_set_promisc_multicast(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		);

mlx_status
nodnic_port_init(
		IN nodnic_port_priv		*port_priv
		);

mlx_status
nodnic_port_close(
		IN nodnic_port_priv		*port_priv
		);

mlx_status
nodnic_port_enable_dma(
		IN nodnic_port_priv		*port_priv
		);

mlx_status
nodnic_port_disable_dma(
		IN nodnic_port_priv		*port_priv
		);

mlx_status
nodnic_port_read_reset_needed(
						IN nodnic_port_priv		*port_priv,
						OUT mlx_boolean			*reset_needed
						);

mlx_status
nodnic_port_read_port_management_change_event(
						IN nodnic_port_priv		*port_priv,
						OUT mlx_boolean			*change_event
						);
#endif /* STUB_NODNIC_PORT_H_ */
