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

  $Id: drv_mac.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output to the Macintosh Sound Manager

==============================================================================*/

/*
	Written by Anders F Bjoerklund <afb@algonet.se>

	Based on free MADlibrary code by Antoine Rosset <RossetAntoine@bluewin.ch>
	(author of PlayerPRO)
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifndef __SOUND__
#include <Sound.h>
#endif

#ifndef __GESTALT__
#include <Gestalt.h>
#endif

#define SOUND_BUFFER_SIZE 4096L

static SndChannelPtr         soundChannel=NULL; /* pointer to a sound channel */
static SndDoubleBufferHeader doubleHeader;      /* pointer to double buffers  */

static pascal void MyDoubleBackProc(SndChannelPtr soundChannel,SndDoubleBufferPtr doubleBuffer)
{
#pragma unused(soundChannel)
	SBYTE *buffer;
	long written;
#if TARGET_CPU_68K
	long oldA5=SetA5(doubleBuffer->dbUserInfo[0]);
#endif

	buffer=(SBYTE*)doubleBuffer->dbSoundData;

	if (Player_Paused()) {
		MUTEX_LOCK
			written=VC_SilenceBytes(buffer,(ULONG)SOUND_BUFFER_SIZE);
		MUTEX_UNLOCK
	} else {
		MUTEX_LOCK
			written=VC_WriteBytes(buffer,(ULONG)SOUND_BUFFER_SIZE);
		MUTEX_UNLOCK
	}

	if (doubleHeader.dbhNumChannels==2) written>>=1;
	if (doubleHeader.dbhSampleSize==16) written>>=1;
        
	doubleBuffer->dbNumFrames=written;
	doubleBuffer->dbFlags|=dbBufferReady;

#if TARGET_CPU_68K
	SetA5(oldA5);
#endif
}

static BOOL MAC_IsThere(void)
{
	NumVersion nVers;

	nVers=SndSoundManagerVersion();
	if (nVers.majorRev>=2)
		return 1; /* need SoundManager 2.0+ for SndPlayDoubleBuffer */
	else
		return 0;
}

