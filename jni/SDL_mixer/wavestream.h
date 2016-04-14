/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

/* This file supports streaming WAV files, without volume adjustment */

#include <stdio.h>

typedef struct {
    SDL_RWops *src;
    SDL_bool freesrc;
    long  start;
    long  stop;
    SDL_AudioCVT cvt;
} WAVStream;

/* Initialize the WAVStream player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
extern int WAVStream_Init(SDL_AudioSpec *mixer);

/* Unimplemented */
extern void WAVStream_SetVolume(int volume);

/* Load a WAV stream from an SDL_RWops object */
extern WAVStream *WAVStream_LoadSong_RW(SDL_RWops *src, int freesrc);

/* Start playback of a given WAV stream */
extern void WAVStream_Start(WAVStream *wave);

/* Play some of a stream previously started with WAVStream_Start() */
extern int WAVStream_PlaySome(Uint8 *stream, int len);

/* Stop playback of a stream previously started with WAVStream_Start() */
extern void WAVStream_Stop(void);

/* Close the given WAV stream */
extern void WAVStream_FreeSong(WAVStream *wave);

/* Return non-zero if a stream is currently playing */
extern int WAVStream_Active(void);
