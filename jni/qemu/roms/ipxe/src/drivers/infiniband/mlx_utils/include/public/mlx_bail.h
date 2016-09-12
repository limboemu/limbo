#ifndef INCLUDE_PUBLIC_MLXBAIL_H_
#define INCLUDE_PUBLIC_MLXBAIL_H_

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

#include "mlx_types.h"

#define MLX_BAIL_ERROR(id, status,message) MLX_CHECK_STATUS(id, status, bail, message)

#define MLX_FATAL_CHECK_STATUS(status, label, message)						\
        do {																	\
            if (status != MLX_SUCCESS) {										\
                MLX_DEBUG_FATAL_ERROR(message " (Status = %d)\n", status);	\
                goto label;														\
            }																	\
        } while (0)

#define MLX_CHECK_STATUS(id, status, label, message)					\
        do {															\
            if (status != MLX_SUCCESS) {								\
                MLX_DEBUG_ERROR(id, message " (Status = %d)\n", status);\
                goto label;												\
            }															\
        } while (0)



#endif /* INCLUDE_PUBLIC_MLXBAIL_H_ */
