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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_timer.h"

#include "SDL_mixer.h"

#ifdef CMD_MUSIC
#include "music_cmd.h"
#endif
#ifdef WAV_MUSIC
#include "wavestream.h"
#endif
#ifdef MODPLUG_MUSIC
#include "music_modplug.h"
#endif
#ifdef MOD_MUSIC
#include "music_mod.h"
#endif
#ifdef MID_MUSIC
#  ifdef USE_TIMIDITY_MIDI
#    include "timidity.h"
#  endif
#  ifdef USE_FLUIDSYNTH_MIDI
#    include "fluidsynth.h"
#  endif
#  ifdef USE_NATIVE_MIDI
#    include "native_midi.h"
#  endif
#endif
#ifdef OGG_MUSIC
#include "music_ogg.h"
#endif
#ifdef MP3_MUSIC
#include "dynamic_mp3.h"
#endif
#ifdef MP3_MAD_MUSIC
#include "music_mad.h"
#endif
#ifdef FLAC_MUSIC
#include "music_flac.h"
#endif

#if defined(MP3_MUSIC) || defined(MP3_MAD_MUSIC)
static SDL_AudioSpec used_mixer;
#endif


int volatile music_active = 1;
static int volatile music_stopped = 0;
static int music_loops = 0;
static char *music_cmd = NULL;
static Mix_Music * volatile music_playing = NULL;
static int music_volume = MIX_MAX_VOLUME;

struct _Mix_Music {
    Mix_MusicType type;
    union {
#ifdef CMD_MUSIC
        MusicCMD *cmd;
#endif
#ifdef WAV_MUSIC
        WAVStream *wave;
#endif
#ifdef MODPLUG_MUSIC
        modplug_data *modplug;
#endif
#ifdef MOD_MUSIC
        struct MODULE *module;
#endif
#ifdef MID_MUSIC
#ifdef USE_TIMIDITY_MIDI
        MidiSong *midi;
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        FluidSynthMidiSong *fluidsynthmidi;
#endif
#ifdef USE_NATIVE_MIDI
        NativeMidiSong *nativemidi;
#endif
#endif
#ifdef OGG_MUSIC
        OGG_music *ogg;
#endif
#ifdef MP3_MUSIC
        SMPEG *mp3;
#endif
#ifdef MP3_MAD_MUSIC
        mad_data *mp3_mad;
#endif
#ifdef FLAC_MUSIC
        FLAC_music *flac;
#endif
    } data;
    Mix_Fading fading;
    int fade_step;
    int fade_steps;
    int error;
};
#ifdef MID_MUSIC
#ifdef USE_TIMIDITY_MIDI
static int timidity_ok;
static int samplesize;
#endif
#ifdef USE_FLUIDSYNTH_MIDI
static int fluidsynth_ok;
#endif
#ifdef USE_NATIVE_MIDI
static int native_midi_ok;
#endif
#endif

/* Used to calculate fading steps */
static int ms_per_step;

/* rcg06042009 report available decoders at runtime. */
static const char **music_decoders = NULL;
static int num_decoders = 0;

/* Semicolon-separated SoundFont paths */
#ifdef MID_MUSIC
char* soundfont_paths = NULL;
#endif

int Mix_GetNumMusicDecoders(void)
{
    return(num_decoders);
}

const char *Mix_GetMusicDecoder(int index)
{
    if ((index < 0) || (index >= num_decoders)) {
        return NULL;
    }
    return(music_decoders[index]);
}

static void add_music_decoder(const char *decoder)
{
    void *ptr = SDL_realloc((void *)music_decoders, (num_decoders + 1) * sizeof (const char *));
    if (ptr == NULL) {
        return;  /* oh well, go on without it. */
    }
    music_decoders = (const char **) ptr;
    music_decoders[num_decoders++] = decoder;
}

/* Local low-level functions prototypes */
static void music_internal_initialize_volume(void);
static void music_internal_volume(int volume);
static int  music_internal_play(Mix_Music *music, double position);
static int  music_internal_position(double position);
static int  music_internal_playing();
static void music_internal_halt(void);


/* Support for hooking when the music has finished */
static void (*music_finished_hook)(void) = NULL;

void Mix_HookMusicFinished(void (*music_finished)(void))
{
    SDL_LockAudio();
    music_finished_hook = music_finished;
    SDL_UnlockAudio();
}


/* If music isn't playing, halt it if no looping is required, restart it */
/* othesrchise. NOP if the music is playing */
static int music_halt_or_loop (void)
{
    /* Restart music if it has to loop */

    if (!music_internal_playing())
    {
#ifdef USE_NATIVE_MIDI
        /* Native MIDI handles looping internally */
        if (music_playing->type == MUS_MID && native_midi_ok) {
            music_loops = 0;
        }
#endif

        /* Restart music if it has to loop at a high level */
        if (music_loops)
        {
            Mix_Fading current_fade;
            if (music_loops > 0) {
                --music_loops;
            }
            current_fade = music_playing->fading;
            music_internal_play(music_playing, 0.0);
            music_playing->fading = current_fade;
        }
        else
        {
            music_internal_halt();
            if (music_finished_hook)
                music_finished_hook();

            return 0;
        }
    }

    return 1;
}



