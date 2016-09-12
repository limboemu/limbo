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

#include "../include/mlx_port.h"
#include "../include/mlx_cmd.h"
#include "../../mlx_utils/include/public/mlx_memory.h"
#include "../../mlx_utils/include/public/mlx_pci.h"
#include "../../mlx_utils/include/public/mlx_bail.h"

#define PortDataEntry( _option, _offset, _align, _mask) { \
  .option = _option,                     \
  .offset = _offset,                   \
  .align = _align,                  \
  .mask = _mask,                    \
  }

#define QpDataEntry( _type, _send_offset, _recv_offset) { \
  .type = _type,                     \
  .send_offset = _send_offset,                   \
  .recv_offset = _recv_offset,                  \
  }


struct nodnic_port_data_entry nodnic_port_data_table[] = {
		PortDataEntry(nodnic_port_option_link_type, 0x0, 4, 0x1),
		PortDataEntry(nodnic_port_option_mac_low, 0xc, 0, 0xFFFFFFFF),
		PortDataEntry(nodnic_port_option_mac_high, 0x8, 0, 0xFFFF),
		PortDataEntry(nodnic_port_option_log_cq_size, 0x6c, 0, 0x3F),
		PortDataEntry(nodnic_port_option_reset_needed, 0x0, 31, 0x1),
		PortDataEntry(nodnic_port_option_mac_filters_en, 0x4, 0, 0x1F),
		PortDataEntry(nodnic_port_option_port_state, 0x0, 0, 0xF),
		PortDataEntry(nodnic_port_option_network_en, 0x4, 31, 0x1),
		PortDataEntry(nodnic_port_option_dma_en, 0x4, 30, 0x1),
		PortDataEntry(nodnic_port_option_eq_addr_low, 0x74, 0, 0xFFFFFFFF),
		PortDataEntry(nodnic_port_option_eq_addr_high, 0x70, 0, 0xFFFFFFFF),
		PortDataEntry(nodnic_port_option_cq_addr_low, 0x6c, 12, 0xFFFFF),
		PortDataEntry(nodnic_port_option_cq_addr_high, 0x68, 0, 0xFFFFFFFF),
		PortDataEntry(nodnic_port_option_port_management_change_event, 0x0, 30, 0x1),
		PortDataEntry(nodnic_port_option_port_promisc_en, 0x4, 29, 0x1),
		PortDataEntry(nodnic_port_option_arm_cq, 0x78, 8, 0xffff),
		PortDataEntry(nodnic_port_option_port_promisc_multicast_en, 0x4, 28, 0x1),
#ifdef DEVICE_CX3
		PortDataEntry(nodnic_port_option_crspace_en, 0x4, 27, 0x1),
#endif
};

#define MAX_QP_DATA_ENTRIES 5
struct nodnic_qp_data_entry nodnic_qp_data_teable[MAX_QP_DATA_ENTRIES] = {
		QpDataEntry(NODNIC_QPT_SMI, 0, 0),
		QpDataEntry(NODNIC_QPT_GSI, 0, 0),
		QpDataEntry(NODNIC_QPT_UD, 0, 0),
		QpDataEntry(NODNIC_QPT_RC, 0, 0),
		QpDataEntry(NODNIC_QPT_ETH, 0x80, 0xC0),
};

#define MAX_NODNIC_PORTS 2
int nodnic_port_offset_table[MAX_NODNIC_PORTS] = {
	0x100, //port 1 offset
	0x280, //port 1 offset
};

mlx_status
nodnic_port_get_state(
					IN  nodnic_port_priv	*port_priv,
					OUT nodnic_port_state			*state
					)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 out = 0;

	status = nodnic_port_query(port_priv,
			nodnic_port_option_port_state, &out);
	MLX_CHECK_STATUS(port_priv->device, status, query_err,
			"nodnic_port_query failed");
	*state = (nodnic_port_state)out;
