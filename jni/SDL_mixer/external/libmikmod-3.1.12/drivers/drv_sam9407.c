/*  MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS
	for complete list.

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
  
  $Id: drv_sam9407.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $
  
  Driver for the Linux sam9407 driver
  
==============================================================================*/

/*

   Written by Gerd Rausch <gerd@alf.gun.de>
   Tempo contributions by Xavier Hosxe <xhosxe@cyrano.com>
   Released with libmikmod under LGPL license with the author's permission.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_SAM9407

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sys/sam9407.h>

/* Backwards compatibility with driver version 0.9.x */
#ifndef SAM_MOD_API_VERSION
#define SAM_MOD_API_VERSION SAM_API_VERSION
#endif

#define SAM_NUM_BANKS	256
#define SAM_NUM_VOICES	32
#define NO_LOOP_SAMPLES	8

#define PENDING_PLAY	(1<<0)
#define PENDING_STOP	(1<<1)
#define PENDING_VOLUME	(1<<2)
#define PENDING_FREQ	(1<<3)

typedef struct {
	BOOL inUse;

	ULONG length, loopstart, loopend;
	UWORD flags;
} Bank;

typedef struct {
	BOOL playing;

	ULONG pending;

	SWORD handle;
	ULONG start;
	ULONG length;
	ULONG loopstart;
	ULONG repend;
	UWORD flags;

	ULONG freq;
	UWORD vol;
	ULONG pan;
} Voice;

static Bank banks[SAM_NUM_BANKS];

static Voice voices[SAM_NUM_VOICES];

static int card=0;
static int modfd=-1;

static void commandLine(CHAR *cmdline)
{
	CHAR *ptr;

	if((ptr=MD_GetAtom("card", cmdline, 0))) {
		card=atoi(ptr);
		free(ptr);
	}
}

static BOOL isPresent(void)
{
	int fd;
	char devName[256];

	sprintf(devName, "/dev/sam%d_mod", card);
	if((fd=open(devName, O_RDWR))>=0) {
		close(fd);
		return 1;
	}
	return (errno == EACCES ? 1 : 0);
}

static ULONG freeSampleSpace(int type)
{
	SamModMemInfo memInfo;

	if(ioctl(modfd, SAM_IOC_MOD_MEM_INFO, &memInfo)<0)
		return 0;

	return memInfo.memFree<<1;
}

