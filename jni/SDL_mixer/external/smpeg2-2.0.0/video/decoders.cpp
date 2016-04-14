/*
 * decoders.c
 *
 * This file contains all the routines for Huffman decoding required in 
 * MPEG
 *
 */

/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include <stdio.h>
#include <assert.h>
#include "decoders.h"
#include "util.h" 
#include "video.h"
#include "proto.h"

/*
   Changes to make the code reentrant:
     curVidStream now not global - DecodeDCT* functioned #ifdef'd out
       (are they needed?)
   Additional changes:
     none
   -lsh@cs.brown.edu (Loring Holden)
 */

/* Decoding table for macroblock_address_increment */
mb_addr_inc_entry     mb_addr_inc[2048];

/* Decoding table for macroblock_type in predictive-coded pictures */
mb_type_entry         mb_type_P[64];

/* Decoding table for macroblock_type in bidirectionally-coded pictures */
mb_type_entry         mb_type_B[64];

/* Decoding table for motion vectors */
motion_vectors_entry  motion_vectors[2048];

/* Decoding table for coded_block_pattern */

const coded_block_pattern_entry coded_block_pattern[512] = 
{ {(unsigned int)ERROR, 0}, {(unsigned int)ERROR, 0}, {39, 9}, {27, 9}, {59, 9}, {55, 9}, {47, 9}, {31, 9},
    {58, 8}, {58, 8}, {54, 8}, {54, 8}, {46, 8}, {46, 8}, {30, 8}, {30, 8},
    {57, 8}, {57, 8}, {53, 8}, {53, 8}, {45, 8}, {45, 8}, {29, 8}, {29, 8},
    {38, 8}, {38, 8}, {26, 8}, {26, 8}, {37, 8}, {37, 8}, {25, 8}, {25, 8},
    {43, 8}, {43, 8}, {23, 8}, {23, 8}, {51, 8}, {51, 8}, {15, 8}, {15, 8},
    {42, 8}, {42, 8}, {22, 8}, {22, 8}, {50, 8}, {50, 8}, {14, 8}, {14, 8},
    {41, 8}, {41, 8}, {21, 8}, {21, 8}, {49, 8}, {49, 8}, {13, 8}, {13, 8},
    {35, 8}, {35, 8}, {19, 8}, {19, 8}, {11, 8}, {11, 8}, {7, 8}, {7, 8},
    {34, 7}, {34, 7}, {34, 7}, {34, 7}, {18, 7}, {18, 7}, {18, 7}, {18, 7},
    {10, 7}, {10, 7}, {10, 7}, {10, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, 
    {33, 7}, {33, 7}, {33, 7}, {33, 7}, {17, 7}, {17, 7}, {17, 7}, {17, 7}, 
    {9, 7}, {9, 7}, {9, 7}, {9, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, 
    {63, 6}, {63, 6}, {63, 6}, {63, 6}, {63, 6}, {63, 6}, {63, 6}, {63, 6}, 
    {3, 6}, {3, 6}, {3, 6}, {3, 6}, {3, 6}, {3, 6}, {3, 6}, {3, 6}, 
    {36, 6}, {36, 6}, {36, 6}, {36, 6}, {36, 6}, {36, 6}, {36, 6}, {36, 6}, 
    {24, 6}, {24, 6}, {24, 6}, {24, 6}, {24, 6}, {24, 6}, {24, 6}, {24, 6}, 
    {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5},
    {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5}, {62, 5},
    {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, 
    {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, {2, 5}, 
    {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, 
    {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, {61, 5}, 
    {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, 
    {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, {1, 5}, 
    {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, 
    {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, {56, 5}, 
    {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, 
    {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, {52, 5}, 
    {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, 
    {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, {44, 5}, 
    {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, 
    {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, {28, 5}, 
    {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, 
    {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, {40, 5}, 
    {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, 
    {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, {20, 5}, 
    {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, 
    {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, {48, 5}, 
    {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, 
    {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, {12, 5}, 
    {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, 
    {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, 
    {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, 
    {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, 
    {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, 
    {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, 
    {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, 
    {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, 
    {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, 
    {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, 
    {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, 
    {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4},
    {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4},
    {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, 
    {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, 
    {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4}, {4, 4},
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, 
    {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}, {60, 3}
};

/* Decoding tables for dct_dc_size_luminance */
const dct_dc_size_entry dct_dc_size_luminance[32] =
{   {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, 
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, 
    {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3}, 
    {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {(unsigned int)ERROR, 0}
};

const dct_dc_size_entry dct_dc_size_luminance1[16] =
{   {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
    {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}
};

/* Decoding table for dct_dc_size_chrominance */
const dct_dc_size_entry dct_dc_size_chrominance[32] =
{   {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, 
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, 
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, 
    {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {(unsigned int)ERROR, 0}
};

const dct_dc_size_entry dct_dc_size_chrominance1[32] =
{   {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, 
    {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, 
    {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, 
    {8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10, 10}, {11, 10}
};

/* DCT coeff tables. */

const unsigned short int dct_coeff_tbl_0[256] =
{
0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 
0x052f, 0x051f, 0x050f, 0x04ff, 
0x183f, 0x402f, 0x3c2f, 0x382f, 
0x342f, 0x302f, 0x2c2f, 0x7c1f, 
0x781f, 0x741f, 0x701f, 0x6c1f, 
0x028e, 0x028e, 0x027e, 0x027e, 
0x026e, 0x026e, 0x025e, 0x025e, 
0x024e, 0x024e, 0x023e, 0x023e, 
0x022e, 0x022e, 0x021e, 0x021e, 
0x020e, 0x020e, 0x04ee, 0x04ee, 
0x04de, 0x04de, 0x04ce, 0x04ce, 
0x04be, 0x04be, 0x04ae, 0x04ae, 
0x049e, 0x049e, 0x048e, 0x048e, 
0x01fd, 0x01fd, 0x01fd, 0x01fd, 
0x01ed, 0x01ed, 0x01ed, 0x01ed, 
0x01dd, 0x01dd, 0x01dd, 0x01dd, 
0x01cd, 0x01cd, 0x01cd, 0x01cd, 
0x01bd, 0x01bd, 0x01bd, 0x01bd, 
0x01ad, 0x01ad, 0x01ad, 0x01ad, 
0x019d, 0x019d, 0x019d, 0x019d, 
0x018d, 0x018d, 0x018d, 0x018d, 
0x017d, 0x017d, 0x017d, 0x017d, 
0x016d, 0x016d, 0x016d, 0x016d, 
0x015d, 0x015d, 0x015d, 0x015d, 
0x014d, 0x014d, 0x014d, 0x014d, 
0x013d, 0x013d, 0x013d, 0x013d, 
0x012d, 0x012d, 0x012d, 0x012d, 
0x011d, 0x011d, 0x011d, 0x011d, 
0x010d, 0x010d, 0x010d, 0x010d, 
0x282c, 0x282c, 0x282c, 0x282c, 
0x282c, 0x282c, 0x282c, 0x282c, 
0x242c, 0x242c, 0x242c, 0x242c, 
0x242c, 0x242c, 0x242c, 0x242c, 
0x143c, 0x143c, 0x143c, 0x143c, 
0x143c, 0x143c, 0x143c, 0x143c, 
0x0c4c, 0x0c4c, 0x0c4c, 0x0c4c, 
0x0c4c, 0x0c4c, 0x0c4c, 0x0c4c, 
0x085c, 0x085c, 0x085c, 0x085c, 
0x085c, 0x085c, 0x085c, 0x085c, 
0x047c, 0x047c, 0x047c, 0x047c, 
0x047c, 0x047c, 0x047c, 0x047c, 
0x046c, 0x046c, 0x046c, 0x046c, 
0x046c, 0x046c, 0x046c, 0x046c, 
0x00fc, 0x00fc, 0x00fc, 0x00fc, 
0x00fc, 0x00fc, 0x00fc, 0x00fc, 
0x00ec, 0x00ec, 0x00ec, 0x00ec, 
0x00ec, 0x00ec, 0x00ec, 0x00ec, 
0x00dc, 0x00dc, 0x00dc, 0x00dc, 
0x00dc, 0x00dc, 0x00dc, 0x00dc, 
0x00cc, 0x00cc, 0x00cc, 0x00cc, 
0x00cc, 0x00cc, 0x00cc, 0x00cc, 
0x681c, 0x681c, 0x681c, 0x681c, 
0x681c, 0x681c, 0x681c, 0x681c, 
0x641c, 0x641c, 0x641c, 0x641c, 
0x641c, 0x641c, 0x641c, 0x641c, 
0x601c, 0x601c, 0x601c, 0x601c, 
0x601c, 0x601c, 0x601c, 0x601c, 
0x5c1c, 0x5c1c, 0x5c1c, 0x5c1c, 
0x5c1c, 0x5c1c, 0x5c1c, 0x5c1c, 
0x581c, 0x581c, 0x581c, 0x581c, 
0x581c, 0x581c, 0x581c, 0x581c, 
};

const unsigned short int dct_coeff_tbl_1[16] = 
{
0x00bb, 0x202b, 0x103b, 0x00ab, 
0x084b, 0x1c2b, 0x541b, 0x501b, 
0x009b, 0x4c1b, 0x481b, 0x045b, 
0x0c3b, 0x008b, 0x182b, 0x441b, 
};

const unsigned short int dct_coeff_tbl_2[4] =
{
0x4019, 0x1429, 0x0079, 0x0839, 
};

const unsigned short int dct_coeff_tbl_3[4] = 
{
0x0449, 0x3c19, 0x3819, 0x1029, 
};

const unsigned short int dct_coeff_next[256] = 
{
0xffff, 0xffff, 0xffff, 0xffff, 
0xf7d5, 0xf7d5, 0xf7d5, 0xf7d5, 
0x0826, 0x0826, 0x2416, 0x2416, 
0x0046, 0x0046, 0x2016, 0x2016, 
0x1c15, 0x1c15, 0x1c15, 0x1c15, 
0x1815, 0x1815, 0x1815, 0x1815, 
0x0425, 0x0425, 0x0425, 0x0425, 
0x1415, 0x1415, 0x1415, 0x1415, 
0x3417, 0x0067, 0x3017, 0x2c17, 
0x0c27, 0x0437, 0x0057, 0x2817, 
0x0034, 0x0034, 0x0034, 0x0034, 
0x0034, 0x0034, 0x0034, 0x0034, 
0x1014, 0x1014, 0x1014, 0x1014, 
0x1014, 0x1014, 0x1014, 0x1014, 
0x0c14, 0x0c14, 0x0c14, 0x0c14, 
0x0c14, 0x0c14, 0x0c14, 0x0c14, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0xfbe1, 0xfbe1, 0xfbe1, 0xfbe1, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
0x0011, 0x0011, 0x0011, 0x0011, 
};

const unsigned short int dct_coeff_first[256] = 
{
0xffff, 0xffff, 0xffff, 0xffff, 
0xf7d5, 0xf7d5, 0xf7d5, 0xf7d5, 
0x0826, 0x0826, 0x2416, 0x2416, 
0x0046, 0x0046, 0x2016, 0x2016, 
0x1c15, 0x1c15, 0x1c15, 0x1c15, 
0x1815, 0x1815, 0x1815, 0x1815, 
0x0425, 0x0425, 0x0425, 0x0425, 
0x1415, 0x1415, 0x1415, 0x1415, 
0x3417, 0x0067, 0x3017, 0x2c17, 
0x0c27, 0x0437, 0x0057, 0x2817, 
0x0034, 0x0034, 0x0034, 0x0034, 
0x0034, 0x0034, 0x0034, 0x0034, 
0x1014, 0x1014, 0x1014, 0x1014, 
0x1014, 0x1014, 0x1014, 0x1014, 
0x0c14, 0x0c14, 0x0c14, 0x0c14, 
0x0c14, 0x0c14, 0x0c14, 0x0c14, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0023, 0x0023, 0x0023, 0x0023, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0813, 0x0813, 0x0813, 0x0813, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0412, 0x0412, 0x0412, 0x0412, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
0x0010, 0x0010, 0x0010, 0x0010, 
};

/* Macro for filling up the decoding table for mb_addr_inc */
#define ASSIGN1(start, end, step, val, num) \
  for (i = start; i < end; i+= step) { \
    for (j = 0; j < step; j++) { \
      mb_addr_inc[i+j].value = val; \
      mb_addr_inc[i+j].num_bits = num; \
    } \
    val--; \
    }



/*
 *--------------------------------------------------------------
 *
 * init_mb_addr_inc --
 *
 *	Initialize the VLC decoding table for macro_block_address_increment
 *
 * Results:
 *	The decoding table for macro_block_address_increment will
 *      be filled; illegal values will be filled as ERROR.
 *
 * Side effects:
 *	The global array mb_addr_inc will be filled.
 *
 *--------------------------------------------------------------
 */
static void
init_mb_addr_inc()
{
  int i, j, val;

  for (i = 0; i < 8; i++) {
    mb_addr_inc[i].value = ERROR;
    mb_addr_inc[i].num_bits = 0;
  }

  mb_addr_inc[8].value = MACRO_BLOCK_ESCAPE;
  mb_addr_inc[8].num_bits = 11;

  for (i = 9; i < 15; i++) {
    mb_addr_inc[i].value = ERROR;
    mb_addr_inc[i].num_bits = 0;
  }

  mb_addr_inc[15].value = MACRO_BLOCK_STUFFING;
  mb_addr_inc[15].num_bits = 11;

  for (i = 16; i < 24; i++) {
    mb_addr_inc[i].value = ERROR;
    mb_addr_inc[i].num_bits = 0;
  }

  val = 33;

  ASSIGN1(24, 36, 1, val, 11);
  ASSIGN1(36, 48, 2, val, 10);
  ASSIGN1(48, 96, 8, val, 8);
  ASSIGN1(96, 128, 16, val, 7);
  ASSIGN1(128, 256, 64, val, 5);
  ASSIGN1(256, 512, 128, val, 4);
  ASSIGN1(512, 1024, 256, val, 3);
  ASSIGN1(1024, 2048, 1024, val, 1);
}


/* Macro for filling up the decoding table for mb_type */
#define ASSIGN2(start, end, quant, motion_forward, motion_backward, pattern, intra, num, mb_type) \
  for (i = start; i < end; i ++) { \
    mb_type[i].mb_quant = quant; \
    mb_type[i].mb_motion_forward = motion_forward; \
    mb_type[i].mb_motion_backward = motion_backward; \
    mb_type[i].mb_pattern = pattern; \
    mb_type[i].mb_intra = intra; \
    mb_type[i].num_bits = num; \
  }
	 


/*
 *--------------------------------------------------------------
 *
 * init_mb_type_P --
 *
 *	Initialize the VLC decoding table for macro_block_type in
 *      predictive-coded pictures.
 *
 * Results:
 *	The decoding table for macro_block_type in predictive-coded
 *      pictures will be filled; illegal values will be filled as ERROR.
 *
 * Side effects:
 *	The global array mb_type_P will be filled.
 *
 *--------------------------------------------------------------
 */
static void
init_mb_type_P()
{
  int i;

  mb_type_P[0].mb_quant = mb_type_P[0].mb_motion_forward 
    = mb_type_P[0].mb_motion_backward = mb_type_P[0].mb_pattern 
      = mb_type_P[0].mb_intra = ERROR;
  mb_type_P[0].num_bits = 0;

  ASSIGN2(1, 2, 1, 0, 0, 0, 1, 6, mb_type_P)
  ASSIGN2(2, 4, 1, 0, 0, 1, 0, 5, mb_type_P)
  ASSIGN2(4, 6, 1, 1, 0, 1, 0, 5, mb_type_P);
  ASSIGN2(6, 8, 0, 0, 0, 0, 1, 5, mb_type_P);
  ASSIGN2(8, 16, 0, 1, 0, 0, 0, 3, mb_type_P);
  ASSIGN2(16, 32, 0, 0, 0, 1, 0, 2, mb_type_P);
  ASSIGN2(32, 64, 0, 1, 0, 1, 0, 1, mb_type_P);
}




/*
 *--------------------------------------------------------------
 *
 * init_mb_type_B --
 *
 *	Initialize the VLC decoding table for macro_block_type in
 *      bidirectionally-coded pictures.
 *
 * Results:
 *	The decoding table for macro_block_type in bidirectionally-coded
 *      pictures will be filled; illegal values will be filled as ERROR.
 *
 * Side effects:
 *	The global array mb_type_B will be filled.
 *
 *--------------------------------------------------------------
 */
static void
init_mb_type_B()
{
  int i;

  mb_type_B[0].mb_quant = mb_type_B[0].mb_motion_forward 
    = mb_type_B[0].mb_motion_backward = mb_type_B[0].mb_pattern 
      = mb_type_B[0].mb_intra = ERROR;
  mb_type_B[0].num_bits = 0;

  ASSIGN2(1, 2, 1, 0, 0, 0, 1, 6, mb_type_B);
  ASSIGN2(2, 3, 1, 0, 1, 1, 0, 6, mb_type_B);
  ASSIGN2(3, 4, 1, 1, 0, 1, 0, 6, mb_type_B);
  ASSIGN2(4, 6, 1, 1, 1, 1, 0, 5, mb_type_B);
  ASSIGN2(6, 8, 0, 0, 0, 0, 1, 5, mb_type_B);
  ASSIGN2(8, 12, 0, 1, 0, 0, 0, 4, mb_type_B);
  ASSIGN2(12, 16, 0, 1, 0, 1, 0, 4, mb_type_B);
  ASSIGN2(16, 24, 0, 0, 1, 0, 0, 3, mb_type_B);
  ASSIGN2(24, 32, 0, 0, 1, 1, 0, 3, mb_type_B);
  ASSIGN2(32, 48, 0, 1, 1, 0, 0, 2, mb_type_B);
  ASSIGN2(48, 64, 0, 1, 1, 1, 0, 2, mb_type_B);
}


/* Macro for filling up the decoding tables for motion_vectors */
#define ASSIGN3(start, end, step, val, num) \
  for (i = start; i < end; i+= step) { \
    for (j = 0; j < step / 2; j++) { \
      motion_vectors[i+j].code = val; \
      motion_vectors[i+j].num_bits = num; \
    } \
    for (j = step / 2; j < step; j++) { \
      motion_vectors[i+j].code = -val; \
      motion_vectors[i+j].num_bits = num; \
    } \
    val--; \
  }



/*
 *--------------------------------------------------------------
 *
 * init_motion_vectors --
 *
 *	Initialize the VLC decoding table for the various motion
 *      vectors, including motion_horizontal_forward_code, 
 *      motion_vertical_forward_code, motion_horizontal_backward_code,
 *      and motion_vertical_backward_code.
 *
 * Results:
 *	The decoding table for the motion vectors will be filled;
 *      illegal values will be filled as ERROR.
 *
 * Side effects:
 *	The global array motion_vector will be filled.
 *
 *--------------------------------------------------------------
 */
static void
init_motion_vectors()
{
  int i, j, val = 16;

  for (i = 0; i < 24; i++) {
    motion_vectors[i].code = ERROR;
    motion_vectors[i].num_bits = 0;
  }

  ASSIGN3(24, 36, 2, val, 11);
  ASSIGN3(36, 48, 4, val, 10);
  ASSIGN3(48, 96, 16, val, 8);
  ASSIGN3(96, 128, 32, val, 7);
  ASSIGN3(128, 256, 128, val, 5);
  ASSIGN3(256, 512, 256, val, 4);
  ASSIGN3(512, 1024, 512, val, 3);
  ASSIGN3(1024, 2048, 1024, val, 1);
}



extern void init_pre_idct();


/*
 *--------------------------------------------------------------
 *
 * decodeInitTables --
 *
 *	Initialize all the tables for VLC decoding; this must be
 *      called when the system is set up before any decoding can
 *      take place.
 *
 * Results:
 *	All the decoding tables will be filled accordingly.
 *
 * Side effects:
 *	The corresponding global array for each decoding table 
 *      will be filled.
 *
 *--------------------------------------------------------------
 */    
void decodeInitTables()
{
  init_mb_addr_inc();
  init_mb_type_P();
  init_mb_type_B();
  init_motion_vectors();

#ifdef FLOATDCT
  if (qualityFlag)
    init_float_idct();
  else
#endif
    init_pre_idct();

#ifdef ANALYSIS
  {
    init_stats();
  }
#endif
}

#if OLDCODE

/*
 *--------------------------------------------------------------
 *
 * DecodeDCTDCSizeLum --
 *
 *	Huffman Decoder for dct_dc_size_luminance; location where
 *      the result of decoding will be placed is passed as argument.
 *      The decoded values are obtained by doing a table lookup on
 *      dct_dc_size_luminance.
 *
 * Results:
 *	The decoded value for dct_dc_size_luminance or ERROR for 
 *      unbound values will be placed in the location specified.
 *
 * Side effects:
 *	Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */        
void
decodeDCTDCSizeLum(value)
unsigned int *value;
{
  unsigned int index;

  show_bits5(index);
  
  if (index < 31) {
  	*value = dct_dc_size_luminance[index].value;
  	flush_bits(dct_dc_size_luminance[index].num_bits);
  }
  else {
	show_bits9(index);
	index -= 0x1f0;
	*value = dct_dc_size_luminance1[index].value;
	flush_bits(dct_dc_size_luminance1[index].num_bits);
  }
}




/*
 *--------------------------------------------------------------
 *
 * DecodeDCTDCSizeChrom --
 *
 *	Huffman Decoder for dct_dc_size_chrominance; location where
 *      the result of decoding will be placed is passed as argument.
 *      The decoded values are obtained by doing a table lookup on
 *      dct_dc_size_chrominance.
 *
 * Results:
 *	The decoded value for dct_dc_size_chrominance or ERROR for
 *      unbound values will be placed in the location specified.
 *
 * Side effects:
 *	Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */    
void    
decodeDCTDCSizeChrom(value)
unsigned int *value;
{
  unsigned int index;

  show_bits5(index);
  
  if (index < 31) {
  	*value = dct_dc_size_chrominance[index].value;
  	flush_bits(dct_dc_size_chrominance[index].num_bits);
  }
  else {
	show_bits10(index);
	index -= 0x3e0;
	*value = dct_dc_size_chrominance1[index].value;
	flush_bits(dct_dc_size_chrominance1[index].num_bits);
  }
}



/*
 *--------------------------------------------------------------
 *
 * decodeDCTCoeff --
 *
 *	Huffman Decoder for dct_coeff_first and dct_coeff_next;
 *      locations where the results of decoding: run and level, are to
 *      be placed and also the type of DCT coefficients, either
 *      dct_coeff_first or dct_coeff_next, are being passed as argument.
 *      
 *      The decoder first examines the next 8 bits in the input stream,
 *      and perform according to the following cases:
 *      
 *      '0000 0000' - examine 8 more bits (i.e. 16 bits total) and
 *                    perform a table lookup on dct_coeff_tbl_0.
 *                    One more bit is then examined to determine the sign
 *                    of level.
 *
 *      '0000 0001' - examine 4 more bits (i.e. 12 bits total) and 
 *                    perform a table lookup on dct_coeff_tbl_1.
 *                    One more bit is then examined to determine the sign
 *                    of level.
 *      
 *      '0000 0010' - examine 2 more bits (i.e. 10 bits total) and
 *                    perform a table lookup on dct_coeff_tbl_2.
 *                    One more bit is then examined to determine the sign
 *                    of level.
 *
 *      '0000 0011' - examine 2 more bits (i.e. 10 bits total) and 
 *                    perform a table lookup on dct_coeff_tbl_3.
 *                    One more bit is then examined to determine the sign
 *                    of level.
 *
 *      otherwise   - perform a table lookup on dct_coeff_tbl. If the
 *                    value of run is not ESCAPE, extract one more bit
 *                    to determine the sign of level; otherwise 6 more
 *                    bits will be extracted to obtain the actual value 
 *                    of run , and then 8 or 16 bits to get the value of level.
 *                    
 *      
 *
 * Results:
 *	The decoded values of run and level or ERROR for unbound values
 *      are placed in the locations specified.
 *
 * Side effects:
 *	Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
static void
decodeDCTCoeff(dct_coeff_tbl, run, level)
unsigned short int *dct_coeff_tbl;                                       
unsigned int *run;
int *level;
{
  unsigned int temp, index /*, num_bits */;
  unsigned int value, next32bits, flushed;

  /*
   * Grab the next 32 bits and use it to improve performance of
   * getting the bits to parse. Thus, calls are translated as:
   *
   *	show_bitsX  <-->   next32bits >> (32-X)
   *	get_bitsX   <-->   val = next32bits >> (32-flushed-X);
   *			   flushed += X;
   *			   next32bits &= bitMask[flushed];
   *	flush_bitsX <-->   flushed += X;
   *			   next32bits &= bitMask[flushed];
   *
   */
  show_bits32(next32bits);
  flushed = 0;

  /* show_bits8(index); */
  index = next32bits >> 24;

  if (index > 3) {
    value = dct_coeff_tbl[index];
    *run = (value & RUN_MASK) >> RUN_SHIFT;
    if (*run == END_OF_BLOCK) {
      *level = END_OF_BLOCK;
    }
    else {
      /* num_bits = (value & NUM_MASK) + 1; */
      /* flush_bits(num_bits); */
      flushed = (value & NUM_MASK) + 1;
      next32bits &= bitMask[flushed];
      if (*run != ESCAPE) {
         *level = (value & LEVEL_MASK) >> LEVEL_SHIFT;
	 /* get_bits1(value); */
	 /* if (value) *level = -*level; */
	 if (next32bits >> (31-flushed)) *level = -*level;
	 flushed++;
	 /* next32bits &= bitMask[flushed];  last op before update */
#ifdef NO_GRIFF_MODS
#else
	 /* CG: moved into if case 12jul2000 */
         /* Update bitstream... */
         flush_bits(flushed);
#endif
       }
       else {    /* *run == ESCAPE */
         /* get_bits14(temp); */
	 temp = next32bits >> (18-flushed);
	 flushed += 14;
	 next32bits &= bitMask[flushed];
	 *run = temp >> 8;
	 temp &= 0xff;
	 if (temp == 0) {
            /* get_bits8(*level); */
	    *level = next32bits >> (24-flushed);
	    flushed += 8;
	    /* next32bits &= bitMask[flushed];  last op before update */
#ifdef NO_GRIFF_MODS
 	    assert(*level >= 128);
#else
	    /* CG: Try to overcome the assertion and incorrect decoding in
	     * case of lost packets. 12jul2000 */
 	    if (*level >= 128) {
              flush_bits(flushed);
	    }
	    else {
              *run = END_OF_BLOCK;
              *level = END_OF_BLOCK;
	    }
#endif
	 } else if (temp != 128) {
	    /* Grab sign bit */
	    *level = ((int) (temp << 24)) >> 24;
#ifdef NO_GRIFF_MODS
#else
	    /* CG: moved into else case 12jul2000 */
            /* Update bitstream... */
            flush_bits(flushed);
#endif
	 } else {
            /* get_bits8(*level); */
	    *level = next32bits >> (24-flushed);
	    flushed += 8;
	    /* next32bits &= bitMask[flushed];  last op before update */
	    *level = *level - 256;
#ifdef NO_GRIFF_MODS
	    assert(*level <= -128 && *level >= -255);
#else
	    /* CG: Try to overcome the assertion and incorrect decoding in
	     * case of lost packets. 12jul2000 */
	    if (*level <= -128 && *level >= -255) {
              flush_bits(flushed);
	    }
	    else {
              *run = END_OF_BLOCK;
              *level = END_OF_BLOCK;
	    }
#endif
	 }
       }
#ifdef NO_GRIFF_MODS
       /* Update bitstream... */
       flush_bits(flushed);
#endif
    }
  }
  else {
    if (index == 2) { 
      /* show_bits10(index); */
      index = next32bits >> 22;
      value = dct_coeff_tbl_2[index & 3];
    }
    else if (index == 3) { 
      /* show_bits10(index); */
      index = next32bits >> 22;
      value = dct_coeff_tbl_3[index & 3];
    }
    else if (index) {	/* index == 1 */
      /* show_bits12(index); */
      index = next32bits >> 20;
      value = dct_coeff_tbl_1[index & 15];
    }
    else {   /* index == 0 */
      /* show_bits16(index); */
      index = next32bits >> 16;
      value = dct_coeff_tbl_0[index & 255];
    }
    *run = (value & RUN_MASK) >> RUN_SHIFT;
    *level = (value & LEVEL_MASK) >> LEVEL_SHIFT;

    /*
     * Fold these operations together to make it fast...
     */
    /* num_bits = (value & NUM_MASK) + 1; */
    /* flush_bits(num_bits); */
    /* get_bits1(value); */
    /* if (value) *level = -*level; */

    flushed = (value & NUM_MASK) + 2;
    if ((next32bits >> (32-flushed)) & 0x1) *level = -*level;

    /* Update bitstream ... */
    flush_bits(flushed);
  }
}


/*
 *--------------------------------------------------------------
 *
 * decodeDCTCoeffFirst --
 *
 *	Huffman Decoder for dct_coeff_first. Locations for the
 *      decoded results: run and level, are being passed as
 *      arguments. Actual work is being done by calling DecodeDCTCoeff,
 *      with the table dct_coeff_first.
 *
 * Results:
 *	The decoded values of run and level for dct_coeff_first or
 *      ERROR for unbound values are placed in the locations given.
 *
 * Side effects:
 *	Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */        
void
decodeDCTCoeffFirst(run, level)
unsigned int *run;
int *level;
{
  decodeDCTCoeff(dct_coeff_first, run, level);
}




/*
 *--------------------------------------------------------------
 *
 * decodeDCTCoeffNext --
 *
 *	Huffman Decoder for dct_coeff_first. Locations for the
 *      decoded results: run and level, are being passed as
 *      arguments. Actual work is being done by calling DecodeDCTCoeff,
 *      with the table dct_coeff_next.
 *
 * Results:
 *	The decoded values of run and level for dct_coeff_next or
 *      ERROR for unbound values are placed in the locations given.
 *
 * Side effects:
 *	Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */ 
void       
decodeDCTCoeffNext(run, level)
unsigned int *run;
int *level;
{
  decodeDCTCoeff(dct_coeff_next, run, level);
}
#endif
