/*
 * types.h
 *
 *  Created on: Jan 18, 2015
 *      Author: maord
 */

#ifndef A_MLXUTILS_INCLUDE_PUBLIC_TYPES_H_
#define A_MLXUTILS_INCLUDE_PUBLIC_TYPES_H_
#include <stdint.h>
//#include <errno.h>
#include <ipxe/pci.h>

#define MLX_SUCCESS 0
#define MLX_OUT_OF_RESOURCES (-1)
//(-ENOMEM)
#define MLX_INVALID_PARAMETER (-2)
//(-EINVAL)
#define MLX_UNSUPPORTED (-3)
//(-ENOSYS)
#define MLX_NOT_FOUND (-4)

#define MLX_FAILED (-5)

#undef TRUE
#define TRUE	1
#undef FALSE
#define FALSE	!TRUE

typedef int mlx_status;

typedef uint8_t		mlx_uint8;
typedef uint16_t	mlx_uint16;
typedef uint32_t	mlx_uint32;
typedef uint64_t	mlx_uint64;
typedef uint32_t 	mlx_uintn;

typedef int8_t		mlx_int8;
typedef int16_t		mlx_int16;;
typedef int32_t		mlx_int32;
typedef int64_t		mlx_int64;
typedef uint8_t		mlx_boolean;

typedef struct pci_device	mlx_pci;

typedef size_t		mlx_size;

typedef void		mlx_void;

#define MAC_ADDR_LEN 6
typedef unsigned long	mlx_physical_address;
typedef union {
	struct {
		uint32_t low;
		uint32_t high;
	} __attribute__ (( packed ));
	uint8_t addr[MAC_ADDR_LEN];
} mlx_mac_address;

#endif /* A_MLXUTILS_INCLUDE_PUBLIC_TYPES_H_ */
