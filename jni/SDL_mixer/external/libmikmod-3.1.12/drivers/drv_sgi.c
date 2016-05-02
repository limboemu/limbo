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

  $Id: drv_sgi.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on SGI audio system (needs libaudio from the dmedia
  package).

==============================================================================*/

/*

	Written by Stephan Kanthak <kanthak@i6.informatik.rwth-aachen.de>

	Fragment configuration:
	=======================

	You can use the driver options fragsize and bufsize to override the
	default size of the audio buffer. If you experience crackles & pops,
	try experimenting with these values.

	Please read the SGI section of libmikmod's README file first before
	contacting the author because there are some things to know about the
	specials of the SGI audio driver.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_SGI

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <dmedia/audio.h>

#define DEFAULT_SGI_FRAGSIZE  20000
#define DEFAULT_SGI_BUFSIZE   40000

static	ALconfig sgi_config;
static	ALport sgi_port;
static	int sample_factor;
static	int sgi_fragsize=DEFAULT_SGI_FRAGSIZE;
static	int sgi_bufsize=DEFAULT_SGI_BUFSIZE;
static	SBYTE *audiobuffer=NULL;

static void SGI_CommandLine(CHAR *cmdline)
{
	CHAR *ptr;
	
	if ((ptr=MD_GetAtom("fragsize",cmdline,0))) {
		sgi_fragsize=atol(ptr);
		free(ptr);
	} else sgi_fragsize=DEFAULT_SGI_FRAGSIZE;

	if ((ptr=MD_GetAtom("bufsize",cmdline,0))) {
		sgi_bufsize=atol(ptr);
		free(ptr);
	} else sgi_bufsize=DEFAULT_SGI_BUFSIZE;
}

static BOOL SGI_IsThere(void)
{
	ALseterrorhandler(0);
	return(ALqueryparams(AL_DEFAULT_DEVICE,0,0))?1:0;
}

static BOOL SGI_Init(void)
{
	long chpars[] = { AL_OUTPUT_RATE, AL_RATE_22050 };

	switch(md_mixfreq) {
		case 8000:
			chpars[1] = AL_RATE_8000;
			break;
		case 11025:
			chpars[1] = AL_RATE_11025;
			break;
		case 16000:
			chpars[1] = AL_RATE_16000;
			break;
		case 22050:
			chpars[1] = AL_RATE_22050;
			break;
		case 32000:
			chpars[1] = AL_RATE_32000;
			break;
		case 44100:
			chpars[1] = AL_RATE_44100;
			break;
		case 48000:
			chpars[1] = AL_RATE_48000;
			break;
		default:
			_mm_errno=MMERR_SGI_SPEED;
			return 1;
	}
	ALseterrorhandler(0);
	ALsetparams(AL_DEFAULT_DEVICE, chpars, 2);

	if (!(sgi_config=ALnewconfig())) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}
	
	if (md_mode&DMODE_16BITS) {
		if (ALsetwidth(sgi_config,AL_SAMPLE_16)<0) {
			_mm_errno=MMERR_SGI_16BIT;
			return 1;
		}
		sample_factor = 1;
	} else {
		if (ALsetwidth(sgi_config,AL_SAMPLE_8)<0) {
			_mm_errno=MMERR_SGI_8BIT;
			return 1;
		}
		sample_factor = 0;
	}

	if (md_mode&DMODE_STEREO) {
		if (ALsetchannels(sgi_config,AL_STEREO)<0) {
			_mm_errno=MMERR_SGI_STEREO;
			return 1;
		}
	} else {
		if (ALsetchannels(sgi_config,AL_MONO)<0) {
			_mm_errno=MMERR_SGI_MONO;
			return 1;
		}
	}

	if ((getenv("MM_SGI_FRAGSIZE"))&&(sgi_fragsize!=DEFAULT_SGI_FRAGSIZE))
		sgi_fragsize=atol(getenv("MM_SGI_FRAGSIZE"));
	if (!sgi_fragsize) sgi_fragsize=DEFAULT_SGI_FRAGSIZE;
	if ((getenv("MM_SGI_BUFSIZE"))&&(sgi_bufsize!=DEFAULT_SGI_BUFSIZE))
		sgi_bufsize=atol(getenv("MM_SGI_BUFSIZE"));
	if (!sgi_bufsize) sgi_fragsize=DEFAULT_SGI_BUFSIZE;

	ALsetqueuesize(sgi_config, sgi_bufsize);
	if (!(sgi_port=ALopenport("libmikmod","w",sgi_config))) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	if(!(audiobuffer=(SBYTE*)_mm_malloc(sgi_fragsize))) return 1;
	
	return VC_Init();
}

static void SGI_Exit(void)
{
	VC_Exit();
	_mm_free(audiobuffer);
}

static void SGI_Update(void)
{
	ALwritesamps(sgi_port,audiobuffer,
	             VC_WriteBytes(audiobuffer,sgi_fragsize)>>sample_factor);
}

MIKMODAPI MDRIVER drv_sgi={
	NULL,
	"SGI Audio System",
	"SGI Audio System driver v0.5",
	0,255,
	"sgi",

	SGI_CommandLine,
	SGI_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	SGI_Init,
	SGI_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	SGI_Update,
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

MISSING(drv_sgi);

#endif

/* ex:set ts=4: */
