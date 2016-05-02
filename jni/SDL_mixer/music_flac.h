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

  Header to handle loading FLAC music files in SDL.
    ~ Austen Dicken (admin@cvpcs.org)
*/

/* $Id:  $ */

#ifdef FLAC_MUSIC

#include <FLAC/stream_decoder.h>

typedef struct {
    FLAC__uint64 sample_size;
    unsigned sample_rate;
    unsigned channels;
    unsigned bits_per_sample;
    FLAC__uint64 total_samples;

    // the following are used to handle the callback nature of the writer
    int max_to_read;
    char *data;             // pointer to beginning of data array
    int data_len;           // size of data array
    int data_read;          // amount of data array used
    char *overflow;         // pointer to beginning of overflow array
    int overflow_len;       // size of overflow array
    int overflow_read;      // amount of overflow array used
} FLAC_Data;

typedef struct {
    int playing;
    int volume;
    int section;
    FLAC__StreamDecoder *flac_decoder;
    FLAC_Data flac_data;
    SDL_RWops *src;
    int freesrc;
    SDL_AudioCVT cvt;
    int len_available;
    Uint8 *snd_available;
} FLAC_music;

/* Initialize the FLAC player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
extern int FLAC_init(SDL_AudioSpec *mixer);

/* Set the volume for a FLAC stream */
extern void FLAC_setvolume(FLAC_music *music, int volume);

/* Load an FLAC stream from an SDL_RWops object */
extern FLAC_music *FLAC_new_RW(SDL_RWops *src, int freesrc);

/* Start playback of a given FLAC stream */
extern void FLAC_play(FLAC_music *music);

/* Return non-zero if a stream is currently playing */
extern int FLAC_playing(FLAC_music *music);

/* Play some of a stream previously started with FLAC_play() */
extern int FLAC_playAudio(FLAC_music *music, Uint8 *stream, int len);

/* Stop playback of a stream previously started with FLAC_play() */
extern void FLAC_stop(FLAC_music *music);

/* Close the given FLAC stream */
extern void FLAC_delete(FLAC_music *music);

/* Jump (seek) to a given position (time is in seconds) */
extern void FLAC_jump_to_time(FLAC_music *music, double time);

#endif /* FLAC_MUSIC */
