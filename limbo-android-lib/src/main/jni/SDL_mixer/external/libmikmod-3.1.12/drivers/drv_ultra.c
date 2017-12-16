/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: drv_ultra.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for the Linux Ultrasound driver

==============================================================================*/

/*

	Written by Andy Lo A Foe <andy@alsa-project.org>

	Updated to work with later versions of both the ultrasound driver and
	libmikmod by C. Ray C. <crayc@pyro.net>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_ULTRA

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef MIKMOD_DYNAMIC
#include <dlfcn.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libgus.h>

/* just in case */
#ifndef LIBGUS_VERSION_MAJOR
#define LIBGUS_VERSION_MAJOR 0x0003
#endif

#ifdef MIKMOD_DYNAMIC
/* runtime link with libgus */
static int(*libgus_cards)(void);

#if LIBGUS_VERSION_MAJOR < 0x0004
static int(*libgus_close)(int);
static int(*libgus_do_flush)(void);
static void(*libgus_do_tempo)(unsigned int);
static void(*libgus_do_voice_frequency)(unsigned char,unsigned int);
static void(*libgus_do_voice_pan)(unsigned char,unsigned short);
static void(*libgus_do_voice_start)(unsigned char,unsigned int,unsigned int,unsigned short,unsigned short);
static void(*libgus_do_voice_start_position)(unsigned char,unsigned int,unsigned int,unsigned short,unsigned short,unsigned int);
static void(*libgus_do_voice_volume)(unsigned char,unsigned short);
static void(*libgus_do_wait)(unsigned int);
static int(*libgus_get_handle)(void);
static int(*libgus_info)(gus_info_t*,int);
static int(*libgus_memory_alloc)(gus_instrument_t*);
static int(*libgus_memory_free)(gus_instrument_t*);
static int(*libgus_memory_free_size)(void);
static int(*libgus_memory_pack)(void);
static int(*libgus_open)(int,size_t,int);
static int(*libgus_queue_flush)(void);
static int(*libgus_queue_read_set_size)(int);
static int(*libgus_queue_write_set_size)(int);
static int(*libgus_reset)(int,unsigned int);
static int(*libgus_select)(int);
static int(*libgus_timer_start)(void);
static int(*libgus_timer_stop)(void);
static int(*libgus_timer_tempo)(int);
#else
static int(*libgus_close)(void*);
static int(*libgus_do_flush)(void*);
static void(*libgus_do_tempo)(void*,unsigned int);
static void(*libgus_do_voice_frequency)(void*,unsigned char,unsigned int);
static void(*libgus_do_voice_pan)(void*,unsigned char,unsigned short);
static void(*libgus_do_voice_start)(void*,unsigned char,unsigned int,unsigned int,unsigned short,unsigned short);
static void(*libgus_do_voice_start_position)(void*,unsigned char,unsigned int,unsigned int,unsigned short,unsigned short,unsigned int);
static void(*libgus_do_voice_volume)(void*,unsigned char,unsigned short);
static void(*libgus_do_wait)(void*,unsigned int);
static int(*libgus_get_file_descriptor)(void*);
static int(*libgus_info)(void*,gus_info_t*,int);
static int(*libgus_memory_alloc)(void*,gus_instrument_t*);
static int(*libgus_memory_free)(void*,gus_instrument_t*);
static int(*libgus_memory_free_size)(void*);
static int(*libgus_memory_pack)(void*);
static int(*libgus_open)(void *,int,int,size_t,int);
static int(*libgus_queue_flush)(void*);
static int(*libgus_queue_read_set_size)(void*,int);
static int(*libgus_queue_write_set_size)(void*,int);
static int(*libgus_reset)(void*,int,unsigned int);
static int(*libgus_timer_start)(void*);
static int(*libgus_timer_stop)(void*);
static int(*libgus_timer_tempo)(void*,int);
#endif
static void* libgus=NULL;
#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif
#else
/* compile-time link with libgus */
#define libgus_cards					gus_cards
#define libgus_close					gus_close
#define libgus_do_flush					gus_do_flush
#define libgus_do_tempo					gus_do_tempo
#define libgus_do_voice_frequency		gus_do_voice_frequency
#define libgus_do_voice_pan				gus_do_voice_pan
#define libgus_do_voice_start			gus_do_voice_start
#define libgus_do_voice_start_position	gus_do_voice_start_position
#define libgus_do_voice_volume			gus_do_voice_volume
#define libgus_do_wait					gus_do_wait
#define libgus_get_handle				gus_get_handle
#define libgus_get_file_descriptor		gus_get_file_descriptor
#define libgus_info						gus_info
#define libgus_memory_alloc				gus_memory_alloc
#define libgus_memory_free				gus_memory_free
#define libgus_memory_free_size			gus_memory_free_size
#define libgus_memory_pack				gus_memory_pack
#define libgus_open						gus_open
#define libgus_queue_flush				gus_queue_flush
#define libgus_queue_read_set_size		gus_queue_read_set_size
#define libgus_queue_write_set_size		gus_queue_write_set_size
#define libgus_reset					gus_reset
#define libgus_select					gus_select
#define libgus_timer_start				gus_timer_start
#define libgus_timer_stop				gus_timer_stop
#define libgus_timer_tempo				gus_timer_tempo
#endif

