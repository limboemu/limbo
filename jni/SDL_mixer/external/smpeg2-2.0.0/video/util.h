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

/*
   Changes to make the code reentrant:
     deglobalized: curBits, curVidStream
     deglobalized: bitOffset, bitLength, bitBuffer in vid_stream, not used
       here
   Additional changes:
   -lsh@cs.brown.edu (Loring Holden)
 */

/* Status codes for bit stream i/o operations. */

#include "MPEG.h"

#define NO_VID_STREAM (-1)
#define STREAM_UNDERFLOW (-2)
#define OK 1

/* Size increment of extension data buffers. */

#define EXT_BUF_SIZE 1024

/* External declarations for bitstream i/o operations. */
extern unsigned int bitMask[];
extern unsigned int nBitMask[];
extern unsigned int rBitMask[];
extern unsigned int bitTest[];

/* Macro for updating bit counter if analysis tool is on. */
#ifdef ANALYSIS
#define UPDATE_COUNT(numbits) bitCount += numbits
#else
#define UPDATE_COUNT(numbits)
#endif

#ifdef NO_SANITY_CHECKS
#define get_bits1(result)                                                 \
{                                                                         \
  UPDATE_COUNT(1);                                                        \
  result = ((vid_stream->curBits & 0x80000000) != 0);                     \
  vid_stream->curBits <<= 1;                                              \
  vid_stream->bit_offset++;                                               \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset = 0;                                           \
    vid_stream->buffer++;                                                 \
    vid_stream->curBits = *vid_stream->buffer;                            \
    vid_stream->buf_length--;                                             \
  }                                                                       \
}

#define get_bits2(result)                                                 \
{                                                                         \
  UPDATE_COUNT(2);                                                        \
  vid_stream->bit_offset += 2;                                            \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |=                                              \
	 (*vid_stream->buffer >> (2 - vid_stream->bit_offset));           \
    }                                                                     \
    result = ((vid_stream->curBits & 0xc0000000) >> 30);                  \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
                                                                          \
  result = ((vid_stream->curBits & 0xc0000000) >> 30);                    \
  vid_stream->curBits <<= 2;                                              \
}

#define get_bitsX(num, mask, shift,  result)                              \
{                                                                         \
  UPDATE_COUNT(num);                                                      \
  vid_stream->bit_offset += num;                                          \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (num - vid_stream->bit_offset));                                    \
    }                                                                     \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
  else {                                                                  \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits <<= num;                                          \
  }                                                                       \
}
#else

#define get_bits1(result)                                                 \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(1);                                                        \
  result = ((vid_stream->curBits & 0x80000000) != 0);                     \
  vid_stream->curBits <<= 1;                                              \
  vid_stream->bit_offset++;                                               \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset = 0;                                           \
    vid_stream->buffer++;                                                 \
    vid_stream->curBits = *vid_stream->buffer;                            \
    vid_stream->buf_length--;                                             \
  }                                                                       \
}

#define get_bits2(result)                                                 \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(2);                                                        \
  vid_stream->bit_offset += 2;                                            \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (2 - vid_stream->bit_offset));                                      \
    }                                                                     \
    result = ((vid_stream->curBits & 0xc0000000) >> 30);                  \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
                                                                          \
  result = ((vid_stream->curBits & 0xc0000000) >> 30);                    \
  vid_stream->curBits <<= 2;                                              \
}

#define get_bitsX(num, mask, shift,  result)                              \
{                                                                         \
  /* Check for underflow. */                                              \
                                                                          \
  if (vid_stream->buf_length < 2) {                                       \
    correct_underflow(vid_stream);                                        \
  }                                                                       \
  UPDATE_COUNT(num);                                                      \
  vid_stream->bit_offset += num;                                          \
                                                                          \
  if (vid_stream->bit_offset & 0x20) {                                    \
    vid_stream->bit_offset -= 32;                                         \
    vid_stream->buffer++;                                                 \
    vid_stream->buf_length--;                                             \
    if (vid_stream->bit_offset) {                                         \
      vid_stream->curBits |= (*vid_stream->buffer >>                      \
      (num - vid_stream->bit_offset));                                    \
    }                                                                     \
    result = ((vid_stream->curBits & mask) >> shift);                     \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;  \
  }                                                                       \
  else {                                                                  \
   result = ((vid_stream->curBits & mask) >> shift);                      \
   vid_stream->curBits <<= num;                                           \
  }                                                                       \
}
#endif

