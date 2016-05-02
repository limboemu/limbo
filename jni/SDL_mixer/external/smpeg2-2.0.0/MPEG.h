/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software
    
    - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn -

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* A class used to parse and play MPEG streams */

#ifndef _MPEG_H_
#define _MPEG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"

#include "MPEGerror.h"
#include "MPEGstream.h"
#include "MPEGaction.h"
#include "MPEGaudio.h"
#include "MPEGvideo.h"
#include "MPEGsystem.h"

#define LENGTH_TO_CHECK_FOR_SYSTEM 0x50000	// Added by HanishKVC

/* The main MPEG class - parses system streams and creates other streams
 A few design notes:
   Making this derived from MPEGstream allows us to do system stream
   parsing.  We create an additional MPEG object for each type of 
   stream in the MPEG file because each needs a separate pointer to
   the MPEG data.  The MPEG stream then creates an accessor object to
   do all the data parsing for that stream type.  It's a little odd,
   but seemed like the best way to implement stream parsing.
 */
class MPEG : public MPEGerror
{
public:
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    MPEG():MPEGerror(){}
	MPEG(bool Sdlaudio, char *addresse,char *asset,long buffersize){}

    MPEG(const char * name, bool SDLaudio = true);
    MPEG(int Mpeg_FD, bool SDLaudio = true);
    MPEG(void *data, int size, bool SDLaudio = true);
    MPEG(SDL_RWops *mpeg_source, int mpeg_freesrc, bool SDLaudio = true);
    virtual ~MPEG();

    /* Initialize the MPEG */
    void Init(SDL_RWops *mpeg_source, int mpeg_freesrc, bool SDLaudio);
    void InitErrorState();

    /* Enable/Disable audio and video */
    bool AudioEnabled(void);
    void EnableAudio(bool enabled);
    bool VideoEnabled(void);
    void EnableVideo(bool enabled);

    /* MPEG actions */
    void Loop(bool toggle);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    virtual void Play(void);
    void Stop(void);
    void Rewind(void);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    virtual void Pause(void);
    virtual void Seek(int bytes);
    void Skip(float seconds);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    MPEGstatus GetStatus(void);
    void GetSystemInfo(MPEG_SystemInfo *info);

    /* MPEG audio actions */
    bool GetAudioInfo(MPEG_AudioInfo *info);
    void Volume(int vol);
    bool WantedSpec(SDL_AudioSpec *wanted);
    void ActualSpec(const SDL_AudioSpec *actual);
    MPEGaudio *GetAudio(void);

    /* MPEG video actions */
    bool GetVideoInfo(MPEG_VideoInfo *info);
    bool SetDisplay(MPEG_DisplayCallback callback, void *data, SDL_mutex *lock);
    void RenderFrame(int frame);
    void RenderFinal();

public:
    /* We need to have separate audio and video streams */
    MPEGstream * audiostream;
    MPEGstream * videostream;

    MPEGsystem * system;

protected:
    char *mpeg_mem;       // Used to copy MPEG passed in as memory
    SDL_RWops *source;
    int freesrc;
    MPEGaudioaction *audioaction;
    MPEGvideoaction *videoaction;

    MPEGaudio *audio;
    MPEGvideo *video;

    bool audioaction_enabled;
    bool videoaction_enabled;

    bool sdlaudio;

    bool loop;
    bool pause;

    void parse_stream_list();
    bool seekIntoStream(int position);
};

#endif /* _MPEG_H_ */