/* Mixing function */
void music_mixer(void *udata, Uint8 *stream, int len)
{
    int left = 0;

    if ( music_playing && music_active ) {
        /* Handle fading */
        if ( music_playing->fading != MIX_NO_FADING ) {
            if ( music_playing->fade_step++ < music_playing->fade_steps ) {
                int volume;
                int fade_step = music_playing->fade_step;
                int fade_steps = music_playing->fade_steps;

                if ( music_playing->fading == MIX_FADING_OUT ) {
                    volume = (music_volume * (fade_steps-fade_step)) / fade_steps;
                } else { /* Fading in */
                    volume = (music_volume * fade_step) / fade_steps;
                }
                music_internal_volume(volume);
            } else {
                if ( music_playing->fading == MIX_FADING_OUT ) {
                    music_internal_halt();
                    if ( music_finished_hook ) {
                        music_finished_hook();
                    }
                    return;
                }
                music_playing->fading = MIX_NO_FADING;
            }
        }

        music_halt_or_loop();
        if (!music_internal_playing())
            return;

        switch (music_playing->type) {
#ifdef CMD_MUSIC
            case MUS_CMD:
                /* The playing is done externally */
                break;
#endif
#ifdef WAV_MUSIC
            case MUS_WAV:
                left = WAVStream_PlaySome(stream, len);
                break;
#endif
#ifdef MODPLUG_MUSIC
            case MUS_MODPLUG:
                left = modplug_playAudio(music_playing->data.modplug, stream, len);
                break;
#endif
#ifdef MOD_MUSIC
            case MUS_MOD:
                left = MOD_playAudio(music_playing->data.module, stream, len);
                break;
#endif
#ifdef MID_MUSIC
            case MUS_MID:
#ifdef USE_NATIVE_MIDI
                if ( native_midi_ok ) {
                    /* Native midi is handled asynchronously */
                    goto skip;
                }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
                if ( fluidsynth_ok ) {
                    fluidsynth_playsome(music_playing->data.fluidsynthmidi, stream, len);
                    goto skip;
                }
#endif
#ifdef USE_TIMIDITY_MIDI
                if ( timidity_ok ) {
                    int samples = len / samplesize;
                    Timidity_PlaySome(stream, samples);
                    goto skip;
                }
#endif
                break;
#endif
#ifdef OGG_MUSIC
            case MUS_OGG:

                left = OGG_playAudio(music_playing->data.ogg, stream, len);
                break;
#endif
#ifdef FLAC_MUSIC
            case MUS_FLAC:
                left = FLAC_playAudio(music_playing->data.flac, stream, len);
                break;
#endif
#ifdef MP3_MUSIC
            case MUS_MP3:
                left = (len - smpeg.SMPEG_playAudio(music_playing->data.mp3, stream, len));
                break;
#endif
#ifdef MP3_MAD_MUSIC
            case MUS_MP3_MAD:
                left = mad_getSamples(music_playing->data.mp3_mad, stream, len);
                break;
#endif
            default:
                /* Unknown music type?? */
                break;
        }
    }

skip:
    /* Handle seamless music looping */
    if (left > 0 && left < len) {
        music_halt_or_loop();
        if (music_internal_playing())
            music_mixer(udata, stream+(len-left), left);
    }
}

/* Initialize the music players with a certain desired audio format */
int open_music(SDL_AudioSpec *mixer)
{
#ifdef WAV_MUSIC
    if ( WAVStream_Init(mixer) == 0 ) {
        add_music_decoder("WAVE");
    }
#endif
#ifdef MODPLUG_MUSIC
    if ( modplug_init(mixer) == 0 ) {
        add_music_decoder("MODPLUG");
    }
#endif
#ifdef MOD_MUSIC
    if ( MOD_init(mixer) == 0 ) {
        add_music_decoder("MIKMOD");
    }
#endif
#ifdef MID_MUSIC
#ifdef USE_TIMIDITY_MIDI
    samplesize = mixer->size / mixer->samples;
    if ( Timidity_Init(mixer->freq, mixer->format,
                        mixer->channels, mixer->samples) == 0 ) {
        timidity_ok = 1;
        add_music_decoder("TIMIDITY");
    } else {
        timidity_ok = 0;
    }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
    if ( fluidsynth_init(mixer) == 0 ) {
        fluidsynth_ok = 1;
        add_music_decoder("FLUIDSYNTH");
    } else {
        fluidsynth_ok = 0;
    }
#endif
#ifdef USE_NATIVE_MIDI
#ifdef USE_FLUIDSYNTH_MIDI
    native_midi_ok = !fluidsynth_ok;
    if ( native_midi_ok )
#endif
#ifdef USE_TIMIDITY_MIDI
        native_midi_ok = !timidity_ok;
    if ( !native_midi_ok ) {
        native_midi_ok = (getenv("SDL_NATIVE_MUSIC") != NULL);
    }
    if ( native_midi_ok )
#endif
        native_midi_ok = native_midi_detect();
    if ( native_midi_ok )
        add_music_decoder("NATIVEMIDI");
#endif
#endif
#ifdef OGG_MUSIC
    if ( OGG_init(mixer) == 0 ) {
        add_music_decoder("OGG");
    }
#endif
#ifdef FLAC_MUSIC
    if ( FLAC_init(mixer) == 0 ) {
        add_music_decoder("FLAC");
    }
#endif
#if defined(MP3_MUSIC) || defined(MP3_MAD_MUSIC)
    /* Keep a copy of the mixer */
    used_mixer = *mixer;
    add_music_decoder("MP3");
#endif

    music_playing = NULL;
    music_stopped = 0;
    Mix_VolumeMusic(SDL_MIX_MAXVOLUME);

    /* Calculate the number of ms for each callback */
    ms_per_step = (int) (((float)mixer->samples * 1000.0) / mixer->freq);

    return(0);
}

