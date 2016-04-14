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

#ifdef WIN32
#define __STDC__ 1
#endif
#ifdef __STDC__
# define	P(s) s
#include <stdlib.h>	/* used by almost all modules */
#else
# define P(s) ()
#endif

/* util.c */
void correct_underflow P((VidStream *vid_stream ));
int next_bits P((int num , unsigned int mask , VidStream *vid_stream ));
char *get_ext_data P((VidStream *vid_stream ));
int next_start_code P((VidStream *vid_stream));
char *get_extra_bit_info P((VidStream *vid_stream ));

/* video.c */
void init_stats P((void ));
void PrintAllStats P((VidStream *vid_stream ));
double ReadSysClock P((void ));
void PrintTimeInfo P(( VidStream *vid_stream ));
void InitCrop P((void ));
void InitIDCT P((void ));
VidStream *NewVidStream P((unsigned int buffer_len ));
void ResetVidStream P((VidStream *vid ));
void DestroyVidStream P((VidStream *astream ));
PictImage *NewPictImage P(( VidStream *vid_stream ));
bool InitPictImages P(( VidStream *vid_stream, int w, int h ));
void DestroyPictImage P(( VidStream *vid_stream, PictImage *apictimage ));
VidStream *mpegVidRsrc P((TimeStamp time_stamp,VidStream *vid_stream, int first  ));
void SetBFlag P((BOOLEAN val ));
void SetPFlag P((BOOLEAN val ));

/* parseblock.c */
void ParseReconBlock P((int n, VidStream *vid_stream ));
void ParseAwayBlock P((int n , VidStream *vid_stream ));

/* motionvec.c */
void ComputeForwVector P((int *recon_right_for_ptr , int *recon_down_for_ptr , VidStream *the_stream ));
void ComputeBackVector P((int *recon_right_back_ptr , int *recon_down_back_ptr, VidStream *the_stream ));

/* decoders.c */
void decodeInitTables P((void ));
void decodeDCTDCSizeLum P((unsigned int *value ));
void decodeDCTDCSizeChrom P((unsigned int *value ));
void decodeDCTCoeffFirst P((unsigned int *run , int *level ));
void decodeDCTCoeffNext P((unsigned int *run , int *level ));

/* gdith.c */
void InitColor P((void ));

/* fs2.c */
void InitFS2Dither P((void ));
void FS2DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *disp , int rows , int cols ));

/* fs2fast.c */
void InitFS2FastDither P((void ));
void FS2FastDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* fs4.c */
void InitFS4Dither P((void ));
void FS4DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *disp , int rows , int cols ));

/* hybrid.c */
void InitHybridDither P((void ));
void HybridDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* hybriderr.c */
void InitHybridErrorDither P((void ));
void HybridErrorDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* gray.c */
void GrayDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));
void Gray2DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));
void Gray16DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));
void Gray216DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));
void Gray32DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));
void Gray232DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* mono.c */
void MonoThresholdImage(unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int h, int w);
void MonoDitherImage(unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int h, int w);

/* jrevdct.c */
void init_pre_idct P((void ));
void j_rev_dct_sparse P((DCTBLOCK data , int pos ));
void j_rev_dct P((DCTBLOCK data ));
void j_rev_dct_sparse P((DCTBLOCK data , int pos ));
void j_rev_dct P((DCTBLOCK data ));

/* floatdct.c */
void init_float_idct P((void ));
void float_idct P((short* block ));

/* 16bit.c */
void InitColorDither P(( int bpp, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask ));
void Color16DitherImageMod P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod ));
void Color16DitherImageMMX P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod ));
void Color32DitherImageMod P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod ));
void Color32DitherImageMMX P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod ));
void Color16DitherImageModInterlace P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int start ));
void Color32DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int rows , int cols ));
void ScaleColor16DitherImageMod P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int scale ));
void ScaleColor32DitherImageMod P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int scale ));
void ScaleColor16DitherImageModInterlace P((unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int start, int scale ));
void Twox2Color32DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int rows , int cols ));

/* ordered.c */
void InitOrderedDither P((void ));
void OrderedDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* ordered2.c */
void InitOrdered2Dither P((void ));
void Ordered2DitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w ));

/* mb_ordered.c */
void InitMBOrderedDither P((void ));
void MBOrderedDitherImage P((unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w, char *ditherFlags ));
void MBOrderedDitherDisplayCopy P((VidStream *vid_stream , int mb_addr , int motion_forw , int r_right_forw , int r_down_forw , int motion_back , int r_right_back , int r_down_back , unsigned char *past , unsigned char *future ));

/* readfile.c */
void SeekStream P((VidStream *vid_stream ));
void clear_data_stream P(( VidStream *vid_stream));
int get_more_data P(( VidStream *vid_stream ));
int pure_get_more_data P((unsigned int *buf_start , int max_length , int *length_ptr , unsigned int **buf_ptr, VidStream *vid_stream ));
int read_sys P(( VidStream *vid_stream, unsigned int start ));
int ReadStartCode P(( unsigned int *startCode, VidStream *vid_stream ));

int ReadPackHeader P((
   double *systemClockTime,
   unsigned long *muxRate,
   VidStream *vid_stream ));

int ReadSystemHeader P(( VidStream *vid_stream ));

int find_start_code P(( FILE *input ));

int ReadPacket P(( unsigned char packetID, VidStream *vid_stream ));

void ReadTimeStamp P((
   unsigned char *inputBuffer,
   unsigned char *hiBit,
   unsigned long *low4Bytes));

void ReadSTD P((
   unsigned char *inputBuffer,
   unsigned char *stdBufferScale,
   unsigned long *stdBufferSize));

void ReadRate P((
   unsigned char *inputBuffer,
   unsigned long *rate));

int MakeFloatClockTime P((
   unsigned char hiBit,
   unsigned long low4Bytes,
   double *floatClockTime));


#ifndef NOCONTROLS
/* ctrlbar.c */
double StopWatch P((int action ));
bool WindowMapped P((Display *dsp, XEvent *xev, char *window ));
bool IfEventType P((Display *dsp, XEvent *xev, char *type ));
void MakeControlBar P(( XInfo *xinfo ));
void UpdateFrameTotal P((Display *display));
void UpdateControlDisplay P((Display *display));
void ControlBar P((VidStream **vid_stream, XInfo *xinfo, int numMovies ));
void ControlLoop P((VidStream **theStream, XInfo *xinfo, int numStreams ));
#endif /* !NOCONTROLS */

#undef P