#define MAX_INSTRUMENTS		128	/* Max. instruments loadable   */
#define GUS_CHANNELS		32	/* Max. GUS channels available */
#define SIZE_OF_SEQBUF		(8 * 1024) /* Size of the sequence buffer */
#define ULTRA_PAN_MIDDLE	(16384 >> 1)

#define	CH_FREQ 1
#define CH_VOL  2
#define	CH_PAN  4

/*	This structure holds the information regarding a loaded sample, which
	is also kept in normal memory. */
typedef struct GUS_SAMPLE {
	SWORD *sample;
	ULONG length;
	ULONG loopstart;
	ULONG loopend;
	UWORD flags;
	UWORD active;
} GUS_SAMPLE;

/*	This structure holds the current state of a GUS voice channel. */
typedef struct GUS_VOICE {
	UBYTE kick;
	UBYTE active;
	UWORD flags;
	SWORD handle;
	ULONG start;
	ULONG size;
	ULONG reppos;
	ULONG repend;
	ULONG frq;
	int vol;
	int pan;

	int changes;
	time_t started;
} GUS_VOICE;

/* Global declarations follow */

static	GUS_SAMPLE instrs[MAX_INSTRUMENTS];
static	GUS_VOICE voices[GUS_CHANNELS];

static int ultra_dev=0;
#if LIBGUS_VERSION_MAJOR < 0x0004
static int gf1flag=-1;
#else
static void* ultra_h=NULL;
#endif
static int nr_instrs=0;
static int ultra_fd=-1;
static UWORD ultra_bpm;

#ifdef MIKMOD_DYNAMIC
static BOOL Ultra_Link(void)
{
	if(libgus) return 0;

	/* load libgus.so */
	libgus=dlopen("libgus.so",RTLD_LAZY|RTLD_GLOBAL);
	if(!libgus) return 1;

	/* resolve function references */
	if(!(libgus_cards                  =dlsym(libgus,"gus_cards"))) return 1;
	if(!(libgus_close                  =dlsym(libgus,"gus_close"))) return 1;
	if(!(libgus_do_flush               =dlsym(libgus,"gus_do_flush"))) return 1;
	if(!(libgus_do_tempo               =dlsym(libgus,"gus_do_tempo"))) return 1;
	if(!(libgus_do_voice_frequency     =dlsym(libgus,"gus_do_voice_frequency"))) return 1;
	if(!(libgus_do_voice_pan           =dlsym(libgus,"gus_do_voice_pan"))) return 1;
	if(!(libgus_do_voice_start         =dlsym(libgus,"gus_do_voice_start"))) return 1;
	if(!(libgus_do_voice_start_position=dlsym(libgus,"gus_do_voice_start_position"))) return 1;
	if(!(libgus_do_voice_volume        =dlsym(libgus,"gus_do_voice_volume"))) return 1;
	if(!(libgus_do_wait                =dlsym(libgus,"gus_do_wait"))) return 1;
#if LIBGUS_VERSION_MAJOR < 0x0004
	if(!(libgus_get_handle             =dlsym(libgus,"gus_get_handle"))) return 1;
#else
	if(!(libgus_get_file_descriptor    =dlsym(libgus,"gus_get_file_descriptor"))) return 1;
#endif
	if(!(libgus_info                   =dlsym(libgus,"gus_info"))) return 1;
	if(!(libgus_memory_alloc           =dlsym(libgus,"gus_memory_alloc"))) return 1;
	if(!(libgus_memory_free            =dlsym(libgus,"gus_memory_free"))) return 1;
	if(!(libgus_memory_free_size       =dlsym(libgus,"gus_memory_free_size"))) return 1;
	if(!(libgus_memory_pack            =dlsym(libgus,"gus_memory_pack"))) return 1;
	if(!(libgus_open                   =dlsym(libgus,"gus_open"))) return 1;
	if(!(libgus_queue_flush            =dlsym(libgus,"gus_queue_flush"))) return 1;
	if(!(libgus_queue_read_set_size    =dlsym(libgus,"gus_queue_read_set_size"))) return 1;
	if(!(libgus_queue_write_set_size   =dlsym(libgus,"gus_queue_write_set_size"))) return 1;
	if(!(libgus_reset                  =dlsym(libgus,"gus_reset"))) return 1;
#if LIBGUS_VERSION_MAJOR < 0x0004
	if(!(libgus_select                 =dlsym(libgus,"gus_select"))) return 1;
#endif
	if(!(libgus_timer_start            =dlsym(libgus,"gus_timer_start"))) return 1;
	if(!(libgus_timer_stop             =dlsym(libgus,"gus_timer_stop"))) return 1;
	if(!(libgus_timer_tempo            =dlsym(libgus,"gus_timer_tempo"))) return 1;

	return 0;
}

