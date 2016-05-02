/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software
    
    - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn -

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

/* A class based on the MPEG stream class, used to parse and play audio */

#ifndef _MPEGAUDIO_H_
#define _MPEGAUDIO_H_

#include "SDL.h"
#include "MPEGerror.h"
#include "MPEGaction.h"

#ifdef THREADED_AUDIO
#include "MPEGring.h"
#endif

void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len);
#ifdef THREADED_AUDIO
    int Decode_MPEGaudio(void *udata);
#endif

class MPEGstream;

/* MPEG/WAVE Sound library
   (C) 1997 by Woo-jae Jung */
    
/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK     12
#define CALCBUFFERSIZE 512
#define MAXSUBBAND     32
#define MAXCHANNEL     2
#define MAXTABLE       2
#define SCALE          32768
#define MAXSCALE       (SCALE-1)
#define MINSCALE       (-SCALE)
#define RAWDATASIZE    (2*2*32*SSLIMIT)

#define LS 0
#define RS 1

#define SSLIMIT      18
#define SBLIMIT      32

#define WINDOWSIZE    4096

// Huffmancode
#define HTN 34

/********************/
/* Type definitions */
/********************/
typedef float REAL;

typedef struct
{
  bool         generalflag;
  unsigned int part2_3_length;
  unsigned int big_values;
  unsigned int global_gain;
  unsigned int scalefac_compress;
  unsigned int window_switching_flag;
  unsigned int block_type;
  unsigned int mixed_block_flag;
  unsigned int table_select[3];
  unsigned int subblock_gain[3];
  unsigned int region0_count;
  unsigned int region1_count;
  unsigned int preflag;
  unsigned int scalefac_scale;
  unsigned int count1table_select;
}layer3grinfo;

typedef struct
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct
  {
    unsigned scfsi[4];
    layer3grinfo gr[2];
  }ch[2];
}layer3sideinfo;

typedef struct
{
  int l[23];            /* [cb] */
  int s[3][13];         /* [window][cb] */
}layer3scalefactor;     /* [ch] */

typedef struct
{
  int tablename;
  unsigned int xlen,ylen;
  unsigned int linbits;
  unsigned int treelen;
  const unsigned int (*val)[2];
}HUFFMANCODETABLE;


// Class for Mpeg layer3
class Mpegbitwindow
{
public:
  Mpegbitwindow(){bitindex=point=0;};

  void initialize(void)  {bitindex=point=0;};
  int  gettotalbit(void) const {return bitindex;};
  void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;};
  void wrap(void);
  void rewind(int bits)  {bitindex-=bits;};
  void forward(int bits) {bitindex+=bits;};
  int getbit(void) {
      register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;
      bitindex++;
      return r;
  }
  int getbits9(int bits)
  {
      register unsigned short a;
      { int offset=bitindex>>3;

        a=(((unsigned char)buffer[offset])<<8) | ((unsigned char)buffer[offset+1]);
      }
      a<<=(bitindex&7);
      bitindex+=bits;
      return (int)((unsigned int)(a>>(16-bits)));
  }
  int  getbits(int bits);

private:
  int  point,bitindex;
  char buffer[2*WINDOWSIZE];
};

/* The actual MPEG audio class */
class MPEGaudio : public MPEGerror, public MPEGaudioaction {

public:
    MPEGaudio(MPEGstream *stream, bool initSDL = true);
    virtual ~MPEGaudio();

    /* MPEG actions */
    bool GetAudioInfo(MPEG_AudioInfo *info);
    double Time(void);
    void Play(void);
    void Stop(void);
    void Rewind(void);
    void ResetSynchro(double time);
    void Skip(float seconds);
    void Volume(int vol);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name in popcorn */
    MPEGstatus GetStatus(void);

    /* Returns the desired SDL audio spec for this stream */
    bool WantedSpec(SDL_AudioSpec *wanted);

    /* Inform SMPEG of the actual audio format if configuring SDL
       outside of this class */
    void ActualSpec(const SDL_AudioSpec *actual);

protected:
    bool sdl_audio;
    MPEGstream *mpeg;
    int valid_stream;
    bool stereo;
    double rate_in_s;
    Uint32 frags_playing;
    Uint32 frag_time;
#ifdef THREADED_AUDIO
    bool decoding;
    SDL_Thread *decode_thread;

    void StartDecoding(void);
    void StopDecoding(void);
#endif

/* Code from splay 1.8.2 */

  /*****************************/
  /* Constant tables for layer */
  /*****************************/
private:
  static const int bitrate[2][3][15],frequencies[2][3];
  static const REAL scalefactorstable[64];
  static const HUFFMANCODETABLE ht[HTN];
  static const REAL filter[512];
  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

