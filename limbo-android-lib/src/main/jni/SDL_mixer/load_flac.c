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

  This is the source needed to decode a FLAC into a waveform.
    ~ Austen Dicken (admin@cvpcs.org).
*/

#ifdef FLAC_MUSIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_mutex.h"
#include "SDL_endian.h"
#include "SDL_timer.h"

#include "SDL_mixer.h"
#include "dynamic_flac.h"
#include "load_flac.h"

#include <FLAC/stream_decoder.h>

typedef struct {
    SDL_RWops* sdl_src;
    SDL_AudioSpec* sdl_spec;
    Uint8** sdl_audio_buf;
    Uint32* sdl_audio_len;
    int sdl_audio_read;
    FLAC__uint64 flac_total_samples;
    unsigned flac_bps;
} FLAC_SDL_Data;

static FLAC__StreamDecoderReadStatus flac_read_load_cb(
                                    const FLAC__StreamDecoder *decoder,
                                    FLAC__byte buffer[],
                                    size_t *bytes,
                                    void *client_data)
{
    // make sure there is something to be reading
    if (*bytes > 0) {
        FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

        *bytes = SDL_RWread (data->sdl_src, buffer, sizeof (FLAC__byte),
                                *bytes);

        if (*bytes == 0) { // error or no data was read (EOF)
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        } else { // data was read, continue
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
    } else {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
}

static FLAC__StreamDecoderSeekStatus flac_seek_load_cb(
                                    const FLAC__StreamDecoder *decoder,
                                    FLAC__uint64 absolute_byte_offset,
                                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    if (SDL_RWseek (data->sdl_src, absolute_byte_offset, RW_SEEK_SET) < 0) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    } else {
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
    }
}

static FLAC__StreamDecoderTellStatus flac_tell_load_cb(
                                    const FLAC__StreamDecoder *decoder,
                                    FLAC__uint64 *absolute_byte_offset,
                                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    Sint64 pos = SDL_RWtell (data->sdl_src);

    if (pos < 0) {
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    } else {
        *absolute_byte_offset = (FLAC__uint64)pos;
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }
}

static FLAC__StreamDecoderLengthStatus flac_length_load_cb(
                                    const FLAC__StreamDecoder *decoder,
                                    FLAC__uint64 *stream_length,
                                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    Sint64 pos = SDL_RWtell (data->sdl_src);
    Sint64 length = SDL_RWseek (data->sdl_src, 0, RW_SEEK_END);

    if (SDL_RWseek (data->sdl_src, pos, RW_SEEK_SET) != pos || length < 0) {
        /* there was an error attempting to return the stream to the original
         * position, or the length was invalid. */
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    } else {
        *stream_length = (FLAC__uint64)length;
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }
}

static FLAC__bool flac_eof_load_cb(const FLAC__StreamDecoder *decoder,
                                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    Sint64 pos = SDL_RWtell (data->sdl_src);
    Sint64 end = SDL_RWseek (data->sdl_src, 0, RW_SEEK_END);

    // was the original position equal to the end (a.k.a. the seek didn't move)?
    if (pos == end) {
        // must be EOF
        return true;
    } else {
        // not EOF, return to the original position
        SDL_RWseek (data->sdl_src, pos, RW_SEEK_SET);
        return false;
    }
}

static FLAC__StreamDecoderWriteStatus flac_write_load_cb(
                                    const FLAC__StreamDecoder *decoder,
                                    const FLAC__Frame *frame,
                                    const FLAC__int32 *const buffer[],
                                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;
    size_t i;
    Uint8 *buf;

    if (data->flac_total_samples == 0) {
        SDL_SetError ("Given FLAC file does not specify its sample count.");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    if (data->sdl_spec->channels != 2 || data->flac_bps != 16) {
        SDL_SetError ("Current FLAC support is only for 16 bit Stereo files.");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    // check if it is the first audio frame so we can initialize the output
    // buffer
    if (frame->header.number.sample_number == 0) {
        *(data->sdl_audio_len) = data->sdl_spec->size;
        data->sdl_audio_read = 0;
        *(data->sdl_audio_buf) = SDL_malloc (*(data->sdl_audio_len));

        if (*(data->sdl_audio_buf) == NULL) {
            SDL_SetError
                    ("Unable to allocate memory to store the FLAC stream.");
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    buf = *(data->sdl_audio_buf);

    for (i = 0; i < frame->header.blocksize; i++) {
        FLAC__int16 i16;
        FLAC__uint16 ui16;

        i16 = (FLAC__int16)buffer[0][i];
        ui16 = (FLAC__uint16)i16;

        *(buf + (data->sdl_audio_read++)) = (char)(ui16);
        *(buf + (data->sdl_audio_read++)) = (char)(ui16 >> 8);

        i16 = (FLAC__int16)buffer[1][i];
        ui16 = (FLAC__uint16)i16;

        *(buf + (data->sdl_audio_read++)) = (char)(ui16);
        *(buf + (data->sdl_audio_read++)) = (char)(ui16 >> 8);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_metadata_load_cb(
                    const FLAC__StreamDecoder *decoder,
                    const FLAC__StreamMetadata *metadata,
                    void *client_data)
{
    FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;
    FLAC__uint64 total_samples;
    unsigned bps;

    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        // save the metadata right now for use later on
        *(data->sdl_audio_buf) = NULL;
        *(data->sdl_audio_len) = 0;
        memset (data->sdl_spec, '\0', sizeof (SDL_AudioSpec));

        data->sdl_spec->format = AUDIO_S16;
        data->sdl_spec->freq = (int)(metadata->data.stream_info.sample_rate);
        data->sdl_spec->channels = (Uint8)(metadata->data.stream_info.channels);
        data->sdl_spec->samples = 8192; /* buffer size */

        total_samples = metadata->data.stream_info.total_samples;
        bps = metadata->data.stream_info.bits_per_sample;

        data->sdl_spec->size = (Uint32)(total_samples * data->sdl_spec->channels * (bps / 8));
        data->flac_total_samples = total_samples;
        data->flac_bps = bps;
    }
}

static void flac_error_load_cb(
                const FLAC__StreamDecoder *decoder,
                FLAC__StreamDecoderErrorStatus status,
                void *client_data)
{
    // print an SDL error based on the error status
    switch (status) {
        case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
            SDL_SetError ("Error processing the FLAC file [LOST_SYNC].");
        break;
        case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
            SDL_SetError ("Error processing the FLAC file [BAD_HEADER].");
        break;
        case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
            SDL_SetError ("Error processing the FLAC file [CRC_MISMATCH].");
        break;
        case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
            SDL_SetError ("Error processing the FLAC file [UNPARSEABLE].");
        break;
        default:
            SDL_SetError ("Error processing the FLAC file [UNKNOWN].");
        break;
    }
}

/* don't call this directly; use Mix_LoadWAV_RW() for now. */
SDL_AudioSpec *Mix_LoadFLAC_RW (SDL_RWops *src, int freesrc,
        SDL_AudioSpec *spec, Uint8 **audio_buf, Uint32 *audio_len)
{
    FLAC__StreamDecoder *decoder = 0;
    FLAC__StreamDecoderInitStatus init_status;
    int was_error = 1;
    int was_init = 0;
    Uint32 samplesize;

    // create the client data passing information
    FLAC_SDL_Data* client_data;
    client_data = (FLAC_SDL_Data *)SDL_malloc (sizeof (FLAC_SDL_Data));

    if ((!src) || (!audio_buf) || (!audio_len))   /* sanity checks. */
        goto done;

    if (!Mix_Init(MIX_INIT_FLAC))
        goto done;

    if ((decoder = flac.FLAC__stream_decoder_new ()) == NULL) {
        SDL_SetError ("Unable to allocate FLAC decoder.");
        goto done;
    }

    init_status = flac.FLAC__stream_decoder_init_stream (decoder,
                                flac_read_load_cb, flac_seek_load_cb,
                                flac_tell_load_cb, flac_length_load_cb,
                                flac_eof_load_cb, flac_write_load_cb,
                                flac_metadata_load_cb, flac_error_load_cb,
                                client_data);

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        SDL_SetError ("Unable to initialize FLAC stream decoder.");
        goto done;
    }

    was_init = 1;

    client_data->sdl_src = src;
    client_data->sdl_spec = spec;
    client_data->sdl_audio_buf = audio_buf;
    client_data->sdl_audio_len = audio_len;

    if (!flac.FLAC__stream_decoder_process_until_end_of_stream (decoder)) {
        SDL_SetError ("Unable to process FLAC file.");
        goto done;
    }

    was_error = 0;

    /* Don't return a buffer that isn't a multiple of samplesize */
    samplesize = ((spec->format & 0xFF) / 8) * spec->channels;
    *audio_len &= ~(samplesize - 1);

done:
    if (was_init && decoder) {
        flac.FLAC__stream_decoder_finish (decoder);
    }

    if (decoder) {
        flac.FLAC__stream_decoder_delete (decoder);
    }

    if (freesrc && src) {
        SDL_RWclose (src);
    }

    if (was_error) {
        spec = NULL;
    }

    return spec;
}

#endif // FLAC_MUSIC