static void Ultra_Unlink(void)
{
	libgus_cards                  =NULL;
	libgus_close                  =NULL;
	libgus_do_flush               =NULL;
	libgus_do_tempo               =NULL;
	libgus_do_voice_frequency     =NULL;
	libgus_do_voice_pan           =NULL;
	libgus_do_voice_start         =NULL;
	libgus_do_voice_start_position=NULL;
	libgus_do_voice_volume        =NULL;
	libgus_do_wait                =NULL;
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_get_handle             =NULL;
#else
	libgus_get_file_descriptor    =NULL;
#endif
	libgus_info                   =NULL;
	libgus_memory_alloc           =NULL;
	libgus_memory_free            =NULL;
	libgus_memory_free_size       =NULL;
	libgus_memory_pack            =NULL;
	libgus_open                   =NULL;
	libgus_queue_flush            =NULL;
	libgus_queue_read_set_size    =NULL;
	libgus_queue_write_set_size   =NULL;
	libgus_reset                  =NULL;
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_select                 =NULL;
#endif
	libgus_timer_start            =NULL;
	libgus_timer_stop             =NULL;
	libgus_timer_tempo            =NULL;

	if(libgus) {
		dlclose(libgus);
		libgus=NULL;
	}
}
#endif

/* Checks for the presence of GUS cards */
static BOOL Ultra_IsThere(void)
{
	int retval;

#ifdef MIKMOD_DYNAMIC
	if (Ultra_Link()) return 0;
#endif
	retval=libgus_cards()?1:0;
#ifdef MIKMOD_DYNAMIC
	Ultra_Unlink();
#endif
	return retval;
}

