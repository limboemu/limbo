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

  This is the source needed to decode an Ogg Vorbis into a waveform.
  This file by Vaclav Slavik (vaclav.slavik@matfyz.cz).
*/

/* $Id$ */

#ifdef OGG_MUSIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_mutex.h"
#include "SDL_endian.h"
#include "SDL_timer.h"

#include "SDL_mixer.h"
#include "dynamic_ogg.h"
#include "load_ogg.h"

static size_t sdl_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    return SDL_RWread((SDL_RWops*)datasource, ptr, size, nmemb);
}

static int sdl_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    return (int)SDL_RWseek((SDL_RWops*)datasource, offset, whence);
}

static int sdl_close_func_freesrc(void *datasource)
{
    return SDL_RWclose((SDL_RWops*)datasource);
}

static int sdl_close_func_nofreesrc(void *datasource)
{
    SDL_RWseek((SDL_RWops*)datasource, 0, RW_SEEK_SET);
    return 0;
}

static long sdl_tell_func(void *datasource)
{
    return (long)SDL_RWtell((SDL_RWops*)datasource);
}


/* don't call this directly; use Mix_LoadWAV_RW() for now. */
SDL_AudioSpec *Mix_LoadOGG_RW (SDL_RWops *src, int freesrc,
        SDL_AudioSpec *spec, Uint8 **audio_buf, Uint32 *audio_len)
{
    OggVorbis_File vf;
    ov_callbacks callbacks;
    vorbis_info *info;
    Uint8 *buf;
    int bitstream = -1;
    long samplesize;
    long samples;
    int read, to_read;
    int must_close = 1;
    int was_error = 1;

    if ( (!src) || (!audio_buf) || (!audio_len) )   /* sanity checks. */
        goto done;

    if ( !Mix_Init(MIX_INIT_OGG) )
        goto done;

    callbacks.read_func = sdl_read_func;
    callbacks.seek_func = sdl_seek_func;
    callbacks.tell_func = sdl_tell_func;
    callbacks.close_func = freesrc ?
                           sdl_close_func_freesrc : sdl_close_func_nofreesrc;

    if (vorbis.ov_open_callbacks(src, &vf, NULL, 0, callbacks) != 0)
    {
        SDL_SetError("OGG bitstream is not valid Vorbis stream!");
        goto done;
    }

    must_close = 0;

    info = vorbis.ov_info(&vf, -1);

    *audio_buf = NULL;
    *audio_len = 0;
    memset(spec, '\0', sizeof (SDL_AudioSpec));

    spec->format = AUDIO_S16;
    spec->channels = info->channels;
    spec->freq = info->rate;
    spec->samples = 4096; /* buffer size */

    samples = (long)vorbis.ov_pcm_total(&vf, -1);

    *audio_len = spec->size = samples * spec->channels * 2;
    *audio_buf = (Uint8 *)SDL_malloc(*audio_len);
    if (*audio_buf == NULL)
        goto done;

    buf = *audio_buf;
    to_read = *audio_len;
#ifdef OGG_USE_TREMOR
    for (read = vorbis.ov_read(&vf, (char *)buf, to_read, &bitstream);
     read > 0;
     read = vorbis.ov_read(&vf, (char *)buf, to_read, &bitstream))
#else
    for (read = vorbis.ov_read(&vf, (char *)buf, to_read, 0/*LE*/, 2/*16bit*/, 1/*signed*/, &bitstream);
         read > 0;
         read = vorbis.ov_read(&vf, (char *)buf, to_read, 0, 2, 1, &bitstream))
#endif
    {
        if (read == OV_HOLE || read == OV_EBADLINK)
            break; /* error */

        to_read -= read;
        buf += read;
    }

    vorbis.ov_clear(&vf);
    was_error = 0;

    /* Don't return a buffer that isn't a multiple of samplesize */
    samplesize = ((spec->format & 0xFF)/8)*spec->channels;
    *audio_len &= ~(samplesize-1);

done:
    if (freesrc && src && must_close) {
        SDL_RWclose(src);
    }

    if (was_error) {
        spec = NULL;
    }

    return(spec);
} /* Mix_LoadOGG_RW */

/* end of load_ogg.c ... */

#endif
