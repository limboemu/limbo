/*
 * parseblock.c --
 *
 *      Procedures to read in the values of a block and store them
 *      in a place where the player can use them.
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

#define NO_SANITY_CHECKS
#include <assert.h>
#include <stdlib.h>
#include "util.h"
#include "video.h"
#include "proto.h"
#include "decoders.h"
#ifdef USE_MMX
extern "C" {
extern unsigned int cpu_flags(void);
extern void IDCT_mmx(DCTBLOCK data);
};
#define mmx_ok()	(cpu_flags() & 0x800000)
#endif


/*
   Changes to make the code reentrant:
      deglobalized: curBits, bitOffset, bitLength, bitBuffer, curVidStream,
      zigzag_direct now a const int variable initialized once
   -lsh@cs.brown.edu (Loring Holden)
 */

/* External declarations. */
#ifdef DCPREC
extern int dcprec;
#endif

/* Macro for returning 1 if num is positive, -1 if negative, 0 if 0. */

#define Sign(num) ((num > 0) ? 1 : ((num == 0) ? 0 : -1))


#ifdef USE_MMX

/* This is global for the ditherer as well */
int mmx_available = 0;

static const int zigzag_direct_nommx[64] = {
  0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12,
  19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
  42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};


static const int zigzag_direct_mmx[64] = {

    0*8+0/* 0*/, 1*8+0/* 1*/, 0*8+1/* 8*/, 0*8+2/*16*/, 1*8+1/* 9*/, 2*8+0/* 2*/, 3*8+0/* 3*/, 2*8+1/*10*/,
    1*8+2/*17*/, 0*8+3/*24*/, 0*8+4/*32*/, 1*8+3/*25*/, 2*8+2/*18*/, 3*8+1/*11*/, 4*8+0/* 4*/, 5*8+0/* 5*/,
    4*8+1/*12*/, 5*8+2/*19*/, 2*8+3/*26*/, 1*8+4/*33*/, 0*8+5/*40*/, 0*8+6/*48*/, 1*8+5/*41*/, 2*8+4/*34*/,
    3*8+3/*27*/, 4*8+2/*20*/, 5*8+1/*13*/, 6*8+0/* 6*/, 7*8+0/* 7*/, 6*8+1/*14*/, 5*8+2/*21*/, 4*8+3/*28*/,
    3*8+4/*35*/, 2*8+5/*42*/, 1*8+6/*49*/, 0*8+7/*56*/, 1*8+7/*57*/, 2*8+6/*50*/, 3*8+5/*43*/, 4*8+4/*36*/,
    5*8+3/*29*/, 6*8+2/*22*/, 7*8+1/*15*/, 7*8+2/*23*/, 6*8+3/*30*/, 5*8+4/*37*/, 4*8+5/*44*/, 3*8+6/*51*/,
    2*8+7/*58*/, 3*8+7/*59*/, 4*8+6/*52*/, 5*8+5/*45*/, 6*8+4/*38*/, 7*8+3/*31*/, 7*8+4/*39*/, 6*8+5/*46*/,
    7*8+6/*53*/, 4*8+7/*60*/, 5*8+7/*61*/, 6*8+6/*54*/, 7*8+5/*47*/, 7*8+6/*55*/, 6*8+7/*62*/, 7*8+7/*63*/
};

static int zigzag_direct[256];

void InitIDCT(void)
{
  int i;
  char *use_mmx;

  use_mmx = getenv("SMPEG_USE_MMX");
  if ( use_mmx ) {
    mmx_available = atoi(use_mmx);
  } else {
    mmx_available = mmx_ok();
  }
  if (mmx_available) {
//printf("Using MMX IDCT algorithm!\n");
    for(i=0;i<64;i++) {
      zigzag_direct[i]=zigzag_direct_mmx[i];
    }
  } else {
    for(i=0;i<64;i++) {
      zigzag_direct[i]=zigzag_direct_nommx[i];
    }  
  }
  while ( i < 256 )
    zigzag_direct[i++] = 0;
}

