/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see file
	AUTHORS for complete list.

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

  $Id: drv_sun.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output on the Sun audio device (/dev/audio).
  Also works under NetBSD and OpenBSD

==============================================================================*/

/*

	Written by Valtteri Vuorikoski <vuori@sci.fi>
	NetBSD/OpenBSD code from Miodrag Vallat <miod@mikmod.org>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_SUN

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>

#ifdef HAVE_SUN_AUDIOIO_H
#include <sun/audioio.h>
#endif
#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/audioio.h>
#endif

#ifdef SUNOS
extern int ioctl(int, unsigned long, ...);
extern int fputs(const char *, FILE *);
#endif

#define DEFAULT_FRAGSIZE 12

#if !defined __NetBSD__  && !defined __OpenBSD__
#ifdef HAVE_SUN_AUDIOIO_H
#define SUNOS4
#else
#define SOLARIS
#endif
#endif

/* Sound device to open */
#ifdef SUNOS4
#define SOUNDDEVICE "/dev/sound"
#else /* Solaris, *BSD */
#define SOUNDDEVICE "/dev/audio"
#endif

/* Compatibility defines, for old *BSD or SunOS systems */
#ifndef AUDIO_ENCODING_PCM16
#define AUDIO_ENCODING_PCM16 AUDIO_ENCODING_LINEAR
#endif
#ifndef AUDIO_ENCODING_PCM8
#define AUDIO_ENCODING_PCM8 AUDIO_ENCODING_LINEAR8
#endif
#ifndef AUDIO_ENCODING_SLINEAR_LE
#define AUDIO_ENCODING_SLINEAR_LE AUDIO_ENCODING_PCM16
#endif
#ifndef AUDIO_ENCODING_ULINEAR_LE
#define AUDIO_ENCODING_ULINEAR_LE AUDIO_ENCODING_PCM8
#endif
#ifndef AUDIO_ENCODING_SLINEAR
#if BYTE_ORDER == BIG_ENDIAN
#define AUDIO_ENCODING_SLINEAR AUDIO_ENCODING_SLINEAR_BE
#else
#define AUDIO_ENCODING_SLINEAR AUDIO_ENCODING_SLINEAR_LE
#endif
#endif
#ifndef AUDIO_ENCODING_ULINEAR
#if BYTE_ORDER == BIG_ENDIAN
#define AUDIO_ENCODING_ULINEAR AUDIO_ENCODING_ULINEAR_BE
#else
#define AUDIO_ENCODING_ULINEAR AUDIO_ENCODING_ULINEAR_LE
#endif
#endif

/* Compatibility defines, for old *BSD systems */
#ifndef AUDIO_SPEAKER
#define AUDIO_SPEAKER 0x01
#endif
#ifndef AUDIO_HEADPHONE
#define AUDIO_HEADPHONE 0x02
#endif

/* ``normalize'' AUDIO_ENCODING_xxx values for comparison */
static int normalize(int encoding)
{
	switch (encoding) {
#ifdef AUDIO_ENCODING_LINEAR
	case AUDIO_ENCODING_LINEAR:
			return AUDIO_ENCODING_PCM16;
#endif
#ifdef AUDIO_ENCODING_LINEAR8
	case AUDIO_ENCODING_LINEAR8:
			return AUDIO_ENCODING_PCM8;
#endif
#if BYTE_ORDER == BIG_ENDIAN
#ifdef AUDIO_ENCODING_SLINEAR_BE
	case AUDIO_ENCODING_SLINEAR:
		return AUDIO_ENCODING_SLINEAR_BE;
#endif
#ifdef AUDIO_ENCODING_ULINEAR_BE
	case AUDIO_ENCODING_ULINEAR:
		return AUDIO_ENCODING_ULINEAR_BE;
#endif
#else
#ifdef AUDIO_ENCODING_SLINEAR_LE
	case AUDIO_ENCODING_SLINEAR:
		return AUDIO_ENCODING_SLINEAR_LE;
#endif
#ifdef AUDIO_ENCODING_ULINEAR_LE
	case AUDIO_ENCODING_ULINEAR:
		return AUDIO_ENCODING_ULINEAR_LE;
#endif
#endif
	default:
		return encoding;
	}
}

static int sndfd = -1;
static unsigned int port = 0;
static int play_encoding;
static int play_precision;
static int fragsize = 1 << DEFAULT_FRAGSIZE;
static SBYTE *audiobuffer = NULL;

static void Sun_CommandLine(CHAR *cmdline)
{
	CHAR *ptr;

	if ((ptr = MD_GetAtom("buffer", cmdline, 0))) {
		int buf = atoi(ptr);

		if (buf >= 7 && buf <= 17)
			fragsize = 1 << buf;

		free(ptr);
	}

	if ((ptr = MD_GetAtom("headphone", cmdline, 1))) {
		port = AUDIO_HEADPHONE;
		free(ptr);
	} else if ((ptr = MD_GetAtom("speaker", cmdline, 1))) {
		port = AUDIO_SPEAKER;
		free(ptr);
	}
}