/* Loads all the samples into the GUS memory */
static BOOL loadsamples(void) {
	int i;
	GUS_SAMPLE *smp;

	for (i=0,smp=instrs;i<MAX_INSTRUMENTS;i++,smp++)
		if(smp->active) {
			ULONG length,loopstart,loopend;
			gus_instrument_t instrument;
			gus_layer_t layer;
			gus_wave_t wave;
			unsigned int type;

			/* convert position/length data from samples to bytes */
			length   =smp->length;
			loopstart=smp->loopstart;
			loopend  =smp->loopend;
			if (smp->flags & SF_16BITS) {
				length   <<=1;
				loopstart<<=1;
				loopend  <<=1;
			}

			memset(&instrument,0,sizeof(instrument));
			memset(&layer,0,sizeof(layer));
			memset(&wave,0,sizeof(wave));

			instrument.mode=layer.mode=wave.mode=GUS_INSTR_SIMPLE;
			instrument.number.instrument=i;
			instrument.info.layer=&layer;
			layer.wave=&wave;
			type=((smp->flags & SF_16BITS)?GUS_WAVE_16BIT:0)|
			     ((smp->flags & SF_DELTA) ?GUS_WAVE_DELTA:0)|
			     ((smp->flags & SF_LOOP)  ?GUS_WAVE_LOOP :0)|
			     ((smp->flags & SF_BIDI)  ?GUS_WAVE_BIDIR:0);

			wave.format    =(unsigned char)type;
			wave.begin.ptr =(char*)smp->sample;
			wave.loop_start=loopstart<<4;
			wave.loop_end  =loopend<<4;
			wave.size      =length;

			if (smp->flags&SF_LOOP) {
				smp->sample[loopend]=smp->sample[loopstart];
				if ((smp->flags&SF_16BITS) && loopstart && loopend)
					smp->sample[loopend-1]=smp->sample[loopstart-1];
			}

			/* Download the sample to GUS RAM */
#if LIBGUS_VERSION_MAJOR < 0x0004
			if(libgus_memory_alloc(&instrument))
#else
			if(libgus_memory_alloc(ultra_h,&instrument))
#endif
			{
				_mm_errno=MMERR_SAMPLE_TOO_BIG;
				return 1;
			}
		}
	return 0;
}

