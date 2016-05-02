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

  $Id: drv_aix.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output to AIX series audio device

==============================================================================*/

/*

	Written by Lutz Vieweg <lkv@mania.robin.de>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_AIX

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdlib.h>
#include <sys/audio.h>
#include <sys/acpa.h>

#define BUFFERSIZE 15
static	int buffersize=1<<BUFFERSIZE;
static	int fd=-1;
static	SBYTE *audiobuffer=NULL;

static void AIX_CommandLine(CHAR *cmdline)
{
	char *buffer=MD_GetAtom("buffer",cmdline,0);

	if(buffer) {
		int buf=atoi(buffer);

		if((buf<12)||(buf>19)) buf=BUFFERSIZE;
		buffersize=1<<buf;

		free(buffer);
	}
}

static BOOL AIX_IsThere(void)
{
	int fd;

	if((fd=open("/dev/acpa0/1",O_WRONLY))<0)
		if((fd=open("/dev/paud0/1",O_WRONLY))<0)
			if((fd=open("/dev/baud0/1",O_WRONLY))<0)
				return 0;
	close(fd);
	return 1;
}

static BOOL AIX_Init(void)
{
	audio_init    a_init;
	audio_control a_control;
	audio_change  a_change;
	track_info    t_info;

	if (!(md_mode&DMODE_16BITS)) {
		_mm_errno=MMERR_16BIT_ONLY;
		return 1;
	}
	
	if((fd=open("/dev/acpa0/1",O_WRONLY))<0)
		if((fd=open("/dev/paud0/1",O_WRONLY))<0)
			if((fd=open("/dev/baud0/1",O_WRONLY))<0) {
				_mm_errno=MMERR_OPENING_AUDIO;
				return 1;
			}
	
	t_info.master_volume=0x7fff;
	t_info.dither_percent=0;
	
	a_init.srate=md_mixfreq;
	a_init.bits_per_sample=16;
	a_init.channels=(md_mode&DMODE_STEREO)?2:1;
	a_init.mode=PCM;
	a_init.flags=FIXED|BIG_ENDIAN|TWOS_COMPLEMENT;
	a_init.operation=PLAY;
	
	a_change.balance=0x3fff0000;
	a_change.balance_delay=0;
	a_change.volume=(long)((float)1.0*(float)0x7FFFFFFF);
	a_change.volume_delay=0;
	a_change.monitor=AUDIO_IGNORE;
	a_change.input=AUDIO_IGNORE;
	a_change.output=OUTPUT_1;
	a_change.dev_info=&t_info;

	a_control.ioctl_request=AUDIO_CHANGE;
	a_control.position=0;
	a_control.request_info=&a_change;

	if (ioctl(fd,AUDIO_INIT,&a_init)==-1) {
		_mm_errno=MMERR_AIX_CONFIG_INIT;
		return 1;
	}
	if (ioctl(fd,AUDIO_CONTROL,&a_control)==-1) {
		_mm_errno=MMERR_AIX_CONFIG_CONTROL;
		return 1;
	}

	a_control.ioctl_request=AUDIO_START;
	a_control.request_info=NULL;
	if (ioctl(fd,AUDIO_CONTROL,&a_control)==-1) {
		_mm_errno=MMERR_AIX_CONFIG_START;
		return 1;
	}
	
	if (!(audiobuffer=(SBYTE*)_mm_malloc(buffersize))) return 1;
	
	return VC_Init();
}

static void AIX_Exit(void)
{
	VC_Exit();
	if (fd>=0) {
		close(fd);
		fd=-1;
	}
	_mm_free(audiobuffer);
}

static void AIX_Update(void)
{
	write(fd,audiobuffer,VC_WriteBytes(audiobuffer,buffersize));
}

static void AIX_Pause(void)
{
	write(fd,audiobuffer,VC_SilenceBytes(audiobuffer,buffersize));
}

MIKMODAPI MDRIVER drv_aix={
	NULL,
	"AIX Audio",
	"AIX Audio driver v1.2",
	0,255,
	"AIX",

	AIX_CommandLine,
	AIX_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	AIX_Init,
	AIX_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	AIX_Update,
	AIX_Pause,
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

MISSING(drv_aix);

#endif

/* ex:set ts=4: */
