#include <stdlib.h>        /* for realloc() */
#include <string.h>        /* for memmove() */
#include <errno.h>
#include <assert.h>
#ifdef WIN32
#include <sys/types.h>
#include <io.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifndef __BEOS__
#include <sys/select.h>
#endif
#endif

#include "MPEGsystem.h"
#include "MPEGstream.h"

/* Define this if you want to debug the system stream parsing */
//#define DEBUG_SYSTEM

/* Define this if you want to use a separate thread for stream decoding */
//#define USE_SYSTEM_THREAD

Uint8 const PACKET_CODE[]       = { 0x00, 0x00, 0x01, 0xba };
Uint8 const PACKET_MASK[]       = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END_CODE[]          = { 0x00, 0x00, 0x01, 0xb9 };
Uint8 const END_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END2_CODE[]         = { 0x00, 0x00, 0x01, 0xb7 };
Uint8 const END2_MASK[]         = { 0xff, 0xff, 0xff, 0xff };
Uint8 const VIDEOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xe0 };
Uint8 const VIDEOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xe0 };
Uint8 const AUDIOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xc0 };
Uint8 const AUDIOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xc0 };
Uint8 const PADSTREAM_CODE[]    = { 0x00, 0x00, 0x01, 0xbe };
Uint8 const PADSTREAM_MASK[]    = { 0xff, 0xff, 0xff, 0xff };
Uint8 const SYSTEMSTREAM_CODE[] = { 0x00, 0x00, 0x01, 0xbb };
Uint8 const SYSTEMSTREAM_MASK[] = { 0xff, 0xff, 0xff, 0xff };
Uint8 const USERSTREAM_CODE[]   = { 0x00, 0x00, 0x01, 0xb2 };
Uint8 const USERSTREAM_MASK[]   = { 0xff, 0xff, 0xff, 0xff };
Uint8 const VIDEO_CODE[]        = { 0x00, 0x00, 0x01, 0xb3 };
Uint8 const VIDEO_MASK[]        = { 0xff, 0xff, 0xff, 0xff };
Uint8 const AUDIO_CODE[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const AUDIO_MASK[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const GOP_CODE[]          = { 0x00, 0x00, 0x01, 0xb8 };
Uint8 const GOP_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const PICTURE_CODE[]      = { 0x00, 0x00, 0x01, 0x00 };
Uint8 const PICTURE_MASK[]      = { 0xff, 0xff, 0xff, 0xff };
Uint8 const SLICE_CODE[]        = { 0x00, 0x00, 0x01, 0x01 };
Uint8 const SLICE_MASK[]        = { 0xff, 0xff, 0xff, 0x00 };
Uint8 const ZERO_CODE[]         = { 0x00, 0x00, 0x00, 0x00 };
Uint8 const FULL_MASK[]         = { 0xff, 0xff, 0xff, 0xff };

/* The size is arbitrary but should be sufficient to contain */
/* two MPEG packets and reduce disk (or network) access.     */
// Hiroshi Yamashita notes that the original size was too large
#define MPEG_BUFFER_SIZE (16 * 1024)

/* The granularity (2^LG2_GRANULARITY) determine what length of read data */
/* will be a multiple of, e.g. setting LG2_GRANULARITY to 12 will make    */
/* read size (when calling the read function in Read method) to be a      */
/* multiple of 4096                                                       */
#define LG2_GRANULARITY 12
#define READ_ALIGN(x) (((x) >> LG2_GRANULARITY) << LG2_GRANULARITY)

/* This defines the maximum number of buffers that can be preread */
/* It is to prevent filling the whole memory with buffers on systems */
/* where read is immediate such as in the case of files */
#define MPEG_BUFFER_MAX 16

/* Timeout before read fails */
#define READ_TIME_OUT 1000000

/* timestamping */
#define FLOAT_0x10000 (double)((Uint32)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ 90000L

/* This work only on little endian systems */
/*
#define REV(x) ((((x)&0x000000FF)<<24)| \
                (((x)&0x0000FF00)<< 8)| \
                (((x)&0x00FF0000)>> 8)| \
                (((x)&0xFF000000)>>24))
#define MATCH4(x, y, m) (((x) & REV(m)) == REV(y))
*/
const int audio_frequencies[2][3]=
{
  {44100,48000,32000}, // MPEG 1
  {22050,24000,16000}  // MPEG 2
};

const int audio_bitrate[2][3][15]=
{
  // MPEG 1
  {{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
   {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
   {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}},

  // MPEG 2
  {{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}}
};

/* Match two 4-byte codes */
static inline bool Match4(Uint8 const code1[4], Uint8 const code2[4], Uint8 const mask[4])
{
  return( ((code1[0] & mask[0]) == (code2[0] & mask[0])) &&
	  ((code1[1] & mask[1]) == (code2[1] & mask[1])) &&
	  ((code1[2] & mask[2]) == (code2[2] & mask[2])) &&
	  ((code1[3] & mask[3]) == (code2[3] & mask[3])) );
}

static inline double read_time_code(Uint8 *pointer)
{ 
  double timestamp;
  Uint8 hibit; Uint32 lowbytes;

  hibit = (pointer[0]>>3)&0x01;
  lowbytes = (((Uint32)pointer[0] >> 1) & 0x03) << 30;
  lowbytes |= (Uint32)pointer[1] << 22;
  lowbytes |= ((Uint32)pointer[2] >> 1) << 15;
  lowbytes |= (Uint32)pointer[3] << 7;
  lowbytes |= ((Uint32)pointer[4]) >> 1;
  timestamp = (double)hibit*FLOAT_0x10000*FLOAT_0x10000+(double)lowbytes;
  timestamp /= STD_SYSTEM_CLOCK_FREQ;

  return timestamp;
}

/* Return true if there is a valid audio header at the beginning of pointer */
static inline Uint32 audio_header(Uint8 * pointer, Uint32 * framesize, double * frametime)
{
  Uint32 layer, version, frequency, bitrate, mode, padding, size;

  if(((pointer[0] & 0xff) != 0xff) || // No sync bits
     ((pointer[1] & 0xf0) != 0xf0) || //
     ((pointer[2] & 0xf0) == 0x00) || // Bitrate is 0
     ((pointer[2] & 0xf0) == 0xf0) || // Bitrate is 15
     ((pointer[2] & 0x0c) == 0x0c) || // Frequency is 3
     ((pointer[1] & 0x06) == 0x00))   // Layer is 4
     return(0);

  layer = 4 - (((pointer)[1] >> 1) & 3);
  version = (((pointer)[1] >> 3) & 1) ^ 1;
  padding = ((pointer)[2] >> 1) & 1;
  frequency = audio_frequencies[version][(((pointer)[2] >> 2) & 3)];
  bitrate = audio_bitrate[version][layer-1][(((pointer)[2] >> 4) & 15)];
  mode = ((pointer)[3] >> 6) & 3;

  if(layer==1)
  {
      size = 12000 * bitrate / frequency;
      if(frequency==0 && padding) size++;
      size <<= 2;
  }
  else
  {
      size = 144000 * bitrate / (frequency<<version);
      if(padding) size++;
  }
  if(framesize) *framesize = size;
  if(frametime) *frametime = 8.0 * size / (1000. * bitrate);
  
  return(4); /* Audio header size */
}

/* Search the next valid audio header */
static inline bool audio_aligned(Uint8 *pointer, Uint32 size)
{
    Uint32 i, s;

#if 1 // Sam 10/27 - Don't choke on partially corrupt data
    size = 4;
#endif
    /* Check on all data available that next headers are aligned too */
    for(i = 0; i + 3 < size && audio_header(pointer+i, &s, 0); i+=s);

    if(i + 3 < size)
	return(false);
    else
	return(true);
}

/* Return true if there is a valid sequence header at the beginning of pointer */
static inline Uint32 sequence_header(Uint8 * pointer, Uint32 size, double * _frametime)
{
  double frametime;
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, VIDEO_CODE, VIDEO_MASK)) 
    return(0); /* Not a sequence start code */
  
  /* Parse the sequence header information */
  if((header_size+=8) >= size) return(0);
  switch(pointer[7]&0xF)                /*  4 bits of fps */
  {
    case 1: frametime = 1.0/23.97; break;
    case 2: frametime = 1.0/24.00; break;
    case 3: frametime = 1.0/25.00; break;
    case 4: frametime = 1.0/29.97; break;
    case 5: frametime = 1.0/30.00; break;
    case 6: frametime = 1.0/50.00; break;
    case 7: frametime = 1.0/59.94; break;
    case 8: frametime = 1.0/60.00; break;
    case 9: frametime = 1.0/15.00; break;
    default: frametime = 1.0/30.00; break;
  }

  if(_frametime) *_frametime = frametime;
  
  return(header_size); /* sequence header size */
}

/* Return true if there is a valid gop header at the beginning of pointer */
static inline Uint32 gop_header(Uint8 * pointer, Uint32 size, double * timestamp)
{
  Uint32 header_size;
  Uint32 hour, min, sec, frame;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, GOP_CODE, GOP_MASK)) 
    return(0); /* Not a gop start code */
  
  /* Parse the gop header information */
  hour = (pointer[4] >> 2) & 31;
  min = ((pointer[4] & 3) << 4) | ((pointer[5] >> 4) & 15);
  sec = ((pointer[5] & 7) << 3) | ((pointer[6] >> 5) & 7);
  frame = ((pointer[6] & 31) << 1) | ((pointer[7] >> 7) & 1);
  if((header_size+=4) >= size) return(0);

  if(timestamp) *timestamp = sec + 60.*min + 3600.*hour;
  
  return(header_size); /* gop header size */
}