static BOOL MAC_Init(void)
{
	OSErr err,iErr;
	int i;
	SndDoubleBufferPtr doubleBuffer;
	long rate,maxrate,maxbits;
	long gestaltAnswer;
	NumVersion nVers;
	Boolean Stereo,StereoMixing,Audio16;
	Boolean NewSoundManager,NewSoundManager31;

	NewSoundManager31=NewSoundManager=false;

	nVers=SndSoundManagerVersion();
	if (nVers.majorRev>=3) {
		NewSoundManager=true;
		if (nVers.minorAndBugRev>=0x10)
			NewSoundManager31=true;
	} else
	  if (nVers.majorRev<2)
		return 1; /* failure, need SoundManager 2.0+ for SndPlayDoubleBuffer */

	iErr=Gestalt(gestaltSoundAttr,&gestaltAnswer);
	if (iErr==noErr) {
		Stereo=(gestaltAnswer & (1<<gestaltStereoCapability))!=0;
		StereoMixing=(gestaltAnswer & (1<<gestaltStereoMixing))!=0;
		Audio16=(gestaltAnswer & (1<<gestalt16BitSoundIO))!=0;
	} else {
		/* failure, couldn't get any sound info at all ? */
		Stereo=StereoMixing=Audio16=false;
	}

#if !GENERATING68K || !GENERATINGCFM
	if (NewSoundManager31) {
		iErr=GetSoundOutputInfo(0L,siSampleRate,(void*)&maxrate);
		if (iErr==noErr)
			iErr=GetSoundOutputInfo(0L,siSampleSize,(void*)&maxbits);
	}

	if (iErr!=noErr) {
#endif
		maxrate=rate22khz;

		if (NewSoundManager && Audio16)
			maxbits=16;
		else
			maxbits=8;
#if !GENERATING68K || !GENERATINGCFM
	}
#endif

	switch (md_mixfreq) {
		case 48000:rate=rate48khz;break;
		case 44100:rate=rate44khz;break;
		case 22254:rate=rate22khz;break;
		case 22050:rate=rate22050hz;break;
		case 11127:rate=rate11khz;break;
		case 11025:rate=rate11025hz;break;
		default:   rate=0;break;
	}

	if (!rate) {
		_mm_errno=MMERR_MAC_SPEED;
		return 1;
	}

	md_mode|=DMODE_SOFT_MUSIC|DMODE_SOFT_SNDFX;

	if ((md_mode&DMODE_16BITS)&&(maxbits<16))
		md_mode&=~DMODE_16BITS;

	if (!Stereo || !StereoMixing)
		md_mode&=~DMODE_STEREO;

	if (rate>maxrate)
		rate=maxrate;
	if (md_mixfreq>(maxrate>>16))
		md_mixfreq=maxrate>>16;

	err=SndNewChannel(&soundChannel,sampledSynth,
	                  (md_mode&DMODE_STEREO)?initStereo:initMono,NULL);
	if(err!=noErr) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	doubleHeader.dbhCompressionID=0;
	doubleHeader.dbhPacketSize   =0;
	doubleHeader.dbhSampleRate   =rate;
	doubleHeader.dbhSampleSize   =(md_mode&DMODE_16BITS)?16:8;
	doubleHeader.dbhNumChannels  =(md_mode&DMODE_STEREO)?2:1;
	doubleHeader.dbhDoubleBack   =NewSndDoubleBackProc(&MyDoubleBackProc);
    
	for(i=0;i<2;i++) {
		doubleBuffer=(SndDoubleBufferPtr)NewPtrClear(sizeof(SndDoubleBuffer)+
		                                             SOUND_BUFFER_SIZE);
		if(!doubleBuffer) {
			_mm_errno=MMERR_OUT_OF_MEMORY;
			return 1;
		}

		doubleBuffer->dbNumFrames=0;
		doubleBuffer->dbFlags=0;
		doubleBuffer->dbUserInfo[0]=SetCurrentA5();
		doubleBuffer->dbUserInfo[1]=0;

		doubleHeader.dbhBufferPtr[i]=doubleBuffer;
	}
    
	return VC_Init();
}

static void MAC_Exit(void)
{
	int i;
    
	SndDisposeChannel(soundChannel,true);
	soundChannel=NULL;

	DisposeRoutineDescriptor((UniversalProcPtr)doubleHeader.dbhDoubleBack);
	doubleHeader.dbhDoubleBack=NULL;

	for(i=0;i<doubleHeader.dbhNumChannels;i++) {
		DisposePtr((Ptr)doubleHeader.dbhBufferPtr[i]);
		doubleHeader.dbhBufferPtr[i]=NULL;
	}

	VC_Exit();
}

static BOOL MAC_PlayStart(void)
{
	OSErr err;

	MyDoubleBackProc(soundChannel,doubleHeader.dbhBufferPtr[0]);
	MyDoubleBackProc(soundChannel,doubleHeader.dbhBufferPtr[1]);

	err=SndPlayDoubleBuffer(soundChannel,&doubleHeader);
	if(err!=noErr) {
		_mm_errno=MMERR_MAC_START;
		return 1;
	}
        
	return VC_PlayStart();
}

static void MAC_PlayStop(void)
{
	SndCommand cmd;

	cmd.cmd=quietCmd;
	cmd.param1=0;
	cmd.param2=0;
	SndDoImmediate(soundChannel,&cmd);
    
	cmd.cmd=flushCmd;
	cmd.param1=0;
	cmd.param2=0;
	SndDoImmediate(soundChannel,&cmd);

	VC_PlayStop();
}

static void MAC_Update(void)
{
	return;
}

MIKMODAPI MDRIVER drv_mac={
    NULL,
    "Mac Driver",
    "Macintosh Sound Manager Driver v1.0",
    0,255,
	"mac",

	NULL,
    MAC_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    MAC_Init,
    MAC_Exit,
    NULL,
    VC_SetNumVoices,
    MAC_PlayStart,
    MAC_PlayStop,
    MAC_Update,
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

/* ex:set ts=4: */

