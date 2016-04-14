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

#include <stdlib.h>
#include <string.h>

#include "SDL_audio.h"
#include "SDL_mutex.h"
#include "SDL_rwops.h"
#include "SDL_endian.h"

#include "SDL_mixer.h"
#include "wavestream.h"

/*
    Taken with permission from SDL_wave.h, part of the SDL library,
    available at: http://www.libsdl.org/
    and placed under the same license as this mixer library.
*/

/* WAVE files are little-endian */

/*******************************************/
/* Define values for Microsoft WAVE format */
/*******************************************/
#define RIFF        0x46464952      /* "RIFF" */
#define WAVE        0x45564157      /* "WAVE" */
#define FACT        0x74636166      /* "fact" */
#define LIST        0x5453494c      /* "LIST" */
#define FMT     0x20746D66      /* "fmt " */
#define DATA        0x61746164      /* "data" */
#define PCM_CODE    1
#define ADPCM_CODE  2
#define WAVE_MONO   1
#define WAVE_STEREO 2

/* Normally, these three chunks come consecutively in a WAVE file */
typedef struct WaveFMT {
/* Not saved in the chunk we read:
    Uint32  FMTchunk;
    Uint32  fmtlen;
*/
    Uint16  encoding;
    Uint16  channels;       /* 1 = mono, 2 = stereo */
    Uint32  frequency;      /* One of 11025, 22050, or 44100 Hz */
    Uint32  byterate;       /* Average bytes per second */
    Uint16  blockalign;     /* Bytes per sample block */
    Uint16  bitspersample;      /* One of 8, 12, 16, or 4 for ADPCM */
} WaveFMT;

/* The general chunk found in the WAVE file */
typedef struct Chunk {
    Uint32 magic;
    Uint32 length;
    Uint8 *data;            /* Data includes magic and length */
} Chunk;

/*********************************************/
/* Define values for AIFF (IFF audio) format */
/*********************************************/
#define FORM        0x4d524f46      /* "FORM" */
#define AIFF        0x46464941      /* "AIFF" */
#define SSND        0x444e5353      /* "SSND" */
#define COMM        0x4d4d4f43      /* "COMM" */


/* Currently we only support a single stream at a time */
static WAVStream *music = NULL;

/* This is the format of the audio mixer data */
static SDL_AudioSpec mixer;
static int wavestream_volume = MIX_MAX_VOLUME;

/* Function to load the WAV/AIFF stream */
static SDL_RWops *LoadWAVStream (SDL_RWops *rw, SDL_AudioSpec *spec,
                    long *start, long *stop);
static SDL_RWops *LoadAIFFStream (SDL_RWops *rw, SDL_AudioSpec *spec,
                    long *start, long *stop);

/* Initialize the WAVStream player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
int WAVStream_Init(SDL_AudioSpec *mixerfmt)
{
    mixer = *mixerfmt;
    return(0);
}

void WAVStream_SetVolume(int volume)
{
    wavestream_volume = volume;
}

/* Load a WAV stream from the given RWops object */
WAVStream *WAVStream_LoadSong_RW(SDL_RWops *src, int freesrc)
{
    WAVStream *wave;
    SDL_AudioSpec wavespec;

    if ( ! mixer.format ) {
        Mix_SetError("WAV music output not started");
        return(NULL);
    }
    wave = (WAVStream *)SDL_malloc(sizeof *wave);
    if ( wave ) {
        Uint32 magic;

        SDL_zerop(wave);
        wave->freesrc = freesrc;

        magic = SDL_ReadLE32(src);
        if ( magic == RIFF || magic == WAVE ) {
            wave->src = LoadWAVStream(src, &wavespec, &wave->start, &wave->stop);
        } else if ( magic == FORM ) {
            wave->src = LoadAIFFStream(src, &wavespec, &wave->start, &wave->stop);
        } else {
            Mix_SetError("Unknown WAVE format");
        }
        if ( wave->src == NULL ) {
            SDL_free(wave);
            return(NULL);
        }
        SDL_BuildAudioCVT(&wave->cvt,
            wavespec.format, wavespec.channels, wavespec.freq,
            mixer.format, mixer.channels, mixer.freq);
    } else {
        SDL_OutOfMemory();
        return(NULL);
    }
    return(wave);
}

/* Start playback of a given WAV stream */
void WAVStream_Start(WAVStream *wave)
{
    SDL_RWseek (wave->src, wave->start, RW_SEEK_SET);
    music = wave;
}