/* Portable case-insensitive string compare function */
int MIX_string_equals(const char *str1, const char *str2)
{
    while ( *str1 && *str2 ) {
        if ( toupper((unsigned char)*str1) !=
             toupper((unsigned char)*str2) )
            break;
        ++str1;
        ++str2;
    }
    return (!*str1 && !*str2);
}

static int detect_mp3(Uint8 *magic)
{
    if ( strncmp((char *)magic, "ID3", 3) == 0 ) {
        return 1;
    }

    /* Detection code lifted from SMPEG */
    if(((magic[0] & 0xff) != 0xff) || // No sync bits
       ((magic[1] & 0xf0) != 0xf0) || //
       ((magic[2] & 0xf0) == 0x00) || // Bitrate is 0
       ((magic[2] & 0xf0) == 0xf0) || // Bitrate is 15
       ((magic[2] & 0x0c) == 0x0c) || // Frequency is 3
       ((magic[1] & 0x06) == 0x00)) { // Layer is 4
        return(0);
    }
    return 1;
}

/* MUS_MOD can't be auto-detected. If no other format was detected, MOD is
 * assumed and MUS_MOD will be returned, meaning that the format might not
 * actually be MOD-based.
 *
 * Returns MUS_NONE in case of errors. */
static Mix_MusicType detect_music_type(SDL_RWops *src)
{
    Uint8 magic[5];
    Uint8 moremagic[9];

    Sint64 start = SDL_RWtell(src);
    if (SDL_RWread(src, magic, 1, 4) != 4 || SDL_RWread(src, moremagic, 1, 8) != 8 ) {
        Mix_SetError("Couldn't read from RWops");
        return MUS_NONE;
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    magic[4]='\0';
    moremagic[8] = '\0';

    /* WAVE files have the magic four bytes "RIFF"
       AIFF files have the magic 12 bytes "FORM" XXXX "AIFF" */
    if (((strcmp((char *)magic, "RIFF") == 0) && (strcmp((char *)(moremagic+4), "WAVE") == 0)) ||
        (strcmp((char *)magic, "FORM") == 0)) {
        return MUS_WAV;
    }

    /* Ogg Vorbis files have the magic four bytes "OggS" */
    if (strcmp((char *)magic, "OggS") == 0) {
        return MUS_OGG;
    }

    /* FLAC files have the magic four bytes "fLaC" */
    if (strcmp((char *)magic, "fLaC") == 0) {
        return MUS_FLAC;
    }

    /* MIDI files have the magic four bytes "MThd" */
    if (strcmp((char *)magic, "MThd") == 0) {
        return MUS_MID;
    }

    if (detect_mp3(magic)) {
        return MUS_MP3;
    }

    /* Assume MOD format.
     *
     * Apparently there is no way to check if the file is really a MOD,
     * or there are too many formats supported by MikMod/ModPlug, or
     * MikMod/ModPlug does this check by itself. */
    return MUS_MOD;
}

/* Load a music file */
Mix_Music *Mix_LoadMUS(const char *file)
{
    SDL_RWops *src;
    Mix_Music *music;
    Mix_MusicType type;
    char *ext = strrchr(file, '.');

#ifdef CMD_MUSIC
    if ( music_cmd ) {
        /* Allocate memory for the music structure */
        music = (Mix_Music *)SDL_malloc(sizeof(Mix_Music));
        if ( music == NULL ) {
            Mix_SetError("Out of memory");
            return(NULL);
        }
        music->error = 0;
        music->type = MUS_CMD;
        music->data.cmd = MusicCMD_LoadSong(music_cmd, file);
        if ( music->data.cmd == NULL ) {
            SDL_free(music);
            music = NULL;
        }
        return music;
    }
#endif

    src = SDL_RWFromFile(file, "rb");
    if ( src == NULL ) {
        Mix_SetError("Couldn't open '%s'", file);
        return NULL;
    }

    /* Use the extension as a first guess on the file type */
    type = MUS_NONE;
    ext = strrchr(file, '.');
    /* No need to guard these with #ifdef *_MUSIC stuff,
     * since we simply call Mix_LoadMUSType_RW() later */
    if ( ext ) {
        ++ext; /* skip the dot in the extension */
        if ( MIX_string_equals(ext, "WAV") ) {
            type = MUS_WAV;
        } else if ( MIX_string_equals(ext, "MID") ||
                    MIX_string_equals(ext, "MIDI") ||
                    MIX_string_equals(ext, "KAR") ) {
            type = MUS_MID;
        } else if ( MIX_string_equals(ext, "OGG") ) {
            type = MUS_OGG;
        } else if ( MIX_string_equals(ext, "FLAC") ) {
            type = MUS_FLAC;
        } else  if ( MIX_string_equals(ext, "MPG") ||
                     MIX_string_equals(ext, "MPEG") ||
                     MIX_string_equals(ext, "MP3") ||
                     MIX_string_equals(ext, "MAD") ) {
            type = MUS_MP3;
        }
    }
    if ( type == MUS_NONE ) {
        type = detect_music_type(src);
    }

    /* We need to know if a specific error occurs; if not, we'll set a
     * generic one, so we clear the current one. */
    Mix_SetError("");
    music = Mix_LoadMUSType_RW(src, type, SDL_TRUE);
    if ( music == NULL && Mix_GetError()[0] == '\0' ) {
        Mix_SetError("Unrecognized music format");
    }
    return music;
}

Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc)
{
    return Mix_LoadMUSType_RW(src, MUS_NONE, freesrc);
}