query_err:
	return status;
}
mlx_status
nodnic_port_get_type(
					IN  nodnic_port_priv	*port_priv,
					OUT nodnic_port_type	*type
					)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 out = 0;

	if ( port_priv->port_type == NODNIC_PORT_TYPE_UNKNOWN){
		status = nodnic_port_query(port_priv,
				nodnic_port_option_link_type, &out);
		MLX_FATAL_CHECK_STATUS(status, query_err,
				"nodnic_port_query failed");
		port_priv->port_type = (nodnic_port_type)out;
	}
	*type = port_priv->port_type;
query_err:
	return status;
}

mlx_status
nodnic_port_query(
					IN  nodnic_port_priv	*port_priv,
					IN  nodnic_port_option		option,
					OUT	mlx_uint32				*out
					)
{
	mlx_status 				status = MLX_SUCCESS;
	nodnic_device_priv		*device_priv = NULL;
	struct nodnic_port_data_entry *data_entry;
	mlx_uint32				buffer = 0;
	if( port_priv == NULL || out == NULL){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}
	device_priv = port_priv->device;

	data_entry = &nodnic_port_data_table[option];

	status = nodnic_cmd_read(device_priv,
			port_priv->port_offset + data_entry->offset , &buffer);
	MLX_CHECK_STATUS(device_priv, status, read_err,
			"nodnic_cmd_read failed");
	*out = (buffer >> data_entry->align) & data_entry->mask;
read_err:
invalid_parm:
	return status;
}

mlx_status
nodnic_port_set(
					IN  nodnic_port_priv	*port_priv,
					IN  nodnic_port_option		option,
					IN	mlx_uint32				in
					)
{
	mlx_status 				status = MLX_SUCCESS;
	nodnic_device_priv		*device_priv = NULL;
	struct nodnic_port_data_entry *data_entry;
	mlx_uint32				buffer = 0;

	if( port_priv == NULL ){
		MLX_DEBUG_FATAL_ERROR("port_priv is NULL\n");
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}
	device_priv = port_priv->device;
	data_entry = &nodnic_port_data_table[option];

	if( in > data_entry->mask ){
		MLX_DEBUG_FATAL_ERROR("in > data_entry->mask (%d > %d)\n",
				in, data_entry->mask);
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}
	status = nodnic_cmd_read(device_priv,
			port_priv->port_offset + data_entry->offset, &buffer);
	MLX_FATAL_CHECK_STATUS(status, read_err,
			"nodnic_cmd_read failed");
	buffer = buffer & ~(data_entry->mask << data_entry->align);
	buffer = buffer | (in << data_entry->align);
	status = nodnic_cmd_write(device_priv,
			port_priv->port_offset + data_entry->offset, buffer);
	MLX_FATAL_CHECK_STATUS(status, write_err,
			"nodnic_cmd_write failed");
write_err:
read_err:
invalid_parm:
	return status;
}

mlx_status
nodnic_port_read_reset_needed(
						IN nodnic_port_priv		*port_priv,
						OUT mlx_boolean			*reset_needed
						)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 out = 0;
	status = nodnic_port_query(port_priv,
			nodnic_port_option_reset_needed, &out);
	MLX_CHECK_STATUS(port_priv->device, status, query_err,
			"nodnic_port_query failed");
	*reset_needed = (mlx_boolean)out;
query_err:
	return status;
}

mlx_status
nodnic_port_read_port_management_change_event(
						IN nodnic_port_priv		*port_priv,
						OUT mlx_boolean			*change_event
						)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 out = 0;
	status = nodnic_port_query(port_priv,
			nodnic_port_option_port_management_change_event, &out);
	MLX_CHECK_STATUS(port_priv->device, status, query_err,
			"nodnic_port_query failed");
	*change_event = (mlx_boolean)out;
query_err:
	return status;
}