  /*************************/
  /* MPEG header variables */
  /*************************/
private:
  int last_speed;
  int layer,protection,bitrateindex,padding,extendedmode;
  enum _mpegversion  {mpeg1,mpeg2}                               version;
  enum _mode    {fullstereo,joint,dual,single}                   mode;
  enum _frequency {frequency44100,frequency48000,frequency32000} frequency;

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
private:
  bool forcetomonoflag;
  bool forcetostereoflag;
  bool swapendianflag;
  int  downfrequency;

public:
  void setforcetomono(bool flag);
  void setdownfrequency(int value);

  /******************************/
  /* Frame management variables */
  /******************************/
private:
  int decodedframe,currentframe,totalframe;

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/
private:
  int tableindex,channelbitrate;
  int stereobound,subbandnumber,inputstereo,outputstereo;
  REAL scalefactor;
  int framesize;

  /*******************/
  /* Mpegtoraw class */
  /*******************/
public:
  void initialize();
  bool run(int frames, double *timestamp = NULL);
  void clearbuffer(void);

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
private:
  Uint8 _buffer[4096];
  Uint32 _buffer_pos;
  int  bitindex;
  bool fillbuffer(int size);
  void sync(void);
  bool issync(void);
  int getbyte(void);
  int getbit(void);
  int getbits8(void);
  int getbits9(int bits);
  int getbits(int bits);


  /********************/
  /* Global variables */
  /********************/
private:
  int lastfrequency,laststereo;

  // for Layer3
  int layer3slots,layer3framestart,layer3part2start;
  REAL prevblck[2][2][SBLIMIT][SSLIMIT];
  int currentprevblock;
  layer3sideinfo sideinfo;
  layer3scalefactor scalefactors[2];

  Mpegbitwindow bitwindow;
  int wgetbit  (void)    {return bitwindow.getbit  ();    }
  int wgetbits9(int bits){return bitwindow.getbits9(bits);}
  int wgetbits (int bits){return bitwindow.getbits (bits);}


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/
private:
  bool loadheader(void);

  //
  // Subbandsynthesis
  //
  REAL calcbufferL[2][CALCBUFFERSIZE],calcbufferR[2][CALCBUFFERSIZE];
  int  currentcalcbuffer,calcbufferoffset;

  void computebuffer(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle(void);
  void generate(void);
  void subbandsynthesis(REAL *fractionL,REAL *fractionR);

  void computebuffer_2(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle_2(void);
  void generate_2(void);
  void subbandsynthesis_2(REAL *fractionL,REAL *fractionR);

  // Extarctor
  void extractlayer1(void);    // MPEG-1
  void extractlayer2(void);
  void extractlayer3(void);
  void extractlayer3_2(void);  // MPEG-2


  // Functions for layer 3
  void layer3initialize(void);
  bool layer3getsideinfo(void);
  bool layer3getsideinfo_2(void);
  void layer3getscalefactors(int ch,int gr);
  void layer3getscalefactors_2(int ch);
  void layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT]);
  REAL layer3twopow2(int scale,int preflag,int pretab_offset,int l);
  REAL layer3twopow2_1(int a,int b,int c);
  void layer3dequantizesample(int ch,int gr,int   in[SBLIMIT][SSLIMIT],
                                REAL out[SBLIMIT][SSLIMIT]);
  void layer3fixtostereo(int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
  void layer3reorderandantialias(int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
                               REAL out[SBLIMIT][SSLIMIT]);

  void layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
                          REAL out[SSLIMIT][SBLIMIT]);
  
  void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y);
  void huffmandecoder_2(const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

  /********************/
  /* Playing raw data */
  /********************/
private:
  int     samplesperframe;
  int     rawdatareadoffset, rawdatawriteoffset;
  Sint16 *rawdata;
#ifdef THREADED_AUDIO
  MPEG_ring *ring;
#else
  Sint16  spillover[ RAWDATASIZE ];
#endif
  int volume;

  void clearrawdata(void)    {
        rawdatareadoffset=0;
        rawdatawriteoffset=0;
        rawdata=NULL;
  }
  void putraw(short int pcm) {rawdata[rawdatawriteoffset++]=pcm;}

  /********************/
  /* Timestamp sync   */
  /********************/
public:
#define N_TIMESTAMPS 5

  double timestamp[N_TIMESTAMPS];

  /* Functions which access MPEGaudio internals */
  friend void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len);
  friend int Play_MPEGaudio(MPEGaudio *audio, Uint8 *stream, int len);
#ifdef THREADED_AUDIO
  friend int Decode_MPEGaudio(void *udata);
#endif
};

/* Need to duplicate the prototypes, this is not a typo :) */
void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len);
int Play_MPEGaudio(MPEGaudio *audio, Uint8 *stream, int len);
#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata);
#endif

#endif /* _MPEGAUDIO_H_ */
