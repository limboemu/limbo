/*
 * DebugPriv.h
 *
 *  Created on: Jan 19, 2015
 *      Author: maord
 */

#ifndef STUB_MLXUTILS_INCLUDE_PRIVATE_FLEXBOOT_DEBUG_H_
#define STUB_MLXUTILS_INCLUDE_PRIVATE_FLEXBOOT_DEBUG_H_

#include <stdio.h>
#include <compiler.h>

#define MLX_DEBUG_FATAL_ERROR_PRIVATE(...)		do {				\
		DBG("%s: ",__func__);						\
		DBG(__VA_ARGS__);			\
	} while ( 0 )

#define MLX_DEBUG_ERROR_PRIVATE(id, ...)		do {				\
		DBGC(id, "%s: ",__func__);			\
		DBGC(id, __VA_ARGS__);			\
	} while ( 0 )

#define MLX_DEBUG_WARN_PRIVATE(id, ...)		do {				\
		DBGC(id, "%s: ",__func__);			\
		DBGC(id, __VA_ARGS__);			\
	} while ( 0 )

#define MLX_DEBUG_INFO1_PRIVATE(id, ...)		do {				\
		DBGC(id, "%s: ",__func__);			\
		DBGC(id, __VA_ARGS__);			\
	} while ( 0 )

#define MLX_DEBUG_INFO2_PRIVATE(id, ...)		do {				\
		DBGC2(id, "%s: ",__func__);			\
		DBGC2(id, __VA_ARGS__);			\
	} while ( 0 )

#define MLX_DBG_ERROR_PRIVATE(...)		do {				\
		DBG("%s: ",__func__);			\
		DBG(__VA_ARGS__);			\
	} while ( 0 )

#define MLX_DBG_WARN_PRIVATE(...)		do {				\
		DBG("%s: ",__func__);			\
		DBG(__VA_ARGS__);			\
	} while ( 0 )

#define MLX_DBG_INFO1_PRIVATE(...)		do {				\
		DBG("%s: ",__func__);			\
		DBG(__VA_ARGS__);			\
	} while ( 0 )

#define MLX_DBG_INFO2_PRIVATE(...)		do {				\
		DBG2("%s: ",__func__);			\
		DBG2(__VA_ARGS__);			\
	} while ( 0 )



#endif /* STUB_MLXUTILS_INCLUDE_PRIVATE_FLEXBOOT_DEBUG_H_ */
