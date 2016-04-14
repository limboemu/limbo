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

#ifndef MPEG_LIB_VIDEO_HEADER
#define MPEG_LIB_VIDEO_HEADER
#include <stdio.h>

/*
   Changes to make the code reentrant:
     deglobalized: ditherFlags, totNumFrames, realTimeStart, matched_depth,
       filename, ditherType, curBits, ReconPMBlock statics, stream id
       variables, Parse_done, swap, seekValue, input, EOF_flag, ReadPacket
       statics, sys_layer, curVidStream, curVidStream, jmb_buf env (removed)
     X globals now in xinfo: ditherType, visual, depth (also name), hints,
       owncmFlag, et al
     now const: scan
     film_has_ended instead of FilmState
   Additional changes:
     if DISABLE_DITHER defined, don't compile dithering code
   -lsh@cs.brown.edu (Loring Holden)
 */

#include "MPEGvideo.h"
#include "MPEGaudio.h"

typedef Sint32 INT32;
typedef Sint16 INT16;
typedef Sint8 INT8;
typedef Uint32 UINT32;
typedef Uint16 UINT16;
typedef Uint8 UINT8;
 
/* Define Parsing error codes. */

#define SKIP_PICTURE (-10)
#define SKIP_TO_START_CODE (-1)
#define PARSE_OK 1

/* Define BOOLEAN, TRUE, and FALSE. */

#define BOOLEAN int
#define TRUE 1
#define FALSE 0

/* Set ring buffer size. */

#define RING_BUF_SIZE 5

/* Macros for picture code type. */

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

/* Start codes. */

#define SEQ_END_CODE 0x000001b7
#define SEQ_START_CODE 0x000001b3
#define GOP_START_CODE 0x000001b8
#define PICTURE_START_CODE 0x00000100
#define SLICE_MIN_START_CODE 0x00000101
#define SLICE_MAX_START_CODE 0x000001af
#define EXT_START_CODE 0x000001b5
#define USER_START_CODE 0x000001b2
#define SEQUENCE_ERROR_CODE 0x000001b4

/* Number of macroblocks to process in one call to mpegVidRsrc. */

#define MB_QUANTUM 100

/* Macros used with macroblock address decoding. */

#define MB_STUFFING 34
#define MB_ESCAPE 35

/* Lock flags for pict images. */

#define DISPLAY_LOCK 0x01
#define PAST_LOCK 0x02
#define FUTURE_LOCK 0x04

#define HYBRID_DITHER 0
#define HYBRID2_DITHER 1
#define FS4_DITHER 2
#define FS2_DITHER 3
#define FS2FAST_DITHER 4
#define Twox2_DITHER 5
#define GRAY_DITHER 6
#define FULL_COLOR_DITHER 7
#define NO_DITHER 8
#define ORDERED_DITHER 9
#define MONO_DITHER 10
#define MONO_THRESHOLD 11
#define ORDERED2_DITHER 12
#define MBORDERED_DITHER 13
#define GRAY256_DITHER 14
#define PPM_DITHER     15
#define FULL_COLOR2_DITHER 16
#define GRAY2_DITHER 17
#define GRAY2562_DITHER 18

#ifdef DISABLE_DITHER
#define IS_2x2_DITHER(a) (0)
#else
#define IS_2x2_DITHER(a) ((a) == Twox2_DITHER || (a) == FULL_COLOR2_DITHER || (a) == GRAY2_DITHER || (a) == (GRAY2562_DITHER))
#endif

/* Brown - changed to const int because it is a help variable */
extern const int scan[][8];

/* Structure with reconstructed pixel values. */

typedef struct pict_image {
  unsigned char *image;                  /* YV12 format image  */
  unsigned char *luminance;              /* Luminance plane.   */
  unsigned char *Cr;                     /* Cr plane.          */
  unsigned char *Cb;                     /* Cb plane.          */
  unsigned short int *mb_qscale;         /* macroblock info    */
  int locked;                            /* Lock flag.         */
  TimeStamp show_time;                   /* Presentation time. */
} PictImage;

/* Group of pictures structure. */

typedef struct GoP {
  BOOLEAN drop_flag;                     /* Flag indicating dropped frame. */
  unsigned int tc_hours;                 /* Hour component of time code.   */
  unsigned int tc_minutes;               /* Minute component of time code. */
  unsigned int tc_seconds;               /* Second component of time code. */
  unsigned int tc_pictures;              /* Picture counter of time code.  */
  BOOLEAN closed_gop;                    /* Indicates no pred. vectors to
					    previous group of pictures.    */
  BOOLEAN broken_link;                   /* B frame unable to be decoded.  */
  char *ext_data;                        /* Extension data.                */
  char *user_data;                       /* User data.                     */
} GoP;

/* Picture structure. */