static BOOL Sun_IsThere(void)
{
	if (getenv("AUDIODEV"))
		return (access(getenv("AUDIODEV"), W_OK) == 0);
	else {
		if (access(SOUNDDEVICE, W_OK) == 0)
			return 1;
#if defined __NetBSD__ || defined __OpenBSD__
		/* old OpenBSD/sparc installation program creates /dev/audio0 but no
		   /dev/audio. Didn't check NetBSD behaviour */
		if (access(SOUNDDEVICE "0", W_OK) == 0)
			return 1;
#endif
	}
	return 0;
}

static BOOL Sun_Init(void)
{
	int play_stereo, play_rate;
#ifdef SUNOS4
	int audiotype;
#else
	audio_device_t audiotype;
#endif
	struct audio_info audioinfo;

	if (getenv("AUDIODEV"))
		sndfd = open(getenv("AUDIODEV"), O_WRONLY);
	else {
		sndfd = open(SOUNDDEVICE, O_WRONLY);
#if defined __NetBSD__ || defined __OpenBSD__
		if (sndfd < 0)
			sndfd = open(SOUNDDEVICE "0", O_WRONLY);
#endif
	}
	if (sndfd < 0) {
		_mm_errno = MMERR_OPENING_AUDIO;
		return 1;
	}

	if (!(audiobuffer = (SBYTE *)_mm_malloc(fragsize)))
		return 1;

	play_precision = (md_mode & DMODE_16BITS) ? 16 : 8;
	play_stereo = (md_mode & DMODE_STEREO) ? 2 : 1;
	play_rate = md_mixfreq;
	/* attempt to guess the encoding */
	play_encoding = -1;

	if (ioctl(sndfd, AUDIO_GETDEV, &audiotype) < 0) {
#ifdef MIKMOD_DEBUG
		fputs("\rSun driver warning: could not determine audio device type\n",
			  stderr);
#endif
	} else {
#if defined SUNOS4				/* SunOS 4 */
		switch (audiotype) {
		  case AUDIO_DEV_AMD:
			/* AMD 79C30 */
			/* 8bit mono ulaw 8kHz */
		  	play_rate = md_mixfreq = 8000;
			md_mode &= ~(DMODE_STEREO | DMODE_16BITS);
			play_precision = 8;
			play_stereo = 1;
			play_encoding = AUDIO_ENCODING_ULAW;
			break;
		  case AUDIO_DEV_SPEAKERBOX:
		  case AUDIO_DEV_CODEC:
			/* CS 4231 or DBRI or speaker box */
			/* 16bit mono/stereo linear 8kHz - 48kHz */
			if (play_precision == 16)
				play_encoding = AUDIO_ENCODING_LINEAR;
			/* 8bit mono ulaw 8kHz - 48kHz */
			else if (play_precision == 8) {
				md_mode &= ~(DMODE_STEREO);
				play_stereo = 1;
				play_encoding = AUDIO_ENCODING_ULAW;
			} else {
				_mm_errno = MMERR_SUN_INIT;
				return 1;
			}
			break;
		}
#elif defined SOLARIS			/* Solaris */
		if (!strcmp(audiotype.name, "SUNW,am79c30")) {
			/* AMD 79C30 */
			/* 8bit mono ulaw 8kHz */
		  	play_rate = md_mixfreq = 8000;
			md_mode &= ~(DMODE_STEREO | DMODE_16BITS);
			play_precision = 8;
			play_stereo = 1;
			play_encoding = AUDIO_ENCODING_ULAW;
		} else
			if ((!strcmp(audiotype.name, "SUNW,CS4231")) ||
				(!strcmp(audiotype.name, "SUNW,dbri")) ||
				(!strcmp(audiotype.name, "speakerbox"))) {
			/* CS 4231 or DBRI or speaker box */
			/* 16bit mono/stereo linear 8kHz - 48kHz */
			if (play_precision == 16)
				play_encoding = AUDIO_ENCODING_LINEAR;
			/* 8bit mono ulaw 8kHz - 48kHz */
			else if (play_precision == 8) {
				md_mode &= ~(DMODE_STEREO);
				play_stereo = 1;
				play_encoding = AUDIO_ENCODING_ULAW;
			} else {
				_mm_errno = MMERR_SUN_INIT;
				return 1;
			}
		}
#else /* NetBSD, OpenBSD */
		if (!strcmp(audiotype.name, "amd7930")) {
			/* AMD 79C30 */
			/* 8bit mono ulaw 8kHz */
		  	play_rate = md_mixfreq = 8000;
			md_mode &= ~(DMODE_STEREO | DMODE_16BITS);
			play_precision = 8;
			play_stereo = 1;
			play_encoding = AUDIO_ENCODING_ULAW;
		}
		if ((!strcmp(audiotype.name, "Am78C201")) ||
			(!strcmp(audiotype.name, "UltraSound")) 
		   ) {
			/* Gravis UltraSound, AMD Interwave and compatible cards */
			/* 16bit stereo linear 44kHz */
		  	play_rate = md_mixfreq = 44100;
			md_mode |= (DMODE_STEREO | DMODE_16BITS);
			play_precision = 16;
			play_stereo = 2;
			play_encoding = AUDIO_ENCODING_SLINEAR;
		}
#endif
	}

	/* Sound devices which were not handled above don't have specific
	   limitations, so try and guess optimal settings */
	if (play_encoding == -1) {
		if ((play_precision == 8) && (play_stereo == 1) &&
			(play_rate <= 8000)) play_encoding = AUDIO_ENCODING_ULAW;
		else
#ifdef SUNOS4
			play_encoding = AUDIO_ENCODING_LINEAR;
#else
			play_encoding =
				(play_precision ==
				 16) ? AUDIO_ENCODING_SLINEAR : AUDIO_ENCODING_ULINEAR;
#endif
	}

	/* get current audio settings if we want to keep the playback output
	   port */
	if (!port) {
		AUDIO_INITINFO(&audioinfo);
		if (ioctl(sndfd, AUDIO_GETINFO, &audioinfo) < 0) {
			_mm_errno = MMERR_SUN_INIT;
			return 1;
		}
		port = audioinfo.play.port;
	}

	AUDIO_INITINFO(&audioinfo);
	audioinfo.play.precision = play_precision;
	audioinfo.play.channels = play_stereo;
	audioinfo.play.sample_rate = play_rate;
	audioinfo.play.encoding = play_encoding;
	audioinfo.play.port = port;
#if defined __NetBSD__ || defined __OpenBSD__
#if defined AUMODE_PLAY_ALL
	audioinfo.mode = AUMODE_PLAY | AUMODE_PLAY_ALL;
#else
	audioinfo.mode = AUMODE_PLAY;
#endif
#endif

	if (ioctl(sndfd, AUDIO_SETINFO, &audioinfo) < 0) {
		_mm_errno = MMERR_SUN_INIT;
		return 1;
	}

	/* check if our changes were accepted */
	if (ioctl(sndfd, AUDIO_GETINFO, &audioinfo) < 0) {
		_mm_errno = MMERR_SUN_INIT;
		return 1;
	}
	if ((audioinfo.play.precision != play_precision) ||
		(audioinfo.play.channels != play_stereo) ||
		(normalize(audioinfo.play.encoding) != normalize(play_encoding))) {
		_mm_errno = MMERR_SUN_INIT;
		return 1;
	}

	if (audioinfo.play.sample_rate != play_rate) {
		/* Accept a shift inferior to 5% of the expected rate */
		int delta = audioinfo.play.sample_rate - play_rate;

		if (delta < 0)
			delta = -delta;

		if (delta * 20 > play_rate) {
			_mm_errno = MMERR_SUN_INIT;
			return 1;
		}
		/* Align to what the card gave us */
		md_mixfreq = audioinfo.play.sample_rate;
	}

	return VC_Init();
}

