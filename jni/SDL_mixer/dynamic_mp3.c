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

#ifdef MP3_MUSIC

#include "SDL_loadso.h"
#include "SDL_mixer.h"
#include "dynamic_mp3.h"

smpeg_loader smpeg = {
    0, NULL
};

#ifdef MP3_DYNAMIC
int Mix_InitMP3()
{
    if ( smpeg.loaded == 0 ) {
        smpeg.handle = SDL_LoadObject(MP3_DYNAMIC);
        if ( smpeg.handle == NULL ) {
            return -1;
        }
        smpeg.SMPEG_actualSpec =
            (void (*)( SMPEG *, SDL_AudioSpec * ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_actualSpec");
        if ( smpeg.SMPEG_actualSpec == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_delete =
            (void (*)( SMPEG* ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_delete");
        if ( smpeg.SMPEG_delete == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_enableaudio =
            (void (*)( SMPEG*, int ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_enableaudio");
        if ( smpeg.SMPEG_enableaudio == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_enablevideo =
            (void (*)( SMPEG*, int ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_enablevideo");
        if ( smpeg.SMPEG_enablevideo == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_new_rwops =
            (SMPEG* (*)(SDL_RWops *, SMPEG_Info*, int, int))
            SDL_LoadFunction(smpeg.handle, "SMPEG_new_rwops");
        if ( smpeg.SMPEG_new_rwops == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_play =
            (void (*)( SMPEG* ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_play");
        if ( smpeg.SMPEG_play == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_playAudio =
            (int (*)( SMPEG *, Uint8 *, int ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_playAudio");
        if ( smpeg.SMPEG_playAudio == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_rewind =
            (void (*)( SMPEG* ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_rewind");
        if ( smpeg.SMPEG_rewind == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_setvolume =
            (void (*)( SMPEG*, int ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_setvolume");
        if ( smpeg.SMPEG_setvolume == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_skip =
            (void (*)( SMPEG*, float ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_skip");
        if ( smpeg.SMPEG_skip == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_status =
            (SMPEGstatus (*)( SMPEG* ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_status");
        if ( smpeg.SMPEG_status == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
        smpeg.SMPEG_stop =
            (void (*)( SMPEG* ))
            SDL_LoadFunction(smpeg.handle, "SMPEG_stop");
        if ( smpeg.SMPEG_stop == NULL ) {
            SDL_UnloadObject(smpeg.handle);
            return -1;
        }
    }
    ++smpeg.loaded;

    return 0;
}
void Mix_QuitMP3()
{
    if ( smpeg.loaded == 0 ) {
        return;
    }
    if ( smpeg.loaded == 1 ) {
        SDL_UnloadObject(smpeg.handle);
    }
    --smpeg.loaded;
}
#else
int Mix_InitMP3()
{
    if ( smpeg.loaded == 0 ) {
#ifdef __MACOSX__
        extern SMPEG* SMPEG_new_rwops(SDL_RWops*, SMPEG_Info*, int, int) __attribute__((weak_import));
        if ( SMPEG_new_rwops == NULL )
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing smpeg2.framework");
            return -1;
        }
#endif // __MACOSX__

        smpeg.SMPEG_actualSpec = SMPEG_actualSpec;
        smpeg.SMPEG_delete = SMPEG_delete;
        smpeg.SMPEG_enableaudio = SMPEG_enableaudio;
        smpeg.SMPEG_enablevideo = SMPEG_enablevideo;
        smpeg.SMPEG_new_rwops = SMPEG_new_rwops;
        smpeg.SMPEG_play = SMPEG_play;
        smpeg.SMPEG_playAudio = SMPEG_playAudio;
        smpeg.SMPEG_rewind = SMPEG_rewind;
        smpeg.SMPEG_setvolume = SMPEG_setvolume;
        smpeg.SMPEG_skip = SMPEG_skip;
        smpeg.SMPEG_status = SMPEG_status;
        smpeg.SMPEG_stop = SMPEG_stop;
    }
    ++smpeg.loaded;

    return 0;
}
void Mix_QuitMP3()
{
    if ( smpeg.loaded == 0 ) {
        return;
    }
    if ( smpeg.loaded == 1 ) {
    }
    --smpeg.loaded;
}
#endif /* MP3_DYNAMIC */

#endif /* MP3_MUSIC */
