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

  $Id: drv_hp.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output to HP 9000 series /dev/audio

==============================================================================*/

/*

	Written by Lutz Vieweg <lkv@mania.robin.de>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_HP

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdlib.h>
#include <sys/audio.h>

#define BUFFERSIZE 15

static	int fd=-1;
static	SBYTE *audiobuffer=NULL;
static	int buffersize=1<<BUFFERSIZE;
static	int headphone=0;

static void HP_CommandLine(CHAR *cmdline)
{
	char *buffer=MD_GetAtom("buffer",cmdline,0);

	if(buffer) {
		int buf=atoi(buffer);

		if((buf<12)||(buf>19)) buf=BUFFERSIZE;
		buffersize=1<<buf;

		free(buffer);
	}

	if((buffer=MD_GetAtom("headphone",cmdline,1))) {
		headphone=1;
		free(buffer);
	} else
		headphone=0;
}

static BOOL HP_IsThere(void)
{
	int fd;

	if((fd=open("/dev/audio",O_WRONLY))>=0) {
		close(fd);
		return 1;
	}
	return (errno==EACCES?1:0);
}

static BOOL HP_Init(void)
{
	int flags;
	
	if (!(md_mode&DMODE_16BITS)) {
		_mm_errno=MMERR_16BIT_ONLY;
		return 1;
	}

	if ((fd=open("/dev/audio",O_WRONLY|O_NDELAY,0))<0) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	if ((flags=fcntl(fd,F_GETFL,0))<0) {
		_mm_errno=MMERR_NON_BLOCK;
		return 1;
	}
	flags|=O_NDELAY;
	if (fcntl(fd,F_SETFL,flags)<0) {
		_mm_errno=MMERR_NON_BLOCK;
		return 1;
	}
	
	if (ioctl(fd,AUDIO_SET_DATA_FORMAT,AUDIO_FORMAT_LINEAR16BIT)) {
		_mm_errno=MMERR_HP_SETSAMPLESIZE;
		return 1;
	}
	
	if (ioctl(fd,AUDIO_SET_SAMPLE_RATE,md_mixfreq)) {
		_mm_errno=MMERR_HP_SETSPEED;
		return 1;
	}
	
	if (ioctl(fd,AUDIO_SET_CHANNELS,(md_mode&DMODE_STEREO)?2:1)) {
		_mm_errno=MMERR_HP_CHANNELS;
		return 1;
	}
	
	if (ioctl(fd,AUDIO_SET_OUTPUT,
	             headphone?AUDIO_OUT_HEADPHONE:AUDIO_OUT_SPEAKER)) {
		_mm_errno=MMERR_HP_AUDIO_OUTPUT;
		return 1;
	}

	if (ioctl(fd,AUDIO_SET_TXBUFSIZE,buffersize<<3)) {
		_mm_errno=MMERR_HP_BUFFERSIZE;
		return 1;
	}

	if (!(audiobuffer=(SBYTE*)_mm_malloc(buffersize))) return 1;
	
	return VC_Init();
}

static void HP_Exit(void)
{
	VC_Exit();
	if (fd>=0) {
		ioctl(fd,AUDIO_DRAIN,0);
		close(fd);
		fd=-1;
	}
	if (audiobuffer) {
		free(audiobuffer);
		audiobuffer=NULL;
	}
}

static void HP_Update(void)
{
	write(fd,audiobuffer,VC_WriteBytes(audiobuffer,buffersize));
}

MIKMODAPI MDRIVER drv_hp={
	NULL,
	"HP-UX Audio",
	"HP-UX Audio driver v1.3",
	0,255,
	"hp",

	HP_CommandLine,
	HP_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	HP_Init,
	HP_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	HP_Update,
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

MISSING(drv_hp);
		
#endif

/* ex:set ts=4: */