Mix_Music *Mix_LoadMUSType_RW(SDL_RWops *src, Mix_MusicType type, int freesrc)
{
    Mix_Music *music;
    Sint64 start;

    if (!src) {
        Mix_SetError("RWops pointer is NULL");
        return NULL;
    }
    start = SDL_RWtell(src);

    /* If the caller wants auto-detection, figure out what kind of file
     * this is. */
    if (type == MUS_NONE) {
        if ((type = detect_music_type(src)) == MUS_NONE) {
            /* Don't call Mix_SetError() here since detect_music_type()
             * does that. */
            if (freesrc) {
                SDL_RWclose(src);
            }
            return NULL;
        }
    }

    /* Allocate memory for the music structure */
    music = (Mix_Music *)SDL_malloc(sizeof(Mix_Music));
    if (music == NULL ) {
        Mix_SetError("Out of memory");
        if (freesrc) {
            SDL_RWclose(src);
        }
        return NULL;
    }
    music->error = 1;

    switch (type) {
#ifdef WAV_MUSIC
    case MUS_WAV:
        music->type = MUS_WAV;
        music->data.wave = WAVStream_LoadSong_RW(src, freesrc);
        if (music->data.wave) {
            music->error = 0;
        }
        break;
#endif
#ifdef OGG_MUSIC
    case MUS_OGG:
        music->type = MUS_OGG;
        music->data.ogg = OGG_new_RW(src, freesrc);
        if (music->data.ogg) {
            music->error = 0;
        }
        break;
#endif
#ifdef FLAC_MUSIC
    case MUS_FLAC:
        music->type = MUS_FLAC;
        music->data.flac = FLAC_new_RW(src, freesrc);
        if (music->data.flac) {
            music->error = 0;
        }
        break;
#endif
#ifdef MP3_MUSIC
    case MUS_MP3:
        if (Mix_Init(MIX_INIT_MP3)) {
            SMPEG_Info info;
            music->type = MUS_MP3;
            music->data.mp3 = smpeg.SMPEG_new_rwops(src, &info, freesrc, 0);
            if (!info.has_audio) {
                Mix_SetError("MPEG file does not have any audio stream.");
                smpeg.SMPEG_delete(music->data.mp3);
                /* Deleting the MP3 closed the source if desired */
                freesrc = SDL_FALSE;
            } else {
                smpeg.SMPEG_actualSpec(music->data.mp3, &used_mixer);
                music->error = 0;
            }
        }
        break;
#elif defined(MP3_MAD_MUSIC)
    case MUS_MP3:
        music->type = MUS_MP3_MAD;
        music->data.mp3_mad = mad_openFileRW(src, &used_mixer, freesrc);
        if (music->data.mp3_mad) {
            music->error = 0;
        } else {
            Mix_SetError("Could not initialize MPEG stream.");
        }
        break;
#endif
#ifdef MID_MUSIC
    case MUS_MID:
        music->type = MUS_MID;
#ifdef USE_NATIVE_MIDI
        if (native_midi_ok) {
            SDL_RWseek(src, start, RW_SEEK_SET);
            music->data.nativemidi = native_midi_loadsong_RW(src, freesrc);
            if (music->data.nativemidi) {
                music->error = 0;
            } else {
                Mix_SetError("%s", native_midi_error());
            }
            break;
        }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        if (fluidsynth_ok) {
            SDL_RWseek(src, start, RW_SEEK_SET);
            music->data.fluidsynthmidi = fluidsynth_loadsong_RW(src, freesrc);
            if (music->data.fluidsynthmidi) {
                music->error = 0;
            }
            break;
        }
#endif
#ifdef USE_TIMIDITY_MIDI
        if (timidity_ok) {
            SDL_RWseek(src, start, RW_SEEK_SET);
            music->data.midi = Timidity_LoadSong_RW(src, freesrc);
            if (music->data.midi) {
                music->error = 0;
            } else {
                Mix_SetError("%s", Timidity_Error());
            }
        } else {
            Mix_SetError("%s", Timidity_Error());
        }
#endif
        break;
#endif
#if defined(MODPLUG_MUSIC) || defined(MOD_MUSIC)
    case MUS_MOD:
#ifdef MODPLUG_MUSIC
        if (music->error) {
            SDL_RWseek(src, start, RW_SEEK_SET);
            music->type = MUS_MODPLUG;
            music->data.modplug = modplug_new_RW(src, freesrc);
            if (music->data.modplug) {
                music->error = 0;
            }
        }
#endif
#ifdef MOD_MUSIC
        if (music->error) {
            SDL_RWseek(src, start, RW_SEEK_SET);
            music->type = MUS_MOD;
            music->data.module = MOD_new_RW(src, freesrc);
            if (music->data.module) {
                music->error = 0;
            }
        }
#endif
        break;
#endif

    default:
        Mix_SetError("Unrecognized music format");
        break;
    } /* switch (want) */

    if (music->error) {
        SDL_free(music);
        if (freesrc) {
            SDL_RWclose(src);
        } else {
            SDL_RWseek(src, start, RW_SEEK_SET);
        }
        music = NULL;
    }
    return music;
}

