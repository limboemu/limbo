/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

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

/* This is the C interface to the SMPEG library */

#ifndef _SMPEG_H_
#define _SMPEG_H_

#include "SDL.h"
#include "SDL_mutex.h"
#include "SDL_audio.h"
#include "MPEGframe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SMPEG_MAJOR_VERSION      2
#define SMPEG_MINOR_VERSION      0
#define SMPEG_PATCHLEVEL         0

typedef struct {
        Uint8 major;
        Uint8 minor;
        Uint8 patch;
} SMPEG_version;

/* This macro can be used to fill a version structure with the compile-time
 * version of the SDL library.
 */
#define SMPEG_VERSION(X)                                                \
{                                                                       \
        (X)->major = SMPEG_MAJOR_VERSION;                               \
        (X)->minor = SMPEG_MINOR_VERSION;                               \
        (X)->patch = SMPEG_PATCHLEVEL;                                  \
}

/* This is the actual SMPEG object */
typedef struct _SMPEG SMPEG;

/* Used to get information about the SMPEG object */
typedef struct _SMPEG_Info {
    int has_audio;
    int has_video;
    int width;
    int height;
    int current_frame;
    double current_fps;
    char audio_string[80];
    int  audio_current_frame;
    Uint32 current_offset;
    Uint32 total_size;
    double current_time;
    double total_time;
} SMPEG_Info;

/* Possible MPEG status codes */
typedef enum {
    SMPEG_ERROR = -1,
    SMPEG_STOPPED,
    SMPEG_PLAYING
} SMPEGstatus;


/* Matches the declaration of SDL_UpdateRect() */
typedef void(*SMPEG_DisplayCallback)(void *data, SMPEG_Frame *frame);

/* Create a new SMPEG object from an MPEG file.
   On return, if 'info' is not NULL, it will be filled with information 
   about the MPEG object.
   This function returns a new SMPEG object.  Use SMPEG_error() to find out
   whether or not there was a problem building the MPEG stream.
   The sdl_audio parameter indicates if SMPEG should initialize the SDL audio
   subsystem. If not, you will have to use the SMPEG_playaudio() function below
   to extract the decoded data.
 */
extern DECLSPEC SMPEG* SMPEG_new(const char *file, SMPEG_Info* info, int sdl_audio);

/* The same as above for a file descriptor */
extern DECLSPEC SMPEG* SMPEG_new_descr(int file, SMPEG_Info* info, int sdl_audio);

/*
   The same as above but for a raw chunk of data.  SMPEG makes a copy of the
   data, so the application is free to delete after a successful call to this
   function.
 */
extern DECLSPEC SMPEG* SMPEG_new_data(void *data, int size, SMPEG_Info* info, int sdl_audio);

/* The same for a generic SDL_RWops structure.
   'freesrc' should be non-zero if SMPEG should close the source when it's done.
 */
extern DECLSPEC SMPEG* SMPEG_new_rwops(SDL_RWops *src, SMPEG_Info* info, int freesrc, int sdl_audio);

/* Get current information about an SMPEG object */
extern DECLSPEC void SMPEG_getinfo( SMPEG* mpeg, SMPEG_Info* info );

/* Enable or disable audio playback in MPEG stream */
extern DECLSPEC void SMPEG_enableaudio( SMPEG* mpeg, int enable );

/* Enable or disable video playback in MPEG stream */
extern DECLSPEC void SMPEG_enablevideo( SMPEG* mpeg, int enable );

/* Delete an SMPEG object */
extern DECLSPEC void SMPEG_delete( SMPEG* mpeg );

/* Get the current status of an SMPEG object */
extern DECLSPEC SMPEGstatus SMPEG_status( SMPEG* mpeg );

/* Set the audio volume of an MPEG stream, in the range 0-100 */
extern DECLSPEC void SMPEG_setvolume( SMPEG* mpeg, int volume );

/* Set the frame display callback for MPEG video
   'lock' is a mutex used to synchronize access to the frame data, and is held during the update callback.
*/
extern DECLSPEC void SMPEG_setdisplay(SMPEG* mpeg, SMPEG_DisplayCallback callback, void* data, SDL_mutex* lock);

/* Set or clear looping play on an SMPEG object */
extern DECLSPEC void SMPEG_loop( SMPEG* mpeg, int repeat );

/* Play an SMPEG object */
extern DECLSPEC void SMPEG_play( SMPEG* mpeg );

/* Pause/Resume playback of an SMPEG object */
extern DECLSPEC void SMPEG_pause( SMPEG* mpeg );

/* Stop playback of an SMPEG object */
extern DECLSPEC void SMPEG_stop( SMPEG* mpeg );

/* Rewind the play position of an SMPEG object to the beginning of the MPEG */
extern DECLSPEC void SMPEG_rewind( SMPEG* mpeg );

/* Seek 'bytes' bytes in the MPEG stream */
extern DECLSPEC void SMPEG_seek( SMPEG* mpeg, int bytes);

/* Skip 'seconds' seconds in the MPEG stream */
extern DECLSPEC void SMPEG_skip( SMPEG* mpeg, float seconds );

/* Render a particular frame in the MPEG video */
extern DECLSPEC void SMPEG_renderFrame( SMPEG* mpeg, int framenum );

/* Render the last frame of an MPEG video */
extern DECLSPEC void SMPEG_renderFinal( SMPEG* mpeg );

/* Return NULL if there is no error in the MPEG stream, or an error message
   if there was a fatal error in the MPEG stream for the SMPEG object.
*/
extern DECLSPEC char *SMPEG_error( SMPEG* mpeg );

/* Exported callback function for audio playback.
   The function takes a buffer and the amount of data to fill, and returns
   the amount of data in bytes that was actually written.  This will be the
   amount requested unless the MPEG audio has finished.
*/
extern DECLSPEC int SMPEG_playAudio( SMPEG *mpeg, Uint8 *stream, int len );

/* Wrapper for SMPEG_playAudio() that can be passed to SDL and SDL_mixer
*/
extern DECLSPEC void SMPEG_playAudioSDL( void *mpeg, Uint8 *stream, int len );

/* Get the best SDL audio spec for the audio stream */
extern DECLSPEC int SMPEG_wantedSpec( SMPEG *mpeg, SDL_AudioSpec *wanted );

/* Inform SMPEG of the actual SDL audio spec used for sound playback */
extern DECLSPEC void SMPEG_actualSpec( SMPEG *mpeg, SDL_AudioSpec *spec );

#ifdef __cplusplus
}
#endif
#endif /* _SMPEG_H_ */