/* Return true if there is a valid picture header at the beginning of pointer */
static inline Uint32 picture_header(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);

  if(!Match4(pointer, PICTURE_CODE, PICTURE_MASK)) 
    return(0); /* Not a picture start code */
  
  /* Parse the picture header information */
  if((header_size+=4) >= size) return(0);
  
  return(header_size); /* picture header size */
}

/* Return true if there is a valid slice header at the beginning of pointer */
static inline Uint32 slice_header(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!(Match4(pointer, SLICE_CODE, SLICE_MASK) && 
       pointer[3] >= 0x01 && pointer[3] <= 0xaf)) 
    return(0); /* Not a slice start code */
  
  return(header_size); /* slice header size */
}

/* Return true if there is a valid packet header at the beginning of pointer */
static inline Uint32 packet_header(Uint8 * pointer, Uint32 size, double * _timestamp)
{
  double timestamp;
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, PACKET_CODE, PACKET_MASK)) 
    return(0); /* Not a packet start code */

  /* Parse the packet information */
  if((header_size+=8) >= size) return(0);
  timestamp = read_time_code(pointer+4);

  if(_timestamp) *_timestamp = timestamp;
  
  return(header_size); /* packet header size */
}

/* Return true if there is a valid stream header at the beginning of pointer */
static inline Uint32 stream_header(Uint8 * pointer, Uint32 size, Uint32 * _packet_size, Uint8 * _stream_id, double * _stream_timestamp, double timestamp)
{
  Uint32 header_size, packet_size;
  Uint8 stream_id;
  double stream_timestamp;

  header_size = 0;
  if((header_size += 4) >= size) return(0); 

  if(!Match4(pointer, SYSTEMSTREAM_CODE, SYSTEMSTREAM_MASK) &&
     !Match4(pointer, AUDIOSTREAM_CODE, AUDIOSTREAM_MASK)   &&
     !Match4(pointer, VIDEOSTREAM_CODE, VIDEOSTREAM_MASK)   &&
     !Match4(pointer, PADSTREAM_CODE, PADSTREAM_MASK)       &&
     !Match4(pointer, USERSTREAM_CODE, USERSTREAM_MASK))
    return(0); /* Unknown encapsulated stream */

  /* Parse the stream packet */
  /* Get the stream id, and packet length */
  stream_id = pointer[3];

  pointer += 4;
  if((header_size += 2) >= size) return(0); 
  packet_size = (((unsigned short) pointer[0] << 8) | pointer[1]);
  pointer += 2;

  /* Skip stuffing bytes */
  while ( pointer[0] == 0xff ) {
      ++pointer;
      if((++header_size) >= size) return(0); 
      --packet_size;
  }
  if ( (pointer[0] & 0x40) == 0x40 ) {
      pointer += 2;
      if((header_size += 2) >= size) return(0); 
      packet_size -= 2;
  }
  if ( (pointer[0] & 0x20) == 0x20 ) {
      /* get the PTS */
      stream_timestamp = read_time_code(pointer);
      /* we don't care about DTS */
      if ( (pointer[0] & 0x30) == 0x30 ){
	pointer += 5;
	if((header_size += 5) >= size) return(0); 
	packet_size -= 5;
      }
      pointer += 4;
      if((header_size += 4) >= size) return(0); 
      packet_size -= 4;
  }
  else if ( pointer[0] != 0x0f && pointer[0] != 0x80)
      return(0);      /* not a valid header */
  else
      stream_timestamp = timestamp;
    
  if((++header_size) >= size) return(0); 
  --packet_size;

  if(_packet_size) *_packet_size = packet_size;
  if(_stream_id) *_stream_id = stream_id;
  if(_stream_timestamp) *_stream_timestamp = stream_timestamp;
  
  return(header_size);
}