/* Free a music chunk previously loaded */
void Mix_FreeMusic(Mix_Music *music)
{
    if ( music ) {
        /* Stop the music if it's currently playing */
        SDL_LockAudio();
        if ( music == music_playing ) {
            /* Wait for any fade out to finish */
            while ( music->fading == MIX_FADING_OUT ) {
                SDL_UnlockAudio();
                SDL_Delay(100);
                SDL_LockAudio();
            }
            if ( music == music_playing ) {
                music_internal_halt();
            }
        }
        SDL_UnlockAudio();
        switch (music->type) {
#ifdef CMD_MUSIC
            case MUS_CMD:
                MusicCMD_FreeSong(music->data.cmd);
                break;
#endif
#ifdef WAV_MUSIC
            case MUS_WAV:
                WAVStream_FreeSong(music->data.wave);
                break;
#endif
#ifdef MODPLUG_MUSIC
            case MUS_MODPLUG:
                modplug_delete(music->data.modplug);
                break;
#endif
#ifdef MOD_MUSIC
            case MUS_MOD:
                MOD_delete(music->data.module);
                break;
#endif
#ifdef MID_MUSIC
            case MUS_MID:
#ifdef USE_NATIVE_MIDI
                if ( native_midi_ok ) {
                    native_midi_freesong(music->data.nativemidi);
                    goto skip;
                }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
                if ( fluidsynth_ok ) {
                    fluidsynth_freesong(music->data.fluidsynthmidi);
                    goto skip;
                }
#endif
#ifdef USE_TIMIDITY_MIDI
                if ( timidity_ok ) {
                    Timidity_FreeSong(music->data.midi);
                    goto skip;
                }
#endif
                break;
#endif
#ifdef OGG_MUSIC
            case MUS_OGG:
                OGG_delete(music->data.ogg);
                break;
#endif
#ifdef FLAC_MUSIC
            case MUS_FLAC:
                FLAC_delete(music->data.flac);
                break;
#endif
#ifdef MP3_MUSIC
            case MUS_MP3:
                smpeg.SMPEG_delete(music->data.mp3);
                break;
#endif
#ifdef MP3_MAD_MUSIC
            case MUS_MP3_MAD:
                mad_closeFile(music->data.mp3_mad);
                break;
#endif
            default:
                /* Unknown music type?? */
                break;
        }

    skip:
        SDL_free(music);
    }
}

/* Find out the music format of a mixer music, or the currently playing
   music, if 'music' is NULL.
*/
Mix_MusicType Mix_GetMusicType(const Mix_Music *music)
{
    Mix_MusicType type = MUS_NONE;

    if ( music ) {
        type = music->type;
    } else {
        SDL_LockAudio();
        if ( music_playing ) {
            type = music_playing->type;
        }
        SDL_UnlockAudio();
    }
    return(type);
}

/* Play a music chunk.  Returns 0, or -1 if there was an error.
 */
static int music_internal_play(Mix_Music *music, double position)
{
    int retval = 0;

#if defined(__MACOSX__) && defined(USE_NATIVE_MIDI)
    /* This fixes a bug with native MIDI on Mac OS X, where you
       can't really stop and restart MIDI from the audio callback.
    */
    if ( music == music_playing && music->type == MUS_MID && native_midi_ok ) {
        /* Just a seek suffices to restart playing */
        music_internal_position(position);
        return 0;
    }
#endif

    /* Note the music we're playing */
    if ( music_playing ) {
        music_internal_halt();
    }
    music_playing = music;

    /* Set the initial volume */
    if ( music->type != MUS_MOD ) {
        music_internal_initialize_volume();
    }

    /* Set up for playback */
    switch (music->type) {
#ifdef CMD_MUSIC
        case MUS_CMD:
        MusicCMD_Start(music->data.cmd);
        break;
#endif
#ifdef WAV_MUSIC
        case MUS_WAV:
        WAVStream_Start(music->data.wave);
        break;
#endif
#ifdef MODPLUG_MUSIC
        case MUS_MODPLUG:
        /* can't set volume until file is loaded, so finally set it now */
        music_internal_initialize_volume();
        modplug_play(music->data.modplug);
        break;
#endif
#ifdef MOD_MUSIC
        case MUS_MOD:
        MOD_play(music->data.module);
        /* Player_SetVolume() does nothing before Player_Start() */
        music_internal_initialize_volume();
        break;
#endif
#ifdef MID_MUSIC
        case MUS_MID:
#ifdef USE_NATIVE_MIDI
        if ( native_midi_ok ) {
            native_midi_start(music->data.nativemidi, music_loops);
            goto skip;
        }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        if (fluidsynth_ok ) {
            fluidsynth_start(music->data.fluidsynthmidi);
            goto skip;
        }
#endif
#ifdef USE_TIMIDITY_MIDI
        if ( timidity_ok ) {
            Timidity_Start(music->data.midi);
            goto skip;
        }
#endif
        break;
#endif
#ifdef OGG_MUSIC
        case MUS_OGG:
        OGG_play(music->data.ogg);
        break;
#endif
#ifdef FLAC_MUSIC
        case MUS_FLAC:
        FLAC_play(music->data.flac);
        break;
#endif
#ifdef MP3_MUSIC
        case MUS_MP3:
        smpeg.SMPEG_enableaudio(music->data.mp3,1);
        smpeg.SMPEG_enablevideo(music->data.mp3,0);
        smpeg.SMPEG_play(music_playing->data.mp3);
        break;
#endif
#ifdef MP3_MAD_MUSIC
        case MUS_MP3_MAD:
        mad_start(music->data.mp3_mad);
        break;
#endif
        default:
        Mix_SetError("Can't play unknown music type");
        retval = -1;
        break;
    }

skip:
    /* Set the playback position, note any errors if an offset is used */
    if ( retval == 0 ) {
        if ( position > 0.0 ) {
            if ( music_internal_position(position) < 0 ) {
                Mix_SetError("Position not implemented for music type");
                retval = -1;
            }
        } else {
            music_internal_position(0.0);
        }
    }

    /* If the setup failed, we're not playing any music anymore */
    if ( retval < 0 ) {
        music_playing = NULL;
    }
    return(retval);
}
int Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position)
{
    int retval;

    if ( ms_per_step == 0 ) {
        SDL_SetError("Audio device hasn't been opened");
        return(-1);
    }

    /* Don't play null pointers :-) */
    if ( music == NULL ) {
        Mix_SetError("music parameter was NULL");
        return(-1);
    }

    /* Setup the data */
    if ( ms ) {
        music->fading = MIX_FADING_IN;
    } else {
        music->fading = MIX_NO_FADING;
    }
    music->fade_step = 0;
    music->fade_steps = ms/ms_per_step;

    /* Play the puppy */
    SDL_LockAudio();
    /* If the current music is fading out, wait for the fade to complete */
    while ( music_playing && (music_playing->fading == MIX_FADING_OUT) ) {
        SDL_UnlockAudio();
        SDL_Delay(100);
        SDL_LockAudio();
    }
    music_active = 1;
    if (loops == 1) {
        /* Loop is the number of times to play the audio */
        loops = 0;
    }
    music_loops = loops;
    retval = music_internal_play(music, position);
    SDL_UnlockAudio();

    return(retval);
}
int Mix_FadeInMusic(Mix_Music *music, int loops, int ms)
{
    return Mix_FadeInMusicPos(music, loops, ms, 0.0);
}
int Mix_PlayMusic(Mix_Music *music, int loops)
{
    return Mix_FadeInMusicPos(music, loops, 0, 0.0);
}

