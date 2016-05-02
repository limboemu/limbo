/*  MikMod sound library
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

  $Id: drv_os2.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on OS/2 using MMPM/2 MCI interface

==============================================================================*/

/*

	Written by Stefan Tibus <Stefan.Tibus@ThePentagon.com>
	Improvements by Andrew Zabolotny <bit@eltech.ru>

*/

#define INCL_DOS
#define INCL_OS2MM
#include <os2.h>
/* Prevent a warning: PPFN redefined */
#define PPFN _PPFN
#include <os2me.h>
#undef PPFN

#include <stdlib.h>

#include "mikmod_internals.h"

/* fragment count */
#define FRAGMENTS  4

/* global variables */
static ULONG DeviceIndex = 0;
static ULONG BufferSize = (ULONG)-1;
static ULONG DeviceID = 0;
static PVOID AudioBuffer = NULL;
static TID ThreadID = NULLHANDLE;
static BOOL FinishPlayback;		/* flag for update thread to die */
static HEV Play = NULLHANDLE;	/* posted on PlayStart event */
static HEV Update = NULLHANDLE;	/* timer event semaphore */
static HTIMER Timer = NULLHANDLE;

/* playlist-structure */
typedef struct {
	ULONG ulCommand;
	ULONG ulOperand1, ulOperand2, ulOperand3;
} PLAYLISTSTRUCTURE;

static PLAYLISTSTRUCTURE PlayList[FRAGMENTS + 1];

/* thread handling buffer-updates */
/* ARGSUSED */
void OS2_UpdateBufferThread(void *dummy)
{
	/* Run at timecritical priority */
	DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, PRTYD_MAXIMUM - 1, 0);

	while (!FinishPlayback) {
		ULONG count;
static	ULONG NextBuffer = 0;	/* next fragment to be filled */

		/* wait for play enable */
		DosWaitEventSem(Play, SEM_INDEFINITE_WAIT);
		/* wait for timer event */
		DosWaitEventSem(Update, SEM_INDEFINITE_WAIT);
		/* reset timer semaphore */
		DosResetEventSem(Update, &count);

		/* fill all free buffers */
		while (PlayList[NextBuffer].ulOperand3 >=
			   PlayList[NextBuffer].ulOperand2) {
			MUTEX_LOCK(vars);
			if (Player_Paused_internal())
				PlayList[NextBuffer].ulOperand2 =
				  VC_SilenceBytes((SBYTE *)PlayList[NextBuffer].ulOperand1,
								  BufferSize);
			else
				PlayList[NextBuffer].ulOperand2 =
				  VC_WriteBytes((SBYTE *)PlayList[NextBuffer].ulOperand1,
								BufferSize);
			MUTEX_UNLOCK(vars);
			/* Reset play offset, although it seems MMOS2 does it
			   automagically */
			PlayList[NextBuffer].ulOperand3 = 0;
			NextBuffer = (NextBuffer + 1) % FRAGMENTS;
		}
	}
	/* Tell main thread we're done */
	ThreadID = 0;
}

static void OS2_CommandLine(CHAR *cmdline)
{
	char *ptr;
	int buf;

	if ((ptr = MD_GetAtom("buffer", cmdline, 0))) {
		buf = atoi(ptr);
		if (buf >= 12 && buf <= 16)
			BufferSize = 1 << buf;
		free(ptr);
	}
	if ((ptr = MD_GetAtom("device", cmdline, 0))) {
		buf = atoi(ptr);
		if (buf >= 0 && buf <= 8)
			DeviceIndex = buf;
		free(ptr);
	}
}