typedef struct pict {
  unsigned int temp_ref;                 /* Temporal reference.             */
  unsigned int code_type;                /* Frame type: P, B, I             */
  unsigned int vbv_delay;                /* Buffer delay.                   */
  BOOLEAN full_pel_forw_vector;          /* Forw. vectors specified in full
					    pixel values flag.              */
  unsigned int forw_r_size;              /* Used for vector decoding.       */
  unsigned int forw_f;                   /* Used for vector decoding.       */
  BOOLEAN full_pel_back_vector;          /* Back vectors specified in full 
					    pixel values flag.              */
  unsigned int back_r_size;              /* Used in decoding.               */
  unsigned int back_f;                   /* Used in decoding.               */
  char *extra_info;                      /* Extra bit picture info.         */
  char *ext_data;                        /* Extension data.                 */
  char *user_data;                       /* User data.                      */
} Pict;

/* Slice structure. */

typedef struct slice {
  unsigned int vert_pos;                 /* Vertical position of slice. */
  unsigned int quant_scale;              /* Quantization scale.         */
  char *extra_info;                      /* Extra bit slice info.       */
} Slice;

/* Macroblock structure. */

typedef struct macroblock {
  int mb_address;                        /* Macroblock address.              */
  int past_mb_addr;                      /* Previous mblock address.         */
  int motion_h_forw_code;                /* Forw. horiz. motion vector code. */
  unsigned int motion_h_forw_r;          /* Used in decoding vectors.        */
  int motion_v_forw_code;                /* Forw. vert. motion vector code.  */
  unsigned int motion_v_forw_r;          /* Used in decdoinge vectors.       */
  int motion_h_back_code;                /* Back horiz. motion vector code.  */
  unsigned int motion_h_back_r;          /* Used in decoding vectors.        */
  int motion_v_back_code;                /* Back vert. motion vector code.   */
  unsigned int motion_v_back_r;          /* Used in decoding vectors.        */
  unsigned int cbp;                      /* Coded block pattern.             */
  BOOLEAN mb_intra;                      /* Intracoded mblock flag.          */
  BOOLEAN bpict_past_forw;               /* Past B frame forw. vector flag.  */
  BOOLEAN bpict_past_back;               /* Past B frame back vector flag.   */
  int past_intra_addr;                   /* Addr of last intracoded mblock.  */
  int recon_right_for_prev;              /* Past right forw. vector.         */
  int recon_down_for_prev;               /* Past down forw. vector.          */
  int recon_right_back_prev;             /* Past right back vector.          */
  int recon_down_back_prev;              /* Past down back vector.           */
} Macroblock;

/* Block structure. */

typedef struct block {
  short int dct_recon[8][8];             /* Reconstructed dct coeff matrix. */
  short int dct_dc_y_past;               /* Past lum. dc dct coefficient.   */
  short int dct_dc_cr_past;              /* Past cr dc dct coefficient.     */
  short int dct_dc_cb_past;              /* Past cb dc dct coefficient.     */
} Block;

/* Video stream structure. */

typedef struct vid_stream {
  unsigned int h_size;                         /* Horiz. size in pixels.     */
  unsigned int v_size;                         /* Vert. size in pixels.      */
  unsigned int mb_height;                      /* Vert. size in mblocks.     */
  unsigned int mb_width;                       /* Horiz. size in mblocks.    */
  unsigned char aspect_ratio;                  /* Code for aspect ratio.     */
  unsigned char picture_rate;                  /* Code for picture rate.     */
  unsigned int bit_rate;                       /* Bit rate.                  */
  unsigned int vbv_buffer_size;                /* Minimum buffer size.       */
  BOOLEAN const_param_flag;                    /* Contrained parameter flag. */
  unsigned char intra_quant_matrix[8][8];      /* Quantization matrix for
						  intracoded frames.         */
  unsigned char non_intra_quant_matrix[8][8];  /* Quanitization matrix for 
						  non intracoded frames.     */
  char *ext_data;                              /* Extension data.            */
  char *user_data;                             /* User data.                 */
  GoP group;                                   /* Current group of pict.     */
  Pict picture;                                /* Current picture.           */
  Slice slice;                                 /* Current slice.             */
  Macroblock mblock;                           /* Current macroblock.        */
  Block block;                                 /* Current block.             */
  int state;                                   /* State of decoding.         */
  int bit_offset;                              /* Bit offset in stream.      */
  unsigned int *buffer;                        /* Pointer to next byte in
						  buffer.                    */
  int buf_length;                              /* Length of remaining buffer.*/
  unsigned int *buf_start;                     /* Pointer to buffer start.   */

/* VC - beginning of added variables for noise computation */
  short noise_base_matrix[8][8];               /* Square quantization error  */
/* VC - end of added variables */

/* Brown - beginning of added variables that used to be static or global */
  int max_buf_length;                          /* Max length of buffer.      */
  int film_has_ended;                          /* Boolean - film has ended   */
  unsigned int num_left;                       /* from ReadPacket - leftover */
  unsigned int leftover_bytes;                 /* from ReadPacket - leftover */
  int EOF_flag;                                /* stream is EOF              */
  BOOLEAN Parse_done;                          /* from read_sys              */
  int right_for,down_for;                      /* From ReconPMBlock, video.c */
  int right_half_for, down_half_for;
  unsigned int curBits;                        /* current bits               */
  int ditherType;                              /* What type of dithering     */
  char *ditherFlags;                           /* flags for MB Ordered dither*/
  int totNumFrames;                            /* Total Number of Frames     */
  double realTimeStart;                        /* When did the movie start?  */
/* Brown - end of added variables */

  PictImage *past;                             /* Past predictive frame.     */
  PictImage *future;                           /* Future predictive frame.   */
  PictImage *current;                          /* Current frame.             */
  PictImage *ring[RING_BUF_SIZE];              /* Ring buffer of frames.     */

/* KR - beginning of added variables */
  double rate_deal;
  int _skipFrame;
  double _skipCount;
  int _jumpFrame;
  double _oneFrameTime;
  MPEGvideo* _smpeg;
/* KR - end of added variables */

/* SL - beginning of added variables for FPS calculation */
//#define CALCULATE_FPS
#define FPS_WINDOW 60
#ifdef CALCULATE_FPS
  double frame_time[FPS_WINDOW];
  int timestamp_index;
#endif
/* SL - end of added variables */
/* begining of added variables for system stream based sync */
  double timestamp;
  unsigned int *timestamp_mark;
  bool timestamp_used;
/* begining of added variables */
  bool need_frameadjust;
  int  current_frame;

} VidStream;   

