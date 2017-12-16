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


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "SDL_timer.h"

#include "MPEGring.h"


MPEG_ring:: MPEG_ring( Uint32 size, Uint32 count )
{
    Uint32 tSize;

    /* Set up the 'ring' pointer for all the old C code */
    ring = this;

    tSize = (size + sizeof(Uint32)) * count;
    if( tSize )
    {
        ring->begin = (Uint8 *) malloc( tSize );
        ring->timestamps = (double *) malloc( sizeof(double)*count );
    }
    else
        ring->begin = 0;

    if( ring->begin && count )
    {
        ring->end   = ring->begin + tSize;
        ring->read  = ring->begin;
        ring->write = ring->begin;
        ring->timestamp_read  = timestamps;
        ring->timestamp_write = timestamps;
        ring->bufSize  = size;
        
        ring->readwait = SDL_CreateSemaphore(0);
        ring->writewait = SDL_CreateSemaphore(count);
    }
    else
    {
        ring->end   = 0;
        ring->read  = 0;
        ring->write = 0;
        ring->bufSize  = 0;

        ring->readwait = 0;
    }

    if ( ring->begin && ring->readwait && ring->writewait ) {
        ring->active = 1;
    }
}

/* Release any waiting threads on the ring so they can be cleaned up.
   The ring isn't valid after this call, so when threads are done you
   should call MPRing_sdelete() on the ring.
 */
void
MPEG_ring:: ReleaseThreads( void )
{
    /* Let the threads know that the ring is now inactive */
    ring->active = 0;

    if ( ring->readwait ) {
        while ( SDL_SemValue(ring->readwait) == 0 ) {
            SDL_SemPost(ring->readwait);
        }
    }
    if ( ring->writewait ) {
        while ( SDL_SemValue(ring->writewait) == 0 ) {
            SDL_SemPost(ring->writewait);
        }
    }
}


MPEG_ring:: ~MPEG_ring( void )
{
    if( ring )
    {
        /* Free up the semaphores */
        ReleaseThreads();

	/* Destroy the semaphores */
	if( ring->readwait )
	{
	    SDL_DestroySemaphore( ring->readwait );
	    ring->readwait = 0;
	}
	if( ring->writewait )
	{
	    SDL_DestroySemaphore( ring->writewait );
	    ring->writewait = 0;
	}

        /* Free data buffer */
        if ( ring->begin ) {
            free( ring->begin );
            free( ring->timestamps );
            ring->begin = 0;
            ring->timestamps = 0;
        }
    }
}

/*
  Returns free buffer of ring->bufSize to be filled with data.
  Zero is returned if there is no free buffer.
*/

Uint8 *
MPEG_ring:: NextWriteBuffer( void )
{
    Uint8 *buffer;

    buffer = 0;
    if ( ring->active ) {
	//printf("Waiting for write buffer (%d available)\n", SDL_SemValue(ring->writewait));
        SDL_SemWait(ring->writewait);
	//printf("Finished waiting for write buffer\n");
	if ( ring->active ) {
            buffer = ring->write + sizeof(Uint32);
        }
    }
    return buffer;
}


/*
  Call this when the buffer returned from MPRing_nextWriteBuffer() has
  been filled.  The passed length must not be larger than ring->bufSize.
*/

void
MPEG_ring:: WriteDone( Uint32 len, double timestamp)
{
    if ( ring->active ) {
#ifdef NO_GRIFF_MODS
        assert(len <= ring->bufSize);
#else
	if ( len > ring->bufSize )
            len = ring->bufSize;
#endif
        *((Uint32*) ring->write) = len;

        ring->write += ring->bufSize + sizeof(Uint32);
        *(ring->timestamp_write++) = timestamp;
        if( ring->write >= ring->end )
        {
            ring->write = ring->begin;
            ring->timestamp_write = ring->timestamps;
        }
//printf("Finished write buffer of %u bytes, making available for reads (%d+1 available for reads)\n", len, SDL_SemValue(ring->readwait));
        SDL_SemPost(ring->readwait);
    }
}


/*
  Returns the number of bytes in the next ring buffer and sets the buffer
  pointer to this buffer.  If there is no buffer ready then the buffer
  pointer is not changed and zero is returned.
*/

Uint32
MPEG_ring:: NextReadBuffer( Uint8** buffer )
{
    Uint32 size;

    size = 0;
    if ( ring->active ) {
        /* Wait for a buffer to become available */
//printf("Waiting for read buffer (%d available)\n", SDL_SemValue(ring->readwait));
        SDL_SemWait(ring->readwait);
//printf("Finished waiting for read buffer\n");
	if ( ring->active ) {
            size = *((Uint32*) ring->read);
            *buffer = ring->read + sizeof(Uint32);
        }
    }
    return size;
}

/*
  Call this when you have used some of the buffer previously returned by
  MPRing_nextReadBuffer(), and want to update it so the rest of the data
  is returned with the next call to MPRing_nextReadBuffer().
*/

double
MPEG_ring:: ReadTimeStamp(void)
{
  if(ring->active)
    return *ring->timestamp_read;
  return(0);
}

void
MPEG_ring:: ReadSome( Uint32 used )
{
    Uint8 *data;
    Uint32 oldlen;
    Uint32 newlen;

    if ( ring->active ) {
        data = ring->read + sizeof(Uint32);
        oldlen = *((Uint32*) ring->read);
        newlen = oldlen - used;
        memmove(data, data+used, newlen);
        *((Uint32*) ring->read) = newlen;
//printf("Reusing read buffer (%d+1 available)\n", SDL_SemValue(ring->readwait));
        SDL_SemPost(ring->readwait);
    }
}

/*
  Call this when the buffer returned from MPRing_nextReadBuffer() is no
  longer needed.  This assumes there is only one read thread and one write
  thread for a particular ring buffer.
*/

void
MPEG_ring:: ReadDone( void )
{
    if ( ring->active ) {
        ring->read += ring->bufSize + sizeof(Uint32);
        ring->timestamp_read++;
        if( ring->read >= ring->end )
        {
            ring->read = ring->begin;
            ring->timestamp_read = ring->timestamps;
        }
//printf("Finished read buffer, making available for writes (%d+1 available for writes)\n", SDL_SemValue(ring->writewait));
        SDL_SemPost(ring->writewait);
    }
}


/* EOF */