/* checks for availability of an OS/2 MCI-WAVEAUDIO-device */
static BOOL OS2_IsPresent(void)
{
	MCI_OPEN_PARMS mciOpenParms;
	MCI_GENERIC_PARMS mciGenericParms;

	memset(&mciOpenParms, 0, sizeof(mciOpenParms));
	mciOpenParms.pszDeviceType = (PSZ) MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO,
												 (USHORT) DeviceIndex);

	/* try to open WAVEAUDIO-device */
	if (mciSendCommand(0, MCI_OPEN,
					   MCI_WAIT | MCI_OPEN_SHAREABLE | MCI_OPEN_TYPE_ID,
					   (PVOID) & mciOpenParms, 0) != MCIERR_SUCCESS)
		return 0;

	mciSendCommand(mciOpenParms.usDeviceID, MCI_CLOSE, MCI_WAIT,
				   (PVOID) & mciGenericParms, 0);
	return 1;
}

static BOOL OS2_Init(void)
{
	MCI_OPEN_PARMS mciOpenParms;
	MCI_WAVE_SET_PARMS mciWaveSetParms;
	int i, bpsrate;
	ULONG ulInterval;

	if (VC_Init())
		return 1;

	DeviceID = 0;
	ThreadID = 0;
	Timer = NULLHANDLE;
	Update = NULLHANDLE;
	Play = NULLHANDLE;
	AudioBuffer = NULL;

	/* Compute the bytes per second output rate */
	bpsrate = md_mixfreq;
	if (md_mode & DMODE_STEREO)
		bpsrate <<= 1;
	if (md_mode & DMODE_16BITS)
		bpsrate <<= 1;

	/* Compute audio buffer size if necessary */
	if (BufferSize == (ULONG)-1) {
		int bit;

		BufferSize = bpsrate >> 3;
		for (bit = 15; bit >= 12; bit--)
			if (BufferSize & (1 << bit))
				break;
		BufferSize = 1 << bit;
	}

	/* WARNING: with big buffer sizes there is an audible click between
	   sub-blocks. Don't know whenever this is a driver issue (I've tested it
	   only with Gravis Ultrasound driver) but its something to take into
	   consideration. That's why above code chooses relatively small buffer
	   sizes (16K for 44KHz, 16 bit stereo). */

	/* Allocate buffer */
	if (!(AudioBuffer = _mm_malloc(BufferSize * FRAGMENTS))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		return 1;
	}

	/* create playlist */
	for (i = 0; i < FRAGMENTS; i++) {
		PlayList[i].ulCommand = DATA_OPERATION;	/* play data */
		PlayList[i].ulOperand1 = ((ULONG)AudioBuffer) + i * BufferSize;
		PlayList[i].ulOperand2 = BufferSize;	/* size */
		PlayList[i].ulOperand3 = 0;	/* offset */
	}
	PlayList[FRAGMENTS].ulCommand = BRANCH_OPERATION;	/* jump */
	PlayList[FRAGMENTS].ulOperand1 = 0;
	PlayList[FRAGMENTS].ulOperand2 = 0;	/* destination */
	PlayList[FRAGMENTS].ulOperand3 = 0;

	memset(&mciOpenParms, 0, sizeof(mciOpenParms));
	mciOpenParms.pszDeviceType = (PSZ) MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO,
												 (USHORT) DeviceIndex);
	mciOpenParms.pszElementName = (PSZ) & PlayList;

	/* open WAVEAUDIO-device */
	if (mciSendCommand(0, MCI_OPEN,
					   MCI_WAIT | MCI_OPEN_TYPE_ID | MCI_OPEN_PLAYLIST,
					   (PVOID) & mciOpenParms, 0)) {
		_mm_errno = MMERR_OPENING_AUDIO;
		return 1;
	}
	DeviceID = mciOpenParms.usDeviceID;

	memset(&mciWaveSetParms, 0, sizeof(mciWaveSetParms));
	mciWaveSetParms.ulSamplesPerSec = md_mixfreq;
	mciWaveSetParms.usBitsPerSample = (md_mode & DMODE_16BITS) ? 16 : 8;
	mciWaveSetParms.usChannels = (md_mode & DMODE_STEREO) ? 2 : 1;
	mciWaveSetParms.ulAudio = MCI_SET_AUDIO_ALL;

	/* set play-parameters */
	if (mciSendCommand(DeviceID, MCI_SET,
					   MCI_WAIT | MCI_WAVE_SET_SAMPLESPERSEC |
					   MCI_WAVE_SET_BITSPERSAMPLE | MCI_WAVE_SET_CHANNELS,
					   (PVOID) & mciWaveSetParms, 0)) {
		_mm_errno = MMERR_OS2_MIXSETUP;
		return 1;
	}

	/* Setup semaphore for playing and for buffer updates */
	if (DosCreateEventSem(NULL, &Play, DC_SEM_SHARED, FALSE) ||
		DosCreateEventSem(NULL, &Update, DC_SEM_SHARED, FALSE)) {
		_mm_errno = MMERR_OS2_SEMAPHORE;
		return 1;
	}

	/* Compute interval in ms (2 possible updates while playing 1 block) */
	ulInterval = BufferSize * 1000L / bpsrate;
	/* Setup timer for buffer updates */
	if (DosStartTimer(ulInterval, (HSEM) Update, &Timer)) {
		_mm_errno = MMERR_OS2_TIMER;
		return 1;
	}

	/* Create thread for buffer updates */
	FinishPlayback = FALSE;
	ThreadID = _beginthread(OS2_UpdateBufferThread, NULL, 0x4000, NULL);
	if (!ThreadID) {
		_mm_errno = MMERR_OS2_THREAD;
		return 1;
	}

	return 0;
}

