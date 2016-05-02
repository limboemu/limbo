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

  $Id: drv_dart.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on OS/2 MMPM/2 using direct audio (DART)

==============================================================================*/

/*

	Written by Miodrag Vallat <miod@mikmod.org>
	Lots of improvements (basically made usable...) by Andrew Zabolotny
	                                                   <bit@eltech.ru>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_DART

#define INCL_DOS
#define INCL_OS2MM
#include <os2.h>
/* Prevent a warning: PPFN redefined */
#define PPFN _PPFN
#include <os2me.h>
#undef PPFN

#include <stdlib.h>
#include <string.h>

#define MAX_BUFFERCOUNT 8

static MCI_MIX_BUFFER MixBuffers[MAX_BUFFERCOUNT];
static MCI_MIXSETUP_PARMS MixSetupParms;
static MCI_BUFFER_PARMS BufferParms;
static ULONG DeviceIndex = 0;			/* use default waveaudio device */
static ULONG BufferSize = (ULONG)-1;	/* autodetect buffer size */
static ULONG BufferCount = 2;			/* use two buffers */
static ULONG DeviceID = 0;

static void Dart_CommandLine(CHAR *cmdline)
{
	char *ptr;
	int buf;

	if ((ptr = MD_GetAtom("buffer", cmdline, 0))) {
		buf = atoi(ptr);
		if (buf >= 12 && buf <= 16)
			BufferSize = 1 << buf;
		free(ptr);
	}
	
	if ((ptr = MD_GetAtom("count", cmdline, 0))) {
		buf = atoi(ptr);
		if (buf >= 2 && buf <= MAX_BUFFERCOUNT)
			BufferCount = buf;
		free(ptr);
	}
	
	if ((ptr = MD_GetAtom("device", cmdline, 0))) {
		buf = atoi(ptr);
		if (buf >= 0 && buf <= 8)
			DeviceIndex = buf;
		free(ptr);
	}
}

/* Buffer update thread (created and called by DART)
   This is a high-priority thread used to compute and update the audio stream,
   automatically created by the DART subsystem. We compute the next audio
   buffer and feed it to the waveaudio device. */
static LONG APIENTRY Dart_UpdateBuffers(ULONG ulStatus, PMCI_MIX_BUFFER pBuffer, ULONG ulFlags)
{
	/* sanity check */
	if (!pBuffer)
		return TRUE;
	
	/* if we have finished a buffer, we're ready to play a new one */
	if ((ulFlags == MIX_WRITE_COMPLETE) ||
		((ulFlags == (MIX_WRITE_COMPLETE | MIX_STREAM_ERROR)) &&
		 (ulStatus == ERROR_DEVICE_UNDERRUN))) {
		/* refill this audio buffer and feed it again */
		MUTEX_LOCK(vars);
		if (Player_Paused_internal())
			pBuffer->ulBufferLength =
					VC_SilenceBytes(pBuffer->pBuffer, BufferSize);
		else
			pBuffer->ulBufferLength =
					VC_WriteBytes(pBuffer->pBuffer, BufferSize);
		MUTEX_UNLOCK(vars);
		MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle, pBuffer, 1);
	}
	return TRUE;
}

static BOOL Dart_IsPresent(void)
{
	MCI_AMP_OPEN_PARMS AmpOpenParms;
	MCI_GENERIC_PARMS GenericParms;

	memset(&AmpOpenParms, 0, sizeof(MCI_AMP_OPEN_PARMS));
	AmpOpenParms.pszDeviceType = (PSZ) MAKEULONG(MCI_DEVTYPE_AUDIO_AMPMIX,
					(USHORT) DeviceIndex);

	if (mciSendCommand(0, MCI_OPEN, MCI_WAIT | MCI_OPEN_SHAREABLE | MCI_OPEN_TYPE_ID,
					   (PVOID) &AmpOpenParms, 0) != MCIERR_SUCCESS)
		return 0;

	mciSendCommand(AmpOpenParms.usDeviceID, MCI_CLOSE, MCI_WAIT,
				   (PVOID) &GenericParms, 0);
	return 1;
}

