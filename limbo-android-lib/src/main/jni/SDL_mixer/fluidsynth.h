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

  James Le Cuirot
  chewi@aura-online.co.uk
*/

#ifndef _FLUIDSYNTH_H_
#define _FLUIDSYNTH_H_

#ifdef USE_FLUIDSYNTH_MIDI

#include "dynamic_fluidsynth.h"
#include <SDL_rwops.h>
#include <SDL_audio.h>

typedef struct {
    SDL_AudioCVT convert;
    fluid_synth_t *synth;
    fluid_player_t* player;
} FluidSynthMidiSong;

int fluidsynth_init(SDL_AudioSpec *mixer);
FluidSynthMidiSong *fluidsynth_loadsong_RW(SDL_RWops *rw, int freerw);
void fluidsynth_freesong(FluidSynthMidiSong *song);
void fluidsynth_start(FluidSynthMidiSong *song);
void fluidsynth_stop(FluidSynthMidiSong *song);
int fluidsynth_active(FluidSynthMidiSong *song);
void fluidsynth_setvolume(FluidSynthMidiSong *song, int volume);
int fluidsynth_playsome(FluidSynthMidiSong *song, void *stream, int len);

#endif /* USE_FLUIDSYNTH_MIDI */

#endif /* _FLUIDSYNTH_H_ */