/* Search the next valid audio header */
static inline bool system_aligned(Uint8 *pointer, Uint32 size)
{
    Uint32 i, s;

    /* Check that packet contains at least one stream */
    i = 0;
    while((s = packet_header(pointer+i, size-i, 0)) != 0)
      if((i+=s) >= size) return(true);

    if((s = stream_header(pointer+i, size-i, 0, 0, 0, 0)) != 0)
      return(true);
    else
      return(false);
}

/* Skip possible zeros at the beggining of the packet */
Uint32 skip_zeros(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;
  Uint8 const one[4]  = {0x00,0x00,0x00,0x01};

  if(!size) return(0);

  header_size = 0;
  while(Match4(pointer, ZERO_CODE, FULL_MASK))
  {
    pointer++;
    if((++header_size) >= size - 4) return(0); 

    if(Match4(pointer, one, FULL_MASK))
    {
      pointer++;
      if((++header_size) >= size - 4) return(0); 
    }
  }
  return(header_size);
}

MPEGsystem::MPEGsystem(SDL_RWops *mpeg_source)
{
  source = mpeg_source;

  /* Create a new buffer for reading */
  read_buffer = new Uint8[MPEG_BUFFER_SIZE];

  /* Create a mutex to avoid concurrent access to the stream */
  system_mutex = SDL_CreateMutex();
  request_wait = SDL_CreateSemaphore(0);

  /* Invalidate the read buffer */
  pointer = read_buffer;
  read_size = 0;
  read_total = 0;
  packet_total = 0;
  endofstream = errorstream = false;
  frametime = 0.0;
  stream_timestamp = 0.0;

  /* Create an empty stream list */
  stream_list = 
    (MPEGstream **) malloc(sizeof(MPEGstream *));
  stream_list[0] = 0;

  /* Create the system stream and add it to the list */
  if(!get_stream(SYSTEM_STREAMID))
    add_stream(new MPEGstream(this, SYSTEM_STREAMID));

  timestamp = 0.0;
  timedrift = 0.0;
  skip_timestamp = -1;
  system_thread_running = false;
  system_thread = 0;

  /* Search the MPEG for the first header */
  if(!seek_first_header())
  {
    errorstream = true;
    SetError("Could not find the beginning of MPEG data\n");
    return;
  }

#ifdef USE_SYSTEM_THREAD
  /* Start the system thread */
  system_thread = SDL_CreateThread(SystemThread, this);

  /* Wait for the thread to start */
  while(!system_thread_running && !Eof())
    SDL_Delay(1);
#else
  system_thread_running = true;
#endif

  /* Look for streams */
  int tries = 0;
  do
  {
    RequestBuffer();
    Wait();
    if ( tries++ < 20 ) {  // Adjust this to catch more streams
      if ( exist_stream(VIDEO_STREAMID, 0xF0) &&
	   exist_stream(AUDIO_STREAMID, 0xF0) ) {
        break;
      }
    } else {
      if ( exist_stream(VIDEO_STREAMID, 0xF0) ||
	   exist_stream(AUDIO_STREAMID, 0xF0) ) {
        break;
      }
    }
  }
  while(!Eof());
}