/* Play some of a stream previously started with WAVStream_Start() */
int WAVStream_PlaySome(Uint8 *stream, int len)
{
    Sint64 pos;
    Sint64 left = 0;

    if ( music && ((pos=SDL_RWtell(music->src)) < music->stop) ) {
        if ( music->cvt.needed ) {
            int original_len;

            original_len=(int)((double)len/music->cvt.len_ratio);
            if ( music->cvt.len != original_len ) {
                int worksize;
                if ( music->cvt.buf != NULL ) {
                    SDL_free(music->cvt.buf);
                }
                worksize = original_len*music->cvt.len_mult;
                music->cvt.buf=(Uint8 *)SDL_malloc(worksize);
                if ( music->cvt.buf == NULL ) {
                    return 0;
                }
                music->cvt.len = original_len;
            }
            if ( (music->stop - pos) < original_len ) {
                left = (original_len - (music->stop - pos));
                original_len -= (int)left;
                left = (int)((double)left*music->cvt.len_ratio);
            }
            original_len = SDL_RWread(music->src, music->cvt.buf,1,original_len);
            /* At least at the time of writing, SDL_ConvertAudio()
               does byte-order swapping starting at the end of the
               buffer. Thus, if we are reading 16-bit samples, we
               had better make damn sure that we get an even
               number of bytes, or we'll get garbage.
             */
            if ( (music->cvt.src_format & 0x0010) && (original_len & 1) ) {
                original_len--;
            }
            music->cvt.len = original_len;
            SDL_ConvertAudio(&music->cvt);
            SDL_MixAudio(stream, music->cvt.buf, music->cvt.len_cvt, wavestream_volume);
        } else {
            Uint8 *data;
            if ( (music->stop - pos) < len ) {
                left = (len - (music->stop - pos));
                len -= (int)left;
            }
            data = SDL_stack_alloc(Uint8, len);
            if (data)
            {
                SDL_RWread(music->src, data, len, 1);
                SDL_MixAudio(stream, data, len, wavestream_volume);
                SDL_stack_free(data);
            }
        }
    }
    return (int)left;
}

/* Stop playback of a stream previously started with WAVStream_Start() */
void WAVStream_Stop(void)
{
    music = NULL;
}

/* Close the given WAV stream */
void WAVStream_FreeSong(WAVStream *wave)
{
    if ( wave ) {
        /* Clean up associated data */
        if ( wave->cvt.buf ) {
            SDL_free(wave->cvt.buf);
        }
        if ( wave->freesrc ) {
            SDL_RWclose(wave->src);
        }
        SDL_free(wave);
    }
}

/* Return non-zero if a stream is currently playing */
int WAVStream_Active(void)
{
    int active;

    active = 0;
    if ( music && (SDL_RWtell(music->src) < music->stop) ) {
        active = 1;
    }
    return(active);
}

static int ReadChunk(SDL_RWops *src, Chunk *chunk, int read_data)
{
    chunk->magic    = SDL_ReadLE32(src);
    chunk->length   = SDL_ReadLE32(src);
    if ( read_data ) {
        chunk->data = (Uint8 *)SDL_malloc(chunk->length);
        if ( chunk->data == NULL ) {
            Mix_SetError("Out of memory");
            return(-1);
        }
        if ( SDL_RWread(src, chunk->data, chunk->length, 1) != 1 ) {
            Mix_SetError("Couldn't read chunk");
            SDL_free(chunk->data);
            return(-1);
        }
    } else {
        SDL_RWseek(src, chunk->length, RW_SEEK_CUR);
    }
    return(chunk->length);
}

static SDL_RWops *LoadWAVStream (SDL_RWops *src, SDL_AudioSpec *spec,
                    long *start, long *stop)
{
    int was_error;
    Chunk chunk;
    int lenread;

    /* WAV magic header */
    Uint32 wavelen;
    Uint32 WAVEmagic;

    /* FMT chunk */
    WaveFMT *format = NULL;

    was_error = 0;

    /* Check the magic header */
    wavelen     = SDL_ReadLE32(src);
    WAVEmagic   = SDL_ReadLE32(src);

    /* Read the audio data format chunk */
    chunk.data = NULL;
    do {
        /* FIXME! Add this logic to SDL_LoadWAV_RW() */
        if ( chunk.data ) {
            SDL_free(chunk.data);
        }
        lenread = ReadChunk(src, &chunk, 1);
        if ( lenread < 0 ) {
            was_error = 1;
            goto done;
        }
    } while ( (chunk.magic == FACT) || (chunk.magic == LIST) );

    /* Decode the audio data format */
    format = (WaveFMT *)chunk.data;
    if ( chunk.magic != FMT ) {
        SDL_free(chunk.data);
        Mix_SetError("Complex WAVE files not supported");
        was_error = 1;
        goto done;
    }
    switch (SDL_SwapLE16(format->encoding)) {
        case PCM_CODE:
            /* We can understand this */
            break;
        default:
            Mix_SetError("Unknown WAVE data format");
            was_error = 1;
            goto done;
    }
    SDL_memset(spec, 0, (sizeof *spec));
    spec->freq = SDL_SwapLE32(format->frequency);
    switch (SDL_SwapLE16(format->bitspersample)) {
        case 8:
            spec->format = AUDIO_U8;
            break;
        case 16:
            spec->format = AUDIO_S16;
            break;
        default:
            Mix_SetError("Unknown PCM data format");
            was_error = 1;
            goto done;
    }
    spec->channels = (Uint8) SDL_SwapLE16(format->channels);
    spec->samples = 4096;       /* Good default buffer size */

    /* Set the file offset to the DATA chunk data */
    chunk.data = NULL;
    do {
        *start = (long)SDL_RWtell(src) + 2*sizeof(Uint32);
        lenread = ReadChunk(src, &chunk, 0);
        if ( lenread < 0 ) {
            was_error = 1;
            goto done;
        }
    } while ( chunk.magic != DATA );
    *stop = (long)SDL_RWtell(src);

done:
    if ( format != NULL ) {
        SDL_free(format);
    }
    if ( was_error ) {
        return NULL;
    }
    return(src);
}

