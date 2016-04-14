/*
  SDL_image:  An example image loading library for use with SDL
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

/* This is a WEBP image file loading framework */

#include <stdlib.h>
#include <stdio.h>

#include "SDL_image.h"

#ifdef LOAD_WEBP

/*=============================================================================
        File: SDL_webp.c
     Purpose: A WEBP loader for the SDL library
    Revision:
  Created by: Michael Bonfils (Murlock) (26 November 2011)
              murlock42@gmail.com

=============================================================================*/

#include "SDL_endian.h"

#ifdef macintosh
#define MACOS
#endif
#include <webp/decode.h>

static struct {
    int loaded;
    void *handle;
    VP8StatusCode (*webp_get_features_internal) (const uint8_t *data, size_t data_size, WebPBitstreamFeatures* features, int decoder_abi_version);
    uint8_t*    (*webp_decode_rgb_into) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride);
    uint8_t*    (*webp_decode_rgba_into) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride);
} lib;

#ifdef LOAD_WEBP_DYNAMIC
int IMG_InitWEBP()
{
    if ( lib.loaded == 0 ) {
        lib.handle = SDL_LoadObject(LOAD_WEBP_DYNAMIC);
        if ( lib.handle == NULL ) {
            return -1;
        }

        lib.webp_get_features_internal =
            ( VP8StatusCode (*) (const uint8_t *, size_t, WebPBitstreamFeatures*, int) )
            SDL_LoadFunction(lib.handle, "WebPGetFeaturesInternal" );
        if ( lib.webp_get_features_internal == NULL ) {
            SDL_UnloadObject(lib.handle);
            return -1;
        }

        lib.webp_decode_rgb_into =
            ( uint8_t* (*) (const uint8_t*, size_t, uint8_t*, size_t, int ) )
            SDL_LoadFunction(lib.handle, "WebPDecodeRGBInto" );
        if ( lib.webp_decode_rgb_into == NULL ) {
            SDL_UnloadObject(lib.handle);
            return -1;
        }

        lib.webp_decode_rgba_into =
            ( uint8_t* (*) (const uint8_t*, size_t, uint8_t*, size_t, int ) )
            SDL_LoadFunction(lib.handle, "WebPDecodeRGBAInto" );
        if ( lib.webp_decode_rgba_into == NULL ) {
            SDL_UnloadObject(lib.handle);
            return -1;
        }
    }
    ++lib.loaded;

    return 0;
}
void IMG_QuitWEBP()
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
        SDL_UnloadObject(lib.handle);
    }
    --lib.loaded;
}
#else
int IMG_InitWEBP()
{
    if ( lib.loaded == 0 ) {
#ifdef __MACOSX__
        extern VP8StatusCode WebPGetFeaturesInternal(const uint8_t*, size_t, WebPBitstreamFeatures*, int) __attribute__((weak_import));
        if ( WebPGetFeaturesInternal == NULL )
        {
            /* Missing weakly linked framework */
            IMG_SetError("Missing webp.framework");
            return -1;
        }
#endif // __MACOSX__

        lib.webp_get_features_internal = WebPGetFeaturesInternal;
        lib.webp_decode_rgb_into = WebPDecodeRGBInto;
        lib.webp_decode_rgba_into = WebPDecodeRGBAInto;
    }
    ++lib.loaded;

    return 0;
}
void IMG_QuitWEBP()
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
    }
    --lib.loaded;
}
#endif /* LOAD_WEBP_DYNAMIC */

