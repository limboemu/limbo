/*
 * readfile.c --
 *
 *       Procedures concerned with reading data and parsing 
 *       start codes from MPEG files.
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

#include <sys/types.h>
#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#endif
#include <string.h>
#include <signal.h>

#include "SDL_endian.h"
#include "video.h"
#include "proto.h"
#include "util.h"
#include "dither.h"


/*
   Changes to make the code reentrant:
      deglobalized: totNumFrames, realTimeStart, stream id vars, Prase_done,
         swap, seekValue, input, EOF_flag, ReadPacket statics, sys_layer,
	 bitOffset, bitLength, bitBuffer, curVidStream
   removed: [aud,sys,vid]Bytes
   Additional changes:
      get rid on ANSI C complaints about shifting
   -lsh@cs.brown.edu (Loring Holden)
 */

/*
 *--------------------------------------------------------------
 *
 * get_more_data --
 *
 *	Called by get_more_data to read in more data from
 *      video MPG files (non-system-layer)
 *
 * Results:
 *	Input buffer updated, buffer length updated.
 *      Returns 1 if data read, 0 if EOF, -1 if error.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

int 
get_more_data( VidStream* vid_stream )
{
  unsigned int *buf_start;
  int length, num_read, i;
  unsigned int request;
  unsigned char *buffer, *mark;
  unsigned int *lmark;
  Sint32 timestamp_offset;
  Uint32 data_pos;
     
  if (vid_stream->EOF_flag) return 0;
  
  buf_start = vid_stream->buf_start;
  length = vid_stream->buf_length;
  buffer = (unsigned char *) vid_stream->buffer;
  
  if (length > 0) {
    memcpy((unsigned char *) buf_start, buffer, (unsigned int) (length*4));
    mark = ((unsigned char *) (buf_start + length));
  }
  else {
    mark = (unsigned char *) buf_start;
    length = 0;
  }
  
  request = (vid_stream->max_buf_length-length)*4;
  
  data_pos = vid_stream->_smpeg->mpeg->pos;  
  num_read = vid_stream->_smpeg->mpeg->copy_data((Uint8 *)mark, request);

  vid_stream->timestamp = vid_stream->_smpeg->mpeg->timestamp;
  timestamp_offset = vid_stream->_smpeg->mpeg->timestamp_pos - data_pos;
  vid_stream->timestamp_mark = (unsigned int *)(mark+timestamp_offset);
  vid_stream->timestamp_used = false;

  /* Paulo Villegas - 26/1/1993: Correction for 4-byte alignment */
  {
    int num_read_rounded;
    unsigned char *index;
    
    num_read_rounded = 4*(num_read/4);
    
    /* this can happen only if num_read<request; i.e. end of file reached */
    if ( num_read_rounded < num_read ) { 
 	  num_read_rounded = 4*( num_read/4+1 );

 	    /* fill in with zeros */
 	  for( index=mark+num_read; index<mark+num_read_rounded; *(index++)=0 );

 	  /* advance to the next 4-byte boundary */
 	  num_read = num_read_rounded;
    }
  }
  
  if (num_read < 0) {
    return -1;
  }
  if (num_read == 0) {
    vid_stream->buffer = buf_start;
    
    /* Make 32 bits after end equal to 0 and 32
     * bits after that equal to seq end code
     * in order to prevent messy data from infinite
     * recursion.
     */
    
    *(buf_start + length) = 0x0;
    *(buf_start + length+1) = SEQ_END_CODE;
    
    vid_stream->EOF_flag = 1;
    return 0;
  }
  
  lmark = (unsigned int *) mark;
  
  num_read = num_read/4;
  
  for (i = 0; i < num_read; i++) {
    *lmark = SDL_SwapBE32(*lmark);
    lmark++;
  }
  
  vid_stream->buffer = buf_start;
  vid_stream->buf_length = length + num_read;
  
  return 1;
}

/* EOF */
