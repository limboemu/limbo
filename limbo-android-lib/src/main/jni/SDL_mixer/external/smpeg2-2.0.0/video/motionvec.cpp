/*
 * motionvector.c --
 *
 *       Procedures to compute motion vectors.
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

#include "util.h"
#include "video.h"
#include "proto.h"

/*
   Changes to make the code reentrant:
      deglobalize curVidStream
   Additional changes:
      none
   -lsh@cs.brown.edu (Loring Holden)
*/


/*
 *--------------------------------------------------------------
 *
 * ComputeVector --
 *
 *	Computes motion vector given parameters previously parsed
 *      and reconstructed.
 *
 * Results:
 *      Reconstructed motion vector info is put into recon_* parameters
 *      passed to this function. Also updated previous motion vector
 *      information.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

#define ComputeVector(recon_right_ptr, recon_down_ptr, recon_right_prev, recon_down_prev, f, full_pel_vector, motion_h_code, motion_v_code, motion_h_r, motion_v_r)				\
									\
{									\
  int comp_h_r, comp_v_r;						\
  int right_little, right_big, down_little, down_big;			\
  int max, min, new_vector;						\
									\
  /* The following procedure for the reconstruction of motion vectors 	\
     is a direct and simple implementation of the instructions given	\
     in the mpeg December 1991 standard draft. 				\
  */									\
									\
  if (f == 1 || motion_h_code == 0)					\
    comp_h_r = 0;							\
  else 									\
    comp_h_r = f - 1 - motion_h_r;					\
									\
  if (f == 1 || motion_v_code == 0)					\
    comp_v_r = 0;							\
  else 									\
    comp_v_r = f - 1 - motion_v_r;					\
									\
  right_little = motion_h_code * f;					\
  if (right_little == 0)						\
    right_big = 0;							\
  else {								\
    if (right_little > 0) {						\
      right_little = right_little - comp_h_r;				\
      right_big = right_little - 32 * f;				\
    }									\
    else {								\
      right_little = right_little + comp_h_r;				\
      right_big = right_little + 32 * f;				\
    }									\
  }									\
									\
  down_little = motion_v_code * f;					\
  if (down_little == 0)							\
    down_big = 0;							\
  else {								\
    if (down_little > 0) {						\
      down_little = down_little - comp_v_r;				\
      down_big = down_little - 32 * f;					\
    }									\
    else {								\
      down_little = down_little + comp_v_r;				\
      down_big = down_little + 32 * f;					\
    }									\
  }									\
  									\
  max = 16 * f - 1;							\
  min = -16 * f;							\
									\
  new_vector = recon_right_prev + right_little;				\
									\
  if (new_vector <= max && new_vector >= min)				\
    *recon_right_ptr = recon_right_prev + right_little;			\
                      /* just new_vector */				\
  else									\
    *recon_right_ptr = recon_right_prev + right_big;			\
  recon_right_prev = *recon_right_ptr;					\
  if (full_pel_vector)							\
    *recon_right_ptr = *recon_right_ptr << 1;				\
									\
  new_vector = recon_down_prev + down_little;				\
  if (new_vector <= max && new_vector >= min)				\
    *recon_down_ptr = recon_down_prev + down_little;			\
                      /* just new_vector */				\
  else									\
    *recon_down_ptr = recon_down_prev + down_big;			\
  recon_down_prev = *recon_down_ptr;					\
  if (full_pel_vector)							\
    *recon_down_ptr = *recon_down_ptr << 1;				\
}


/*
 *--------------------------------------------------------------
 *
 * ComputeForwVector --
 *
 *	Computes forward motion vector by calling ComputeVector
 *      with appropriate parameters.
 *
 * Results:
 *	Reconstructed motion vector placed in recon_right_for_ptr and
 *      recon_down_for_ptr.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

void ComputeForwVector( int* recon_right_for_ptr, int* recon_down_for_ptr,
                        VidStream* the_stream )
{

  Pict *picture;
  Macroblock *mblock;

  picture = &(the_stream->picture);
  mblock = &(the_stream->mblock);

  ComputeVector(recon_right_for_ptr, recon_down_for_ptr,
		mblock->recon_right_for_prev, 
		mblock->recon_down_for_prev,
		(int) picture->forw_f,
		picture->full_pel_forw_vector,
		mblock->motion_h_forw_code, mblock->motion_v_forw_code,
		mblock->motion_h_forw_r, mblock->motion_v_forw_r); 
}


/*
 *--------------------------------------------------------------
 *
 * ComputeBackVector --
 *
 *	Computes backward motion vector by calling ComputeVector
 *      with appropriate parameters.
 *
 * Results:
 *	Reconstructed motion vector placed in recon_right_back_ptr and
 *      recon_down_back_ptr.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

void ComputeBackVector( int* recon_right_back_ptr, int* recon_down_back_ptr,
                        VidStream* the_stream )
{
  Pict *picture;
  Macroblock *mblock;

  picture = &(the_stream->picture);
  mblock = &(the_stream->mblock);

  ComputeVector(recon_right_back_ptr, recon_down_back_ptr,
		mblock->recon_right_back_prev, 
		mblock->recon_down_back_prev,
		(int) picture->back_f, 
		picture->full_pel_back_vector,
		mblock->motion_h_back_code, mblock->motion_v_back_code,
		mblock->motion_h_back_r, mblock->motion_v_back_r); 

}


/* EOF */
