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

  $Id: drv_oss.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on Linux and FreeBSD Open Sound System (OSS) (/dev/dsp) 

==============================================================================*/

/*

	Written by Chris Conn <cconn@tohs.abacom.com>

	Extended by Miodrag Vallat:
	- compatible with all OSS/Voxware versions on Linux/i386, at least
	- support for uLaw output (for sparc systems)

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_OSS

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#endif
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

/* Compatibility with old versions of OSS
   (Voxware <= 2.03, Linux kernel < 1.1.31) */
#ifndef AFMT_S16_LE
#define AFMT_S16_LE 16
#endif
#ifndef AFMT_S16_BE
#define AFMT_S16_BE 0x20
#endif
#ifndef AFMT_U8
#define AFMT_U8 8
#endif
#ifndef SNDCTL_DSP_SETFMT
#define SNDCTL_DSP_SETFMT SNDCTL_DSP_SAMPLESIZE
#endif

/* Compatibility with not-so-old versions of OSS
   (OSS < 3.7, Linux kernel < 2.1.16) */
#ifndef AFMT_S16_NE
#if defined(_AIX) || defined(AIX) || defined(sparc) || defined(HPPA) || defined(PPC)
#define AFMT_S16_NE AFMT_S16_BE
#else
#define AFMT_S16_NE AFMT_S16_LE
#endif
#endif

static	int sndfd=-1;
static	SBYTE *audiobuffer=NULL;
static	int buffersize;
static	int play_precision;

#define DEFAULT_CARD 0
static	int card=DEFAULT_CARD;

#ifdef SNDCTL_DSP_SETFRAGMENT

#define DEFAULT_FRAGSIZE 14
#define DEFAULT_NUMFRAGS 16

static	int fragsize=DEFAULT_FRAGSIZE;
static	int numfrags=DEFAULT_NUMFRAGS;

#endif

static void OSS_CommandLine(CHAR *cmdline)
{
	CHAR *ptr;

#ifdef SNDCTL_DSP_SETFRAGMENT
	if((ptr=MD_GetAtom("buffer",cmdline,0))) {
		fragsize=atoi(ptr);
		if((fragsize<7)||(fragsize>17)) fragsize=DEFAULT_FRAGSIZE;
		free(ptr);
	}
	if((ptr=MD_GetAtom("count",cmdline,0))) {
		numfrags=atoi(ptr);
		if((numfrags<2)||(numfrags>255)) numfrags=DEFAULT_NUMFRAGS;
		free(ptr);
	}
#endif
	if((ptr=MD_GetAtom("card",cmdline,0))) {
		card = atoi(ptr);
		if((card<0)||(card>99)) card=DEFAULT_CARD;
		free(ptr);
	}
}

static char *OSS_GetDeviceName(void)
{
	static char sounddevice[20];
	
	/* First test for devfs enabled Linux sound devices */
	if (card)
		sprintf(sounddevice,"/dev/sound/dsp%d",card);
	else
		strcpy(sounddevice,"/dev/sound/dsp");
	if(!access(sounddevice,F_OK))
		return sounddevice;

	sprintf(sounddevice,"/dev/dsp%d",card);
	if (!card) {
		/* prefer /dev/dsp0 over /dev/dsp, as /dev/dsp might be a symbolic link
		   to something else than /dev/dsp0. Revert to /dev/dsp if /dev/dsp0
		   does not exist. */
		if(access("/dev/dsp0",F_OK))
			strcpy(sounddevice,"/dev/dsp");
	}
	
	return sounddevice;
}

static BOOL OSS_IsThere(void)
{
	/* under Linux, and perhaps other Unixes, access()ing the device is not
	   enough since it won't fail if the machine doesn't have sound support
	   in the kernel or sound hardware                                      */
	int fd;

	if((fd=open(OSS_GetDeviceName(),O_WRONLY))>=0) {
		close(fd);
		return 1;
	}
	return (errno==EACCES?1:0);
}

static BOOL OSS_Init_internal(void)
{
	int play_stereo,play_rate;
	int orig_precision,orig_stereo;
	long formats;
#if SOUND_VERSION >= 301
	audio_buf_info buffinf;
#endif

#ifdef SNDCTL_DSP_GETFMTS
	/* Ask device for supported formats */
	if(ioctl(sndfd,SNDCTL_DSP_GETFMTS,&formats)<0) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}
#else
	formats=AFMT_S16_NE|AFMT_U8;
#endif

	orig_precision=play_precision=(md_mode&DMODE_16BITS)?AFMT_S16_NE:AFMT_U8;

    /* Device does not support the format we would prefer... */
    if(!(formats & play_precision)) {
        /* We could try 8 bit sound if available */
        if(play_precision==AFMT_S16_NE &&(formats&AFMT_U8)) {
            _mm_errno=MMERR_8BIT_ONLY;
            return 1;
        }
#ifdef AFMT_MU_LAW
        /* We could try uLaw if available */
        if(formats&AFMT_MU_LAW) {
            if((md_mode&DMODE_STEREO)||(md_mode&DMODE_16BITS)||
                md_mixfreq!=8000) {
                _mm_errno=MMERR_ULAW;
                return 1;
            } else
                orig_precision=play_precision=AFMT_MU_LAW;
        } else
#endif
            /* Otherwise, just abort */
        {
            _mm_errno=MMERR_OSS_SETSAMPLESIZE;
            return 1;
        }
    }

	if((ioctl(sndfd,SNDCTL_DSP_SETFMT,&play_precision)<0)||
	   (orig_precision!=play_precision)) {
		_mm_errno=MMERR_OSS_SETSAMPLESIZE;
		return 1;
	}
