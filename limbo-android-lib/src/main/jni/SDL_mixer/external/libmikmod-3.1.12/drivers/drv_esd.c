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

  $Id: drv_esd.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for the Enlightened sound daemon (EsounD)

==============================================================================*/

/*

	You should set the hostname of the machine running esd in the environment
	variable 'ESPEAKER'. If this variable is not set, localhost is used.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_ESD

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef MIKMOD_DYNAMIC
#include <dlfcn.h>
#endif
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <esd.h>

#ifdef MIKMOD_DYNAMIC
/* runtime link with libesd */
/* note that since we only need the network part of EsounD, we don't need to
   load libasound.so or libaudiofile.so as well */
static int(*esd_closestream)(int);
static int(*esd_playstream)(esd_format_t,int,char*,char*);
static void* libesd=NULL;
#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif
#else /* MIKMOD_DYNAMIC */
/* compile-time link with libesd */
#define esd_closestream esd_close
#define esd_playstream esd_play_stream
#endif

#ifdef HAVE_SETENV
#define SETENV setenv("ESD_NO_SPAWN","1",0)
#else
#define SETENV putenv("ESD_NO_SPAWN=1")
#endif

static	int sndfd=-1;
static	esd_format_t format;
static	SBYTE *audiobuffer=NULL;
static	CHAR *espeaker=NULL;

#ifdef MIKMOD_DYNAMIC
/* straight from audio.c in esd sources */
esd_format_t esd_audio_format=ESD_BITS16|ESD_STEREO;
int esd_audio_rate=ESD_DEFAULT_RATE;
#ifdef MIKMOD_DYNAMIC_ESD_NEEDS_ALSA
/* straight from audio_alsa.c in esd 0.2.[678] sources
   non-static variables that should be static are a *bad* thing... */
void *handle;
int writes;
#endif

static BOOL ESD_Link(void)
{
	if (libesd) return 0;

	/* load libesd.so */
	libesd=dlopen("libesd.so",RTLD_LAZY|RTLD_GLOBAL);
	if (!libesd) return 1;

	/* resolve function references */
	if (!(esd_closestream=dlsym(libesd,"esd_close"))) return 1;
	if (!(esd_playstream=dlsym(libesd,"esd_play_stream"))) return 1;

	return 0;
}

static void ESD_Unlink(void)
{
#ifdef HAVE_ESD_CLOSE
	esd_closestream=NULL;
#endif
	esd_playstream=NULL;

	if (libesd) {
		dlclose(libesd);
		libesd=NULL;
	}
}
#endif

/* I hope to have this function integrated into libesd someday...*/
static ssize_t esd_writebuf(int fd,const void *buffer,size_t count)
{
	ssize_t start=0;

	while (start<count) {
		ssize_t res;

		res=write(fd,(char*)buffer+start,count-start);
		if (res<0) {
			/* connection lost */
			if (errno==EPIPE)
				return -1-start;
		} else
			start+=res;
	}
	return start;
}

static void ESD_CommandLine(CHAR *cmdline)
{
	CHAR *ptr=MD_GetAtom("machine",cmdline,0);

	if (ptr) {
		if (espeaker) free(espeaker);
		espeaker=ptr;
	}
}

static BOOL ESD_IsThere(void)
{
	int fd,retval;

#ifdef MIKMOD_DYNAMIC
	if (ESD_Link()) return 0;
#endif
	/* Try to esablish a connection with default esd settings, but we don't
	   want esdlib to spawn esd if esd is not running ! */
	if (SETENV)
		retval=0;
	else {
		if ((fd=esd_playstream(ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,
		                       ESD_DEFAULT_RATE,espeaker,"libmikmod"))<0)
			retval=0;
		else {
			esd_closestream(fd);
			retval=1;
		}
	}
#ifdef MIKMOD_DYNAMIC
	ESD_Unlink();
#endif
	return retval;
}

static BOOL ESD_Init_internal(void)
{
	format=(md_mode&DMODE_16BITS?ESD_BITS16:ESD_BITS8)|
	       (md_mode&DMODE_STEREO?ESD_STEREO:ESD_MONO)|ESD_STREAM|ESD_PLAY;
	
	if (md_mixfreq > ESD_DEFAULT_RATE)
		md_mixfreq = ESD_DEFAULT_RATE;

	/* make sure we can open an esd stream with our parameters */
	if (!(SETENV)) {
		if ((sndfd=esd_playstream(format,md_mixfreq,espeaker,"libmikmod"))<0) {
			_mm_errno=MMERR_OPENING_AUDIO;
			return 1;
		}
	} else {
		_mm_errno=MMERR_OUT_OF_MEMORY;
		return 1;
	}

	if (!(audiobuffer=(SBYTE*)_mm_malloc(ESD_BUF_SIZE*sizeof(char))))
		return 1;

	return VC_Init();
}