#define get_bits3(result) get_bitsX(3,   0xe0000000, 29, result)
#define get_bits4(result) get_bitsX(4,   0xf0000000, 28, result)
#define get_bits5(result) get_bitsX(5,   0xf8000000, 27, result)
#define get_bits6(result) get_bitsX(6,   0xfc000000, 26, result)
#define get_bits7(result) get_bitsX(7,   0xfe000000, 25, result)
#define get_bits8(result) get_bitsX(8,   0xff000000, 24, result)
#define get_bits9(result) get_bitsX(9,   0xff800000, 23, result)
#define get_bits10(result) get_bitsX(10, 0xffc00000, 22, result)
#define get_bits11(result) get_bitsX(11, 0xffe00000, 21, result)
#define get_bits12(result) get_bitsX(12, 0xfff00000, 20, result)
#define get_bits14(result) get_bitsX(14, 0xfffc0000, 18, result)
#define get_bits16(result) get_bitsX(16, 0xffff0000, 16, result)
#define get_bits18(result) get_bitsX(18, 0xffffc000, 14, result)
#define get_bits32(result) get_bitsX(32, 0xffffffff,  0, result)

#define get_bitsn(num, result) get_bitsX((num), nBitMask[num], (32-(num)), result)

#ifdef NO_SANITY_CHECKS
#define show_bits32(result)                              		\
{                                                                       \
  if (vid_stream->bit_offset) {					        \
    result = vid_stream->curBits | (*(vid_stream->buffer+1) >>          \
	 (32 - vid_stream->bit_offset));                                \
  }                                                                     \
  else {                                                                \
    result = vid_stream->curBits;					\
  }                                                                     \
}

#define show_bitsX(num, mask, shift,  result)                           \
{                                                                       \
  int bO;                                                               \
  bO = vid_stream->bit_offset + num;                                    \
  if (bO > 32) {                                                        \
    bO -= 32;                                                           \
    result = ((vid_stream->curBits & mask) >> shift) |                  \
                (*(vid_stream->buffer+1) >> (shift + (num - bO)));      \
  }                                                                     \
  else {                                                                \
    result = ((vid_stream->curBits & mask) >> shift);                   \
  }                                                                     \
}

#else
#define show_bits32(result)                               		\
{                                                                       \
  /* Check for underflow. */                                            \
  if (vid_stream->buf_length < 2) {                                     \
    correct_underflow(vid_stream);                                      \
  }                                                                     \
  if (vid_stream->bit_offset) {						\
    result = vid_stream->curBits | (*(vid_stream->buffer+1) >>          \
    (32 - vid_stream->bit_offset));		                        \
  }                                                                     \
  else {                                                                \
    result = vid_stream->curBits;					\
  }                                                                     \
}

#define show_bitsX(num, mask, shift, result)                            \
{                                                                       \
  int bO;                                                               \
                                                                        \
  /* Check for underflow. */                                            \
  if (vid_stream->buf_length < 2) {                                     \
    correct_underflow(vid_stream);                                      \
  }                                                                     \
  bO = vid_stream->bit_offset + num;                                    \
  if (bO > 32) {                                                        \
    bO -= 32;                                                           \
    result = ((vid_stream->curBits & mask) >> shift) |                  \
                (*(vid_stream->buffer+1) >> (shift + (num - bO)));      \
  }                                                                     \
  else {                                                                \
    result = ((vid_stream->curBits & mask) >> shift);                   \
  }                                                                     \
}
#endif