static SWORD sampleLoad(SAMPLOAD *s, int type)
{
	Bank *bankP;
	SamModSamples modSamples;
	SWORD handle;
	SWORD *samples;
int rc;

	for(handle=0; handle<SAM_NUM_BANKS; handle++)
		if(!banks[handle].inUse)
			break;
	if(handle>=SAM_NUM_BANKS) {
		_mm_errno=MMERR_OUT_OF_HANDLES;
		return -1;
	}

	bankP=banks+handle;

	bankP->inUse=1;

	SL_SampleSigned(s);
	SL_Sample8to16(s);

	bankP->length=s->sample->length;
	bankP->loopstart=s->sample->loopstart;
	bankP->loopend=s->sample->loopend ? s->sample->loopend : s->sample->length;
	bankP->flags=s->sample->flags;

	if(!(samples=(SWORD *)malloc((bankP->length+NO_LOOP_SAMPLES)<<1))) {
		bankP->inUse=0;
		_mm_errno=MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	if(SL_Load(samples, s, bankP->length)) {
		free(samples);
		bankP->inUse=0;
		_mm_errno=MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	if(!(bankP->flags & SF_LOOP)) {
		memset(samples+bankP->length, 0, NO_LOOP_SAMPLES);
		bankP->loopstart=bankP->length;
		bankP->length+=NO_LOOP_SAMPLES;
		bankP->loopend=bankP->length-1;
	}

	modSamples.handle=handle;
	modSamples.data=samples;
	modSamples.size=bankP->length;
	if((rc=ioctl(modfd, SAM_IOC_MOD_SAMPLES_LOAD, &modSamples))<0) {
		free(samples);
		bankP->inUse=0;
		_mm_errno=MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	free(samples);

	return handle;
}

void sampleUnload(SWORD handle)
{
	unsigned char modHandle;

	if(!banks[handle].inUse)
		return;

	modHandle=handle;
	ioctl(modfd, SAM_IOC_MOD_SAMPLES_UNLOAD, &modHandle);
	banks[handle].inUse=0;
}

static ULONG realSampleLength(int type, SAMPLE *s)
{
	if(!s)
		return 0;

	return s->length<<1;
}

static BOOL init(void)
{
	int i;
	Voice *voiceP;
	char devName[256];
	SamApiInfo apiInfo;

	sprintf(devName, "/dev/sam%d_mod", card);
	if((modfd=open(devName, O_RDWR))<0) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	if(ioctl(modfd, SAM_IOC_API_INFO, &apiInfo)<0 ||
	   apiInfo.version!=SAM_MOD_API_VERSION) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	for(i=0; i<SAM_NUM_VOICES; i++) {
		voiceP=voices+i;
		voiceP->freq=44100;
		voiceP->vol=0xFF;
		voiceP->pan=0x80;
	}

	md_mode&=~(DMODE_SOFT_MUSIC|DMODE_SOFT_SNDFX);

	return 0;
}

static void exitHook(void)
{
	if(modfd>=0) {
		close(modfd);
		modfd=-1;
	}
}

static BOOL reset(void)
{
	if(ioctl(modfd, SAM_IOC_MOD_RESET)<0)
		return 1;

	return 0;
}

static BOOL setNumVoices(void)
{
	return 0;
}

static BOOL playStart(void)
{
	ioctl(modfd, SAM_IOC_MOD_TIMER_START);
	return 0;
}

static void playStop(void)
{
	ioctl(modfd, SAM_IOC_MOD_POST);
	ioctl(modfd, SAM_IOC_MOD_TIMER_STOP);
}

static void update(void)
{
	static int wait_echo;
	static long ticker_idx;

	int i;
	Voice *voiceP;
	SamModEvent event, eventBuf[SAM_NUM_VOICES*10+8], *eventP;

	eventP = eventBuf;

	for(i=0; i<md_numchn; i++) {
		voiceP=voices+i;

		if((voiceP->pending & PENDING_STOP) ||
		   ((voiceP->pending & PENDING_PLAY) && voiceP->playing)) {
			voiceP->playing=0;

			eventP->type=SAM_MOD_EVENT_STOP;
			eventP->trigger.voice=i;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModTriggerEvent));

			eventP->type=SAM_MOD_EVENT_CLOSE;
			eventP->trigger.voice=i;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModTriggerEvent));

			voiceP->pending&=~PENDING_STOP;
		}

		if(voiceP->pending & PENDING_PLAY) {
			Bank *bankP=banks+voiceP->handle;

			eventP->type=SAM_MOD_EVENT_OPEN;
			eventP->trigger.voice=i;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModTriggerEvent));

			eventP->type=SAM_MOD_EVENT_MEM;
			eventP->mem.voice=i;
			eventP->mem.handle=voiceP->handle;
			eventP->mem.sampleType=SamModSample16Bit;
			eventP->mem.loopType=(bankP->flags & SF_BIDI) ? SamModLoopReverse : SamModLoopForward;
			eventP->mem.start=voiceP->start;
			eventP->mem.loop=bankP->loopstart;
			eventP->mem.end=bankP->loopend;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModMemEvent));

			eventP->type=SAM_MOD_EVENT_VOL;
			eventP->volOut.voice=i;
			eventP->volOut.volume=0xFF;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModVolOutEvent));

			voiceP->pending|=PENDING_VOLUME|PENDING_FREQ;
		}

		if(voiceP->pending & PENDING_VOLUME) {
			eventP->type=SAM_MOD_EVENT_MAIN;
			eventP->volSend.voice=i;
			eventP->volSend.left=voiceP->vol*(255-voiceP->pan)/255;
			eventP->volSend.right=voiceP->vol*voiceP->pan/255;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModVolSendEvent));

			voiceP->pending&=~PENDING_VOLUME;
		}

		if(voiceP->pending & PENDING_FREQ) {
			eventP->type=SAM_MOD_EVENT_PITCH;
			eventP->pitch.voice=i;
			eventP->pitch.freq=voiceP->freq;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModPitchEvent));

			voiceP->pending&=~PENDING_FREQ;
		}

		if(voiceP->pending & PENDING_PLAY) {
			voiceP->playing=1;

			eventP->type=SAM_MOD_EVENT_START;
			eventP->trigger.voice=i;
			eventP=(SamModEvent *)((char *)eventP+sizeof(SamModTriggerEvent));

			voiceP->pending&=~PENDING_PLAY;
		}
	}

	eventP->type=SAM_MOD_EVENT_WAIT_REL;
	eventP->timer.time.tv_sec=0;
	eventP->timer.time.tv_usec=2500000/md_bpm;
	eventP=(SamModEvent *)((char *)eventP+sizeof(SamModTimerEvent));

	eventP->type=SAM_MOD_EVENT_ECHO;
	eventP->echo.closure=(void *)(ticker_idx+1);
	eventP=(SamModEvent *)((char *)eventP+sizeof(SamModEchoEvent));

	if(wait_echo)
		while(read(modfd, &event, sizeof(event))>0 && (long)event.echo.closure!=ticker_idx);
	wait_echo=1;
	ticker_idx++;

	(*md_player)();

	write(modfd, eventBuf, (char *)eventP-(char *)eventBuf);
}