MPEGsystem::~MPEGsystem()
{
  MPEGstream ** list;

  /* Kill the system thread */
  Stop();

  SDL_DestroySemaphore(request_wait);
  SDL_DestroyMutex(system_mutex);

  /* Delete the streams */
  for(list = stream_list; *list; list ++)
    delete *list;

  free(stream_list);

  /* Delete the read buffer */
  delete[] read_buffer;
}

MPEGstream ** MPEGsystem::GetStreamList()
{
  return(stream_list);
}

void MPEGsystem::Read()
{
  int remaining;
  int timeout;

  /* Lock to prevent concurrent access to the stream */
  SDL_mutexP(system_mutex);

  timeout = READ_TIME_OUT;
  remaining = read_buffer + read_size - pointer;

  /* Only read data if buffer is rather empty */
  if(remaining < MPEG_BUFFER_SIZE / 2)
  {
    if(remaining < 0)
    {
      /* Hum.. we'd better stop if we have already read past the buffer size */
      errorstream = true;
      SDL_mutexV(system_mutex);
      return;
    }

    /* Replace unread data at the beginning of the stream */
    memmove(read_buffer, pointer, remaining);

#ifdef NO_GRIFF_MODS
    read_size = SDL_RWread(source,
                    read_buffer + remaining, 
                    READ_ALIGN(MPEG_BUFFER_SIZE - remaining));
    if(read_size < 0)
    {
      perror("Read");
      errorstream = true;
      SDL_mutexV(system_mutex);
      return;
    }
#else
    /* Read new data */
    int bytes_read    = 0;
    int buffer_offset = remaining;
    int bytes_to_read = READ_ALIGN(MPEG_BUFFER_SIZE - remaining);
    int read_at_once = 0;

    read_size = 0;
    do
    {
      read_at_once = 
        SDL_RWread(source, read_buffer + buffer_offset, 1, bytes_to_read );

      if(read_at_once < 0)
      {
        perror("Read");
        errorstream = true;
        SDL_mutexV(system_mutex);
        return;
      }
      else
      {
        bytes_read    += read_at_once;
        buffer_offset += read_at_once;
        read_size     += read_at_once;
        bytes_to_read -= read_at_once;
      }
    }
    while( read_at_once>0 && bytes_to_read>0 );
#endif

    read_total += read_size;

    packet_total ++;

    if((MPEG_BUFFER_SIZE - remaining) != 0 && read_size <= 0)
    {
      if(read_size != 0)
      {
	errorstream = true;
	SDL_mutexV(system_mutex);
	return;
      }
    }

    read_size += remaining;

    /* Move the pointer */
    pointer = read_buffer;  

    if(read_size == 0)
    {
      /* There is no more data */
      endofstream = true;
      SDL_mutexV(system_mutex);
      return;
    }
  }

  SDL_mutexV(system_mutex);
}