static void Sun_Exit(void)
{
	VC_Exit();
	_mm_free(audiobuffer);
	if (sndfd >= 0) {
		close(sndfd);
		sndfd = -1;
	}
}

static void Sun_Update(void)
{
	int done;

	done = VC_WriteBytes((char *)audiobuffer, fragsize);
	if (play_encoding == AUDIO_ENCODING_ULAW)
		unsignedtoulaw(audiobuffer, done);
	write(sndfd, audiobuffer, done);
}

static void Sun_Pause(void)
{
	int done;

	done = VC_SilenceBytes((char *)audiobuffer, fragsize);
	write(sndfd, audiobuffer, done);
}

static BOOL Sun_PlayStart(void)
{
	struct audio_info audioinfo;

	AUDIO_INITINFO(&audioinfo);
	audioinfo.play.pause = 0;
	if (ioctl(sndfd, AUDIO_SETINFO, &audioinfo) < 0)
		return 1;

	return VC_PlayStart();
}

static void Sun_PlayStop(void)
{
	struct audio_info audioinfo;

	VC_PlayStop();

	if (ioctl(sndfd, AUDIO_DRAIN) < 0)
		return;
	AUDIO_INITINFO(&audioinfo);
	audioinfo.play.pause = 1;
	ioctl(sndfd, AUDIO_SETINFO, &audioinfo);
}

MIKMODAPI MDRIVER drv_sun = {
	NULL,
#if defined __OpenBSD__
	"OpenBSD Audio",
	"OpenBSD audio driver v1.0",
#elif defined __NetBSD__
	"NetBSD Audio",
	"NetBSD audio driver v1.0",
#elif defined SUNOS4
	"SunOS Audio",
	"SunOS audio driver v1.4",
#elif defined SOLARIS
	"Solaris Audio",
	"Solaris audio driver v1.4",
#endif
	0, 255,
	"audio",

	Sun_CommandLine,
	Sun_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	Sun_Init,
	Sun_Exit,
	NULL,
	VC_SetNumVoices,
	Sun_PlayStart,
	Sun_PlayStop,
	Sun_Update,
	Sun_Pause,
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

MISSING(drv_sun);

#endif

/* ex:set ts=4: */
