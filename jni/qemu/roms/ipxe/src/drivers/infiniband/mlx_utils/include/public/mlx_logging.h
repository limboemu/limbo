#ifndef PUBLIC_INCLUDE_MLX_LOGGER_H_
#define PUBLIC_INCLUDE_MLX_LOGGER_H_

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

#include "../../../mlx_utils_flexboot/include/mlx_logging_priv.h"

#define MLX_DEBUG_FATAL_ERROR(...)	MLX_DEBUG_FATAL_ERROR_PRIVATE(__VA_ARGS__)
#define MLX_DEBUG_ERROR(...)		MLX_DEBUG_ERROR_PRIVATE(__VA_ARGS__)
#define MLX_DEBUG_WARN(...)			MLX_DEBUG_WARN_PRIVATE(__VA_ARGS__)
#define MLX_DEBUG_INFO1(...)		MLX_DEBUG_INFO1_PRIVATE(__VA_ARGS__)
#define MLX_DEBUG_INFO2(...)		MLX_DEBUG_INFO2_PRIVATE(__VA_ARGS__)
#define MLX_DBG_ERROR(...)			MLX_DBG_ERROR_PRIVATE(__VA_ARGS__)
#define MLX_DBG_WARN(...)			MLX_DBG_WARN_PRIVATE(__VA_ARGS__)
#define MLX_DBG_INFO1(...)			MLX_DBG_INFO1_PRIVATE(__VA_ARGS__)
#define MLX_DBG_INFO2(...)			MLX_DBG_INFO2_PRIVATE(__VA_ARGS__)

#define MLX_TRACE_1_START()				MLX_DBG_INFO1_PRIVATE("Start\n")
#define MLX_TRACE_1_END()				MLX_DBG_INFO1_PRIVATE("End\n")
#define MLX_TRACE_1_END_STATUS(status)	MLX_DBG_INFO1_PRIVATE("End (%s=%d)\n", #status,status)
#define MLX_TRACE_2_START()				MLX_DBG_INFO2_PRIVATE("Start\n")
#define MLX_TRACE_2_END()				MLX_DBG_INFO2_PRIVATE("End\n")
#define MLX_TRACE_2_END_STATUS(status)	MLX_DBG_INFO2_PRIVATE("End (%s=%d)\n", #status,status)



#endif /* PUBLIC_INCLUDE_MLX_LOGGER_H_ */
