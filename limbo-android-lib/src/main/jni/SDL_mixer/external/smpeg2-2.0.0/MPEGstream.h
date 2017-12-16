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

/* The generic MPEG stream class */

#ifndef _MPEGSTREAM_H_
#define _MPEGSTREAM_H_

#include "SDL_stdinc.h"
#include "MPEGerror.h"
#include "MPEGvideo.h"
#include "MPEGaudio.h"
#include "MPEGlist.h"

#define AUDIO_STREAMID  0xc0
#define VIDEO_STREAMID  0xe0
#define SYSTEM_STREAMID 0xbb

struct MPEGstream_marker
{
    /* Data to mark part of the stream */
    MPEGlist * marked_buffer;
    Uint8 *marked_data;
    Uint8 *marked_stop;  
};

class MPEGstream
{
public:
    MPEGstream(class MPEGsystem * System, Uint8 Streamid);
    ~MPEGstream();

    /* Cleanup the buffers and reset the stream */
    void reset_stream(void);

    /* Rewind the stream */
    void rewind_stream(void);

    /* Go to the next packet in the stream */
    bool next_packet(bool recurse = true, bool update_timestamp = true);
 
    /* Mark a position in the data stream */
    MPEGstream_marker *new_marker(int offset);

    /* Jump to the marked position */
    bool seek_marker(MPEGstream_marker const * marker);

    /* Jump to last successfully marked position */
    void delete_marker(MPEGstream_marker * marker);

    /* Copy data from the stream to a local buffer */
    Uint32 copy_data(Uint8 *area, Sint32 size, bool short_read = false);

    /* Copy a byte from the stream */
    int copy_byte(void);

    /* Check for end of file or an error in the stream */
    bool eof(void) const;

    /* Insert a new packet at the end of the stream */
    void insert_packet(Uint8 * data, Uint32 size, double timestamp=-1);

    /* Check for unused buffers and free them */
    void garbage_collect(void);

    /* Enable or disable the stream */
    void enable(bool toggle);

    /* Get stream time */
    double time();

    Uint32 pos;

    Uint8 streamid;

protected:
    Uint8 *data;
    Uint8 *stop;

    Uint32 preread_size;

    class MPEGsystem * system;
    MPEGlist * br;
    bool cleareof;
    bool enabled;

    SDL_mutex * mutex;

    /* Get a buffer from the stream */
    bool next_system_buffer(void);

public:
    /* "pos" where "timestamp" belongs */
    Uint32 timestamp_pos;
    double timestamp;
};

#endif /* _MPEGSTREAM_H_ */


