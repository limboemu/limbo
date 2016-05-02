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

  $Id: drv_alsa.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for Advanced Linux Sound Architecture (ALSA)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_ALSA

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef MIKMOD_DYNAMIC
#include <dlfcn.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <sys/asoundlib.h>
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x500)
#undef DRV_ALSA
#endif

#endif
#ifdef DRV_ALSA

#ifdef MIKMOD_DYNAMIC
/* runtime link with libasound */
static unsigned int(*alsa_cards_mask)(void);
static int(*alsa_ctl_close)(snd_ctl_t*);
static int(*alsa_ctl_hw_info)(snd_ctl_t*,struct snd_ctl_hw_info*);
static int(*alsa_ctl_open)(snd_ctl_t**,int);
static int(*alsa_ctl_pcm_info)(snd_ctl_t*,int,snd_pcm_info_t*);
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x400)
static int(*alsa_ctl_pcm_playback_info)(snd_ctl_t*,int,int,snd_pcm_playback_info_t*);
#else
static int(*alsa_ctl_pcm_playback_info)(snd_ctl_t*,int,snd_pcm_playback_info_t*);
#endif
static int(*alsa_pcm_close)(snd_pcm_t*);
static int(*alsa_pcm_drain_playback)(snd_pcm_t*);
static int(*alsa_pcm_flush_playback)(snd_pcm_t*);
static int(*alsa_pcm_open)(snd_pcm_t**,int,int,int);
static int(*alsa_pcm_playback_format)(snd_pcm_t*,snd_pcm_format_t*);
static int(*alsa_pcm_playback_info)(snd_pcm_t*,snd_pcm_playback_info_t*);
static int(*alsa_pcm_playback_params)(snd_pcm_t*,snd_pcm_playback_params_t*);
static int(*alsa_pcm_playback_status)(snd_pcm_t*,snd_pcm_playback_status_t*);
static int(*alsa_pcm_write)(snd_pcm_t*,const void*,size_t);
static void* libasound=NULL;
#ifndef HAVE_RTLD_GLOBAL
#define RTLD_GLOBAL (0)
#endif
#else
/* compile-time link with libasound */
#define alsa_cards_mask				snd_cards_mask
#define alsa_ctl_close				snd_ctl_close
#define alsa_ctl_hw_info			snd_ctl_hw_info
#define alsa_ctl_open				snd_ctl_open
#define alsa_ctl_pcm_info			snd_ctl_pcm_info
#define alsa_ctl_pcm_playback_info	snd_ctl_pcm_playback_info
#define alsa_pcm_close				snd_pcm_close
#define alsa_pcm_drain_playback		snd_pcm_drain_playback
#define alsa_pcm_flush_playback		snd_pcm_flush_playback
#define alsa_pcm_open				snd_pcm_open
#define alsa_pcm_playback_format	snd_pcm_playback_format
#define alsa_pcm_playback_info		snd_pcm_playback_info
#define alsa_pcm_playback_params	snd_pcm_playback_params
#define alsa_pcm_playback_status	snd_pcm_playback_status
#define alsa_pcm_write				snd_pcm_write
#endif /* MIKMOD_DYNAMIC */

#define DEFAULT_NUMFRAGS 4

static	snd_pcm_t *pcm_h=NULL;
static	int fragmentsize,numfrags=DEFAULT_NUMFRAGS;
static	SBYTE *audiobuffer=NULL;
static	int cardmin=0,cardmax=SND_CARDS;
static	int device=-1;

#ifdef MIKMOD_DYNAMIC
static BOOL ALSA_Link(void)
{
	if(libasound) return 0;

	/* load libasound.so */
	libasound=dlopen("libasound.so",RTLD_LAZY|RTLD_GLOBAL);
	if(!libasound) return 1;

	/* resolve function references */
	if(!(alsa_cards_mask           =dlsym(libasound,"snd_cards_mask"))) return 1;
	if(!(alsa_ctl_close            =dlsym(libasound,"snd_ctl_close"))) return 1;
	if(!(alsa_ctl_hw_info          =dlsym(libasound,"snd_ctl_hw_info"))) return 1;
	if(!(alsa_ctl_open             =dlsym(libasound,"snd_ctl_open"))) return 1;
	if(!(alsa_ctl_pcm_info         =dlsym(libasound,"snd_ctl_pcm_info"))) return 1;
	if(!(alsa_ctl_pcm_playback_info=dlsym(libasound,"snd_ctl_pcm_playback_info"))) return 1;
	if(!(alsa_pcm_close            =dlsym(libasound,"snd_pcm_close"))) return 1;
	if(!(alsa_pcm_drain_playback   =dlsym(libasound,"snd_pcm_drain_playback"))) return 1;
	if(!(alsa_pcm_flush_playback   =dlsym(libasound,"snd_pcm_flush_playback"))) return 1;
	if(!(alsa_pcm_open             =dlsym(libasound,"snd_pcm_open"))) return 1;
	if(!(alsa_pcm_playback_format  =dlsym(libasound,"snd_pcm_playback_format"))) return 1;
	if(!(alsa_pcm_playback_info    =dlsym(libasound,"snd_pcm_playback_info"))) return 1;
	if(!(alsa_pcm_playback_params  =dlsym(libasound,"snd_pcm_playback_params"))) return 1;
	if(!(alsa_pcm_playback_status  =dlsym(libasound,"snd_pcm_playback_status"))) return 1;
	if(!(alsa_pcm_write            =dlsym(libasound,"snd_pcm_write"))) return 1;

	return 0;
}

