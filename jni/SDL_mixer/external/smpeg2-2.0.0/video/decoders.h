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
 * decoders.h
 *
 * This file contains the declarations of structures required for Huffman
 * decoding
 *
 */

/* Include util.h for bit i/o parsing macros. */

#include "util.h"

/* Code for unbound values in decoding tables */
#define ERROR (-1)
#define DCT_ERROR 63

#define MACRO_BLOCK_STUFFING 34
#define MACRO_BLOCK_ESCAPE 35

/* Two types of DCT Coefficients */
#define DCT_COEFF_FIRST 0
#define DCT_COEFF_NEXT 1

/* Special values for DCT Coefficients */
#define END_OF_BLOCK 62
#define ESCAPE 61

/* Structure for an entry in the decoding table of 
 * macroblock_address_increment */
typedef struct {
  int value;       /* value for macroblock_address_increment */
  int num_bits;             /* length of the Huffman code */
} mb_addr_inc_entry;

/* Decoding table for macroblock_address_increment */
extern mb_addr_inc_entry mb_addr_inc[2048];


/* Structure for an entry in the decoding table of macroblock_type */
typedef struct {
  unsigned int mb_quant;              /* macroblock_quant */
  unsigned int mb_motion_forward;     /* macroblock_motion_forward */
  unsigned int mb_motion_backward;    /* macroblock_motion_backward */
  unsigned int mb_pattern;            /* macroblock_pattern */
  unsigned int mb_intra;              /* macroblock_intra */
  int num_bits;                       /* length of the Huffman code */
} mb_type_entry;

/* Decoding table for macroblock_type in predictive-coded pictures */
extern mb_type_entry mb_type_P[64];

/* Decoding table for macroblock_type in bidirectionally-coded pictures */
extern mb_type_entry mb_type_B[64];


/* Structures for an entry in the decoding table of coded_block_pattern */
typedef struct {
  unsigned int cbp;            /* coded_block_pattern */
  int num_bits;                /* length of the Huffman code */
} coded_block_pattern_entry;

/* External declaration of coded block pattern table. */

extern const coded_block_pattern_entry coded_block_pattern[512];



/* Structure for an entry in the decoding table of motion vectors */
typedef struct {
  int code;              /* value for motion_horizontal_forward_code,
			  * motion_vertical_forward_code, 
			  * motion_horizontal_backward_code, or
			  * motion_vertical_backward_code.
			  */
  int num_bits;          /* length of the Huffman code */
} motion_vectors_entry;


/* Decoding table for motion vectors */
extern motion_vectors_entry motion_vectors[2048];


/* Structure for an entry in the decoding table of dct_dc_size */
typedef struct {
  unsigned int value;    /* value of dct_dc_size (luminance or chrominance) */
  int num_bits;          /* length of the Huffman code */
} dct_dc_size_entry;

/* External declaration of dct dc size lumiance table. */

extern const dct_dc_size_entry dct_dc_size_luminance[32];
extern const dct_dc_size_entry dct_dc_size_luminance1[16];

/* External declaration of dct dc size chrom table. */

extern const dct_dc_size_entry dct_dc_size_chrominance[32];
extern const dct_dc_size_entry dct_dc_size_chrominance1[32];


/* DCT coeff tables. */

#define RUN_MASK 0xfc00
#define LEVEL_MASK 0x03f0
#define NUM_MASK 0x000f
#define RUN_SHIFT 10
#define LEVEL_SHIFT 4

/* External declaration of dct coeff tables. */

extern const unsigned short int dct_coeff_tbl_0[256];
extern const unsigned short int dct_coeff_tbl_1[16];
extern const unsigned short int dct_coeff_tbl_2[4];
extern const unsigned short int dct_coeff_tbl_3[4];
extern const unsigned short int dct_coeff_next[256];
extern const unsigned short int dct_coeff_first[256];

#define DecodeDCTDCSizeLum(macro_val)                    \
{                                                    \
  unsigned int index;	\
	\
  show_bits5(index);	\
  	\
  if (index < 31) {	\
  	macro_val = dct_dc_size_luminance[index].value;	\
  	flush_bits(dct_dc_size_luminance[index].num_bits);	\
  }	\
  else {	\
	show_bits9(index);	\
	index -= 0x1f0;	\
	macro_val = dct_dc_size_luminance1[index].value;	\
	flush_bits(dct_dc_size_luminance1[index].num_bits);	\
  }	\
}

#define DecodeDCTDCSizeChrom(macro_val)                      \
{                                                        \
  unsigned int index;	\
	\
  show_bits5(index);	\
  	\
  if (index < 31) {	\
  	macro_val = dct_dc_size_chrominance[index].value;	\
  	flush_bits(dct_dc_size_chrominance[index].num_bits);	\
  }	\
  else {	\
	show_bits10(index);	\
	index -= 0x3e0;	\
	macro_val = dct_dc_size_chrominance1[index].value;	\
	flush_bits(dct_dc_size_chrominance1[index].num_bits);	\
  }	\
}

