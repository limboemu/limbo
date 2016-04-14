/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* MPEG video frame */

#ifndef _MPEGFRAME_H_
#define _MPEGFRAME_H_

/* A YV12 format video frame */
typedef struct SMPEG_Frame {
  unsigned int w, h;
  unsigned int image_width;
  unsigned int image_height;
  Uint8 *image;
} SMPEG_Frame;

#endif /* _MPEGFRAME_H_ */