static void OS2_Exit(void)
{
	MCI_GENERIC_PARMS mciGenericParms;

	FinishPlayback = TRUE;
	while (ThreadID) {
		DosPostEventSem(Play);
		DosPostEventSem(Update);
		DosSleep(1);
	}
	if (Timer) {
		DosStopTimer(Timer);
		Timer = NULLHANDLE;
	}
	if (Update) {
		DosCloseEventSem(Update);
		Update = NULLHANDLE;
	}
	if (Play) {
		DosCloseEventSem(Play);
		Play = NULLHANDLE;
	}
	VC_Exit();
	if (DeviceID) {
		mciGenericParms.hwndCallback = (HWND) NULL;
		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
					   (PVOID) & mciGenericParms, 0);
		DeviceID = 0;
	}
	_mm_free(AudioBuffer);
}

static BOOL OS2_PlayStart(void)
{
	MCI_PLAY_PARMS mciPlayParms;
	int i;

	VC_SilenceBytes((SBYTE *)AudioBuffer, BufferSize * FRAGMENTS);
	VC_PlayStart();

	/* Signal update of audio fragments */
	for (i = 0; i < FRAGMENTS; i++)
		PlayList[i].ulOperand3 = 0;

	/* Signal play */
	DosPostEventSem(Play);

	/* no MCI_WAIT here, because application program has to continue */
	memset(&mciPlayParms, 0, sizeof(mciPlayParms));
	if (mciSendCommand(DeviceID, MCI_PLAY, MCI_FROM, &mciPlayParms, 0)) {
		_mm_errno = MMERR_OS2_MIXSETUP;
		return 1;
	}

	return 0;
}

static void OS2_PlayStop(void)
{
	MCI_GENERIC_PARMS mciGenericParms;
	ULONG count;

	mciGenericParms.hwndCallback = (HWND) NULL;
	mciSendCommand(DeviceID, MCI_STOP, MCI_WAIT, &mciGenericParms, 0);
	DosResetEventSem(Play, &count);

	VC_PlayStop();
}

static void OS2_Update(void)
{
	/* does nothing since buffer is updated in the background */
}

MIKMODAPI MDRIVER drv_os2 = {
	NULL,
	"OS/2 MMPM/2",
	"OS/2 MMPM/2 MCI driver v1.2",
	0,255,
	"os2",

	OS2_CommandLine,
	OS2_IsPresent,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	OS2_Init,
	OS2_Exit,
	NULL,
	VC_SetNumVoices,
	OS2_PlayStart,
	OS2_PlayStop,
	OS2_Update,
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