#ifdef NO_GRIFF_MODS
#define DecodeDCTCoeff(dct_coeff_tbl, run, level)			\
{									\
  unsigned int temp, index;						\
  unsigned int value, next32bits, flushed;				\
									\
  /*									\
   * Grab the next 32 bits and use it to improve performance of		\
   * getting the bits to parse. Thus, calls are translated as:		\
   *									\
   *	show_bitsX  <-->   next32bits >> (32-X)				\
   *	get_bitsX   <-->   val = next32bits >> (32-flushed-X);		\
   *			   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *	flush_bitsX <-->   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *									\
   * I've streamlined the code a lot, so that we don't have to mask	\
   * out the low order bits and a few of the extra adds are removed.	\
   */									\
  show_bits32(next32bits);						\
									\
  /* show_bits8(index); */						\
  index = next32bits >> 24;						\
									\
  if (index > 3) {							\
    value = dct_coeff_tbl[index];					\
    run = value >> RUN_SHIFT;						\
    if (run != END_OF_BLOCK) {						\
      /* num_bits = (value & NUM_MASK) + 1; */				\
      /* flush_bits(num_bits); */					\
      if (run != ESCAPE) {						\
	 /* get_bits1(value); */					\
	 /* if (value) level = -level; */				\
	 flushed = (value & NUM_MASK) + 2;				\
         level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
	 value = next32bits >> (32-flushed);				\
	 value &= 0x1;							\
	 if (value) level = -level;					\
	 /* next32bits &= ((~0) >> flushed);  last op before update */	\
       }								\
       else {    /* run == ESCAPE */					\
	 /* Get the next six into run, and next 8 into temp */		\
         /* get_bits14(temp); */					\
	 flushed = (value & NUM_MASK) + 1;				\
	 temp = next32bits >> (18-flushed);				\
	 /* Normally, we'd ad 14 to flushed, but I've saved a few	\
	  * instr by moving the add below */				\
	 temp &= 0x3fff;						\
	 run = temp >> 8;						\
	 temp &= 0xff;							\
	 if (temp == 0) {						\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
 	    assert(level >= 128);					\
	 } else if (temp != 128) {					\
	    /* Grab sign bit */						\
	    flushed += 14;						\
	    level = ((int) (temp << 24)) >> 24;				\
	 } else {							\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
	    level = level - 256;					\
	    assert(level <= -128 && level >= -255);			\
	 }								\
       }								\
       /* Update bitstream... */					\
       flush_bits(flushed);						\
       assert (flushed <= 32);						\
    }									\
  }									\
  else {								\
    switch (index) {                                                    \
    case 2: {   							\
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_2[index & 3];				\
      break;                                                            \
    }									\
    case 3: { 						                \
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_3[index & 3];				\
      break;                                                            \
    }									\
    case 1: {                                             		\
      /* show_bits12(index); */						\
      index = next32bits >> 20;						\
      value = dct_coeff_tbl_1[index & 15];				\
      break;                                                            \
    }									\
    default: { /* index == 0 */						\
      /* show_bits16(index); */						\
      index = next32bits >> 16;						\
      value = dct_coeff_tbl_0[index & 255];				\
    }}									\
    run = value >> RUN_SHIFT;						\
    level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
									\
    /*									\
     * Fold these operations together to make it fast...		\
     */									\
    /* num_bits = (value & NUM_MASK) + 1; */				\
    /* flush_bits(num_bits); */						\
    /* get_bits1(value); */						\
    /* if (value) level = -level; */					\
									\
    flushed = (value & NUM_MASK) + 2;					\
    value = next32bits >> (32-flushed);					\
    value &= 0x1;							\
    if (value) level = -level;						\
									\
    /* Update bitstream ... */						\
    flush_bits(flushed);						\
    assert (flushed <= 32);						\
  }									\
}
#else /* NO_GRIFF_MODS */
#define DecodeDCTCoeff(dct_coeff_tbl, run, level)			\
{									\
  unsigned int temp, index;						\
  unsigned int value, next32bits, flushed;				\
									\
  /*									\
   * Grab the next 32 bits and use it to improve performance of		\
   * getting the bits to parse. Thus, calls are translated as:		\
   *									\
   *	show_bitsX  <-->   next32bits >> (32-X)				\
   *	get_bitsX   <-->   val = next32bits >> (32-flushed-X);		\
   *			   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *	flush_bitsX <-->   flushed += X;				\
   *			   next32bits &= bitMask[flushed];		\
   *									\
   * I've streamlined the code a lot, so that we don't have to mask	\
   * out the low order bits and a few of the extra adds are removed.	\
   */									\
  show_bits32(next32bits);						\
									\
  /* show_bits8(index); */						\
  index = next32bits >> 24;						\
									\
  if (index > 3) {							\
    value = dct_coeff_tbl[index];					\
    run = value >> RUN_SHIFT;						\
    if (run != END_OF_BLOCK) {						\
      /* num_bits = (value & NUM_MASK) + 1; */				\
      /* flush_bits(num_bits); */					\
      if (run != ESCAPE) {						\
	 /* get_bits1(value); */					\
	 /* if (value) level = -level; */				\
	 flushed = (value & NUM_MASK) + 2;				\
         level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
	 value = next32bits >> (32-flushed);				\
	 value &= 0x1;							\
	 if (value) level = -level;					\
	 /* next32bits &= ((~0) >> flushed);  last op before update */	\
									\
         /* Update bitstream... */					\
         flush_bits(flushed);						\
         assert (flushed <= 32);					\
       }								\
       else {    /* run == ESCAPE */					\
	 /* Get the next six into run, and next 8 into temp */		\
         /* get_bits14(temp); */					\
	 flushed = (value & NUM_MASK) + 1;				\
	 temp = next32bits >> (18-flushed);				\
	 /* Normally, we'd ad 14 to flushed, but I've saved a few	\
	  * instr by moving the add below */				\
	 temp &= 0x3fff;						\
	 run = temp >> 8;						\
	 temp &= 0xff;							\
	 if (temp == 0) {						\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
 	    /* CG: 12jul2000 - assert(level >= 128); */                 \
 	    if (level >= 128) {                                         \
               /* Update bitstream... */				\
               flush_bits(flushed);					\
               assert (flushed <= 32);					\
	    } else {                                                    \
	      run = END_OF_BLOCK;                                       \
	      level = END_OF_BLOCK;                                     \
	    }                                                           \
	 } else if (temp != 128) {					\
	    /* Grab sign bit */						\
	    flushed += 14;						\
	    level = ((int) (temp << 24)) >> 24;				\
            /* Update bitstream... */					\
            flush_bits(flushed);					\
            assert (flushed <= 32);					\
	 } else {							\
            /* get_bits8(level); */					\
	    level = next32bits >> (10-flushed);				\
	    level &= 0xff;						\
	    flushed += 22;						\
	    level = level - 256;					\
	    /* CG: 12jul2000 - assert(level <= -128 && level >= -255); */ \
	    if ( level <= -128 && level >= -255) {                      \
              /* Update bitstream... */					\
              flush_bits(flushed);					\
              assert (flushed <= 32);					\
	    } else {                                                    \
	      run = END_OF_BLOCK;                                       \
	      level = END_OF_BLOCK;                                     \
	    }                                                           \
	 }								\
       }								\
    }									\
  }									\
  else {								\
    switch (index) {                                                    \
    case 2: {   							\
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_2[index & 3];				\
      break;                                                            \
    }									\
    case 3: { 						                \
      /* show_bits10(index); */						\
      index = next32bits >> 22;						\
      value = dct_coeff_tbl_3[index & 3];				\
      break;                                                            \
    }									\
    case 1: {                                             		\
      /* show_bits12(index); */						\
      index = next32bits >> 20;						\
      value = dct_coeff_tbl_1[index & 15];				\
      break;                                                            \
    }									\
    default: { /* index == 0 */						\
      /* show_bits16(index); */						\
      index = next32bits >> 16;						\
      value = dct_coeff_tbl_0[index & 255];				\
    }}									\
    run = value >> RUN_SHIFT;						\
    level = (value & LEVEL_MASK) >> LEVEL_SHIFT;			\
									\
    /*									\
     * Fold these operations together to make it fast...		\
     */									\
    /* num_bits = (value & NUM_MASK) + 1; */				\
    /* flush_bits(num_bits); */						\
    /* get_bits1(value); */						\
    /* if (value) level = -level; */					\
									\
    flushed = (value & NUM_MASK) + 2;					\
    value = next32bits >> (32-flushed);					\
    value &= 0x1;							\
    if (value) level = -level;						\
									\
    /* Update bitstream ... */						\
    flush_bits(flushed);						\
    assert (flushed <= 32);						\
  }									\
}
#endif /* NO_GRIFF_MODS */