#define show_bits1(result)  show_bitsX(1,  0x80000000, 31, result)
#define show_bits2(result)  show_bitsX(2,  0xc0000000, 30, result)
#define show_bits3(result)  show_bitsX(3,  0xe0000000, 29, result)
#define show_bits4(result)  show_bitsX(4,  0xf0000000, 28, result)
#define show_bits5(result)  show_bitsX(5,  0xf8000000, 27, result)
#define show_bits6(result)  show_bitsX(6,  0xfc000000, 26, result)
#define show_bits7(result)  show_bitsX(7,  0xfe000000, 25, result)
#define show_bits8(result)  show_bitsX(8,  0xff000000, 24, result)
#define show_bits9(result)  show_bitsX(9,  0xff800000, 23, result)
#define show_bits10(result) show_bitsX(10, 0xffc00000, 22, result)
#define show_bits11(result) show_bitsX(11, 0xffe00000, 21, result)
#define show_bits12(result) show_bitsX(12, 0xfff00000, 20, result)
#define show_bits13(result) show_bitsX(13, 0xfff80000, 19, result)
#define show_bits14(result) show_bitsX(14, 0xfffc0000, 18, result)
#define show_bits15(result) show_bitsX(15, 0xfffe0000, 17, result)
#define show_bits16(result) show_bitsX(16, 0xffff0000, 16, result)
#define show_bits17(result) show_bitsX(17, 0xffff8000, 15, result)
#define show_bits18(result) show_bitsX(18, 0xffffc000, 14, result)
#define show_bits19(result) show_bitsX(19, 0xffffe000, 13, result)
#define show_bits20(result) show_bitsX(20, 0xfffff000, 12, result)
#define show_bits21(result) show_bitsX(21, 0xfffff800, 11, result)
#define show_bits22(result) show_bitsX(22, 0xfffffc00, 10, result)
#define show_bits23(result) show_bitsX(23, 0xfffffe00,  9, result)
#define show_bits24(result) show_bitsX(24, 0xffffff00,  8, result)
#define show_bits25(result) show_bitsX(25, 0xffffff80,  7, result)
#define show_bits26(result) show_bitsX(26, 0xffffffc0,  6, result)
#define show_bits27(result) show_bitsX(27, 0xffffffe0,  5, result)
#define show_bits28(result) show_bitsX(28, 0xfffffff0,  4, result)
#define show_bits29(result) show_bitsX(29, 0xfffffff8,  3, result)
#define show_bits30(result) show_bitsX(30, 0xfffffffc,  2, result)
#define show_bits31(result) show_bitsX(31, 0xfffffffe,  1, result)

#define show_bitsn(num,result) show_bitsX((num), (0xffffffff << (32-(num))), (32-(num)), result)

#ifdef NO_SANITY_CHECKS
#define flush_bits32                                                  \
{                                                                     \
  UPDATE_COUNT(32);                                                   \
                                                                      \
  vid_stream->buffer++;                                               \
  vid_stream->buf_length--;                                           \
  vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
}


#define flush_bits(num)                                               \
{                                                                     \
  vid_stream->bit_offset += num;                                      \
                                                                      \
  UPDATE_COUNT(num);                                                  \
                                                                      \
  if (vid_stream->bit_offset & 0x20) {                                \
    vid_stream->bit_offset -= 32;                                     \
    vid_stream->buffer++;                                             \
    vid_stream->buf_length--;                                         \
    vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
  }                                                                   \
  else {                                                              \
    vid_stream->curBits <<= num;                                      \
  }                                                                   \
}
#else
#define flush_bits32                                                  \
{                                                                     \
  if (vid_stream  == NULL) {                                          \
    /* Deal with no vid stream here. */                               \
  }                                                                   \
                                                                      \
  if (vid_stream->buf_length < 2) {                                   \
    correct_underflow(vid_stream);                                    \
  }                                                                   \
                                                                      \
  UPDATE_COUNT(32);                                                   \
                                                                      \
  vid_stream->buffer++;                                               \
  vid_stream->buf_length--;                                           \
  vid_stream->curBits = *vid_stream->buffer  << vid_stream->bit_offset;\
}

#define flush_bits(num)                                               \
{                                                                     \
  if (vid_stream== NULL) {                                            \
    /* Deal with no vid stream here. */                               \
  }                                                                   \
                                                                      \
  if (vid_stream->buf_length < 2) {                                   \
    correct_underflow(vid_stream);                                    \
  }                                                                   \
                                                                      \
  UPDATE_COUNT(num);                                                  \
                                                                      \
  vid_stream->bit_offset += num;                                      \
                                                                      \
  if (vid_stream->bit_offset & 0x20) {                                \
    vid_stream->buf_length--;                                         \
    vid_stream->bit_offset -= 32;                                     \
    vid_stream->buffer++;                                             \
    vid_stream->curBits = *vid_stream->buffer << vid_stream->bit_offset;\
  }                                                                   \
  else {                                                              \
    vid_stream->curBits <<= num;                                      \
  }                                                                   \
}
#endif

#define UTIL2