/* Set the playing music position */
int music_internal_position(double position)
{
    int retval = 0;

    switch (music_playing->type) {
#ifdef MODPLUG_MUSIC
        case MUS_MODPLUG:
        modplug_jump_to_time(music_playing->data.modplug, position);
        break;
#endif
#ifdef MOD_MUSIC
        case MUS_MOD:
        MOD_jump_to_time(music_playing->data.module, position);
        break;
#endif
#ifdef OGG_MUSIC
        case MUS_OGG:
        OGG_jump_to_time(music_playing->data.ogg, position);
        break;
#endif
#ifdef FLAC_MUSIC
        case MUS_FLAC:
        FLAC_jump_to_time(music_playing->data.flac, position);
        break;
#endif
#ifdef MP3_MUSIC
        case MUS_MP3:
        smpeg.SMPEG_rewind(music_playing->data.mp3);
        smpeg.SMPEG_play(music_playing->data.mp3);
        if ( position > 0.0 ) {
            smpeg.SMPEG_skip(music_playing->data.mp3, (float)position);
        }
        break;
#endif
#ifdef MP3_MAD_MUSIC
        case MUS_MP3_MAD:
        mad_seek(music_playing->data.mp3_mad, position);
        break;
#endif
        default:
        /* TODO: Implement this for other music backends */
        retval = -1;
        break;
    }
    return(retval);
}
int Mix_SetMusicPosition(double position)
{
    int retval;

    SDL_LockAudio();
    if ( music_playing ) {
        retval = music_internal_position(position);
        if ( retval < 0 ) {
            Mix_SetError("Position not implemented for music type");
        }
    } else {
        Mix_SetError("Music isn't playing");
        retval = -1;
    }
    SDL_UnlockAudio();

    return(retval);
}

/* Set the music's initial volume */
static void music_internal_initialize_volume(void)
{
    if ( music_playing->fading == MIX_FADING_IN ) {
        music_internal_volume(0);
    } else {
        music_internal_volume(music_volume);
    }
}

/* Set the music volume */
static void music_internal_volume(int volume)
{
    switch (music_playing->type) {
#ifdef CMD_MUSIC
        case MUS_CMD:
        MusicCMD_SetVolume(volume);
        break;
#endif
#ifdef WAV_MUSIC
        case MUS_WAV:
        WAVStream_SetVolume(volume);
        break;
#endif
#ifdef MODPLUG_MUSIC
        case MUS_MODPLUG:
        modplug_setvolume(music_playing->data.modplug, volume);
        break;
#endif
#ifdef MOD_MUSIC
        case MUS_MOD:
        MOD_setvolume(music_playing->data.module, volume);
        break;
#endif
#ifdef MID_MUSIC
        case MUS_MID:
#ifdef USE_NATIVE_MIDI
        if ( native_midi_ok ) {
            native_midi_setvolume(volume);
            return;
        }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        if ( fluidsynth_ok ) {
            fluidsynth_setvolume(music_playing->data.fluidsynthmidi, volume);
            return;
        }
#endif
#ifdef USE_TIMIDITY_MIDI
        if ( timidity_ok ) {
            Timidity_SetVolume(volume);
            return;
        }
#endif
        break;
#endif
#ifdef OGG_MUSIC
        case MUS_OGG:
        OGG_setvolume(music_playing->data.ogg, volume);
        break;
#endif
#ifdef FLAC_MUSIC
        case MUS_FLAC:
        FLAC_setvolume(music_playing->data.flac, volume);
        break;
#endif
#ifdef MP3_MUSIC
        case MUS_MP3:
        smpeg.SMPEG_setvolume(music_playing->data.mp3,(int)(((float)volume/(float)MIX_MAX_VOLUME)*100.0));
        break;
#endif
#ifdef MP3_MAD_MUSIC
        case MUS_MP3_MAD:
        mad_setVolume(music_playing->data.mp3_mad, volume);
        break;
#endif
        default:
        /* Unknown music type?? */
        break;
    }
}
int Mix_VolumeMusic(int volume)
{
    int prev_volume;

    prev_volume = music_volume;
    if ( volume < 0 ) {
        return prev_volume;
    }
    if ( volume > SDL_MIX_MAXVOLUME ) {
        volume = SDL_MIX_MAXVOLUME;
    }
    music_volume = volume;
    SDL_LockAudio();
    if ( music_playing ) {
        music_internal_volume(music_volume);
    }
    SDL_UnlockAudio();
    return(prev_volume);
}