#define DecodeDCTCoeffFirst(runval, levelval)         \
{                                                     \
  DecodeDCTCoeff(dct_coeff_first, runval, levelval);  \
}          

#define DecodeDCTCoeffNext(runval, levelval)          \
{                                                     \
  DecodeDCTCoeff(dct_coeff_next, runval, levelval);   \
}

/*
 *--------------------------------------------------------------
 *
 * DecodeMBAddrInc --
 *
 *      Huffman Decoder for macro_block_address_increment; the location
 *      in which the result will be placed is being passed as argument.
 *      The decoded value is obtained by doing a table lookup on
 *      mb_addr_inc.
 *
 * Results:
 *      The decoded value for macro_block_address_increment or ERROR
 *      for unbound values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBAddrInc(val)				\
{							\
    unsigned int index;					\
    show_bits11(index);					\
    val = mb_addr_inc[index].value;			\
    flush_bits(mb_addr_inc[index].num_bits);		\
}

/*
 *--------------------------------------------------------------
 *
 * DecodeMotionVectors --
 *
 *      Huffman Decoder for the various motion vectors, including
 *      motion_horizontal_forward_code, motion_vertical_forward_code,
 *      motion_horizontal_backward_code, motion_vertical_backward_code.
 *      Location where the decoded result will be placed is being passed
 *      as argument. The decoded values are obtained by doing a table
 *      lookup on motion_vectors.
 *
 * Results:
 *      The decoded value for the motion vector or ERROR for unbound
 *      values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */

#define DecodeMotionVectors(value)			\
{							\
  unsigned int index;					\
  show_bits11(index);					\
  value = motion_vectors[index].code;			\
  flush_bits(motion_vectors[index].num_bits);		\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeB --
 *
 *      Huffman Decoder for macro_block_type in bidirectionally-coded
 *      pictures;locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument. The decoded values are obtained by
 *      doing a table lookup on mb_type_B.
 *
 * Results:
 *      The various decoded values for macro_block_type in
 *      bidirectionally-coded pictures or ERROR for unbound values will
 *      be placed in the locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeB(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
									\
  show_bits6(index);							\
									\
  quant = mb_type_B[index].mb_quant;					\
  motion_fwd = mb_type_B[index].mb_motion_forward;			\
  motion_bwd = mb_type_B[index].mb_motion_backward;			\
  pat = mb_type_B[index].mb_pattern;					\
  intra = mb_type_B[index].mb_intra;					\
  flush_bits(mb_type_B[index].num_bits);				\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeI --
 *
 *      Huffman Decoder for macro_block_type in intra-coded pictures;
 *      locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument.
 *
 * Results:
 *      The various decoded values for macro_block_type in intra-coded
 *      pictures or ERROR for unbound values will be placed in the
 *      locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeI(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
  static int quantTbl[4] = {ERROR, 1, 0, 0};				\
									\
  show_bits2(index);							\
									\
  motion_fwd = 0;							\
  motion_bwd = 0;							\
  pat = 0;								\
  intra = 1;								\
  quant = quantTbl[index];						\
  if (index) {								\
    flush_bits (1 + quant);						\
  }									\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeMBTypeP --
 *
 *      Huffman Decoder for macro_block_type in predictive-coded pictures;
 *      locations in which the decoded results: macroblock_quant,
 *      macroblock_motion_forward, macro_block_motion_backward,
 *      macroblock_pattern, macro_block_intra, will be placed are
 *      being passed as argument. The decoded values are obtained by
 *      doing a table lookup on mb_type_P.
 *
 * Results:
 *      The various decoded values for macro_block_type in
 *      predictive-coded pictures or ERROR for unbound values will be
 *      placed in the locations specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeMBTypeP(quant, motion_fwd, motion_bwd, pat, intra)	\
{									\
  unsigned int index;							\
									\
  show_bits6(index);							\
									\
  quant = mb_type_P[index].mb_quant;					\
  motion_fwd = mb_type_P[index].mb_motion_forward;			\
  motion_bwd = mb_type_P[index].mb_motion_backward;			\
  pat = mb_type_P[index].mb_pattern;					\
  intra = mb_type_P[index].mb_intra;					\
									\
  flush_bits(mb_type_P[index].num_bits);				\
}
/*
 *--------------------------------------------------------------
 *
 * DecodeCBP --
 *
 *      Huffman Decoder for coded_block_pattern; location in which the
 *      decoded result will be placed is being passed as argument. The
 *      decoded values are obtained by doing a table lookup on
 *      coded_block_pattern.
 *
 * Results:
 *      The decoded value for coded_block_pattern or ERROR for unbound
 *      values will be placed in the location specified.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed.
 *
 *--------------------------------------------------------------
 */
#define DecodeCBP(coded_bp)						\
{									\
  unsigned int index;							\
									\
  show_bits9(index);							\
  coded_bp = coded_block_pattern[index].cbp;				\
  flush_bits(coded_block_pattern[index].num_bits);			\
}
