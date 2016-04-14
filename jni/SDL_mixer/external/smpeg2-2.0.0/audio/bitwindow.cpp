/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Bitwindow.cc
// It's bit reservior for MPEG layer 3

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "MPEGaudio.h"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int Mpegbitwindow::getbits(int bits)
{
  union
  {
    char store[4];
    int current;
  }u;
  int bi;

  if(!bits)return 0;

  u.current=0;
  bi=(bitindex&7);
  //  u.store[_KEY]=buffer[(bitindex>>3)&(WINDOWSIZE-1)]<<bi;
  u.store[_KEY]=buffer[bitindex>>3]<<bi;
  bi=8-bi;
  bitindex+=bi;

  while(bits)
  {
    if(!bi)
    {
      //      u.store[_KEY]=buffer[(bitindex>>3)&(WINDOWSIZE-1)];
      u.store[_KEY]=buffer[bitindex>>3];
      bitindex+=8;
      bi=8;
    }

    if(bits>=bi)
    {
      u.current<<=bi;
      bits-=bi;
      bi=0;
    }
    else
    {
      u.current<<=bits;
      bi-=bits;
      bits=0;
    }
  }
  bitindex-=bi;

  return (u.current>>8);
}