/* Load a new sample into memory, but not in the GUS */
static SWORD Ultra_SampleLoad(struct SAMPLOAD *sload,int type)
{
	SAMPLE *s=sload->sample;
	int handle;
	ULONG length,loopstart,loopend;
	GUS_SAMPLE *smp;

	if(type==MD_SOFTWARE) return -1;

	/* Find empty slot to put sample in */
	for(handle=0;handle<MAX_INSTRUMENTS;handle++)
		if(!instrs[handle].active) break;

	if(handle==MAX_INSTRUMENTS) {
		_mm_errno=MMERR_OUT_OF_HANDLES;
		return -1;
	}

	length   =s->length;
	loopstart=s->loopstart;
	loopend  =s->loopend;

	smp=&instrs[handle];
	smp->length   =length;
	smp->loopstart=loopstart;
	smp->loopend  =loopend?loopend:length-1;
	smp->flags    =s->flags; 

	SL_SampleSigned(sload);

	if(!(smp->sample=(SWORD*)_mm_malloc((length+20)<<1))) {
		_mm_errno=MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	if (SL_Load(smp->sample,sload,s->length))
		return -1;

	smp->active = 1;	
	nr_instrs++;

	return handle;
}

/* Discards a sample from the GUS memory, and from computer memory */
static void Ultra_SampleUnload(SWORD handle)
{
	GUS_SAMPLE *smp;

	if((handle>=MAX_INSTRUMENTS)||(handle<0)) return;

	smp = &instrs[handle];
	if (smp->active) {
		gus_instrument_t instrument;

		memset(&instrument,0,sizeof(instrument));
		instrument.mode=GUS_INSTR_SIMPLE;
		instrument.number.instrument=handle;
#if LIBGUS_VERSION_MAJOR < 0x0004
		libgus_memory_free(&instrument);
#else
		libgus_memory_free(ultra_h,&instrument);
#endif
		free(smp->sample);
		nr_instrs--;
		smp->active=0;
	}
}

/* Reports available sample space */
static ULONG Ultra_SampleSpace(int type)
{
	if(type==MD_SOFTWARE) return 0;

#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_memory_pack();
	return (libgus_memory_free_size());
#else
	libgus_memory_pack(ultra_h);
	return (libgus_memory_free_size(ultra_h));
#endif
}

/* Reports the size of a sample */
static ULONG Ultra_SampleLength(int type,SAMPLE *s)
{
	if(!s) return 0;

	return (s->length*(s->flags&SF_16BITS?2:1))+16;
}

/* Initializes the driver */
static BOOL Ultra_Init_internal(void)
{
	gus_info_t info;	

	/* Check that requested settings are compatible with the GUS requirements */
	if((!(md_mode & DMODE_16BITS))||(!(md_mode & DMODE_STEREO))||
	   (md_mixfreq!=44100)) {
		_mm_errno=MMERR_GUS_SETTINGS;
		return 1;
	}

	md_mode&=~(DMODE_SOFT_MUSIC|DMODE_SOFT_SNDFX);

	/* Open the GUS card */
	ultra_dev=getenv("MM_ULTRA")?atoi(getenv("MM_ULTRA")):0;
#if LIBGUS_VERSION_MAJOR < 0x0004
	if ((gf1flag=libgus_open(ultra_dev,SIZE_OF_SEQBUF,0))<0)
#else
	if (libgus_open(&ultra_h,ultra_dev,0,SIZE_OF_SEQBUF,GUS_OPEN_FLAG_NONE)<0)
#endif
	{
		_mm_errno=(errno==ENOMEM)?MMERR_OUT_OF_MEMORY:MMERR_INVALID_DEVICE;
		return 1;
	}
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_select(gf1flag);
	ultra_fd=libgus_get_handle();
#else
	ultra_fd = libgus_get_file_descriptor(ultra_h);
#endif

	/* Get card information */
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_info(&info,0);
#else
	libgus_info(ultra_h,&info,0);
#endif

#ifdef MIKMOD_DEBUG
	switch(info.version) {
		case  0x24:	fputs("GUS 2.4",stderr);break;
		case  0x35:	fputs("GUS 3.7 (flipped)",stderr);break;
		case  0x37:	fputs("GUS 3.7",stderr);break;
		case  0x90:	fputs("GUS ACE",stderr);break;
		case  0xa0:	fputs("GUS MAX 10",stderr);break;
		case  0xa1:	fputs("GUS MAX 11",stderr);break;
		case 0x100:	fputs("Interwave/GUS PnP",stderr);break;
		default:	fprintf(stderr,"Unknown GUS type %x",info.version);break;
	}
	fprintf(stderr," with %dKb RAM on board\n",info.memory_size>>10);
#endif

	return 0;
}

static BOOL Ultra_Init(void)
{
#ifdef MIKMOD_DYNAMIC
	if (Ultra_Link()) {
		_mm_errno=MMERR_DYNAMIC_LINKING;
		return 1;
	}
#endif
	return Ultra_Init_internal();
}

/* Closes the driver */
static void Ultra_Exit_internal(void)
{
#if LIBGUS_VERSION_MAJOR < 0x0004
	if(gf1flag>=0) {
		gf1flag=-1;
		libgus_close(ultra_dev);
	}
#else
	if (ultra_h) {
		libgus_close(ultra_h);
		ultra_h = NULL;
	}
#endif
	ultra_fd=-1;
}

static void Ultra_Exit(void)
{
	Ultra_Exit_internal();
#ifdef MIKMOD_DYNAMIC
	Ultra_Unlink();
#endif
}

/* Poor man's reset function */
static BOOL Ultra_Reset(void)
{
	Ultra_Exit_internal();
	return Ultra_Init_internal();
}

static BOOL Ultra_SetNumVoices(void)
{
	return 0;
}

/* Start playback */
static BOOL Ultra_PlayStart(void) {
	int t;

	for (t=0;t<md_hardchn;t++) {
		voices[t].flags  =0;
		voices[t].handle =0;
		voices[t].size   =0;
		voices[t].start  =0;
		voices[t].reppos =0;
		voices[t].repend =0;
		voices[t].changes=0;
		voices[t].kick   =0;
		voices[t].frq    =10000;
		voices[t].vol    =64;
		voices[t].pan    =ULTRA_PAN_MIDDLE;
		voices[t].active =0;
	}

#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_select(gf1flag);
	if (libgus_reset(md_hardchn,0)<0)
#else
	if (libgus_reset(ultra_h,md_hardchn,0)<0)
#endif
	{
		_mm_errno=MMERR_GUS_RESET;
		return 1;
	}
	if(loadsamples()) return 1;

#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_queue_write_set_size(1024);
	libgus_queue_read_set_size(128);
#else
	libgus_queue_write_set_size(ultra_h,1024);
	libgus_queue_read_set_size(ultra_h,128);
#endif

#if LIBGUS_VERSION_MAJOR < 0x0004
	if (libgus_timer_start()<0)
#else
	if (libgus_timer_start(ultra_h)<0)
#endif
	{
		_mm_errno=MMERR_GUS_TIMER;
		return 1;
	}
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_timer_tempo(50);
#else
	libgus_timer_tempo(ultra_h,50);
#endif
	ultra_bpm=0;

	for (t=0;t<md_hardchn;t++) {
#if LIBGUS_VERSION_MAJOR < 0x0004
		libgus_do_voice_pan(t,ULTRA_PAN_MIDDLE);
		libgus_do_voice_volume(t,64<<8);
#else
		libgus_do_voice_pan(ultra_h,t,ULTRA_PAN_MIDDLE);
		libgus_do_voice_volume(ultra_h,t,64<<8);
#endif
	}

#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_do_flush();
#else
	libgus_do_flush(ultra_h);
#endif

	return 0;
}

/* Stop playback */
static void Ultra_PlayStop(void)
{
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_queue_flush();

	libgus_timer_stop();

	libgus_queue_write_set_size(0);
	libgus_queue_read_set_size(0);
#else
	libgus_queue_flush(ultra_h);

	libgus_timer_stop(ultra_h);

	libgus_queue_write_set_size(ultra_h,0);
	libgus_queue_read_set_size(ultra_h,0);
#endif
}

/* Module player tick function */
static void ultraplayer(void)
{
	int t;
	struct GUS_VOICE *voice;

	md_player();

	for(t=0,voice=voices;t<md_numchn;t++,voice++) {
		if (voice->changes & CH_FREQ)
#if LIBGUS_VERSION_MAJOR < 0x0004
			libgus_do_voice_frequency(t,voice->frq);
#else
			libgus_do_voice_frequency(ultra_h,t,voice->frq);
#endif
		if (voice->changes & CH_VOL);
#if LIBGUS_VERSION_MAJOR < 0x0004
			libgus_do_voice_volume(t,voice->vol<<8);
#else
			libgus_do_voice_volume(ultra_h,t,voice->vol<<8);
#endif
		if (voice->changes & CH_PAN) {
			if (voice->pan==PAN_SURROUND)
#if LIBGUS_VERSION_MAJOR < 0x0004
				libgus_do_voice_pan(t,ULTRA_PAN_MIDDLE);
#else
				libgus_do_voice_pan(ultra_h,t,ULTRA_PAN_MIDDLE);
#endif
			else
#if LIBGUS_VERSION_MAJOR < 0x0004
				libgus_do_voice_pan(t,voice->pan<<6);
#else
				libgus_do_voice_pan(ultra_h,t,voice->pan<<6);
#endif
		}
		voice->changes=0;
		if (voice->kick) {
			voice->kick=0;
			if (voice->start>0)
#if LIBGUS_VERSION_MAJOR < 0x0004
				libgus_do_voice_start_position(t,voice->handle,voice->frq,
				                            voice->vol<<8,voice->pan<<6,
				                            voice->start<<4);
#else
				libgus_do_voice_start_position(ultra_h,t,voice->handle,voice->frq,
				                            voice->vol<<8,voice->pan<<6,
				                            voice->start<<4);
#endif
			else
#if LIBGUS_VERSION_MAJOR < 0x0004
				libgus_do_voice_start(t,voice->handle,voice->frq,
				                   voice->vol<<8,voice->pan<<6);
#else
				libgus_do_voice_start(ultra_h,t,voice->handle,voice->frq,
				                   voice->vol<<8,voice->pan<<6);
#endif
		}
	}
#if LIBGUS_VERSION_MAJOR < 0x0004
	libgus_do_wait(1);
#else
	libgus_do_wait(ultra_h,1);
#endif
}

/* Play sound */
static void Ultra_Update(void)
{
	fd_set write_fds;
	int need_write;

	if (ultra_bpm!=md_bpm) {
#if LIBGUS_VERSION_MAJOR < 0x0004
		libgus_do_tempo((md_bpm*50)/125);
#else
		libgus_do_tempo(ultra_h,(md_bpm*50)/125);
#endif
		ultra_bpm=md_bpm;
	}

	ultraplayer();

	do {
#if LIBGUS_VERSION_MAJOR < 0x0004
		need_write=libgus_do_flush();
#else
		need_write=libgus_do_flush(ultra_h);
#endif

		FD_ZERO(&write_fds);
		do {
			FD_SET(ultra_fd,&write_fds);

			select(ultra_fd+1,NULL,&write_fds,NULL,NULL);
		} while (!FD_ISSET(ultra_fd,&write_fds));
	} while (need_write>0);
}

/* Set the volume for the given voice */
static void Ultra_VoiceSetVolume(UBYTE voice,UWORD vol)
{
	if(voice<md_numchn)
		if (vol!=voices[voice].vol) {
			voices[voice].vol=vol;
			voices[voice].changes|=CH_VOL;
		}
}

/* Returns the volume of the given voice */
static UWORD Ultra_VoiceGetVolume(UBYTE voice)
{
	return (voice<md_numchn)?voices[voice].vol:0;
}

/* Set the pitch for the given voice */
static void Ultra_VoiceSetFrequency(UBYTE voice,ULONG frq)
{
	if(voice<md_numchn)
		if (frq!=voices[voice].frq) {
			voices[voice].frq=frq;
			voices[voice].changes|=CH_FREQ;
		}
}

/* Returns the frequency of the given voice */
static ULONG Ultra_VoiceGetFrequency(UBYTE voice)
{
	return (voice<md_numchn)?voices[voice].frq:0;
}

/* Set the panning position for the given voice */
static void Ultra_VoiceSetPanning(UBYTE voice,ULONG pan)
{
	if(voice<md_numchn)
		if (pan!=voices[voice].pan) {
			voices[voice].pan=pan;
			voices[voice].changes|=CH_PAN;
		}
}

/* Returns the panning of the given voice */
static ULONG Ultra_VoiceGetPanning(UBYTE voice)
{
	return (voice<md_numchn)?voices[voice].pan:0;
}

/* Start a new sample on a voice */
static void Ultra_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
	if((voice>=md_numchn)||(start>=size)) return;

	if (flags&SF_LOOP)
		if (repend>size) repend=size;

	voices[voice].flags  =flags;
	voices[voice].handle =handle;
	voices[voice].start  =start;
	voices[voice].size   =size;
	voices[voice].reppos =reppos;
	voices[voice].repend =repend;
	voices[voice].kick   =1;
	voices[voice].active =1;
	voices[voice].started=time(NULL);
}