static void voiceSetVolume(UBYTE voice, UWORD vol)
{
	if(voice>=SAM_NUM_VOICES)
		return;

	voices[voice].vol=vol<=255 ? vol : 255;
	voices[voice].pending|=PENDING_VOLUME;
}

static UWORD voiceGetVolume(UBYTE voice)
{
	return voice<SAM_NUM_VOICES ? voices[voice].vol : 0;
}

static void voiceSetFrequency(UBYTE voice, ULONG freq)
{
	if(voice>=SAM_NUM_VOICES)
		return;

	voices[voice].freq=freq;
	voices[voice].pending|=PENDING_FREQ;
}

static ULONG voiceGetFrequency(UBYTE voice)
{
	return voice<SAM_NUM_VOICES ? voices[voice].freq : 0;
}

static void voiceSetPanning(UBYTE voice, ULONG pan)
{
	if(voice>=SAM_NUM_VOICES)
		return;

	voices[voice].pan=pan<=255 ? pan : 255;
	voices[voice].pending|=PENDING_VOLUME;
}

static ULONG voiceGetPanning(UBYTE voice)
{
	return voice<SAM_NUM_VOICES ? voices[voice].pan : 0x80;
}

static void voicePlay(UBYTE voice, SWORD handle, ULONG start, ULONG length, ULONG loopstart, ULONG repend, UWORD flags)
{
	Voice *voiceP;

	if(voice>=SAM_NUM_VOICES)
		return;

	voiceP=voices+voice;

	voiceP->handle=handle;
	voiceP->flags=flags;
	voiceP->start=start;
	voiceP->length=length;
	voiceP->loopstart=loopstart;
	voiceP->repend=repend;
	voiceP->flags=flags;

	voiceP->pending&=~PENDING_STOP;
	voiceP->pending|=PENDING_PLAY;
}

static void voiceStop(UBYTE voice)
{
	if(voice>=SAM_NUM_VOICES)
		return;

	voices[voice].pending&=~PENDING_PLAY;
	voices[voice].pending|=PENDING_STOP;
}

static BOOL voiceStopped(UBYTE voice)
{
	if(voice>=SAM_NUM_VOICES)
		return 1;

	return voice<SAM_NUM_VOICES ? !voices[voice].playing : 1;
}

static SLONG voiceGetPosition(UBYTE voice)
{
	return -1;
}

static ULONG voiceRealVolume(UBYTE voice)
{
	return 0;
}

MIKMODAPI MDRIVER drv_sam9407={
	NULL,
	"sam9407 driver",
	"Linux sam9407 driver v1.0",
	SAM_NUM_VOICES, 0,
	"sam9407",

	commandLine,
	isPresent,
	sampleLoad,
	sampleUnload,
	freeSampleSpace,
	realSampleLength,
	init,
	exitHook,
	reset,
	setNumVoices,
	playStart,
	playStop,
	update,
	NULL,
	voiceSetVolume,
	voiceGetVolume,
	voiceSetFrequency,
	voiceGetFrequency,
	voiceSetPanning,
	voiceGetPanning,
	voicePlay,
	voiceStop,
	voiceStopped,
	voiceGetPosition,
	voiceRealVolume
};

#else

MISSING(drv_sam9407);

#endif

/* ex:set ts=4: */