#else
/* Array mapping zigzag to array pointer offset. */
const int zigzag_direct[64] =
{
  0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12,
  19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
  42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

void InitIDCT(void)
{
  return;
}
#endif

/*
 *--------------------------------------------------------------
 *
 * ParseReconBlock --
 *
 *    Parse values for block structure from bitstream.
 *      n is an indication of the position of the block within
 *      the macroblock (i.e. 0-5) and indicates the type of 
 *      block (i.e. luminance or chrominance). Reconstructs
 *      coefficients from values parsed and puts in 
 *      block.dct_recon array in vid stream structure.
 *      sparseFlag is set when the block contains only one
 *      coeffictient and is used by the IDCT.
 *
 * Results:
 *    
 *
 * Side effects:
 *      Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */

#define DCT_recon blockPtr->dct_recon
#define DCT_dc_y_past blockPtr->dct_dc_y_past
#define DCT_dc_cr_past blockPtr->dct_dc_cr_past
#define DCT_dc_cb_past blockPtr->dct_dc_cb_past

#define DECODE_DCT_COEFF_FIRST DecodeDCTCoeffFirst
#define DECODE_DCT_COEFF_NEXT DecodeDCTCoeffNext


/*
 * Profiling results:
 *  This function only takes about 0.01ms per call, but is called many
 *  many times, taking about 15% of the total time used by playback.
 *
 *--------------------------------------------------------------
 */
void ParseReconBlock( int n, VidStream* vid_stream )
{
  Block *blockPtr = &vid_stream->block;
  int coeffCount=0;
  
  if (vid_stream->buf_length < 100)
    correct_underflow(vid_stream);

    int diff;
    int size, level=0, i, run, pos, coeff;
    short int *reconptr;
    unsigned char *iqmatrixptr, *niqmatrixptr;
    int qscale;

    reconptr = DCT_recon[0];

#if 0
    /* 
     * Hand coded version of memset that's a little faster...
     * Old call:
     *    memset((char *) DCT_recon, 0, 64*sizeof(short int));
     */
    {
      INT32 *p;
      p = (INT32 *) reconptr;

      p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = p[7] = p[8] = p[9] = 
      p[10] = p[11] = p[12] = p[13] = p[14] = p[15] = p[16] = p[17] = p[18] =
      p[19] = p[20] = p[21] = p[22] = p[23] = p[24] = p[25] = p[26] = p[27] =
      p[28] = p[29] = p[30] = p[31] = 0;

    }
#else
    memset(reconptr, 0, 64*sizeof(reconptr[0]));
#endif

    if (vid_stream->mblock.mb_intra) {

      if (n < 4) {

    /*
     * Get the luminance bits.  This code has been hand optimized to
     * get by the normal bit parsing routines.  We get some speedup
     * by grabbing the next 16 bits and parsing things locally.
     * Thus, calls are translated as:
     *
     *    show_bitsX  <-->   next16bits >> (16-X)
     *    get_bitsX   <-->   val = next16bits >> (16-flushed-X);
     *               flushed += X;
     *               next16bits &= bitMask[flushed];
     *    flush_bitsX <-->   flushed += X;
     *               next16bits &= bitMask[flushed];
     *
     * I've streamlined the code a lot, so that we don't have to mask
     * out the low order bits and a few of the extra adds are removed.
     *    bsmith
     */
    unsigned int next16bits, index, flushed;

        show_bits16(next16bits);
        index = next16bits >> (16-5);
        if (index < 31) {
          size = dct_dc_size_luminance[index].value;
          flushed = dct_dc_size_luminance[index].num_bits;
        } else {
          index = next16bits >> (16-9);
          index -= 0x1f0;
          size = dct_dc_size_luminance1[index].value;
          flushed = dct_dc_size_luminance1[index].num_bits;
        }
        next16bits &= bitMask[16+flushed];

        if (size != 0) {
          flushed += size;
          diff = next16bits >> (16-flushed);
          if (!(diff & bitTest[32-size])) {
            diff = rBitMask[size] | (diff + 1);
          }
          diff <<= 3;
        } else {
          diff = 0;
        }
        flush_bits(flushed);

        if ( (n == 0) && ((vid_stream->mblock.mb_address -
                           vid_stream->mblock.past_intra_addr) > 1) ) {
          coeff = diff + 1024;
        } else {
          coeff = diff + DCT_dc_y_past;
        }
        DCT_dc_y_past = coeff;
      } else { /* n = 4 or 5 */
    
    /*
     * Get the chrominance bits.  This code has been hand optimized to
     * as described above
     */

    unsigned int next16bits, index, flushed;

        show_bits16(next16bits);
        index = next16bits >> (16-5);
        if (index < 31) {
          size = dct_dc_size_chrominance[index].value;
          flushed = dct_dc_size_chrominance[index].num_bits;
        } else {
          index = next16bits >> (16-10);
          index -= 0x3e0;
          size = dct_dc_size_chrominance1[index].value;
          flushed = dct_dc_size_chrominance1[index].num_bits;
        }
        next16bits &= bitMask[16+flushed];
    
        if (size != 0) {
          flushed += size;
          diff = next16bits >> (16-flushed);
          if (!(diff & bitTest[32-size])) {
            diff = rBitMask[size] | (diff + 1);
          }
          diff <<= 3;
        } else {
          diff = 0;
        }
        flush_bits(flushed);
    
      /* We test 5 first; a result of the mixup of Cr and Cb */
        coeff = diff;
        if (n == 5) {
          if (vid_stream->mblock.mb_address -
              vid_stream->mblock.past_intra_addr > 1) {
            coeff += 1024;
          } else {
            coeff += DCT_dc_cr_past;
          }
          DCT_dc_cr_past = coeff;
        } else {
          if (vid_stream->mblock.mb_address -
              vid_stream->mblock.past_intra_addr > 1) {
            coeff += 1024;
          } else {
            coeff += DCT_dc_cb_past;
          }
          DCT_dc_cb_past = coeff;
        }
      }
      
      *reconptr = coeff;
#ifdef USE_MMX
      if ( mmx_available ) {
        *reconptr <<= 4;
      }
#endif /* USE_MMX */

      pos = 0;
      coeffCount = (coeff != 0);

      i = 0; 

      if (vid_stream->picture.code_type != 4) {

        qscale = vid_stream->slice.quant_scale;
        iqmatrixptr = vid_stream->intra_quant_matrix[0];
    
        while(1) {
      
          DECODE_DCT_COEFF_NEXT(run, level);

	  if (run >= END_OF_BLOCK) break;

          i = i + run + 1;

          pos = zigzag_direct[i&0x3f];

          /* quantizes and oddifies each coefficient */
          if (level < 0) {
            coeff = ((level<<1) * qscale * 
                     ((int) (iqmatrixptr[pos]))) / 16; 
            coeff += (1 - (coeff & 1));
          } else {
            coeff = ((level<<1) * qscale * 
                     ((int) (*(iqmatrixptr+pos)))) >> 4; 
            coeff -= (1 - (coeff & 1));
          }

#ifdef USE_MMX
          if ( mmx_available )
            coeff *= 16;
#endif

#ifdef QUANT_CHECK
          printf ("coeff: %d\n", coeff);
#endif

          reconptr[pos] = coeff;
          coeffCount++;
        }

#ifdef QUANT_CHECK
	printf ("\n");
#endif

#ifdef ANALYSIS 
        {
          extern unsigned int *mbCoeffPtr;
          mbCoeffPtr[pos]++;
        }
#endif

        flush_bits(2);
	goto end;
      }
    } else { /* non-intra-coded macroblock */

      niqmatrixptr = vid_stream->non_intra_quant_matrix[0];
      qscale = vid_stream->slice.quant_scale;

      DECODE_DCT_COEFF_FIRST(run, level);

      i = run;

      pos = zigzag_direct[i&0x3f];

        /* quantizes and oddifies each coefficient */
      if (level < 0) {
        coeff = (((level<<1) - 1) * qscale * 
                 ((int) (niqmatrixptr[pos]))) / 16; 
	if ((coeff & 1) == 0) {coeff = coeff + 1;}
      } else {
        coeff = (((level<<1) + 1) * qscale * 
                 ((int) (*(niqmatrixptr+pos)))) >> 4; 
	coeff = (coeff-1) | 1; /* equivalent to: if ((coeff&1)==0) coeff = coeff - 1; */
      }

#ifdef USE_MMX
      if ( mmx_available )
        coeff *= 16;
#endif

      reconptr[pos] = coeff;
      if (coeff) {
          coeffCount = 1;
      }

      if (vid_stream->picture.code_type != 4) {
    
        while(1) {
      
          DECODE_DCT_COEFF_NEXT(run, level);

          if (run >= END_OF_BLOCK) {
	       	break;
          }

          i = i+run+1;

          pos = zigzag_direct[i&0x3f];

          if (level < 0) {
            coeff = (((level<<1) - 1) * qscale * 
                     ((int) (niqmatrixptr[pos]))) / 16; 
            if ((coeff & 1) == 0) {coeff = coeff + 1;}
          } else {
            coeff = (((level<<1) + 1) * qscale * 
                     ((int) (*(niqmatrixptr+pos)))) >> 4; 
            coeff = (coeff-1) | 1; /* equivalent to: if ((coeff&1)==0) coeff = coeff - 1; */
          }

#ifdef USE_MMX
          if ( mmx_available )
            coeff *= 16;
#endif
          reconptr[pos] = coeff;
          coeffCount++;
        } /* end while */

#ifdef ANALYSIS
        {
          extern unsigned int *mbCoeffPtr;
          mbCoeffPtr[pos]++;
        }
#endif

        flush_bits(2);

        goto end;
      } /* end if (vid_stream->picture.code_type != 4) */
    }
    
  end:

    if( ! vid_stream->_skipFrame || (vid_stream->picture.code_type != B_TYPE) )
    {
        if( coeffCount == 1 )
        {
#ifdef USE_MMX
          if ( mmx_available )
            IDCT_mmx(reconptr);
          else
            j_rev_dct_sparse (reconptr, pos);
#else
          j_rev_dct_sparse (reconptr, pos);
#endif
        }
        else
        {
#ifdef FLOATDCT
          if (qualityFlag)
          {
            float_idct(reconptr);
          }
          else
#endif
#ifdef USE_MMX
          if ( mmx_available )
            IDCT_mmx(reconptr);
          else
            j_rev_dct(reconptr);
#else
            j_rev_dct(reconptr);
#endif
        }
    }
#ifdef USE_MMX
    if ( mmx_available ) {
      __asm__ ("emms");
    }
#endif
}
    
#undef DCT_recon 
#undef DCT_dc_y_past 
#undef DCT_dc_cr_past 
#undef DCT_dc_cb_past 


/*
 *--------------------------------------------------------------
 *
 * ParseAwayBlock --
 *
 *    Parses off block values, throwing them away.
 *      Used with grayscale dithering.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

void ParseAwayBlock( int n, VidStream* vid_stream )
{
  unsigned int diff;
  unsigned int size, run;
  int level;

  if (vid_stream->buf_length < 100)
    correct_underflow(vid_stream);

  if (vid_stream->mblock.mb_intra) {

    /* If the block is a luminance block... */

    if (n < 4) {

      /* Parse and decode size of first coefficient. */

      DecodeDCTDCSizeLum(size);

      /* Parse first coefficient. */

      if (size != 0) {
        get_bitsn(size, diff);
      }
    }

    /* Otherwise, block is chrominance block... */

    else {

      /* Parse and decode size of first coefficient. */

      DecodeDCTDCSizeChrom(size);

      /* Parse first coefficient. */

      if (size != 0) {
        get_bitsn(size, diff);
      }
    }
  }

  /* Otherwise, block is not intracoded... */

  else {

    /* Decode and set first coefficient. */

    DECODE_DCT_COEFF_FIRST(run, level);
  }

  /* If picture is not D type (i.e. I, P, or B)... */

  if (vid_stream->picture.code_type != 4) {

    /* While end of macroblock has not been reached... */

    while (1) {

      /* Get the dct_coeff_next */

      DECODE_DCT_COEFF_NEXT(run, level);

      if (run >= END_OF_BLOCK) break;
    }

    /* End_of_block */

    flush_bits(2);
  }
}


/* EOF */