mlx_status
nodnic_port_create_cq(
					IN nodnic_port_priv	*port_priv,
					IN mlx_size	cq_size,
					OUT nodnic_cq	**cq
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = NULL;
	mlx_uint64 address = 0;
	if( port_priv == NULL || cq == NULL){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}

	device_priv =  port_priv->device;

	status = mlx_memory_zalloc(device_priv->utils,
				sizeof(nodnic_cq),(mlx_void **)cq);
	MLX_FATAL_CHECK_STATUS(status, alloc_err,
			"cq priv allocation error");

	(*cq)->cq_size = cq_size;
	status = mlx_memory_alloc_dma(device_priv->utils,
			(*cq)->cq_size, NODNIC_MEMORY_ALIGN,
				&(*cq)->cq_virt);
	MLX_FATAL_CHECK_STATUS(status, dma_alloc_err,
				"cq allocation error");

	status = mlx_memory_map_dma(device_priv->utils,
						(*cq)->cq_virt,
						(*cq)->cq_size,
						&(*cq)->cq_physical,
						&(*cq)->map);
	MLX_FATAL_CHECK_STATUS(status, cq_map_err,
				"cq map error");

	/* update cq address */
#define NODIC_CQ_ADDR_HIGH 0x68
#define NODIC_CQ_ADDR_LOW 0x6c
	address = (mlx_uint64)(*cq)->cq_physical;
	nodnic_port_set(port_priv, nodnic_port_option_cq_addr_low,
			(mlx_uint32)(address >> 12));
	address = address >> 32;
	nodnic_port_set(port_priv, nodnic_port_option_cq_addr_high,
				(mlx_uint32)address);

	return status;
	mlx_memory_ummap_dma(device_priv->utils, (*cq)->map);
cq_map_err:
	mlx_memory_free_dma(device_priv->utils, (*cq)->cq_size,
			(void **)&((*cq)->cq_virt));
dma_alloc_err:
	mlx_memory_free(device_priv->utils, (void **)cq);
alloc_err:
invalid_parm:
	return status;
}