/* ASSUME: stream_list[0] = system stream */
/*         packet length < MPEG_BUFFER_SIZE */
Uint8 MPEGsystem::FillBuffer()
{
  Uint8 stream_id;
  Uint32 packet_size;
  Uint32 header_size;

  /* - Read a new packet - */
  Read();

  if(Eof())
  {
    RequestBuffer();
    return(0);
  }

  pointer += skip_zeros(pointer, read_buffer + read_size - pointer); 

  if((header_size = packet_header(pointer, read_buffer + read_size - pointer, &timestamp)) != 0)
  {
      pointer += header_size;
      stream_list[0]->pos += header_size;
#ifdef DEBUG_SYSTEM
      fprintf(stderr, "MPEG packet header time: %lf\n", timestamp);
#endif
  }

  if((header_size = stream_header(pointer, read_buffer + read_size - pointer, &packet_size, &stream_id, &stream_timestamp, timestamp)) != 0)
  {
      pointer += header_size;
      stream_list[0]->pos += header_size;
#ifdef DEBUG_SYSTEM
      fprintf(stderr, "[%d] MPEG stream header [%d|%d] id: %d streamtime: %lf\n", read_total - read_size + (pointer - read_buffer),
header_size, packet_size, stream_id, stream_timestamp);
#endif
  }
  else
  if(Match4(pointer, END_CODE, END_MASK) ||
     Match4(pointer, END2_CODE, END2_MASK))
  {
    /* End codes belong to video stream */
#ifdef DEBUG_SYSTEM
    fprintf(stderr, "[%d] MPEG end code\n", read_total - read_size + (pointer - read_buffer));
#endif
    stream_id = exist_stream(VIDEO_STREAMID, 0xF0);
    packet_size = 4;
  }
  else
  {
    stream_id = stream_list[0]->streamid;

    if(!stream_list[1])
    { 
      //Uint8 * packet_end;

      packet_size = 0;

      /* There is no system info in the stream */

      /* If we're still a system stream, morph to an audio */
      /* or video stream */

      /* Sequence header -> gives framerate */
      while((header_size = sequence_header(pointer+packet_size, read_buffer + read_size - pointer - packet_size, &frametime)) != 0)
      {
	stream_id = VIDEO_STREAMID;
	stream_list[0]->streamid = stream_id;
	packet_size += header_size;
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "MPEG sequence header  frametime: %lf\n", frametime);
#endif
      }

      /* GOP header */
      while((header_size = gop_header(pointer+packet_size, read_buffer + read_size - pointer - packet_size, 0)) != 0)
      {
	packet_size += header_size; 
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "MPEG gop header\n");
#endif
      }

      /* Picture header */
      while((header_size = picture_header(pointer+packet_size, read_buffer + read_size - pointer - packet_size)) != 0)
      {
	packet_size += header_size;    // Warning: header size not quite correct (can be not byte aligned)
	stream_timestamp += frametime; // but this is compensated by skipping a little more, as we don't need to be header aligned	
	packet_size += 4;              // after this, since we then check for the next header to know the slice size.
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "MPEG picture header\n");
#endif
      }

      /* Slice header */
      while((header_size = slice_header(pointer+packet_size, read_buffer + read_size - pointer - packet_size)) != 0)
      {
	packet_size += header_size;    
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "MPEG slice header\n");
#endif
      }

      /* Audio frame */
      if(audio_header(pointer+packet_size, &packet_size, &frametime))
      {
	  stream_id = AUDIO_STREAMID;
	  stream_list[0]->streamid = stream_id;
	  stream_timestamp += frametime;
#ifdef DEBUG_SYSTEM
	  fprintf(stderr, "MPEG audio header [%d] time: %lf @ %lf\n", packet_size, frametime, stream_timestamp);
#endif
      }
      else
      {
	/* Check for next slice, picture, gop or sequence header */
	register Uint8 * p;
	register Uint8 c;
	
	p = pointer + packet_size;
      state0:
	c = *p;
	p++;
	if(p >= read_buffer + read_size) goto end;
	if(c != 0) goto state0;
/* Not explicitly reached:
      state1:
 */
	c = *p;
	p++;
	if(p >= read_buffer + read_size) goto end;
	if(c != 0) goto state0;
      state2:
	c = *p;
	p++;
	if(p >= read_buffer + read_size) goto end;
	if(c == 0) goto state2;
	if(c != 1) goto state0;
/* Not explicitly reached:
      state3:
 */
	c = *p;
	p++;
	if(p >= read_buffer + read_size) goto end;
	if(c <= 0xaf) goto end;
	if(c == 0xb8) goto end;
	if(c == 0xb3) goto end;
	goto state0;
      end:

	if(p >= read_buffer + read_size) packet_size = (read_buffer + read_size) - pointer;
	else packet_size = p - pointer - 4;
      }

      if(stream_id == SYSTEM_STREAMID)
	stream_id = 0;
    }
    else
    {
#ifdef DEBUG_SYSTEM
	fprintf(stderr,
		"Warning: unexpected header %02x%02x%02x%02x at offset %d\n",
		pointer[0],
		pointer[1],
		pointer[2],
		pointer[3],
		read_total - read_size + (pointer - read_buffer));
#endif
	pointer++;
	stream_list[0]->pos++;
	seek_next_header();
	RequestBuffer();
	return(0);
    }
  }

  if(Eof())
  {
    RequestBuffer();
    return(0);
  }

  assert(packet_size <= MPEG_BUFFER_SIZE);

  if(skip_timestamp > timestamp){
    int cur_seconds=int(timestamp)%60;

    if (cur_seconds%5==0){
      fprintf(stderr, "Skipping to %02d:%02d (%02d:%02d)\r",
              int(skip_timestamp)/60, int(skip_timestamp)%60,
              int(timestamp)/60, cur_seconds);
    }
    pointer += packet_size;
    stream_list[0]->pos += packet_size;
    /* since we skip data, request more */
    RequestBuffer();
    return (0);
  }

  switch(stream_id)
  {
    case 0:
      /* Unknown stream, just get another packet */
      pointer += packet_size;
      stream_list[0]->pos += packet_size;
      RequestBuffer();
    return(0);

    case SYSTEM_STREAMID:
      /* System header */

      /* This MPEG contain system information */
      /* Parse the system header and create MPEG streams  */
    
      /* Read the stream table */
      pointer += 5;
      stream_list[0]->pos += 5;

      while (pointer[0] & 0x80 )
      {
	/* If the stream doesn't already exist */
	if(!get_stream(pointer[0]))
	{
	  /* Create a new stream and add it to the list */
	  add_stream(new MPEGstream(this, pointer[0]));
	}
	pointer += 3;
	stream_list[0]->pos += 3;
      }          
      /* Hack to detect video streams that are not advertised */
      if ( ! exist_stream(VIDEO_STREAMID, 0xF0) ) {
	if ( pointer[3] == 0xb3 ) {
	  add_stream(new MPEGstream(this, VIDEO_STREAMID));
	}
      }
      RequestBuffer();
    return(stream_id);

    default:
      MPEGstream * stream;

      /* Look for the stream the data must be given to */
      stream = get_stream(stream_id);

      if(!stream)
      {	
	/* Hack to detect video or audio streams that are not declared in system header */
	if ( ((stream_id & 0xF0) == VIDEO_STREAMID) && !exist_stream(stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
	  fprintf(stderr, "Undeclared video packet, creating a new video stream\n");
#endif
	    stream = new MPEGstream(this, stream_id);
	    add_stream(stream);
	}
	else
	if ( ((stream_id & 0xF0) == AUDIO_STREAMID) && !exist_stream(stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
	  fprintf(stderr, "Undeclared audio packet, creating a new audio stream\n");
#endif
	    stream = new MPEGstream(this, stream_id);
	    add_stream(stream);
	}
	else
	{
	  /* No stream found for packet, skip it */
	  pointer += packet_size;
	  stream_list[0]->pos += packet_size;
	  RequestBuffer();
	  return(stream_id);
	}
      }

      /* Insert the new data at the end of the stream */
      if(pointer + packet_size <= read_buffer + read_size)
      {
	if(packet_size) stream->insert_packet(pointer, packet_size, stream_timestamp);
	pointer += packet_size;
      }
      else
      {
	stream->insert_packet(pointer, 0, stream_timestamp);
	errorstream = true;
	pointer = read_buffer + read_size;
      }
      return(stream_id);
  }
}

void MPEGsystem::Skip(double time)
{
  if (skip_timestamp < timestamp)
    skip_timestamp = timestamp;
  skip_timestamp += time;
}
 
Uint32 MPEGsystem::Tell()
{
  register Uint32 t;
  register int i;

  /* Sum all stream positions */
  for(i = 0, t = 0; stream_list[i]; i++)
    t += stream_list[i]->pos;

  if(t > TotalSize())
    return(TotalSize());
  else
    return(t);
}

Uint32 MPEGsystem::TotalSize()
{
  off_t size;
  off_t pos;

  /* Lock to avoid concurrent access to the stream */
  SDL_mutexP(system_mutex);

  /* I made it this way (instead of fstat) to avoid #ifdef WIN32 everywhere */
  /* in case 'in some weird perversion of nature' someone wants to port this to Win32 :-) */
  if((pos = SDL_RWtell(source)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));
    }
    SDL_mutexV(system_mutex);
    return(0);
  }

  if((size = SDL_RWseek(source, 0, SEEK_END)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));  
    }
    SDL_mutexV(system_mutex);
    return(0);
  }
  
  if((pos = SDL_RWseek(source, pos, SEEK_SET)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));  
    }
    SDL_mutexV(system_mutex);
    return(0);
  }

  SDL_mutexV(system_mutex);
  return(size);
}

