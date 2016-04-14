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

  Implementation of the dynamic loading functionality for libFLAC.
    ~ Austen Dicken (admin@cvpcs.org)
*/

#ifdef FLAC_MUSIC

#include "SDL_loadso.h"
#include "SDL_mixer.h"
#include "dynamic_flac.h"

flac_loader flac = {
    0, NULL
};

#ifdef FLAC_DYNAMIC
int Mix_InitFLAC()
{
    if ( flac.loaded == 0 ) {
        flac.handle = SDL_LoadObject(FLAC_DYNAMIC);
        if ( flac.handle == NULL ) {
            return -1;
        }
        flac.FLAC__stream_decoder_new =
            (FLAC__StreamDecoder *(*)())
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_new");
        if ( flac.FLAC__stream_decoder_new == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_delete =
            (void (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_delete");
        if ( flac.FLAC__stream_decoder_delete == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_init_stream =
            (FLAC__StreamDecoderInitStatus (*)(
                        FLAC__StreamDecoder *,
                        FLAC__StreamDecoderReadCallback,
                        FLAC__StreamDecoderSeekCallback,
                        FLAC__StreamDecoderTellCallback,
                        FLAC__StreamDecoderLengthCallback,
                        FLAC__StreamDecoderEofCallback,
                        FLAC__StreamDecoderWriteCallback,
                        FLAC__StreamDecoderMetadataCallback,
                        FLAC__StreamDecoderErrorCallback,
                        void *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_init_stream");
        if ( flac.FLAC__stream_decoder_init_stream == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_finish =
            (FLAC__bool (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_finish");
        if ( flac.FLAC__stream_decoder_finish == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_flush =
            (FLAC__bool (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_flush");
        if ( flac.FLAC__stream_decoder_flush == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_single =
            (FLAC__bool (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                        "FLAC__stream_decoder_process_single");
        if ( flac.FLAC__stream_decoder_process_single == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_until_end_of_metadata =
            (FLAC__bool (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                        "FLAC__stream_decoder_process_until_end_of_metadata");
        if ( flac.FLAC__stream_decoder_process_until_end_of_metadata == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_until_end_of_stream =
            (FLAC__bool (*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                        "FLAC__stream_decoder_process_until_end_of_stream");
        if ( flac.FLAC__stream_decoder_process_until_end_of_stream == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_seek_absolute =
            (FLAC__bool (*)(FLAC__StreamDecoder *, FLAC__uint64))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_seek_absolute");
        if ( flac.FLAC__stream_decoder_seek_absolute == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_get_state =
            (FLAC__StreamDecoderState (*)(const FLAC__StreamDecoder *decoder))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_get_state");
        if ( flac.FLAC__stream_decoder_get_state == NULL ) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
    }
    ++flac.loaded;

    return 0;
}
void Mix_QuitFLAC()
{
    if ( flac.loaded == 0 ) {
        return;
    }
    if ( flac.loaded == 1 ) {
        SDL_UnloadObject(flac.handle);
    }
    --flac.loaded;
}
#else
int Mix_InitFLAC()
{
    if ( flac.loaded == 0 ) {
#ifdef __MACOSX__
        extern FLAC__StreamDecoder *FLAC__stream_decoder_new(void) __attribute__((weak_import));
        if ( FLAC__stream_decoder_new == NULL )
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing FLAC.framework");
            return -1;
        }
#endif // __MACOSX__

        flac.FLAC__stream_decoder_new = FLAC__stream_decoder_new;
        flac.FLAC__stream_decoder_delete = FLAC__stream_decoder_delete;
        flac.FLAC__stream_decoder_init_stream =
                            FLAC__stream_decoder_init_stream;
        flac.FLAC__stream_decoder_finish = FLAC__stream_decoder_finish;
        flac.FLAC__stream_decoder_flush = FLAC__stream_decoder_flush;
        flac.FLAC__stream_decoder_process_single =
                            FLAC__stream_decoder_process_single;
        flac.FLAC__stream_decoder_process_until_end_of_metadata =
                            FLAC__stream_decoder_process_until_end_of_metadata;
        flac.FLAC__stream_decoder_process_until_end_of_stream =
                            FLAC__stream_decoder_process_until_end_of_stream;
        flac.FLAC__stream_decoder_seek_absolute =
                            FLAC__stream_decoder_seek_absolute;
        flac.FLAC__stream_decoder_get_state =
                            FLAC__stream_decoder_get_state;
    }
    ++flac.loaded;

    return 0;
}
void Mix_QuitFLAC()
{
    if ( flac.loaded == 0 ) {
        return;
    }
    if ( flac.loaded == 1 ) {
    }
    --flac.loaded;
}
#endif /* FLAC_DYNAMIC */

#endif /* FLAC_MUSIC */
