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

  $Id: drv_AF.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on AF audio server.

==============================================================================*/

/*

	Written by Roine Gustafsson <e93_rog@e.kth.se>
  
	Portability:
	Unixes running Digital AudioFile library, available from
	ftp://crl.dec.com/pub/DEC/AF

	Usage:
	Run the audio server (Aaxp&, Amsb&, whatever)
	Set environment variable AUDIOFILE to something like 'mymachine:0'.
	Remember, stereo is default! See commandline switches.

	I have a version which uses 2 computers for stereo.
	Contact me if you want it.
  
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_AF

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#include <AF/AFlib.h>


/* Global variables */

static	SBYTE *audiobuffer=NULL;
static	int AFFragmentSize;
static	AFAudioConn *AFaud=NULL;
static	ATime AFtime;
static	AC AFac;
static	AFDeviceDescriptor *AFdesc;
static	CHAR *soundbox=NULL;

static void AF_CommandLine(CHAR *cmdline)
{
	CHAR *machine=MD_GetAtom("machine",cmdline,0);

	if(machine) {
		if(soundbox) free(soundbox);
		soundbox=machine;
	}
}

static BOOL AF_IsThere(void)
{
	if ((AFaud=AFOpenAudioConn(soundbox)))
		AFCloseAudioConn(AFaud);
		AFaud=NULL;
		return 1;
	} else
		return 0;
}

static BOOL AF_Init(void)
{
	unsigned long mask;
	AFSetACAttributes attributes;
	int srate;
	ADevice device;
	int n;
  
	if (!(AFaud=AFOpenAudioConn(soundbox))) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	/* Search for a suitable device */
	device=-1;
	for (n=0;n<AFaud->ndevices;n++) {
		AFdesc=AAudioDeviceDescriptor(AFaud,n);
		if ((AFdesc->playNchannels==2)&&(md_mode&DMODE_STEREO)) {
			device=n;
			break;
		}
		if ((AFdesc->playNchannels==1)&&!(md_mode&DMODE_STEREO)) {
			device=n;
			break;
		}
	}
	if (device==-1) {
		_mm_errno=MMERR_AF_AUDIO_PORT;
		return 1;
	}

	attributes.preempt=Mix;
	attributes.start_timeout=0;
	attributes.end_silence=0;
	/* in case of an 8bit device, the AF converts the 16 bit data to 8 bit */
	attributes.type=LIN16;
	attributes.channels=(md_mode&DMODE_STEREO)?Stereo:Mono;

	mask=ACPreemption|ACEncodingType|ACStartTimeout|ACEndSilence|ACChannels;
	AFac=AFCreateAC(AFaud,device,mask,&attributes);
	srate=AFac->device->playSampleFreq;

	md_mode|=DMODE_16BITS;			/* This driver only handles 16bits */
	AFFragmentSize=(srate/40)*8;	/* Update 5 times/sec */
	md_mixfreq=srate;				/* set mixing freq */

	if (md_mode&DMODE_STEREO) {
		if (!(audiobuffer=(SBYTE*)_mm_malloc(2*2*AFFragmentSize))) 
			return 1;
	} else {
		if (!(audiobuffer=(SBYTE*)_mm_malloc(2*AFFragmentSize))) 
			return 1;
	}
  
	return VC_Init();
}

static BOOL AF_PlayStart(void)
{
	AFtime=AFGetTime(AFac);
	return VC_PlayStart();
}

static void AF_Exit(void)
{
	VC_Exit();
	_mm_free(audiobuffer);
	if (AFaud) {
		AFCloseAudioConn(AFaud);
		AFaud=NULL;
	}
}

static void AF_Update(void)
{
	ULONG done;
  
	done=VC_WriteBytes(audiobuffer,AFFragmentSize);
	if (md_mode&DMODE_STEREO) {
		AFPlaySamples(AFac,AFtime,done,(unsigned char*)audiobuffer);
		AFtime+=done/4;
		/* while (AFGetTime(AFac)<AFtime-1000); */
	} else {
		AFPlaySamples(AFac,AFtime,done,(unsigned char*)audiobuffer);
		AFtime+=done/2;
		/* while (AFGetTime(AFac)<AFtime-1000); */
	}
}

MIKMODAPI MDRIVER drv_AF={
	NULL,
	"AF driver",
	"AudioFile driver v1.3",
	0,255,
	"audiofile",

	AF_CommandLine,	
	AF_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	AF_Init,
	AF_Exit,
	NULL,
	VC_SetNumVoices,
	AF_PlayStart,
	VC_PlayStop,
	AF_Update,
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

MISSING(drv_AF);

#endif

/* ex:set ts=4: */
