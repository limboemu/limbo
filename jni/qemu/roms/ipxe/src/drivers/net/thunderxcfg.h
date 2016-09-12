#ifndef _THUNDERXCFG_H
#define _THUNDERXCFG_H

/** @file
 *
 * Cavium ThunderX Board Configuration
 *
 * The definitions in this section are extracted from BSD-licensed
 * (but non-public) portions of ThunderPkg.
 *
 */

FILE_LICENCE ( BSD2 );

#include <ipxe/efi/efi.h>

/******************************************************************************
 *
 * From ThunderxBoardConfig.h
 *
 ******************************************************************************
 *
 *  Header file for Cavium ThunderX Board Configurations
 *  Copyright (c) 2015, Cavium Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 */

#define MAX_NODES		2
#define CLUSTER_COUNT		3
#define CORE_PER_CLUSTER_COUNT  16
#define CORE_COUNT		(CLUSTER_COUNT*CORE_PER_CLUSTER_COUNT)
#define BGX_PER_NODE_COUNT	2
#define LMAC_PER_BGX_COUNT	4
#define PEM_PER_NODE_COUNT	6
#define LMC_PER_NODE_COUNT	4
#define DIMM_PER_LMC_COUNT	2

#define THUNDERX_CPU_ID(node, cluster, core) (((node) << 16) | ((cluster) << 8) | (core))

//TODO: Put common type definitions in separate common include file
typedef enum {
    BGX_MODE_SGMII, /* 1 lane, 1.250 Gbaud */
    BGX_MODE_XAUI,  /* 4 lanes, 3.125 Gbaud */
    BGX_MODE_DXAUI, /* 4 lanes, 6.250 Gbaud */
    BGX_MODE_RXAUI, /* 2 lanes, 6.250 Gbaud */
    BGX_MODE_XFI,   /* 1 lane, 10.3125 Gbaud */
    BGX_MODE_XLAUI, /* 4 lanes, 10.3125 Gbaud */
    BGX_MODE_10G_KR,/* 1 lane, 10.3125 Gbaud */
    BGX_MODE_40G_KR,/* 4 lanes, 10.3125 Gbaud */
    BGX_MODE_UNKNOWN
} BGX_MODE_T;

typedef enum {
    EBB8800,
    EBB8804,
    CRB_1S,
    CRB_2S,
    ASIANCAT,
    GBT_MT60,
    INVENTEC_P3E003,
    BOARD_MAX
} BOARD_TYPE;

typedef struct {
    BOOLEAN Enabled;
    UINT64  LaneToSds;
    UINT64  MacAddress;
} LMAC_CFG;

typedef struct {
    BOOLEAN BgxEnabled;
    BGX_MODE_T BgxMode;
    UINTN    LmacCount; //Maximum number of LMAcs
    UINT64   BaseAddress;
    UINT64   LmacType;
    /* Bit mask of QLMs connected to this BGX */
    UINT64   QlmMask;
    UINT64   QlmFreq;
    BOOLEAN  UseTraining;
    LMAC_CFG Lmacs[LMAC_PER_BGX_COUNT];
} BGX_CFG;

typedef struct {
    BOOLEAN PemUsable;
} PEM_CFG;

typedef struct {
    enum { NotPresent, Empty, Available } Status;
    UINT8 Type;
    UINT8 SubType;
    UINT8 Rank;
    UINT16 Mfg;
    UINTN SizeMb;
    UINTN Speed;
    CHAR8 Serial[16];
    CHAR8 PartNo[24];
} DIMM_CFG;

typedef struct {
    DIMM_CFG DimmCfg[DIMM_PER_LMC_COUNT];
} LMC_CFG;

typedef struct {
    BOOLEAN Core[CORE_COUNT];
    BGX_CFG BgxCfg[BGX_PER_NODE_COUNT];
    PEM_CFG PemCfg[PEM_PER_NODE_COUNT];
    LMC_CFG LmcCfg[LMC_PER_NODE_COUNT];
    UINT64 RamStart;
    UINT64 RamReserve;
    UINT64 RamSize;
    UINTN CPUSpeed;
    UINTN CPUVersion;
} NODE_CFG;

#define MAX_SERIAL 32
#define MAX_REVISION 32
typedef struct {
    BOARD_TYPE BoardType;
    CHAR8 Serial[MAX_SERIAL];
    CHAR8 Revision[MAX_REVISION];
    UINTN NumNodes;
    UINTN BmcBootTwsiBus;
    UINTN BmcBootTwsiAddr;
    UINTN RtcTwsiBus;
    UINTN RtcTwsiAddr;
    /* IPMI support*/
    UINTN BmcIpmiTwsiBus;
    UINTN BmcIpmiTwsiAddr;
    NODE_CFG Node[MAX_NODES];
    UINT16 CpuClusterCount;
    UINT16 CpuPerClusterCount;
    UINT16 PcieSegmentCount;
    UINT64 MacAddrRangeStart;
    UINTN DdrSpeed;
    UINT64 AcpiOemTableId;
} BOARD_CFG;

/******************************************************************************
 *
 * From ThunderConfigProtocol.h
 *
 ******************************************************************************
 *
 *  Thunder board Configuration Protocol
 *
 *  Copyright (c) 2015, Cavium Inc. All rights reserved.<BR>
 *
 *  This program and the accompanying materials are licensed and made
 *  available under the terms and conditions of the BSD License which
 *  accompanies this distribution.  The full text of the license may
 *  be found at http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
 *  BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
 *  EXPRESS OR IMPLIED.
 *
 */

#define EFI_THUNDER_CONFIG_PROTOCOL_GUID \
  {0xb75a0608, 0x99ff, 0x11e5, {0x9b, 0xeb, 0x00, 0x14, 0xd1, 0xfa, 0x23, 0x5c}}

///
/// Forward declaration
///
typedef struct _EFI_THUNDER_CONFIG_PROTOCOL EFI_THUNDER_CONFIG_PROTOCOL;

///
/// Function prototypes
///
typedef
EFI_STATUS
(EFIAPI *EFI_THUNDER_CONFIG_PROTOCOL_GET_CONFIG)(
  IN EFI_THUNDER_CONFIG_PROTOCOL  *This,
  OUT BOARD_CFG** cfg
  );

///
/// Protocol structure
///
struct _EFI_THUNDER_CONFIG_PROTOCOL {
  EFI_THUNDER_CONFIG_PROTOCOL_GET_CONFIG GetConfig;
  BOARD_CFG* BoardConfig;
};

#endif /* _THUNDERXCFG_H */