mlx_status
nodnic_port_destroy_cq(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_cq	*cq
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = NULL;

	if( port_priv == NULL || cq == NULL){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}
	device_priv =  port_priv->device;

	mlx_memory_ummap_dma(device_priv->utils, cq->map);

	mlx_memory_free_dma(device_priv->utils, cq->cq_size,
			(void **)&(cq->cq_virt));

	mlx_memory_free(device_priv->utils, (void **)&cq);
invalid_parm:
	return status;
}
mlx_status
nodnic_port_create_qp(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_queue_pair_type	type,
					IN mlx_size	send_wq_size,
					IN mlx_uint32 send_wqe_num,
					IN mlx_size	receive_wq_size,
					IN mlx_uint32 recv_wqe_num,
					OUT nodnic_qp	**qp
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = NULL;
	mlx_uint32 max_ring_size = 0;
	mlx_uint64 address = 0;
	mlx_uint32 log_size = 0;
	if( port_priv == NULL || qp == NULL){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}

	device_priv =  port_priv->device;
	max_ring_size = (1 << device_priv->device_cap.log_max_ring_size);
	if( send_wq_size > max_ring_size ||
			receive_wq_size > max_ring_size ){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}

	status = mlx_memory_zalloc(device_priv->utils,
			sizeof(nodnic_qp),(mlx_void **)qp);
	MLX_FATAL_CHECK_STATUS(status, alloc_err,
			"qp allocation error");

	if( nodnic_qp_data_teable[type].send_offset == 0 ||
			nodnic_qp_data_teable[type].recv_offset == 0){
		status = MLX_INVALID_PARAMETER;
		goto invalid_type;
	}

	(*qp)->send.nodnic_ring.offset = port_priv->port_offset +
				nodnic_qp_data_teable[type].send_offset;
	(*qp)->receive.nodnic_ring.offset = port_priv->port_offset +
			nodnic_qp_data_teable[type].recv_offset;

	status = mlx_memory_alloc_dma(device_priv->utils,
			send_wq_size, NODNIC_MEMORY_ALIGN,
			(void*)&(*qp)->send.wqe_virt);
	MLX_FATAL_CHECK_STATUS(status, send_alloc_err,
				"send wq allocation error");

	status = mlx_memory_alloc_dma(device_priv->utils,
				receive_wq_size, NODNIC_MEMORY_ALIGN,
				&(*qp)->receive.wqe_virt);
	MLX_FATAL_CHECK_STATUS(status, receive_alloc_err,
				"receive wq allocation error");

	status = mlx_memory_map_dma(device_priv->utils,
						(*qp)->send.wqe_virt,
						send_wq_size,
						&(*qp)->send.nodnic_ring.wqe_physical,
						&(*qp)->send.nodnic_ring.map);
	MLX_FATAL_CHECK_STATUS(status, send_map_err,
				"send wq map error");

	status = mlx_memory_map_dma(device_priv->utils,
						(*qp)->receive.wqe_virt,
						receive_wq_size,
						&(*qp)->receive.nodnic_ring.wqe_physical,
						&(*qp)->receive.nodnic_ring.map);
	MLX_FATAL_CHECK_STATUS(status, receive_map_err,
				"receive wq map error");

	(*qp)->send.nodnic_ring.wq_size = send_wq_size;
	(*qp)->send.nodnic_ring.num_wqes = send_wqe_num;
	(*qp)->receive.nodnic_ring.wq_size = receive_wq_size;
	(*qp)->receive.nodnic_ring.num_wqes = recv_wqe_num;

	/* Set Ownership bit in Send/receive queue (0 - recv ; 1 - send) */
	mlx_memory_set(device_priv->utils, (*qp)->send.wqe_virt, 0xff, send_wq_size );
	mlx_memory_set(device_priv->utils, (*qp)->receive.wqe_virt, 0, recv_wqe_num );

	/* update send ring */
#define NODIC_RING_QP_ADDR_HIGH 0x0
#define NODIC_RING_QP_ADDR_LOW 0x4
	address = (mlx_uint64)(*qp)->send.nodnic_ring.wqe_physical;
	status = nodnic_cmd_write(device_priv, (*qp)->send.nodnic_ring.offset +
			NODIC_RING_QP_ADDR_HIGH,
			(mlx_uint32)(address >> 32));
	MLX_FATAL_CHECK_STATUS(status, write_send_addr_err,
					"send address write error 1");
	mlx_utils_ilog2((*qp)->send.nodnic_ring.wq_size, &log_size);
	address = address | log_size;
	status = nodnic_cmd_write(device_priv, (*qp)->send.nodnic_ring.offset +
			NODIC_RING_QP_ADDR_LOW,
				(mlx_uint32)address);
	MLX_FATAL_CHECK_STATUS(status, write_send_addr_err,
						"send address write error 2");
	/* update receive ring */
	address = (mlx_uint64)(*qp)->receive.nodnic_ring.wqe_physical;
	status = nodnic_cmd_write(device_priv, (*qp)->receive.nodnic_ring.offset +
			NODIC_RING_QP_ADDR_HIGH,
			(mlx_uint32)(address >> 32));
	MLX_FATAL_CHECK_STATUS(status, write_recv_addr_err,
						"receive address write error 1");
	mlx_utils_ilog2((*qp)->receive.nodnic_ring.wq_size, &log_size);
	address = address | log_size;
	status = nodnic_cmd_write(device_priv, (*qp)->receive.nodnic_ring.offset +
			NODIC_RING_QP_ADDR_LOW,
				(mlx_uint32)address);
	MLX_FATAL_CHECK_STATUS(status, write_recv_addr_err,
						"receive address write error 2");

	return status;
write_recv_addr_err:
write_send_addr_err:
	mlx_memory_ummap_dma(device_priv->utils, (*qp)->receive.nodnic_ring.map);
receive_map_err:
	mlx_memory_ummap_dma(device_priv->utils, (*qp)->send.nodnic_ring.map);
send_map_err:
	mlx_memory_free_dma(device_priv->utils, receive_wq_size,
			&((*qp)->receive.wqe_virt));
receive_alloc_err:
	mlx_memory_free_dma(device_priv->utils, send_wq_size,
			(void **)&((*qp)->send.wqe_virt));
send_alloc_err:
invalid_type:
	mlx_memory_free(device_priv->utils, (void **)qp);
alloc_err:
invalid_parm:
	return status;
}

