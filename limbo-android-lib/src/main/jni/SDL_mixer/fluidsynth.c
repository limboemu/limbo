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

#ifdef USE_FLUIDSYNTH_MIDI

#include <stdio.h>
#include <sys/types.h>

#include "SDL_mixer.h"
#include "fluidsynth.h"

static Uint16 format;
static Uint8 channels;
static int freq;

int fluidsynth_check_soundfont(const char *path, void *data)
{
    FILE *file = fopen(path, "r");

    if (file) {
        fclose(file);
        return 1;
    } else {
        Mix_SetError("Failed to access the SoundFont %s", path);
        return 0;
    }
}

int fluidsynth_load_soundfont(const char *path, void *data)
{
    /* If this fails, it's too late to try Timidity so pray that at least one works. */
    fluidsynth.fluid_synth_sfload((fluid_synth_t*) data, path, 1);
    return 1;
}

int fluidsynth_init(SDL_AudioSpec *mixer)
{
    if (!Mix_EachSoundFont(fluidsynth_check_soundfont, NULL))
        return -1;

    format = mixer->format;
    channels = mixer->channels;
    freq = mixer->freq;

    return 0;
}

static FluidSynthMidiSong *fluidsynth_loadsong_common(int (*function)(FluidSynthMidiSong*, void*), void *data)
{
    FluidSynthMidiSong *song;
    fluid_settings_t *settings = NULL;

    if (!Mix_Init(MIX_INIT_FLUIDSYNTH)) {
        return NULL;
    }

    if ((song = SDL_malloc(sizeof(FluidSynthMidiSong)))) {
        SDL_memset(song, 0, sizeof(FluidSynthMidiSong));

        if (SDL_BuildAudioCVT(&song->convert, AUDIO_S16, 2, freq, format, channels, freq) >= 0) {
            if ((settings = fluidsynth.new_fluid_settings())) {
                fluidsynth.fluid_settings_setnum(settings, "synth.sample-rate", (double) freq);

                if ((song->synth = fluidsynth.new_fluid_synth(settings))) {
                    if (Mix_EachSoundFont(fluidsynth_load_soundfont, (void*) song->synth)) {
                        if ((song->player = fluidsynth.new_fluid_player(song->synth))) {
                            if (function(song, data)) return song;
                            fluidsynth.delete_fluid_player(song->player);
                        } else {
                            Mix_SetError("Failed to create FluidSynth player");
                        }
                    }
                    fluidsynth.delete_fluid_synth(song->synth);
                } else {
                    Mix_SetError("Failed to create FluidSynth synthesizer");
                }
                fluidsynth.delete_fluid_settings(settings);
            } else {
                Mix_SetError("Failed to create FluidSynth settings");
            }
        } else {
            Mix_SetError("Failed to set up audio conversion");
        }
        SDL_free(song);
    } else {
        Mix_SetError("Insufficient memory for song");
    }
    return NULL;
}

static int fluidsynth_loadsong_RW_internal(FluidSynthMidiSong *song, void *data)
{
    Sint64 offset;
    size_t size;
    char *buffer;
    SDL_RWops *src = (SDL_RWops*) data;

    offset = SDL_RWtell(src);
    SDL_RWseek(src, 0, RW_SEEK_END);
    size = (size_t)(SDL_RWtell(src) - offset);
    SDL_RWseek(src, offset, RW_SEEK_SET);

    if ((buffer = (char*) SDL_malloc(size))) {
        if(SDL_RWread(src, buffer, size, 1) == 1) {
            if (fluidsynth.fluid_player_add_mem(song->player, buffer, size) == FLUID_OK) {
                SDL_free(buffer);
                return 1;
            } else {
                Mix_SetError("FluidSynth failed to load in-memory song");
            }
        } else {
            Mix_SetError("Failed to read in-memory song");
        }
        SDL_free(buffer);
    } else {
        Mix_SetError("Insufficient memory for song");
    }
    return 0;
}

FluidSynthMidiSong *fluidsynth_loadsong_RW(SDL_RWops *src, int freesrc)
{
    FluidSynthMidiSong *song;

    song = fluidsynth_loadsong_common(fluidsynth_loadsong_RW_internal, (void*) src);
    if (song && freesrc) {
        SDL_RWclose(src);
    }
    return song;
}

void fluidsynth_freesong(FluidSynthMidiSong *song)
{
    if (!song) return;
    fluidsynth.delete_fluid_player(song->player);
    fluidsynth.delete_fluid_settings(fluidsynth.fluid_synth_get_settings(song->synth));
    fluidsynth.delete_fluid_synth(song->synth);
    SDL_free(song);
}

void fluidsynth_start(FluidSynthMidiSong *song)
{
    fluidsynth.fluid_player_set_loop(song->player, 1);
    fluidsynth.fluid_player_play(song->player);
}

void fluidsynth_stop(FluidSynthMidiSong *song)
{
    fluidsynth.fluid_player_stop(song->player);
}

int fluidsynth_active(FluidSynthMidiSong *song)
{
    return fluidsynth.fluid_player_get_status(song->player) == FLUID_PLAYER_PLAYING ? 1 : 0;
}

void fluidsynth_setvolume(FluidSynthMidiSong *song, int volume)
{
    /* FluidSynth's default is 0.2. Make 1.2 the maximum. */
    fluidsynth.fluid_synth_set_gain(song->synth, (float) (volume * 1.2 / MIX_MAX_VOLUME));
}

int fluidsynth_playsome(FluidSynthMidiSong *song, void *dest, int dest_len)
{
    int result = -1;
    int frames = dest_len / channels / ((format & 0xFF) / 8);
    int src_len = frames * 4; /* 16-bit stereo */
    void *src = dest;

    if (dest_len < src_len) {
        if (!(src = SDL_malloc(src_len))) {
            Mix_SetError("Insufficient memory for audio conversion");
            return result;
        }
    }

    if (fluidsynth.fluid_synth_write_s16(song->synth, frames, src, 0, 2, src, 1, 2) != FLUID_OK) {
        Mix_SetError("Error generating FluidSynth audio");
        goto finish;
    }

    song->convert.buf = src;
    song->convert.len = src_len;

    if (SDL_ConvertAudio(&song->convert) < 0) {
        Mix_SetError("Error during audio conversion");
        goto finish;
    }

    if (src != dest)
        SDL_memcpy(dest, src, dest_len);

    result = 0;

finish:
    if (src != dest)
        SDL_free(src);

    return result;
}

#endif /* USE_FLUIDSYNTH_MIDI */
