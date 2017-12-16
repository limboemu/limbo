/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MODPLUG_MUSIC

#ifdef MODPLUG_HEADER
#include MODPLUG_HEADER
#else
#include <libmodplug/modplug.h>
#endif
#include "SDL_rwops.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"

typedef struct {
    ModPlugFile *file;
    int playing;
} modplug_data;

int modplug_init(SDL_AudioSpec *mixer);

/* Uninitialize the music players */
void modplug_exit(void);

/* Set the volume for a modplug stream */
void modplug_setvolume(modplug_data *music, int volume);

/* Load a modplug stream from an SDL_RWops object */
modplug_data *modplug_new_RW(SDL_RWops *rw, int freerw);

/* Start playback of a given modplug stream */
void modplug_play(modplug_data *music);

/* Return non-zero if a stream is currently playing */
int modplug_playing(modplug_data *music);

/* Play some of a stream previously started with modplug_play() */
int modplug_playAudio(modplug_data *music, Uint8 *stream, int len);

/* Stop playback of a stream previously started with modplug_play() */
void modplug_stop(modplug_data *music);

/* Close the given modplug stream */
void modplug_delete(modplug_data *music);

/* Jump (seek) to a given position (time is in seconds) */
void modplug_jump_to_time(modplug_data *music, double time);

#endif /* MODPLUG_MUSIC */