double MPEGsystem::TotalTime()
{
  off_t size, pos;
  off_t file_ptr;
  Uint8 * buffer, * p;
  double time;

  /* Lock to avoid concurrent access to the stream */
  SDL_mutexP(system_mutex);

  /* Save current position */
  if((pos = SDL_RWtell(source)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));
    }
    SDL_mutexV(system_mutex);
    return(false);
  }

  file_ptr = 0;
  p = buffer = new Uint8[MPEG_BUFFER_SIZE];
  time = 0;

  /* If audio, compute total time according to bitrate of the first header and total size */
  /* Note: this doesn't work on variable bitrate streams */
  if(stream_list[0]->streamid == AUDIO_STREAMID)
  {
    do
    {
      if((size = SDL_RWseek(source, file_ptr, SEEK_SET)) < 0)
      {
        if(errno != ESPIPE)
        {
          errorstream = true;
          SetError(strerror(errno));  
        }
        SDL_mutexV(system_mutex);
        return(false);
      }
    
      if(SDL_RWread(source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;

      /* Search for a valid audio header */
      for(p = buffer; p < buffer + MPEG_BUFFER_SIZE; p++)
	if(audio_aligned(p, buffer + MPEG_BUFFER_SIZE - p)) break;
    
      file_ptr += MPEG_BUFFER_SIZE;
    }
    while(p >= MPEG_BUFFER_SIZE + buffer);

    /* Extract time info from the first header */
    Uint32 framesize = 0;
    double frametime = 0.0;
    Uint32 totalsize;

    audio_header(p, &framesize, &frametime);
    totalsize = TotalSize();
    if(framesize)
      time = frametime * totalsize / framesize;
  }
  else
  {
    bool last_chance = false;
    do
    {
    /* Otherwise search the stream backwards for a valid header */
      file_ptr -= MPEG_BUFFER_SIZE;
      if ( file_ptr < -(Sint32)TotalSize() ) {
          last_chance = true;
          file_ptr = -(Sint32)TotalSize();
      }
     
      if((size = SDL_RWseek(source, file_ptr, SEEK_END)) < 0)
      {
        if(errno != ESPIPE)
        {
          errorstream = true;
          SetError(strerror(errno));  
        }
        SDL_mutexV(system_mutex);
        return(false);
      }
      
      if(SDL_RWread(source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;
      
      if(stream_list[0]->streamid == SYSTEM_STREAMID)
	for(p = buffer + MPEG_BUFFER_SIZE - 1; p >= buffer;)
	{
	  if(*p-- != 0xba) continue; // Packet header
	  if(*p-- != 1) continue;
	  if(*p-- != 0) continue;
	  if(*p-- != 0) continue;
          ++p;
	  break;
	}
      if(stream_list[0]->streamid == VIDEO_STREAMID)
	for(p = buffer + MPEG_BUFFER_SIZE - 1; p >= buffer;)
	{
	  if(*p-- != 0xb8) continue; // GOP header
	  if(*p-- != 1) continue;
	  if(*p-- != 0) continue;
	  if(*p-- != 0) continue;
          ++p;
	  break;
	}
    }
    while( !last_chance && (p < buffer) );

    if ( p >= buffer ) {
      /* Extract time info from the last header */
      if(stream_list[0]->streamid == SYSTEM_STREAMID)
        packet_header(p, buffer + MPEG_BUFFER_SIZE - p, &time);
    
      if(stream_list[0]->streamid == VIDEO_STREAMID)
        gop_header(p, buffer + MPEG_BUFFER_SIZE - p, &time);
    }
  }

  delete[] buffer;

  /* Get back to saved position */
  if((pos = SDL_RWseek(source, pos, SEEK_SET)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));  
    }
    time = 0;
  }

  SDL_mutexV(system_mutex);

  return(time);
}

double MPEGsystem::TimeElapsedAudio(int atByte)
{
  off_t size, pos;
  off_t file_ptr;
  Uint8 * buffer, * p;
  double time;
  
  if (atByte < 0)
  {
      return -1;
  }
  
  /* Lock to avoid concurrent access to the stream */
  SDL_mutexP(system_mutex);

  /* Save current position */
  if((pos = SDL_RWtell(source)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));
    }
    SDL_mutexV(system_mutex);
    return(false);
  }

  file_ptr = 0;
  p = buffer = new Uint8[MPEG_BUFFER_SIZE];

  /* If audio, compute total time according to bitrate of the first header and total size */
  /* Note: this doesn't work on variable bitrate streams */
  if(stream_list[0]->streamid == AUDIO_STREAMID)
  {
    do
    {
      if((size = SDL_RWseek(source, file_ptr, SEEK_SET)) < 0)
      {
        if(errno != ESPIPE)
        {
          errorstream = true;
          SetError(strerror(errno));  
        }
        SDL_mutexV(system_mutex);
        return(false);
      }
    
      if(SDL_RWread(source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;

      /* Search for a valid audio header */
      for(p = buffer; p < buffer + MPEG_BUFFER_SIZE; p++)
	if(audio_aligned(p, buffer + MPEG_BUFFER_SIZE - p)) break;
    
      file_ptr += MPEG_BUFFER_SIZE;
    }
    while(p >= MPEG_BUFFER_SIZE + buffer);

    /* Extract time info from the first header */
    Uint32 framesize = 0;
    double frametime = 0.0;
    Uint32 totalsize;

    audio_header(p, &framesize, &frametime);
    totalsize = TotalSize();
    if(framesize)
      //is there a better way to do this?
      time = (frametime * (atByte ? atByte:totalsize)) / framesize;
    else
      time = 0;
  }
  else
  //This is not a purely audio stream. This doesn't make sense!
  {
      time = -1;
  }

  delete[] buffer;

  /* Get back to saved position */
  if((pos = SDL_RWseek(source, pos, SEEK_SET)) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));  
    }
    SDL_mutexV(system_mutex);
    return(0);
  }

  SDL_mutexV(system_mutex);
  return(time);
}

