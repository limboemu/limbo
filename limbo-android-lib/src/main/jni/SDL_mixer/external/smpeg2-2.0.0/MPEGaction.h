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

/* A virtual class to provide basic MPEG playback actions */

#ifndef _MPEGACTION_H_
#define _MPEGACTION_H_

#include "SDL.h"
#include "MPEGframe.h"

typedef enum {
    MPEG_ERROR = -1,
    MPEG_STOPPED,
    MPEG_PLAYING
} MPEGstatus;

class MPEGaction {
public:
    MPEGaction() {
        playing = false;
        paused = false;
        looping = false;
	play_time = 0.0;
    }
    virtual void Loop(bool toggle) {
        looping = toggle;
    }
    virtual double Time(void) {  /* Returns the time in seconds since start */
        return play_time;
    }
    virtual void Play(void) = 0;
    virtual void Stop(void) = 0;
    virtual void Rewind(void) = 0;
    virtual void ResetSynchro(double) = 0;
    virtual void Skip(float seconds) = 0;
    virtual void Pause(void) {  /* A toggle action */
        if ( paused ) {
            paused = false;
            Play();
        } else {
            Stop();
            paused = true;
        }
    }
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  conflict name in popcorn */
    virtual MPEGstatus GetStatus(void) = 0;

protected:
    bool playing;
    bool paused;
    bool looping;
    double play_time;

    bool force_exit;
    
    void ResetPause(void) {
        paused = false;
    }
};

/* For getting info about the audio portion of the stream */
typedef struct MPEG_AudioInfo {
    int mpegversion;
    int mode;
    int frequency;
    int layer;
    int bitrate;
    int current_frame;
} MPEG_AudioInfo;

/* Audio action class */
class MPEGaudioaction : public MPEGaction {
public:
    virtual bool GetAudioInfo(MPEG_AudioInfo *info) {
        return(true);
    }
    virtual void Volume(int vol) = 0;
};

typedef void(*MPEG_DisplayCallback)(void *data, SMPEG_Frame *frame);

/* For getting info about the video portion of the stream */
typedef struct MPEG_VideoInfo {
    int width;
    int height;
    int current_frame;
    double current_fps;
} MPEG_VideoInfo;

/* Video action class */
class MPEGvideoaction : public MPEGaction {
public:
    virtual void SetTimeSource(MPEGaudioaction *source) {
        time_source = source;
    }
    virtual bool GetVideoInfo(MPEG_VideoInfo *info) {
        return(false);
    }
    virtual bool SetDisplay(MPEG_DisplayCallback callback, void *data, SDL_mutex *lock) = 0;
    virtual void RenderFrame(int frame) = 0;
    virtual void RenderFinal() = 0;
protected:
    MPEGaudioaction *time_source;
};


/* For getting info about the system portion of the stream */
typedef struct MPEG_SystemInfo {
    int total_size;
    int current_offset;
    double total_time;
    double current_time;
} MPEG_SystemInfo;

#endif /* _MPEGACTION_H_ */