#ifdef SNDCTL_DSP_CHANNELS
	orig_stereo=play_stereo=(md_mode&DMODE_STEREO)?2:1;
	if((ioctl(sndfd,SNDCTL_DSP_CHANNELS,&play_stereo)<0)||
	   (orig_stereo!=play_stereo)) {
		_mm_errno=MMERR_OSS_SETSTEREO;
		return 1;
	}
#else
	orig_stereo=play_stereo=(md_mode&DMODE_STEREO)?1:0;
	if((ioctl(sndfd,SNDCTL_DSP_STEREO,&play_stereo)<0)||
	   (orig_stereo!=play_stereo)) {
		_mm_errno=MMERR_OSS_SETSTEREO;
		return 1;
	}
#endif

	play_rate=md_mixfreq;
	if((ioctl(sndfd,SNDCTL_DSP_SPEED,&play_rate)<0)) {
		_mm_errno=MMERR_OSS_SETSPEED;
		return 1;
	}
	md_mixfreq=play_rate;

#if SOUND_VERSION >= 301
	/* This call fails on Linux/PPC */
	if((ioctl(sndfd,SNDCTL_DSP_GETOSPACE,&buffinf)<0))
		ioctl(sndfd,SNDCTL_DSP_GETBLKSIZE,&buffinf.fragsize);
	if(!(audiobuffer=(SBYTE*)_mm_malloc(buffinf.fragsize)))
		return 1;
	
	buffersize = buffinf.fragsize;
#else
	ioctl(sndfd,SNDCTL_DSP_GETBLKSIZE,&buffersize);
	if(!(audiobuffer=(SBYTE*)_mm_malloc(buffersize)))
		return 1;
#endif

	return VC_Init();
}

static BOOL OSS_Init(void)
{
#ifdef SNDCTL_DSP_SETFRAGMENT
	int fragmentsize;
#endif

	if((sndfd=open(OSS_GetDeviceName(),O_WRONLY))<0) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

#ifdef SNDCTL_DSP_SETFRAGMENT
	if((fragsize==DEFAULT_FRAGSIZE)&&(getenv("MM_FRAGSIZE"))) {
		fragsize=atoi(getenv("MM_FRAGSIZE"));
		if((fragsize<7)||(fragsize>17)) fragsize=DEFAULT_FRAGSIZE;
	}
	if((numfrags==DEFAULT_NUMFRAGS)&&(getenv("MM_NUMFRAGS"))) {
		numfrags=atoi(getenv("MM_NUMFRAGS"));
		if((numfrags<2)||(numfrags>255)) numfrags=DEFAULT_NUMFRAGS;
	}

	fragmentsize=(numfrags<<16)|fragsize;
	
	if(ioctl(sndfd,SNDCTL_DSP_SETFRAGMENT,&fragmentsize)<0) {
		_mm_errno=MMERR_OSS_SETFRAGMENT;
		return 1;
	}
#endif

	return OSS_Init_internal();
}

static void OSS_Exit_internal(void)
{
	VC_Exit();
	_mm_free(audiobuffer);
}

static void OSS_Exit(void)
{
	OSS_Exit_internal();

	if (sndfd>=0) {
		close(sndfd);
		sndfd=-1;
	}
}

static void OSS_PlayStop(void)
{
	VC_PlayStop();

	ioctl(sndfd,SNDCTL_DSP_POST);
}

static void OSS_Update(void)
{
	int done;

#if SOUND_VERSION >= 301
	audio_buf_info buffinf;

	buffinf.fragments = 2;
	for(;;) {
		/* This call fails on Linux/PPC */
		if ((ioctl(sndfd,SNDCTL_DSP_GETOSPACE,&buffinf)<0)) {
			buffinf.fragments--;
			buffinf.fragsize = buffinf.bytes = buffersize;
		}
		if(!buffinf.fragments)
			break;
		done=VC_WriteBytes(audiobuffer,buffinf.fragsize>buffinf.bytes?
						   buffinf.bytes:buffinf.fragsize);
#ifdef AFMT_MU_LAW
		if (play_precision==AFMT_MU_LAW)
			unsignedtoulaw(audiobuffer,done);
#endif
		write(sndfd,audiobuffer,done);
	}
#else
	done=VC_WriteBytes(audiobuffer,buffersize);
#ifdef AFMT_MU_LAW
	if (play_precision==AFMT_MU_LAW)
		unsignedtoulaw(audiobuffer,done);
#endif
	write(sndfd,audiobuffer,done);
#endif
}

static BOOL OSS_Reset(void)
{
	OSS_Exit_internal();
	ioctl(sndfd,SNDCTL_DSP_RESET);
	return OSS_Init_internal();
}

MIKMODAPI MDRIVER drv_oss={
	NULL,
	"Open Sound System",
	"Open Sound System driver v1.7",
	0,255,
	"oss",

	OSS_CommandLine,
	OSS_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	OSS_Init,
	OSS_Exit,
	OSS_Reset,
	VC_SetNumVoices,
	VC_PlayStart,
	OSS_PlayStop,
	OSS_Update,
	NULL,
	VC_VoiceSetVolume,
	VC_VoiceGetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceGetFrequency,
	VC_VoiceSetPanning,
	VC_VoiceGetPanning,
	VC_VoicePlay,
	VC_VoiceStop,
	VC_VoiceStopped,
	VC_VoiceGetPosition,
	VC_VoiceRealVolume
};

#else

MISSING(drv_oss);

#endif

/* ex:set ts=4: */