mlx_status
nodnic_port_destroy_qp(
					IN nodnic_port_priv	*port_priv,
					IN nodnic_queue_pair_type	type __attribute__((unused)),
					IN nodnic_qp	*qp
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = port_priv->device;

	status = mlx_memory_ummap_dma(device_priv->utils,
			qp->receive.nodnic_ring.map);
	if( status != MLX_SUCCESS){
		MLX_DEBUG_ERROR(device_priv, "mlx_memory_ummap_dma failed (Status = %d)\n", status);
	}

	status = mlx_memory_ummap_dma(device_priv->utils, qp->send.nodnic_ring.map);
	if( status != MLX_SUCCESS){
		MLX_DEBUG_ERROR(device_priv, "mlx_memory_ummap_dma failed (Status = %d)\n", status);
	}

	status = mlx_memory_free_dma(device_priv->utils,
			qp->receive.nodnic_ring.wq_size,
			(void **)&(qp->receive.wqe_virt));
	if( status != MLX_SUCCESS){
		MLX_DEBUG_ERROR(device_priv, "mlx_memory_free_dma failed (Status = %d)\n", status);
	}
	status = mlx_memory_free_dma(device_priv->utils,
			qp->send.nodnic_ring.wq_size,
			(void **)&(qp->send.wqe_virt));
	if( status != MLX_SUCCESS){
		MLX_DEBUG_ERROR(device_priv, "mlx_memory_free_dma failed (Status = %d)\n", status);
	}
	status = mlx_memory_free(device_priv->utils, (void **)&qp);
	if( status != MLX_SUCCESS){
		MLX_DEBUG_ERROR(device_priv, "mlx_memory_free failed (Status = %d)\n", status);
	}
	return status;
}

mlx_status
nodnic_port_get_qpn(
			IN nodnic_port_priv	*port_priv,
			IN struct nodnic_ring  *ring,
			OUT mlx_uint32 *qpn
			)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 buffer = 0;
	if( ring == NULL || qpn == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	if( ring->qpn != 0 ){
		*qpn = ring->qpn;
		goto success;
	}
#define NODNIC_RING_QPN_OFFSET 0xc
#define NODNIC_RING_QPN_MASK 0xFFFFFF
	status = nodnic_cmd_read(port_priv->device,
			ring->offset + NODNIC_RING_QPN_OFFSET,
			&buffer);
	MLX_FATAL_CHECK_STATUS(status, read_err,
			"nodnic_cmd_read failed");
	ring->qpn = buffer & NODNIC_RING_QPN_MASK;
	*qpn = ring->qpn;
read_err:
success:
bad_param:
	return status;
}

#ifdef DEVICE_CX3
static
mlx_status
nodnic_port_send_db_connectx3(
		IN nodnic_port_priv	*port_priv,
		IN struct nodnic_ring *ring __attribute__((unused)),
		IN mlx_uint16 index
		)
{
	nodnic_port_data_flow_gw *ptr = port_priv->data_flow_gw;
	mlx_uint32 index32 = index;
	mlx_pci_mem_write(port_priv->device->utils, MlxPciWidthUint32, 0,
			(mlx_uint64)&(ptr->send_doorbell), 1, &index32);
	return MLX_SUCCESS;
}

static
mlx_status
nodnic_port_recv_db_connectx3(
		IN nodnic_port_priv	*port_priv,
		IN struct nodnic_ring *ring __attribute__((unused)),
		IN mlx_uint16 index
		)
{
	nodnic_port_data_flow_gw *ptr = port_priv->data_flow_gw;
	mlx_uint32 index32 = index;
	mlx_pci_mem_write(port_priv->device->utils, MlxPciWidthUint32, 0,
			(mlx_uint64)&(ptr->recv_doorbell), 1, &index32);
	return MLX_SUCCESS;
}
#endif

mlx_status
nodnic_port_update_ring_doorbell(
					IN nodnic_port_priv	*port_priv,
					IN struct nodnic_ring *ring,
					IN mlx_uint16 index
					)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 buffer = 0;
	if( ring == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
#define NODNIC_RING_RING_OFFSET 0x8
	buffer = (mlx_uint32)((index & 0xFFFF)<< 8);
	status = nodnic_cmd_write(port_priv->device,
				ring->offset + NODNIC_RING_RING_OFFSET,
				buffer);
	MLX_CHECK_STATUS(port_priv->device, status, write_err,
				"nodnic_cmd_write failed");
write_err:
bad_param:
	return status;
}

