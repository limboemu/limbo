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

  $Id: drv_stdout.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Output data to stdout

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#include "mikmod_internals.h"

#define BUFFERSIZE 32768

static	SBYTE *audiobuffer=NULL;

static BOOL stdout_IsThere(void)
{
	/* only allow this driver on pipes */
	return 1-isatty(1);
}

static BOOL stdout_Init(void)
{
	if(!(audiobuffer=(SBYTE*)_mm_malloc(BUFFERSIZE))) return 1;
#ifdef __EMX__
	_fsetmode(stdout,"b");
#endif
	return VC_Init();
}

static void stdout_Exit(void)
{
	VC_Exit();
#ifdef __EMX__
	_fsetmode(stdout,"t");
#endif
	if (audiobuffer) {
		free(audiobuffer);
		audiobuffer=NULL;
	}
}

static void stdout_Update(void)
{
#ifdef WIN32
	_write
#else
	write
#endif
	     (1,audiobuffer,VC_WriteBytes((SBYTE*)audiobuffer,BUFFERSIZE));
}

static BOOL stdout_Reset(void)
{
	VC_Exit();
	return VC_Init();
}

MIKMODAPI MDRIVER drv_stdout={
	NULL,
	"stdout",
	"Standard output driver v1.1",
	0,255,
	"stdout",

	NULL,
	stdout_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	stdout_Init,
	stdout_Exit,
	stdout_Reset,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	stdout_Update,
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