void MPEGsystem::Rewind()
{
  Seek(0);
}

bool MPEGsystem::Seek(int length)
{
  /* Stop the system thread */
  Stop();

  /* Lock to avoid concurrent access to the stream */
  SDL_mutexP(system_mutex);
  
  /* Get into the stream */
  if(SDL_RWseek(source, length, SEEK_SET) < 0)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));
    }
    return(false);
  }

  /* Reinitialize the read buffer */
  pointer = read_buffer;
  read_size = 0;
  read_total = length;
  stream_list[0]->pos += length;
  packet_total = 0;
  endofstream = false;
  errorstream = false;
  timestamp = 0.0;
  skip_timestamp = -1;
  reset_all_streams();

  SDL_mutexV(system_mutex);

  /* Restart the system thread */
  Start();

  return(true);
}

void MPEGsystem::RequestBuffer()
{
  SDL_SemPost(request_wait);
}

void MPEGsystem::Start()
{
  if(system_thread_running) return;

  /* Get the next header */
  if(!seek_next_header())
  {
    if(!Eof())
    {
      errorstream = true;
      SetError("Could not find the beginning of MPEG data\n");
    }
  }
  
#ifdef USE_SYSTEM_THREAD
  /* Start the system thread */
  system_thread = SDL_CreateThread(SystemThread, this);

  /* Wait for the thread to start */
  while(!system_thread_running && !Eof())
    SDL_Delay(1);
#else
  system_thread_running = true;
#endif
}