mlx_status
nodnic_port_get_cq_size(
		IN nodnic_port_priv	*port_priv,
		OUT mlx_uint64 *cq_size
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 out = 0;
	status = nodnic_port_query(port_priv, nodnic_port_option_log_cq_size, &out);
	MLX_FATAL_CHECK_STATUS(status, query_err,
			"nodnic_port_query failed");
	*cq_size = 1 << out;
query_err:
	return status;
}

mlx_status
nodnic_port_allocate_eq(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_uint8			log_eq_size
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = NULL;
	mlx_uint64 address = 0;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	device_priv = port_priv->device;
	port_priv->eq.eq_size = ( ( 1 << log_eq_size ) * 1024 ); /* Size is in KB */
	status = mlx_memory_alloc_dma(device_priv->utils,
								port_priv->eq.eq_size,
								NODNIC_MEMORY_ALIGN,
								&port_priv->eq.eq_virt);
	MLX_FATAL_CHECK_STATUS(status, alloc_err,
							"eq allocation error");

	status = mlx_memory_map_dma(device_priv->utils,
							port_priv->eq.eq_virt,
							port_priv->eq.eq_size,
							&port_priv->eq.eq_physical,
							&port_priv->eq.map);
	MLX_FATAL_CHECK_STATUS(status, map_err,
								"eq map error");

	address = port_priv->eq.eq_physical;
	status = nodnic_port_set(port_priv, nodnic_port_option_eq_addr_low,
						(mlx_uint32)address);
	MLX_FATAL_CHECK_STATUS(status, set_err,
			"failed to set eq addr low");
	address = (address >> 32);
	status = nodnic_port_set(port_priv, nodnic_port_option_eq_addr_high,
						(mlx_uint32)address);
	MLX_FATAL_CHECK_STATUS(status, set_err,
				"failed to set eq addr high");
	return status;
set_err:
	mlx_memory_ummap_dma(device_priv->utils, port_priv->eq.map);
map_err:
	mlx_memory_free_dma(device_priv->utils,
			port_priv->eq.eq_size,
			(void **)&(port_priv->eq.eq_virt));
alloc_err:
bad_param:
	return status;
}
mlx_status
nodnic_port_free_eq(
					IN  nodnic_port_priv	*port_priv
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = NULL;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	device_priv = port_priv->device;
	mlx_memory_ummap_dma(device_priv->utils, port_priv->eq.map);

	mlx_memory_free_dma(device_priv->utils,
			port_priv->eq.eq_size,
			(void **)&(port_priv->eq.eq_virt));

bad_param:
	return status;
}

mlx_status
nodnic_port_add_mac_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device= NULL;;
	mlx_uint8 index = 0;
	mlx_uint32 out = 0;
	mlx_uint32 mac_filters_en = 0;
	mlx_uint32 address = 0;
	mlx_mac_address zero_mac;
	mlx_utils *utils = NULL;

	if( port_priv == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	memset(&zero_mac, 0, sizeof(zero_mac));

	device = port_priv->device;
	utils = device->utils;

	/* check if mac already exists */
	for( ; index < NODNIC_MAX_MAC_FILTERS ; index ++) {
		mlx_memory_cmp(utils, &port_priv->mac_filters[index], &mac,
				sizeof(mac), &out);
		if ( out == 0 ){
			status = MLX_FAILED;
			goto already_exists;
		}
	}

	/* serch for available mac filter slot */
	for (index = 0 ; index < NODNIC_MAX_MAC_FILTERS ; index ++) {
		mlx_memory_cmp(utils, &port_priv->mac_filters[index], &zero_mac,
				sizeof(zero_mac), &out);
		if ( out == 0 ){
			break;
		}
	}
	if ( index >= NODNIC_MAX_MAC_FILTERS ){
		status = MLX_FAILED;
		goto mac_list_full;
	}

	status = nodnic_port_query(port_priv, nodnic_port_option_mac_filters_en,
			&mac_filters_en);
	MLX_CHECK_STATUS(device, status , query_err,
			"nodnic_port_query failed");
	if(mac_filters_en & (1 << index)){
		status = MLX_FAILED;
		goto mac_list_full;
	}
	port_priv->mac_filters[index] = mac;

	// set mac filter
	address = port_priv->port_offset + NODNIC_PORT_MAC_FILTERS_OFFSET +
				(0x8 * index);

	status = nodnic_cmd_write(device, address, mac.high );
	MLX_CHECK_STATUS(device, status, write_err,	"set mac high failed");
	status = nodnic_cmd_write(device, address + 0x4, mac.low );
	MLX_CHECK_STATUS(device, status, write_err, "set mac low failed");

	// enable mac filter
	mac_filters_en = mac_filters_en | (1 << index);
	status = nodnic_port_set(port_priv, nodnic_port_option_mac_filters_en,
				mac_filters_en);
	MLX_CHECK_STATUS(device, status , set_err,
			"nodnic_port_set failed");
set_err:
write_err:
query_err:
mac_list_full:
already_exists:
bad_param:
	return status;
}