/* I couldn't get SANE_to_double() to work, so I stole this from libsndfile.
 * I don't pretend to fully understand it.
 */

static Uint32 SANE_to_Uint32 (Uint8 *sanebuf)
{
    /* Negative number? */
    if (sanebuf[0] & 0x80)
        return 0;

    /* Less than 1? */
    if (sanebuf[0] <= 0x3F)
        return 1;

    /* Way too big? */
    if (sanebuf[0] > 0x40)
        return 0x4000000;

    /* Still too big? */
    if (sanebuf[0] == 0x40 && sanebuf[1] > 0x1C)
        return 800000000;

    return ((sanebuf[2] << 23) | (sanebuf[3] << 15) | (sanebuf[4] << 7)
        | (sanebuf[5] >> 1)) >> (29 - sanebuf[1]);
}

static SDL_RWops *LoadAIFFStream (SDL_RWops *src, SDL_AudioSpec *spec,
                    long *start, long *stop)
{
    int was_error;
    int found_SSND;
    int found_COMM;

    Uint32 chunk_type;
    Uint32 chunk_length;
    Sint64 next_chunk;

    /* AIFF magic header */
    Uint32 AIFFmagic;
    /* SSND chunk        */
    Uint32 offset;
    Uint32 blocksize;
    /* COMM format chunk */
    Uint16 channels = 0;
    Uint32 numsamples = 0;
    Uint16 samplesize = 0;
    Uint8 sane_freq[10];
    Uint32 frequency = 0;

    was_error = 0;

    /* Check the magic header */
    chunk_length    = SDL_ReadBE32(src);
    AIFFmagic   = SDL_ReadLE32(src);
    if ( AIFFmagic != AIFF ) {
        Mix_SetError("Unrecognized file type (not AIFF)");
        was_error = 1;
        goto done;
    }

    /* From what I understand of the specification, chunks may appear in
         * any order, and we should just ignore unknown ones.
     *
     * TODO: Better sanity-checking. E.g. what happens if the AIFF file
     *       contains compressed sound data?
     */

    found_SSND = 0;
    found_COMM = 0;

    do {
        chunk_type      = SDL_ReadLE32(src);
        chunk_length    = SDL_ReadBE32(src);
        next_chunk      = SDL_RWtell(src) + chunk_length;

        /* Paranoia to avoid infinite loops */
        if (chunk_length == 0)
        break;

        switch (chunk_type) {
        case SSND:
            found_SSND      = 1;
            offset      = SDL_ReadBE32(src);
            blocksize       = SDL_ReadBE32(src);
            *start      = (long)SDL_RWtell(src) + offset;
            break;

        case COMM:
            found_COMM      = 1;

            /* Read the audio data format chunk */
            channels        = SDL_ReadBE16(src);
            numsamples      = SDL_ReadBE32(src);
            samplesize      = SDL_ReadBE16(src);
            SDL_RWread(src, sane_freq, sizeof(sane_freq), 1);
            frequency       = SANE_to_Uint32(sane_freq);
            break;

        default:
            break;
        }
    } while ((!found_SSND || !found_COMM)
         && SDL_RWseek(src, next_chunk, RW_SEEK_SET) != -1);

    if (!found_SSND) {
        Mix_SetError("Bad AIFF file (no SSND chunk)");
        was_error = 1;
        goto done;
    }

    if (!found_COMM) {
        Mix_SetError("Bad AIFF file (no COMM chunk)");
        was_error = 1;
        goto done;
    }

    *stop = *start + channels * numsamples * (samplesize / 8);

    /* Decode the audio data format */
    SDL_memset(spec, 0, (sizeof *spec));
    spec->freq = frequency;
    switch (samplesize) {
        case 8:
            spec->format = AUDIO_S8;
            break;
        case 16:
            spec->format = AUDIO_S16MSB;
            break;
        default:
            Mix_SetError("Unknown samplesize in data format");
            was_error = 1;
            goto done;
    }
    spec->channels = (Uint8) channels;
    spec->samples = 4096;       /* Good default buffer size */

done:
    if ( was_error ) {
        return NULL;
    }
    return(src);
}