void MPEGsystem::Stop()
{
  if(!system_thread_running) return;

  /* Force the system thread to die */
  system_thread_running = false;
#ifdef USE_SYSTEM_THREAD
  SDL_SemPost(request_wait);
  SDL_WaitThread(system_thread, NULL);
#endif

  /* Reset the streams */
  reset_all_streams();
}

bool MPEGsystem::Wait()
{
#ifdef USE_SYSTEM_THREAD
  if ( ! errorstream ) {
    while(SDL_SemValue(request_wait) != 0)
      SDL_Delay(1);
  }
#else
  while(SDL_SemValue(request_wait) != 0)
      if ( ! SystemLoop(this) ) break;
#endif
  return(!errorstream);
}

bool MPEGsystem::Eof() const
{
  return(errorstream || endofstream);
}

bool MPEGsystem::SystemLoop(MPEGsystem *system)
{
  /* Check for end of file */
  if(system->Eof())
  {
    /* Set the eof mark on all streams */
    system->end_all_streams();

    /* Get back to the beginning of the stream if possible */
    if(SDL_RWseek(system->source, 0, SEEK_SET) < 0)
    {
      if(errno != ESPIPE)
      {
        system->errorstream = true;
        system->SetError(strerror(errno));
      }
      return(false);
    }

    /* Reinitialize the read buffer */
    system->pointer = system->read_buffer;
    system->read_size = 0;
    system->read_total = 0;
    system->packet_total = 0;
    system->endofstream = false;
    system->errorstream = false;

    /* Get the first header */
    if(!system->seek_first_header())
    {
      system->errorstream = true;
      system->SetError("Could not find the beginning of MPEG data\n");
      return(false);
    }
  }

  /* Wait for a buffer request */
  SDL_SemWait(system->request_wait);

  /* Read the buffer */
  system->FillBuffer();

  return(true);
}

int MPEGsystem::SystemThread(void * udata)
{
  MPEGsystem * system = (MPEGsystem *) udata;

  system->system_thread_running = true;

  while(system->system_thread_running)
  {
    if ( ! SystemLoop(system) ) {
      system->system_thread_running = false;
    }
  }
  return(true);
}

void MPEGsystem::add_stream(MPEGstream * stream)
{
  register int i;

  /* Go to the end of the list */
  for(i = 0; stream_list[i]; i++);

  /* Resize list */
  stream_list = 
    (MPEGstream **) realloc(stream_list, (i+2)*sizeof(MPEGstream *));

  /* Write the stream */
  stream_list[i] = stream;

  /* Put the end marker (null) */
  stream_list[i+1] = 0;
}

MPEGstream * MPEGsystem::get_stream(Uint8 stream_id)
{
  register int i;

  for(i = 0; stream_list[i]; i++)
    if(stream_list[i]->streamid == stream_id)
      break;

  return(stream_list[i]);
}

Uint8 MPEGsystem::exist_stream(Uint8 stream_id, Uint8 mask)
{
  register int i;

  for(i = 0; stream_list[i]; i++)
    if(((stream_list[i]->streamid) & mask) == (stream_id & mask))
      return(stream_list[i]->streamid);

  return(0);
}

void MPEGsystem::reset_all_streams()
{
  register int i;

  /* Reset the streams */
  for(i = 0; stream_list[i]; i++)
    stream_list[i]->reset_stream();
}

void MPEGsystem::end_all_streams()
{
  register int i;

  /* End the streams */
  /* We use a null buffer as the end of stream marker */
  for(i = 0; stream_list[i]; i++)
    stream_list[i]->insert_packet(0, 0);
}

bool MPEGsystem::seek_first_header()
{
  Read();

  if(Eof())
    return(false);

  while(!(audio_aligned(pointer, read_buffer + read_size - pointer) ||
	  system_aligned(pointer, read_buffer + read_size - pointer) ||
 	  Match4(pointer, VIDEO_CODE, VIDEO_MASK)))
  {
       ++pointer;
       stream_list[0]->pos++;
      /* Make sure buffer is always full */
      Read();

      if(Eof())
	return(false);
  }
  return(true);
}

bool MPEGsystem::seek_next_header()
{
  Read();

  if(Eof())
    return(false);

  while(!( (stream_list[0]->streamid == AUDIO_STREAMID && audio_aligned(pointer, read_buffer + read_size - pointer)) ||
	   (stream_list[0]->streamid == SYSTEM_STREAMID && system_aligned(pointer, read_buffer + read_size - pointer)) ||
	   (stream_list[0]->streamid == VIDEO_STREAMID && Match4(pointer, GOP_CODE, GOP_MASK))
	 ) )
  {
       ++pointer;
       stream_list[0]->pos++;
      /* Make sure buffer is always full */
      Read();

      if(Eof())
	return(false);
  }

  return(true);
}