mlx_status
nodnic_port_remove_mac_filter(
					IN  nodnic_port_priv	*port_priv,
					IN  mlx_mac_address 	mac
					)
{
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device= NULL;;
	mlx_uint8 index = 0;
	mlx_uint32 out = 0;
	mlx_uint32 mac_filters_en = 0;
	mlx_mac_address zero_mac;
	mlx_utils *utils = NULL;

	if( port_priv == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	memset(&zero_mac, 0, sizeof(zero_mac));

	device = port_priv->device;
	utils = device->utils;

	/* serch for mac filter */
	for( ; index < NODNIC_MAX_MAC_FILTERS ; index ++) {
		mlx_memory_cmp(utils, &port_priv->mac_filters[index], &mac,
				sizeof(mac), &out);
		if ( out == 0 ){
			break;
		}
	}
	if ( index == NODNIC_MAX_MAC_FILTERS ){
		status = MLX_FAILED;
		goto mac_not_found;
	}

	status = nodnic_port_query(port_priv, nodnic_port_option_mac_filters_en,
			&mac_filters_en);
	MLX_CHECK_STATUS(device, status , query_err,
			"nodnic_port_query failed");
	if((mac_filters_en & (1 << index)) == 0){
		status = MLX_FAILED;
		goto mac_not_en;
	}
	port_priv->mac_filters[index] = zero_mac;

	// disable mac filter
	mac_filters_en = mac_filters_en & ~(1 << index);
	status = nodnic_port_set(port_priv, nodnic_port_option_mac_filters_en,
				mac_filters_en);
	MLX_CHECK_STATUS(device, status , set_err,
			"nodnic_port_set failed");
set_err:
query_err:
mac_not_en:
mac_not_found:
bad_param:
	return status;
}

static
mlx_status
nodnic_port_set_network(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		)
{
	mlx_status status = MLX_SUCCESS;
	/*mlx_uint32 network_valid = 0;
	mlx_uint8 try = 0;*/

	status = nodnic_port_set(port_priv, nodnic_port_option_network_en, value);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
			"nodnic_port_set failed");
	port_priv->network_state = value;
set_err:
	return status;
}

#ifdef DEVICE_CX3
static
mlx_status
nodnic_port_set_dma_connectx3(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		)
{
	mlx_utils *utils = port_priv->device->utils;
	nodnic_port_data_flow_gw *ptr = port_priv->data_flow_gw;
	mlx_uint32 data = (value ? 0xffffffff : 0x0);
	mlx_pci_mem_write(utils, MlxPciWidthUint32, 0,
			(mlx_uint64)&(ptr->dma_en), 1, &data);
	return MLX_SUCCESS;
}
#endif

static
mlx_status
nodnic_port_set_dma(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		)
{
	return nodnic_port_set(port_priv, nodnic_port_option_dma_en, value);
}

static
mlx_status
nodnic_port_check_and_set_dma(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		)
{
	mlx_status status = MLX_SUCCESS;
	if ( port_priv->dma_state == value ) {
		MLX_DEBUG_WARN(port_priv->device,
				"nodnic_port_check_and_set_dma: already %s\n",
				(value ? "enabled" : "disabled"));
		status = MLX_SUCCESS;
		goto set_out;
	}

	status = port_priv->set_dma(port_priv, value);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
			"nodnic_port_set failed");
	port_priv->dma_state = value;
