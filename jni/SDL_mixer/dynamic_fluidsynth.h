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

  James Le Cuirot
  chewi@aura-online.co.uk
*/

#ifdef USE_FLUIDSYNTH_MIDI

#include <fluidsynth.h>

typedef struct {
    int loaded;
    void *handle;

    int (*delete_fluid_player)(fluid_player_t*);
    void (*delete_fluid_settings)(fluid_settings_t*);
    int (*delete_fluid_synth)(fluid_synth_t*);
    int (*fluid_player_add)(fluid_player_t*, const char*);
    int (*fluid_player_add_mem)(fluid_player_t*, const void*, size_t);
    int (*fluid_player_get_status)(fluid_player_t*);
    int (*fluid_player_play)(fluid_player_t*);
    int (*fluid_player_set_loop)(fluid_player_t*, int);
    int (*fluid_player_stop)(fluid_player_t*);
    int (*fluid_settings_setnum)(fluid_settings_t*, const char*, double);
    fluid_settings_t* (*fluid_synth_get_settings)(fluid_synth_t*);
    void (*fluid_synth_set_gain)(fluid_synth_t*, float);
    int (*fluid_synth_sfload)(fluid_synth_t*, const char*, int);
    int (*fluid_synth_write_s16)(fluid_synth_t*, int, void*, int, int, void*, int, int);
    fluid_player_t* (*new_fluid_player)(fluid_synth_t*);
    fluid_settings_t* (*new_fluid_settings)(void);
    fluid_synth_t* (*new_fluid_synth)(fluid_settings_t*);
} fluidsynth_loader;

extern fluidsynth_loader fluidsynth;

#endif /* USE_FLUIDSYNTH_MIDI */

extern int Mix_InitFluidSynth();
extern void Mix_QuitFluidSynth();
