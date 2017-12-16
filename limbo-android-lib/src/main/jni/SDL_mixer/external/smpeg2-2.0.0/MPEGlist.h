/* bufferlist.h */

/* A class for buffering the I/O and allow multiple streams to read the data
   asynchronously */

#ifndef _MPEGLIST_H_
#define _MPEGLIST_H_

#include "SDL.h"

class MPEGlist {
public:
  MPEGlist();
  ~MPEGlist();

  /* Get to the next free buffer or allocate a new one if none is free */
  MPEGlist * Alloc(Uint32 Buffer_Size);

  /* Lock current buffer */
  void Lock();

  /* Unlock current buffer */
  void Unlock();

  /* Get the buffer */
  inline void * Buffer() { return(data); };

  inline Uint32 Size() { return(size); }; 

  inline MPEGlist * Next() { return(next); };

  inline MPEGlist * Prev() { return(prev); };

  inline Uint32 IsLocked() { return(lock); };

  double TimeStamp;

private:
  class MPEGlist * next;
  class MPEGlist * prev;
  Uint32 lock;
  Uint8 * data;
  Uint32 size;
};
#endif