set_err:
set_out:
	return status;
}


mlx_status
nodnic_port_set_promisc(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		){
	mlx_status status = MLX_SUCCESS;
	mlx_uint32	buffer = value;

	status = nodnic_port_set(port_priv, nodnic_port_option_port_promisc_en, buffer);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
			"nodnic_port_set failed");
set_err:
	return status;
}

mlx_status
nodnic_port_set_promisc_multicast(
		IN nodnic_port_priv		*port_priv,
		IN mlx_boolean			value
		){
	mlx_status status = MLX_SUCCESS;
	mlx_uint32	buffer = value;

	status = nodnic_port_set(port_priv, nodnic_port_option_port_promisc_multicast_en, buffer);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
			"nodnic_port_set failed");
set_err:
	return status;
}

mlx_status
nodnic_port_init(
		IN nodnic_port_priv		*port_priv
		)
{
	mlx_status status = MLX_SUCCESS;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nodnic_port_set_network(port_priv, TRUE);
	MLX_FATAL_CHECK_STATUS(status, set_err,
					"nodnic_port_set_network failed");
set_err:
bad_param:
	return status;
}

mlx_status
nodnic_port_close(
		IN nodnic_port_priv		*port_priv
		)
{
	mlx_status status = MLX_SUCCESS;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nodnic_port_set_network(port_priv, FALSE);
	MLX_FATAL_CHECK_STATUS(status, set_err,
					"nodnic_port_set_network failed");
set_err:
bad_param:
	return status;
}

mlx_status
nodnic_port_enable_dma(
		IN nodnic_port_priv		*port_priv
		)
{
	mlx_status status = MLX_SUCCESS;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nodnic_port_check_and_set_dma(port_priv, TRUE);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
					"nodnic_port_check_and_set_dma failed");
set_err:
bad_param:
	return status;
}

mlx_status
nodnic_port_disable_dma(
		IN nodnic_port_priv		*port_priv
		)
{
	mlx_status status = MLX_SUCCESS;

	if( port_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nodnic_port_check_and_set_dma(port_priv, FALSE);
	MLX_CHECK_STATUS(port_priv->device, status, set_err,
					"nodnic_port_check_and_set_dma failed");
set_err:
bad_param:
	return status;
}

mlx_status
nodnic_port_thin_init(
		IN nodnic_device_priv	*device_priv,
		IN nodnic_port_priv		*port_priv,
		IN mlx_uint8			port_index
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_boolean	reset_needed = 0;
#ifdef DEVICE_CX3
	mlx_uint32 offset;
#endif

	if( device_priv == NULL || port_priv == NULL || port_index > 1){
		status = MLX_INVALID_PARAMETER;
		goto invalid_parm;
	}

	port_priv->device = device_priv;

	port_priv->port_offset = device_priv->device_offset +
			nodnic_port_offset_table[port_index];

	port_priv->port_num = port_index + 1;

	port_priv->send_doorbell = nodnic_port_update_ring_doorbell;
	port_priv->recv_doorbell = nodnic_port_update_ring_doorbell;
	port_priv->set_dma = nodnic_port_set_dma;
#ifdef DEVICE_CX3
	if (device_priv->device_cap.crspace_doorbells) {
		status = nodnic_cmd_read(device_priv, (port_priv->port_offset + 0x100),
				&offset);
		if (status != MLX_SUCCESS) {
			return status;
		} else {
			port_priv->data_flow_gw = (nodnic_port_data_flow_gw *)
					(device_priv->utils->config + offset);
		}
		if ( nodnic_port_set ( port_priv, nodnic_port_option_crspace_en, 1 ) ) {
			return MLX_FAILED;
		}
		port_priv->send_doorbell = nodnic_port_send_db_connectx3;
		port_priv->recv_doorbell = nodnic_port_recv_db_connectx3;
		port_priv->set_dma = nodnic_port_set_dma_connectx3;
	}
#endif
	/* clear reset_needed */
	nodnic_port_read_reset_needed(port_priv, &reset_needed);

	port_priv->port_type = NODNIC_PORT_TYPE_UNKNOWN;
invalid_parm:
	return status;
}
