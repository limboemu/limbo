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

  $Id: drv_pipe.c,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Driver for output via a pipe to another command

==============================================================================*/

/*

	Written by Simon Hosie <gumboot@clear.net.nz>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#ifdef DRV_PIPE

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#if defined unix || (defined __APPLE__ && defined __MACH__)
#include <errno.h>
#include <sys/wait.h>
#endif

#ifdef SUNOS
extern int fclose(FILE *);
#endif

#define BUFFERSIZE 32768

static	MWRITER *pipeout=NULL;
static	FILE *pipefile=NULL;
#if defined unix || (defined __APPLE__ && defined __MACH__)
static	int pipefd[2]={-1,-1};
static	pid_t pid;
#endif
static	SBYTE *audiobuffer=NULL;

static	CHAR *target=NULL;

static void pipe_CommandLine(CHAR *cmdline)
{
	CHAR *ptr=MD_GetAtom("pipe",cmdline,0);

	if(ptr) {
		_mm_free(target);
		target=ptr;
	}
}

static BOOL pipe_IsThere(void)
{
	return 1;
}

static BOOL pipe_Init(void)
{
	if(!target) {
		_mm_errno=MMERR_OPENING_FILE;
		return 1;
	}
#if !defined unix && (!defined __APPLE__ || !defined __MACH__)
#ifdef __EMX__
	_fsetmode(stdout, "b");
#endif
#ifdef __WATCOMC__
	pipefile = _popen(target, "wb");
#else
	pipefile = popen(target, "wb");
#endif
	if (!pipefile) {
		_mm_errno=MMERR_OPENING_FILE;
		return 1;
	}
#else
	/* poor man's popen() */
	if (pipe(pipefd)) {
		_mm_errno = MMERR_OPENING_FILE;
		return 1;
	}
	switch (pid=fork()) {
		case -1:
			close(pipefd[0]);
			close(pipefd[1]);
			pipefd[0]=pipefd[1]=-1;
			_mm_errno=MMERR_OPENING_FILE;
			return 1;
		case 0:
			if (pipefd[0]) {
				dup2(pipefd[0],0);
				close(pipefd[0]);
			}
			close(pipefd[1]);
			if (!MD_DropPrivileges())
				execl("/bin/sh","sh","-c",target,NULL);
			exit(127);
	}
	close(pipefd[0]);
	if (!(pipefile=fdopen(pipefd[1],"wb"))) {
		_mm_errno=MMERR_OPENING_FILE;
		return 1;
	}
#endif
	if(!(pipeout=_mm_new_file_writer(pipefile)))
		return 1;
	if(!(audiobuffer=(SBYTE*)_mm_malloc(BUFFERSIZE)))
		return 1;

	md_mode|=DMODE_SOFT_MUSIC|DMODE_SOFT_SNDFX;

	return VC_Init();
}

static void pipe_Exit(void)
{
#if defined unix || (defined __APPLE__ && defined __MACH__)
	int pstat;
	pid_t pid2;
#endif

	VC_Exit();
	_mm_free(audiobuffer);
	if(pipeout) {
		_mm_delete_file_writer(pipeout);
		pipeout=NULL;
	}
	if(pipefile) {
#if !defined unix && (!defined __APPLE__ || !defined __MACH__)
#ifdef __WATCOMC__
		_pclose(pipefile);
#else
		pclose(pipefile);
#endif
#ifdef __EMX__
		_fsetmode(stdout,"t");
#endif
#else
		fclose(pipefile);
		do {
			pid2=waitpid(pid,&pstat,0);
		} while (pid2==-1 && errno==EINTR);
#endif
		pipefile=NULL;
	}
}

static void pipe_Update(void)
{
	_mm_write_UBYTES(audiobuffer,VC_WriteBytes(audiobuffer,BUFFERSIZE),pipeout);
}

MIKMODAPI MDRIVER drv_pipe={
	NULL,
	"Piped writer",
	"Piped Output driver v0.2",
	0,255,
	"pipe",

	pipe_CommandLine,
	pipe_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	pipe_Init,
	pipe_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	pipe_Update,
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

MISSING(drv_pipe);
		
#endif

/* ex:set ts=4: */
