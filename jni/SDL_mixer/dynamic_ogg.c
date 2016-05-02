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
*/

#ifdef OGG_MUSIC

#include "SDL_loadso.h"
#include "SDL_mixer.h"
#include "dynamic_ogg.h"

vorbis_loader vorbis = {
    0, NULL
};

#ifdef OGG_DYNAMIC
int Mix_InitOgg()
{
    if ( vorbis.loaded == 0 ) {
        vorbis.handle = SDL_LoadObject(OGG_DYNAMIC);
        if ( vorbis.handle == NULL ) {
            return -1;
        }
        vorbis.ov_clear =
            (int (*)(OggVorbis_File *))
            SDL_LoadFunction(vorbis.handle, "ov_clear");
        if ( vorbis.ov_clear == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_info =
            (vorbis_info *(*)(OggVorbis_File *,int))
            SDL_LoadFunction(vorbis.handle, "ov_info");
        if ( vorbis.ov_info == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_open_callbacks =
            (int (*)(void *, OggVorbis_File *, const char *, long, ov_callbacks))
            SDL_LoadFunction(vorbis.handle, "ov_open_callbacks");
        if ( vorbis.ov_open_callbacks == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_pcm_total =
            (ogg_int64_t (*)(OggVorbis_File *,int))
            SDL_LoadFunction(vorbis.handle, "ov_pcm_total");
        if ( vorbis.ov_pcm_total == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_read =
#ifdef OGG_USE_TREMOR
            (long (*)(OggVorbis_File *,char *,int,int *))
#else
            (long (*)(OggVorbis_File *,char *,int,int,int,int,int *))
#endif
            SDL_LoadFunction(vorbis.handle, "ov_read");
        if ( vorbis.ov_read == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_time_seek =
#ifdef OGG_USE_TREMOR
            (long (*)(OggVorbis_File *,ogg_int64_t))
#else
            (int (*)(OggVorbis_File *,double))
#endif
            SDL_LoadFunction(vorbis.handle, "ov_time_seek");
        if ( vorbis.ov_time_seek == NULL ) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
    }
    ++vorbis.loaded;

    return 0;
}
void Mix_QuitOgg()
{
    if ( vorbis.loaded == 0 ) {
        return;
    }
    if ( vorbis.loaded == 1 ) {
        SDL_UnloadObject(vorbis.handle);
    }
    --vorbis.loaded;
}
#else
int Mix_InitOgg()
{
    if ( vorbis.loaded == 0 ) {
#ifdef __MACOSX__
        extern int ov_open_callbacks(void*, OggVorbis_File*, const char*, long, ov_callbacks) __attribute__((weak_import));
        if ( ov_open_callbacks == NULL )
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing Vorbis.framework");
            return -1;
        }
#endif // __MACOSX__

        vorbis.ov_clear = ov_clear;
        vorbis.ov_info = ov_info;
        vorbis.ov_open_callbacks = ov_open_callbacks;
        vorbis.ov_pcm_total = ov_pcm_total;
        vorbis.ov_read = ov_read;
        vorbis.ov_time_seek = ov_time_seek;
    }
    ++vorbis.loaded;

    return 0;
}
void Mix_QuitOgg()
{
    if ( vorbis.loaded == 0 ) {
        return;
    }
    if ( vorbis.loaded == 1 ) {
    }
    --vorbis.loaded;
}
#endif /* OGG_DYNAMIC */

#endif /* OGG_MUSIC */