/* Declaration of global display pointer. */

/* Quiet mode flag. */
extern int quietFlag;

/* Flag controlling the "Press return" prompt */
extern int requireKeypressFlag;

/* Flag controlling speed vs. quality */
extern int qualityFlag;

/* Gamma correction stuff */
extern int gammaCorrectFlag;
extern double gammaCorrect;

/* Chroma correction stuff */
extern int chromaCorrectFlag;
extern double chromaCorrect;

/* Definition of Contant integer scale factor. */

#define CONST_BITS 13

/* Misc DCT definitions */
#define DCTSIZE		8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	64	/* DCTSIZE squared; # of elements in a block */

#define GLOBAL			/* a function referenced thru EXTERNs */
  
typedef short DCTELEM;
typedef DCTELEM DCTBLOCK[DCTSIZE2];
 

#ifdef ANALYSIS
extern unsigned int bitCount;
extern int showEachFlag;
extern unsigned int cacheHit[8][8];
extern unsigned int cacheMiss[8][8];
#endif

#if !defined(__MIPSEL__) && (defined(MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(__mipsel) || defined(__mipsel__))
#define __MIPSEL__ 1
#endif

#if !defined(__MIPSEB__) && (defined(MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__) || defined(__mipseb) || defined(__mipseb__))
#define __MIPSEB__ 1
#endif

#if !defined(__SPARC__) && (defined(SPARC) || defined(__SPARC) || defined(__SPARC__) || defined(__sparc) || defined(__sparc__))
#define __SPARC__ 1
#endif

#if !defined(__alpha__) && (defined(ALPHA) || defined(__ALPHA) || defined(__ALPHA__) || defined(__alpha))
#define __alpha__ 1
#endif

#if !defined(__680x0__) && (defined(__680x0) || defined(__680x0__))
#define __680x0__ 1
#endif

#if !defined(__AIX__) && (defined(AIX) || defined(_AIX) || defined(__AIX) || defined(__AIX__))
#define __AIX__ 1
#endif

#if !defined(__RS6000__) && (defined(__AIX__) || defined(RS6000) || defined(_RS6000) || defined(__RS6000) || defined(__RS6000__))
#define __RS6000__ 1
#endif

#if !defined(__HPUX__) && (defined(HPUX) || defined(_HPUX) || defined(__HPUX) || defined(__HPUX__))
#define __HPUX__ 1
#endif
#if !defined(__HPUX__) && (defined(hpux) || defined(_hpux) || defined(__hpux) || defined(__hpux__))
#define __HPUX__ 1
#endif

#if !defined(__VAX__) && (defined(VAX) || defined (__VAX))
#define __VAX__ 1
#endif

#if !defined(__SCO__) && (defined(SCO) || defined(__SCO) || defined(sco) || defined(__sco__))
#define __SCO__ 1
#endif

#include "SDL_endian.h"
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#ifdef LITTLE_ENDIAN_ARCHITECTURE
#undef LITTLE_ENDIAN_ARCHITECTURE
#endif
#define BIG_ENDIAN_ARCHITECTURE 1
#else
#ifdef BIG_ENDIAN_ARCHITECTURE
#undef BIG_ENDIAN_ARCHITECTURE
#endif
#define LITTLE_ENDIAN_ARCHITECTURE 1
#endif

#if !defined(LITTLE_ENDIAN_ARCHITECTURE) && !defined(BIG_ENDIAN_ARCHITECTURE)
#error Unknown endianism of architecture
#endif

#ifdef __alpha__
#define SIXTYFOUR_BIT
#endif

/* Warnings that may help in debugging MPEG streams */
//#define VERBOSE_WARNINGS
//#define VERBOSE_DEBUG

#endif /* video.h already included */