/* Halt playing of music */
static void music_internal_halt(void)
{
    switch (music_playing->type) {
#ifdef CMD_MUSIC
        case MUS_CMD:
        MusicCMD_Stop(music_playing->data.cmd);
        break;
#endif
#ifdef WAV_MUSIC
        case MUS_WAV:
        WAVStream_Stop();
        break;
#endif
#ifdef MODPLUG_MUSIC
        case MUS_MODPLUG:
        modplug_stop(music_playing->data.modplug);
        break;
#endif
#ifdef MOD_MUSIC
        case MUS_MOD:
        MOD_stop(music_playing->data.module);
        break;
#endif
#ifdef MID_MUSIC
        case MUS_MID:
#ifdef USE_NATIVE_MIDI
        if ( native_midi_ok ) {
            native_midi_stop();
            goto skip;
        }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        if ( fluidsynth_ok ) {
            fluidsynth_stop(music_playing->data.fluidsynthmidi);
            goto skip;
        }
#endif
#ifdef USE_TIMIDITY_MIDI
        if ( timidity_ok ) {
            Timidity_Stop();
            goto skip;
        }
#endif
        break;
#endif
#ifdef OGG_MUSIC
        case MUS_OGG:
        OGG_stop(music_playing->data.ogg);
        break;
#endif
#ifdef FLAC_MUSIC
        case MUS_FLAC:
        FLAC_stop(music_playing->data.flac);
        break;
#endif
#ifdef MP3_MUSIC
        case MUS_MP3:
        smpeg.SMPEG_stop(music_playing->data.mp3);
        break;
#endif
#ifdef MP3_MAD_MUSIC
        case MUS_MP3_MAD:
        mad_stop(music_playing->data.mp3_mad);
        break;
#endif
        default:
        /* Unknown music type?? */
        return;
    }

skip:
    music_playing->fading = MIX_NO_FADING;
    music_playing = NULL;
}
int Mix_HaltMusic(void)
{
    SDL_LockAudio();
    if ( music_playing ) {
        music_internal_halt();
        if ( music_finished_hook ) {
            music_finished_hook();
        }
    }
    SDL_UnlockAudio();

    return(0);
}

/* Progressively stop the music */
int Mix_FadeOutMusic(int ms)
{
    int retval = 0;

    if ( ms_per_step == 0 ) {
        SDL_SetError("Audio device hasn't been opened");
        return 0;
    }

    if (ms <= 0) {  /* just halt immediately. */
        Mix_HaltMusic();
        return 1;
    }

    SDL_LockAudio();
    if ( music_playing) {
                int fade_steps = (ms + ms_per_step - 1)/ms_per_step;
                if ( music_playing->fading == MIX_NO_FADING ) {
                music_playing->fade_step = 0;
                } else {
                        int step;
                        int old_fade_steps = music_playing->fade_steps;
                        if ( music_playing->fading == MIX_FADING_OUT ) {
                                step = music_playing->fade_step;
                        } else {
                                step = old_fade_steps
                                        - music_playing->fade_step + 1;
                        }
                        music_playing->fade_step = (step * fade_steps)
                                / old_fade_steps;
                }
        music_playing->fading = MIX_FADING_OUT;
        music_playing->fade_steps = fade_steps;
        retval = 1;
    }
    SDL_UnlockAudio();

    return(retval);
}

Mix_Fading Mix_FadingMusic(void)
{
    Mix_Fading fading = MIX_NO_FADING;

    SDL_LockAudio();
    if ( music_playing ) {
        fading = music_playing->fading;
    }
    SDL_UnlockAudio();

    return(fading);
}

/* Pause/Resume the music stream */
void Mix_PauseMusic(void)
{
    music_active = 0;
}

void Mix_ResumeMusic(void)
{
    music_active = 1;
}

void Mix_RewindMusic(void)
{
    Mix_SetMusicPosition(0.0);
}

int Mix_PausedMusic(void)
{
    return (music_active == 0);
}

