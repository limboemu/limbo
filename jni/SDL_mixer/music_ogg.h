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

#ifdef OGG_MUSIC

/* This file supports Ogg Vorbis music streams */

#ifdef OGG_USE_TREMOR
#include <ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

typedef struct {
    SDL_RWops *src;
    int freesrc;
    int playing;
    int volume;
    OggVorbis_File vf;
    int section;
    SDL_AudioCVT cvt;
    int len_available;
    Uint8 *snd_available;
} OGG_music;

/* Initialize the Ogg Vorbis player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
extern int OGG_init(SDL_AudioSpec *mixer);

/* Set the volume for an OGG stream */
extern void OGG_setvolume(OGG_music *music, int volume);

/* Load an OGG stream from an SDL_RWops object */
extern OGG_music *OGG_new_RW(SDL_RWops *src, int freesrc);

/* Start playback of a given OGG stream */
extern void OGG_play(OGG_music *music);

/* Return non-zero if a stream is currently playing */
extern int OGG_playing(OGG_music *music);

/* Play some of a stream previously started with OGG_play() */
extern int OGG_playAudio(OGG_music *music, Uint8 *stream, int len);

/* Stop playback of a stream previously started with OGG_play() */
extern void OGG_stop(OGG_music *music);

/* Close the given OGG stream */
extern void OGG_delete(OGG_music *music);

/* Jump (seek) to a given position (time is in seconds) */
extern void OGG_jump_to_time(OGG_music *music, double time);

#endif /* OGG_MUSIC */