static BOOL Dart_Init(void)
{
	MCI_AMP_OPEN_PARMS AmpOpenParms;
	MCI_GENERIC_PARMS GenericParms;

	MixBuffers[0].pBuffer = NULL;	/* marker */
	DeviceID = 0;
	memset(&GenericParms, 0, sizeof(MCI_GENERIC_PARMS));

	/* open AMP device */
	memset(&AmpOpenParms, 0, sizeof(MCI_AMP_OPEN_PARMS));
	AmpOpenParms.usDeviceID = 0;
	AmpOpenParms.pszDeviceType = (PSZ) MAKEULONG(MCI_DEVTYPE_AUDIO_AMPMIX,
					(USHORT) DeviceIndex);


	if (mciSendCommand(0, MCI_OPEN, MCI_WAIT | MCI_OPEN_SHAREABLE | MCI_OPEN_TYPE_ID,
					   (PVOID) & AmpOpenParms, 0) != MCIERR_SUCCESS) {
		_mm_errno = MMERR_OPENING_AUDIO;
		return 1;
	}

	DeviceID = AmpOpenParms.usDeviceID;

	/* setup playback parameters */
	memset(&MixSetupParms, 0, sizeof(MCI_MIXSETUP_PARMS));
	MixSetupParms.ulBitsPerSample = (md_mode & DMODE_16BITS) ? 16 : 8;
	MixSetupParms.ulFormatTag = MCI_WAVE_FORMAT_PCM;
	MixSetupParms.ulSamplesPerSec = md_mixfreq;
	MixSetupParms.ulChannels = (md_mode & DMODE_STEREO) ? 2 : 1;
	MixSetupParms.ulFormatMode = MCI_PLAY;
	MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
	MixSetupParms.pmixEvent = Dart_UpdateBuffers;

	if (mciSendCommand(DeviceID, MCI_MIXSETUP,
					   MCI_WAIT | MCI_MIXSETUP_INIT,
					   (PVOID) & MixSetupParms, 0) != MCIERR_SUCCESS) {
		_mm_errno = MMERR_OS2_MIXSETUP;
		return 1;
	}

	/* Compute audio buffer size now if necessary */
	if (BufferSize == (ULONG)-1) {
		/* DART suggested buffer size is somewhat too big. We compute a size
		   for circa 1/4" of playback. */
		int bit;
		
		BufferSize = md_mixfreq >> 2;
		if (md_mode & DMODE_STEREO)
			BufferSize <<= 1;
		if (md_mode & DMODE_16BITS)
			BufferSize <<= 1;
		for (bit = 15; bit >= 12; bit--)
			if (BufferSize & (1 << bit))
				break;
		BufferSize = (1 << bit);
	}
	/* make sure buffer is not greater than 64 Kb, as DART can't handle this
	   situation. */
	if (BufferSize > 65536)
		BufferSize = 65536;

	BufferParms.ulStructLength = sizeof(BufferParms);
	BufferParms.ulNumBuffers = BufferCount;
	BufferParms.ulBufferSize = BufferSize;
	BufferParms.pBufList = MixBuffers;

	if (mciSendCommand(DeviceID, MCI_BUFFER,
					   MCI_WAIT | MCI_ALLOCATE_MEMORY,
					   (PVOID) &BufferParms, 0) != MCIERR_SUCCESS) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		return 1;
	}

	return VC_Init();
}

static void Dart_Exit(void)
{
	MCI_GENERIC_PARMS GenericParms;

	VC_Exit();
	if (MixBuffers[0].pBuffer) {
		mciSendCommand(DeviceID, MCI_BUFFER, MCI_WAIT | MCI_DEALLOCATE_MEMORY,
					   &BufferParms, 0);
		MixBuffers[0].pBuffer = NULL;
	}
	if (DeviceID) {
		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT, (PVOID) &GenericParms, 0);
		DeviceID = 0;
	}
}

static BOOL Dart_PlayStart(void)
{
	MCI_GENERIC_PARMS GenericParms;
	int i;

	if (VC_PlayStart())
		return 1;

	/* silence buffers */
	for (i = 0; i < BufferCount; i++) {
		VC_SilenceBytes(MixBuffers[i].pBuffer, BufferSize);
		MixBuffers[i].ulBufferLength = BufferSize;
	}

	/* grab exclusive rights to device instance (not entire device) */
	GenericParms.hwndCallback = 0;  /* Not needed, so set to 0 */
	if (mciSendCommand(DeviceID, MCI_ACQUIREDEVICE, MCI_EXCLUSIVE_INSTANCE,
	                   (PVOID) &GenericParms, 0) != MCIERR_SUCCESS) {
		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT, (PVOID) &GenericParms, 0);
		_mm_errno = MMERR_OPENING_AUDIO;
		return 1;
	}

	/* wake DART up */
	MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle, MixBuffers, BufferCount);

	return 0;
}

static void Dart_Update(void)
{
	/* Nothing to do: everything is done in DART_UpdateBuffers() */
}

static void Dart_PlayStop(void)
{
	MCI_GENERIC_PARMS GenericParms;

	GenericParms.hwndCallback = 0;
	mciSendCommand(DeviceID, MCI_STOP, MCI_WAIT, (PVOID) &GenericParms, 0);

	VC_PlayStop();
}

MIKMODAPI MDRIVER drv_dart = {
	NULL,
	"Direct audio (DART)",
	"OS/2 DART driver v1.2",
	0, 255,
	"dart",

	Dart_CommandLine,
	Dart_IsPresent,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	Dart_Init,
	Dart_Exit,
	NULL,
	VC_SetNumVoices,
	Dart_PlayStart,
	Dart_PlayStop,
	Dart_Update,
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

MISSING(drv_dart);

#endif

/* ex:set ts=4: */