static int webp_getinfo( SDL_RWops *src, int *datasize ) {
    Sint64 start;
    int is_WEBP;
    Uint8 magic[20];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_WEBP = 0;
    if ( SDL_RWread(src, magic, 1, sizeof(magic)) == sizeof(magic) ) {
        if ( magic[ 0] == 'R' &&
                     magic[ 1] == 'I' &&
                     magic[ 2] == 'F' &&
                     magic[ 3] == 'F' &&
                                         magic[ 8] == 'W' &&
                                         magic[ 9] == 'E' &&
                                         magic[10] == 'B' &&
                                         magic[11] == 'P' &&
                                         magic[12] == 'V' &&
                                         magic[13] == 'P' &&
                                         magic[14] == '8' &&
#if WEBP_DECODER_ABI_VERSION < 0x0003 /* old versions don't support WEBPVP8X and WEBPVP8L */
                                         magic[15] == ' ') {
#else
                                         (magic[15] == ' ' || magic[15] == 'X' || magic[15] == 'L')) {
#endif
            is_WEBP = 1;
            if ( datasize ) {
                *datasize = (int)SDL_RWseek(src, 0, SEEK_END);
            }
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_WEBP);
}

/* See if an image is contained in a data source */
int IMG_isWEBP(SDL_RWops *src)
{
    return webp_getinfo( src, NULL );
}

SDL_Surface *IMG_LoadWEBP_RW(SDL_RWops *src)
{
    Sint64 start;
    const char *error = NULL;
    SDL_Surface *volatile surface = NULL;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;
    WebPBitstreamFeatures features;
    int raw_data_size;
    uint8_t *raw_data = NULL;
    int r;
    uint8_t *ret;

    if ( !src ) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }

    start = SDL_RWtell(src);

    if ( !IMG_Init(IMG_INIT_WEBP) ) {
        goto error;
    }

    raw_data_size = -1;
    if ( !webp_getinfo( src, &raw_data_size ) ) {
        error = "Invalid WEBP";
        goto error;
    }

    // seek to start of file
    SDL_RWseek(src, 0, RW_SEEK_SET );

    raw_data = (uint8_t*) SDL_malloc( raw_data_size );
    if ( raw_data == NULL ) {
        error = "Failed to allocate enought buffer for WEBP";
        goto error;
    }

    r = SDL_RWread(src, raw_data, 1, raw_data_size );
    if ( r != raw_data_size ) {
        error = "Failed to read WEBP";
        goto error;
    }

#if 0
    // extract size of picture, not interesting since we don't know about alpha channel
    int width = -1, height = -1;
    if ( !WebPGetInfo( raw_data, raw_data_size, &width, &height ) ) {
        printf("WebPGetInfo has failed\n" );
        return NULL;
    }
#endif

    if ( lib.webp_get_features_internal( raw_data, raw_data_size, &features, WEBP_DECODER_ABI_VERSION ) != VP8_STATUS_OK ) {
        error = "WebPGetFeatures has failed";
        goto error;
    }

    /* Check if it's ok !*/
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Rmask = 0x000000FF;
    Gmask = 0x0000FF00;
    Bmask = 0x00FF0000;
    Amask = (features.has_alpha) ? 0xFF000000 : 0;
#else
    s = (features.has_alpha) ? 0 : 8;
    Rmask = 0xFF000000 >> s;
    Gmask = 0x00FF0000 >> s;
    Bmask = 0x0000FF00 >> s;
    Amask = 0x000000FF >> s;
#endif

    surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
            features.width, features.height,
            features.has_alpha?32:24, Rmask,Gmask,Bmask,Amask);

    if ( surface == NULL ) {
        error = "Failed to allocate SDL_Surface";
        goto error;
    }

    if ( features.has_alpha ) {
        ret = lib.webp_decode_rgba_into( raw_data, raw_data_size, (uint8_t *)surface->pixels, surface->pitch * surface->h,  surface->pitch );
    } else {
        ret = lib.webp_decode_rgb_into( raw_data, raw_data_size, (uint8_t *)surface->pixels, surface->pitch * surface->h,  surface->pitch );
    }

    if ( !ret ) {
        error = "Failed to decode WEBP";
        goto error;
    }

    return surface;


error:

    if ( surface ) {
        SDL_FreeSurface( surface );
    }

    if ( raw_data ) {
        SDL_free( raw_data );
    }

    if ( error ) {
        IMG_SetError( error );
    }

    SDL_RWseek(src, start, RW_SEEK_SET);
    return(NULL);
}

#else

int IMG_InitWEBP()
{
    IMG_SetError("WEBP images are not supported");
    return(-1);
}

void IMG_QuitWEBP()
{
}

/* See if an image is contained in a data source */
int IMG_isWEBP(SDL_RWops *src)
{
    return(0);
}

/* Load a WEBP type image from an SDL datasource */
SDL_Surface *IMG_LoadWEBP_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_WEBP */