static BOOL ESD_Init(void)
{
#ifdef MIKMOD_DYNAMIC
	if (ESD_Link()) {
		_mm_errno=MMERR_DYNAMIC_LINKING;
		return 1;
	}
#endif
	return ESD_Init_internal();
}

static void ESD_Exit_internal(void)
{
	VC_Exit();
	_mm_free(audiobuffer);
	if (sndfd>=0) {
		esd_closestream(sndfd);
		sndfd=-1;
		signal(SIGPIPE,SIG_DFL);
	}
}

static void ESD_Exit(void)
{
	ESD_Exit_internal();
#ifdef MIKMOD_DYNAMIC
	ESD_Unlink();
#endif
}

static void ESD_Update_internal(int count)
{
	static time_t losttime;

	if (sndfd>=0) {
		if (esd_writebuf(sndfd,audiobuffer,count)<0) {
			/* if we lost our connection with esd, clean up and work as the
			   nosound driver until we can reconnect */
			esd_closestream(sndfd);
			sndfd=-1;
			signal(SIGPIPE,SIG_DFL);
			losttime=time(NULL);
		}
	} else {
		/* an alarm would be better, but then the library user could not use
		   alarm(2) himself... */
		if (time(NULL)-losttime>=5) {
			losttime=time(NULL);
			/* Attempt to reconnect every 5 seconds */
			if (!(SETENV))
				if ((sndfd=esd_playstream(format,md_mixfreq,espeaker,"libmikmod"))>=0) {
					VC_SilenceBytes(audiobuffer,ESD_BUF_SIZE);
					esd_writebuf(sndfd,audiobuffer,ESD_BUF_SIZE);
				}
		}
	}
}

static void ESD_Update(void)
{
	ESD_Update_internal(VC_WriteBytes(audiobuffer,ESD_BUF_SIZE));
}

static void ESD_Pause(void)
{
	ESD_Update_internal(VC_SilenceBytes(audiobuffer,ESD_BUF_SIZE));
}

static BOOL ESD_PlayStart(void)
{
	if (sndfd<0)
		if (!(SETENV))
			if ((sndfd=esd_playstream(format,md_mixfreq,espeaker,"libmikmod"))<0) {
				_mm_errno=MMERR_OPENING_AUDIO;
				return 1;
			}
	/* since the default behaviour of SIGPIPE on most Unices is to kill the
	   program, we'll prefer handle EPIPE ourselves should the esd die - recent
	   esdlib use a do-nothing handler, which prevents us from receiving EPIPE,
	   so we override this */
	signal(SIGPIPE,SIG_IGN);

	/* silence buffers */
	VC_SilenceBytes(audiobuffer,ESD_BUF_SIZE);
	esd_writebuf(sndfd,audiobuffer,ESD_BUF_SIZE);

	return VC_PlayStart();
}

static void ESD_PlayStop(void)
{
	if (sndfd>=0) {
		/* silence buffers */
		VC_SilenceBytes(audiobuffer,ESD_BUF_SIZE);
		esd_writebuf(sndfd,audiobuffer,ESD_BUF_SIZE);

		signal(SIGPIPE,SIG_DFL);
	}

	VC_PlayStop();
}

static BOOL ESD_Reset(void)
{
	ESD_Exit_internal();
	return ESD_Init_internal();
}

MIKMODAPI MDRIVER drv_esd={
	NULL,
	"Enlightened sound daemon",
	/* use the same version number as the EsounD release it works best with */
	"Enlightened sound daemon (EsounD) driver v0.2.23",
	0,255,
	"esd",

	ESD_CommandLine,
	ESD_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	ESD_Init,
	ESD_Exit,
	ESD_Reset,
	VC_SetNumVoices,
	ESD_PlayStart,
	ESD_PlayStop,
	ESD_Update,
	ESD_Pause,
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

MISSING(drv_esd);

#endif

/* ex:set ts=4: */