static void ALSA_Unlink(void)
{
	alsa_cards_mask           =NULL;
	alsa_ctl_close            =NULL;
	alsa_ctl_hw_info          =NULL;
	alsa_ctl_open             =NULL;
	alsa_ctl_pcm_info         =NULL;
	alsa_ctl_pcm_playback_info=NULL;
	alsa_pcm_close            =NULL;
	alsa_pcm_drain_playback   =NULL;
	alsa_pcm_flush_playback   =NULL;
	alsa_pcm_open             =NULL;
	alsa_pcm_playback_format  =NULL;
	alsa_pcm_playback_info    =NULL;
	alsa_pcm_playback_params  =NULL;
	alsa_pcm_playback_status  =NULL;
	alsa_pcm_write            =NULL;

	if(libasound) {
		dlclose(libasound);
		libasound=NULL;
	}
}
#endif /* MIKMOD_DYNAMIC */

static void ALSA_CommandLine(CHAR *cmdline)
{
	CHAR *ptr;

	if((ptr=MD_GetAtom("card",cmdline,0))) {
		cardmin=atoi(ptr);cardmax=cardmin+1;
		free(ptr);
	} else {
		cardmin=0;cardmax=SND_CARDS;
	}
	if((ptr=MD_GetAtom("pcm",cmdline,0))) {
		device=atoi(ptr);
		free(ptr);
	} else device=-1;
	if((ptr=MD_GetAtom("buffer",cmdline,0))) {
		numfrags=atoi(ptr);
		if ((numfrags<2)||(numfrags>16)) numfrags=DEFAULT_NUMFRAGS;
		free(ptr);
	} else numfrags=DEFAULT_NUMFRAGS;
}

static BOOL ALSA_IsThere(void)
{
	int retval;

#ifdef MIKMOD_DYNAMIC
	if (ALSA_Link()) return 0;
#endif
	retval=(alsa_cards_mask())?1:0;
#ifdef MIKMOD_DYNAMIC
	ALSA_Unlink();
#endif
	return retval;
}