/* Stops a voice */
static void Ultra_VoiceStop(UBYTE voice)
{
	if(voice<md_numchn)
		voices[voice].active=0;
}

/* Returns whether a voice is stopped */
static BOOL Ultra_VoiceStopped(UBYTE voice)
{
	if(voice<md_numchn) return 1;

	if(voices[voice].active) {
		/* is sample looping ? */
		if(voices[voice].flags&(SF_LOOP|SF_BIDI))
			return 0;
		else
			if(time(NULL)-voices[voice].started>=
			   ((voices[voice].size-voices[voice].start+44099)/44100)) {
				voices[voice].active=0;
				return 1;
			}
		return 0;
	} else
		return 1;
}

/* Returns current voice position */
static SLONG Ultra_VoiceGetPosition(UBYTE voice)
{
	/* NOTE This information can not be determined. */
	return -1;
}

/* Returns voice real volume */
static ULONG Ultra_VoiceRealVolume(UBYTE voice)
{
	/* NOTE This information can not be accurately determined. */
	return 0;
}

MIKMODAPI MDRIVER drv_ultra={
	NULL,
	"Ultrasound driver",
	"Linux Ultrasound driver v1.12",
	GUS_CHANNELS,0,
	"ultra",

	NULL,
	Ultra_IsThere,
	Ultra_SampleLoad,
	Ultra_SampleUnload,
	Ultra_SampleSpace,
	Ultra_SampleLength,
	Ultra_Init,
	Ultra_Exit,
	Ultra_Reset,
	Ultra_SetNumVoices,
	Ultra_PlayStart,
	Ultra_PlayStop,
	Ultra_Update, 
	NULL,
	Ultra_VoiceSetVolume,
	Ultra_VoiceGetVolume,
	Ultra_VoiceSetFrequency,
	Ultra_VoiceGetFrequency,
	Ultra_VoiceSetPanning,
	Ultra_VoiceGetPanning,
	Ultra_VoicePlay,
	Ultra_VoiceStop,
	Ultra_VoiceStopped,
	Ultra_VoiceGetPosition,
	Ultra_VoiceRealVolume
};

#else

MISSING(drv_ultra);

#endif

/* ex:set ts=4: */