/* Check the status of the music */
static int music_internal_playing()
{
    int playing = 1;

    if (music_playing == NULL) {
        return 0;
    }

    switch (music_playing->type) {
#ifdef CMD_MUSIC
        case MUS_CMD:
        if (!MusicCMD_Active(music_playing->data.cmd)) {
            playing = 0;
        }
        break;
#endif
#ifdef WAV_MUSIC
        case MUS_WAV:
        if ( ! WAVStream_Active() ) {
            playing = 0;
        }
        break;
#endif
#ifdef MODPLUG_MUSIC
        case MUS_MODPLUG:
        if ( ! modplug_playing(music_playing->data.modplug) ) {
            playing = 0;
        }
        break;
#endif
#ifdef MOD_MUSIC
        case MUS_MOD:
        if ( ! MOD_playing(music_playing->data.module) ) {
            playing = 0;
        }
        break;
#endif
#ifdef MID_MUSIC
        case MUS_MID:
#ifdef USE_NATIVE_MIDI
        if ( native_midi_ok ) {
            if ( ! native_midi_active() )
                playing = 0;
            goto skip;
        }
#endif
#ifdef USE_FLUIDSYNTH_MIDI
        if ( fluidsynth_ok ) {
            if ( ! fluidsynth_active(music_playing->data.fluidsynthmidi) )
                playing = 0;
            goto skip;
        }
#endif
#ifdef USE_TIMIDITY_MIDI
        if ( timidity_ok ) {
            if ( ! Timidity_Active() )
                playing = 0;
            goto skip;
        }
#endif
        break;
#endif
#ifdef OGG_MUSIC
        case MUS_OGG:
        if ( ! OGG_playing(music_playing->data.ogg) ) {
            playing = 0;
        }
        break;
#endif
#ifdef FLAC_MUSIC
        case MUS_FLAC:
        if ( ! FLAC_playing(music_playing->data.flac) ) {
            playing = 0;
        }
        break;
#endif
#ifdef MP3_MUSIC
        case MUS_MP3:
        if ( smpeg.SMPEG_status(music_playing->data.mp3) != SMPEG_PLAYING )
            playing = 0;
        break;
#endif
#ifdef MP3_MAD_MUSIC
        case MUS_MP3_MAD:
        if (!mad_isPlaying(music_playing->data.mp3_mad)) {
            playing = 0;
        }
        break;
#endif
        default:
        playing = 0;
        break;
    }

skip:
    return(playing);
}
int Mix_PlayingMusic(void)
{
    int playing = 0;

    SDL_LockAudio();
    if ( music_playing ) {
        playing = music_loops || music_internal_playing();
    }
    SDL_UnlockAudio();

    return(playing);
}

/* Set the external music playback command */
int Mix_SetMusicCMD(const char *command)
{
    Mix_HaltMusic();
    if ( music_cmd ) {
        SDL_free(music_cmd);
        music_cmd = NULL;
    }
    if ( command ) {
        music_cmd = (char *)SDL_malloc(strlen(command)+1);
        if ( music_cmd == NULL ) {
            return(-1);
        }
        strcpy(music_cmd, command);
    }
    return(0);
}

int Mix_SetSynchroValue(int i)
{
    /* Not supported by any players at this time */
    return(-1);
}

int Mix_GetSynchroValue(void)
{
    /* Not supported by any players at this time */
    return(-1);
}


/* Uninitialize the music players */
void close_music(void)
{
    Mix_HaltMusic();
#ifdef CMD_MUSIC
    Mix_SetMusicCMD(NULL);
#endif
#ifdef MODPLUG_MUSIC
    modplug_exit();
#endif
#ifdef MOD_MUSIC
    MOD_exit();
#endif
#ifdef MID_MUSIC
# ifdef USE_TIMIDITY_MIDI
    Timidity_Close();
# endif
#endif

    /* rcg06042009 report available decoders at runtime. */
    SDL_free((void *)music_decoders);
    music_decoders = NULL;
    num_decoders = 0;

    ms_per_step = 0;
}

int Mix_SetSoundFonts(const char *paths)
{
#ifdef MID_MUSIC
    if (soundfont_paths) {
        SDL_free(soundfont_paths);
        soundfont_paths = NULL;
    }

    if (paths) {
        if (!(soundfont_paths = SDL_strdup(paths))) {
            Mix_SetError("Insufficient memory to set SoundFonts");
            return 0;
        }
    }
#endif
    return 1;
}

#ifdef MID_MUSIC
const char* Mix_GetSoundFonts(void)
{
    const char* force = getenv("SDL_FORCE_SOUNDFONTS");

    if (!soundfont_paths || (force && force[0] == '1')) {
        return getenv("SDL_SOUNDFONTS");
    } else {
        return soundfont_paths;
    }
}

int Mix_EachSoundFont(int (*function)(const char*, void*), void *data)
{
    char *context, *path, *paths;
    const char* cpaths = Mix_GetSoundFonts();

    if (!cpaths) {
        Mix_SetError("No SoundFonts have been requested");
        return 0;
    }

    if (!(paths = SDL_strdup(cpaths))) {
        Mix_SetError("Insufficient memory to iterate over SoundFonts");
        return 0;
    }

#if defined(__MINGW32__) || defined(__MINGW64__)
    for (path = strtok(paths, ";"); path; path = strtok(NULL, ";")) {
#elif defined(_WIN32)
    for (path = strtok_s(paths, ";", &context); path; path = strtok_s(NULL, ";", &context)) {
#else
    for (path = strtok_r(paths, ":;", &context); path; path = strtok_r(NULL, ":;", &context)) {
#endif
        if (!function(path, data)) {
            SDL_free(paths);
            return 0;
        }
    }

    SDL_free(paths);
    return 1;
}
#endif