static BOOL ALSA_Init_internal(void)
{
	snd_pcm_format_t pformat;
	int mask,card;

	/* adjust user-configurable settings */
	if((getenv("MM_NUMFRAGS"))&&(numfrags==DEFAULT_NUMFRAGS)) {
		numfrags=atoi(getenv("MM_NUMFRAGS"));
		if ((numfrags<2)||(numfrags>16)) numfrags=DEFAULT_NUMFRAGS;
	}
	if((getenv("ALSA_CARD"))&&(!cardmin)&&(cardmax==SND_CARDS)) {
		cardmin=atoi(getenv("ALSA_CARD"));
		cardmax=cardmin+1;
		if(getenv("ALSA_PCM"))
			device=atoi(getenv("ALSA_PCM"));
	}

	/* setup playback format structure */
	memset(&pformat,0,sizeof(pformat));
#ifdef SND_LITTLE_ENDIAN
	pformat.format=(md_mode&DMODE_16BITS)?SND_PCM_SFMT_S16_LE:SND_PCM_SFMT_U8;
#else
	pformat.format=(md_mode&DMODE_16BITS)?SND_PCM_SFMT_S16_BE:SND_PCM_SFMT_U8;
#endif
	pformat.channels=(md_mode&DMODE_STEREO)?2:1;
	pformat.rate=md_mixfreq;

	/* scan for appropriate sound card */
	mask=alsa_cards_mask();
	_mm_errno=MMERR_OPENING_AUDIO;
	for (card=cardmin;card<cardmax;card++) {
		struct snd_ctl_hw_info info;
		snd_ctl_t *ctl_h;
		int dev,devmin,devmax;

		/* no card here, onto the next */
		if (!(mask&(1<<card))) continue;

		/* try to open the card in query mode */
		if(alsa_ctl_open(&ctl_h,card)<0)
			continue;

		/* get hardware information */
		if(alsa_ctl_hw_info(ctl_h,&info)<0) {
			alsa_ctl_close(ctl_h);
			continue;
		}

		/* scan subdevices */
		if(device==-1) {
			devmin=0;devmax=info.pcmdevs;
		} else
			devmin=devmax=device;
		for(dev=devmin;dev<devmax;dev++) {
			snd_pcm_info_t pcminfo;
			snd_pcm_playback_info_t ctlinfo;
			struct snd_pcm_playback_info pinfo;
			struct snd_pcm_playback_params pparams;
			int size,bps;

			/* get PCM capabilities */
			if(alsa_ctl_pcm_info(ctl_h,dev,&pcminfo)<0)
				continue;

			/* look for playback capability */
			if(!(pcminfo.flags&SND_PCM_INFO_PLAYBACK))
				continue;

			/* get playback information */
#if defined(SND_LIB_VERSION) && (SND_LIB_VERSION >= 0x400)
			if(alsa_ctl_pcm_playback_info(ctl_h,dev,0,&ctlinfo)<0)
				continue;
#else
			if(alsa_ctl_pcm_playback_info(ctl_h,dev,&ctlinfo)<0)
				continue;
#endif

	/*
	   If control goes here, we have found a sound device able to play PCM data.
	   Let's open in in playback mode and see if we have compatible playback
	   settings.
	*/

			if (alsa_pcm_open(&pcm_h,card,dev,SND_PCM_OPEN_PLAYBACK)<0)
				continue;

			if (alsa_pcm_playback_info(pcm_h,&pinfo)<0) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			/* check we have compatible settings */
			if((pinfo.min_rate>pformat.rate)||(pinfo.max_rate<pformat.rate)||
			   (!(pinfo.formats&(1<<pformat.format)))) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			fragmentsize=pinfo.buffer_size/numfrags;
#ifdef MIKMOD_DEBUG
			if ((fragmentsize<512)||(fragmentsize>16777216L))
				fprintf(stderr,"\rweird pinfo.buffer_size:%d\n",pinfo.buffer_size);
#endif

			alsa_pcm_flush_playback(pcm_h);

			/* set new parameters */
			if(alsa_pcm_playback_format(pcm_h,&pformat)<0) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			/* compute a fragmentsize hint
			   each fragment should be shorter than, but close to, half a
			   second of playback */
			bps=(pformat.rate*pformat.channels*(md_mode&DMODE_16BITS?2:1))>>1;
			size=fragmentsize;while (size>bps) size>>=1;
#ifdef MIKMOD_DEBUG
			if (size < 16) {
				fprintf(stderr,"\rweird hint result:%d from %d, bps=%d\n",size,fragmentsize,bps);
				size=16;
			}
#endif

			memset(&pparams,0,sizeof(pparams));
			pparams.fragment_size=size;
			pparams.fragments_max=-1; /* choose the best */
			pparams.fragments_room=-1;
			if(alsa_pcm_playback_params(pcm_h,&pparams)<0) {
				alsa_pcm_close(pcm_h);
				pcm_h=NULL;
				continue;
			}

			if (!(audiobuffer=(SBYTE*)_mm_malloc(fragmentsize))) {
				alsa_ctl_close(ctl_h);
				return 1;
			}

			/* sound device is ready to work */
			if (VC_Init()) {
				alsa_ctl_close(ctl_h);
				return 1;
			} else
			  return 0;
		}

		alsa_ctl_close(ctl_h);
	}
	return 1;
}

static BOOL ALSA_Init(void)
{
#ifdef MIKMOD_DYNAMIC
	if (ALSA_Link()) {
		_mm_errno=MMERR_DYNAMIC_LINKING;
		return 1;
	}
#endif
	return ALSA_Init_internal();
}

static void ALSA_Exit_internal(void)
{
	VC_Exit();
	if (pcm_h) {
		alsa_pcm_drain_playback(pcm_h);
		alsa_pcm_close(pcm_h);
		pcm_h=NULL;
	}
	_mm_free(audiobuffer);
}

static void ALSA_Exit(void)
{
	ALSA_Exit_internal();
#ifdef MIKMOD_DYNAMIC
	ALSA_Unlink();
#endif
}

static void ALSA_Update(void)
{
	snd_pcm_playback_status_t status;
	int total, count;

	if (alsa_pcm_playback_status(pcm_h, &status) >= 0) {
		/* Update md_mixfreq if necessary */
		if (md_mixfreq != status.rate)
			md_mixfreq = status.rate;

		/* Using status.count would cause clicks, as this is always less than
		   the freespace  in the buffer - so compute how many bytes we can
		   afford */
		total = status.fragments * status.fragment_size - status.queue;
		if (total < fragmentsize)
			total = fragmentsize;
	} else
		total = fragmentsize;
	
	/* Don't send data if ALSA is too busy */
	while (total) {
		count = fragmentsize > total ? total : fragmentsize;
		total -= count;
		alsa_pcm_write(pcm_h,audiobuffer,VC_WriteBytes(audiobuffer,count));
	}
}

static void ALSA_PlayStop(void)
{
	VC_PlayStop();
	
	alsa_pcm_flush_playback(pcm_h);
}

static BOOL ALSA_Reset(void)
{
	ALSA_Exit_internal();
	return ALSA_Init_internal();
}

MIKMODAPI MDRIVER drv_alsa={
	NULL,
	"ALSA",
	"Advanced Linux Sound Architecture (ALSA) driver v0.4",
	0,255,
	"alsa",

	ALSA_CommandLine,
	ALSA_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	ALSA_Init,
	ALSA_Exit,
	ALSA_Reset,
	VC_SetNumVoices,
	VC_PlayStart,
	ALSA_PlayStop,
	ALSA_Update,
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

MISSING(drv_alsa);

#endif

/* ex:set ts=4: */
