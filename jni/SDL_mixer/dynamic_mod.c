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

#ifdef MOD_MUSIC

#include "SDL_loadso.h"
#include "SDL_mixer.h"
#include "dynamic_mod.h"

mikmod_loader mikmod = {
    0, NULL
};

#ifdef MOD_DYNAMIC
int Mix_InitMOD()
{
    if ( mikmod.loaded == 0 ) {
        mikmod.handle = SDL_LoadObject(MOD_DYNAMIC);
        if ( mikmod.handle == NULL ) {
            return -1;
        }
        mikmod.MikMod_Exit =
            (void (*)(void))
            SDL_LoadFunction(mikmod.handle, "MikMod_Exit");
        if ( mikmod.MikMod_Exit == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_InfoDriver =
            (CHAR* (*)(void))
            SDL_LoadFunction(mikmod.handle, "MikMod_InfoDriver");
        if ( mikmod.MikMod_InfoDriver == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_InfoLoader =
            (CHAR* (*)(void))
            SDL_LoadFunction(mikmod.handle, "MikMod_InfoLoader");
        if ( mikmod.MikMod_InfoLoader == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_Init =
            (BOOL (*)(CHAR*))
            SDL_LoadFunction(mikmod.handle, "MikMod_Init");
        if ( mikmod.MikMod_Init == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_RegisterAllLoaders =
            (void (*)(void))
            SDL_LoadFunction(mikmod.handle, "MikMod_RegisterAllLoaders");
        if ( mikmod.MikMod_RegisterAllLoaders == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_RegisterDriver =
            (void (*)(struct MDRIVER*))
            SDL_LoadFunction(mikmod.handle, "MikMod_RegisterDriver");
        if ( mikmod.MikMod_RegisterDriver == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_errno =
            (int*)
            SDL_LoadFunction(mikmod.handle, "MikMod_errno");
        if ( mikmod.MikMod_errno == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_strerror =
            (char* (*)(int))
            SDL_LoadFunction(mikmod.handle, "MikMod_strerror");
        if ( mikmod.MikMod_strerror == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.MikMod_free =
            (void (*)(void*))
            SDL_LoadFunction(mikmod.handle, "MikMod_free");
        if ( mikmod.MikMod_free == NULL ) {
            /* libmikmod 3.1 and earlier doesn't have it */
            mikmod.MikMod_free = free;
        }
        mikmod.Player_Active =
            (BOOL (*)(void))
            SDL_LoadFunction(mikmod.handle, "Player_Active");
        if ( mikmod.Player_Active == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_Free =
            (void (*)(MODULE*))
            SDL_LoadFunction(mikmod.handle, "Player_Free");
        if ( mikmod.Player_Free == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_LoadGeneric =
            (MODULE* (*)(MREADER*,int,BOOL))
            SDL_LoadFunction(mikmod.handle, "Player_LoadGeneric");
        if ( mikmod.Player_LoadGeneric == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_SetPosition =
            (void (*)(UWORD))
            SDL_LoadFunction(mikmod.handle, "Player_SetPosition");
        if ( mikmod.Player_SetPosition == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_SetVolume =
            (void (*)(SWORD))
            SDL_LoadFunction(mikmod.handle, "Player_SetVolume");
        if ( mikmod.Player_SetVolume == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_Start =
            (void (*)(MODULE*))
            SDL_LoadFunction(mikmod.handle, "Player_Start");
        if ( mikmod.Player_Start == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.Player_Stop =
            (void (*)(void))
            SDL_LoadFunction(mikmod.handle, "Player_Stop");
        if ( mikmod.Player_Stop == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.VC_WriteBytes =
            (ULONG (*)(SBYTE*,ULONG))
            SDL_LoadFunction(mikmod.handle, "VC_WriteBytes");
        if ( mikmod.VC_WriteBytes == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.drv_nos =
            (MDRIVER*)
            SDL_LoadFunction(mikmod.handle, "drv_nos");
        if ( mikmod.drv_nos == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_device =
            (UWORD*)
            SDL_LoadFunction(mikmod.handle, "md_device");
        if ( mikmod.md_device == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_mixfreq =
            (UWORD*)
            SDL_LoadFunction(mikmod.handle, "md_mixfreq");
        if ( mikmod.md_mixfreq == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_mode =
            (UWORD*)
            SDL_LoadFunction(mikmod.handle, "md_mode");
        if ( mikmod.md_mode == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_musicvolume =
            (UBYTE*)
            SDL_LoadFunction(mikmod.handle, "md_musicvolume");
        if ( mikmod.md_musicvolume == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_pansep =
            (UBYTE*)
            SDL_LoadFunction(mikmod.handle, "md_pansep");
        if ( mikmod.md_pansep == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_reverb =
            (UBYTE*)
            SDL_LoadFunction(mikmod.handle, "md_reverb");
        if ( mikmod.md_reverb == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_sndfxvolume =
            (UBYTE*)
            SDL_LoadFunction(mikmod.handle, "md_sndfxvolume");
        if ( mikmod.md_sndfxvolume == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
        mikmod.md_volume =
            (UBYTE*)
            SDL_LoadFunction(mikmod.handle, "md_volume");
        if ( mikmod.md_volume == NULL ) {
            SDL_UnloadObject(mikmod.handle);
            return -1;
        }
    }
    ++mikmod.loaded;

    return 0;
}
void Mix_QuitMOD()
{
    if ( mikmod.loaded == 0 ) {
        return;
    }
    if ( mikmod.loaded == 1 ) {
        SDL_UnloadObject(mikmod.handle);
    }
    --mikmod.loaded;
}
#else
int Mix_InitMOD()
{
    if ( mikmod.loaded == 0 ) {
#ifdef __MACOSX__
        extern void Player_Start(MODULE*) __attribute__((weak_import));
        if ( Player_Start == NULL )
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing mikmod.framework");
            return -1;
        }
#endif // __MACOSX__

        mikmod.MikMod_Exit = MikMod_Exit;
        mikmod.MikMod_InfoDriver = MikMod_InfoDriver;
        mikmod.MikMod_InfoLoader = MikMod_InfoLoader;
        mikmod.MikMod_Init = MikMod_Init;
        mikmod.MikMod_RegisterAllLoaders = MikMod_RegisterAllLoaders;
        mikmod.MikMod_RegisterDriver = MikMod_RegisterDriver;
        mikmod.MikMod_errno = &MikMod_errno;
        mikmod.MikMod_strerror = MikMod_strerror;
#if LIBMIKMOD_VERSION < ((3<<16)|(2<<8))
        mikmod.MikMod_free = free;
#else
        mikmod.MikMod_free = MikMod_free;
#endif
        mikmod.Player_Active = Player_Active;
        mikmod.Player_Free = Player_Free;
        mikmod.Player_LoadGeneric = Player_LoadGeneric;
        mikmod.Player_SetPosition = Player_SetPosition;
        mikmod.Player_SetVolume = Player_SetVolume;
        mikmod.Player_Start = Player_Start;
        mikmod.Player_Stop = Player_Stop;
        mikmod.VC_WriteBytes = VC_WriteBytes;
        mikmod.drv_nos = &drv_nos;
        mikmod.md_device = &md_device;
        mikmod.md_mixfreq = &md_mixfreq;
        mikmod.md_mode = &md_mode;
        mikmod.md_musicvolume = &md_musicvolume;
        mikmod.md_pansep = &md_pansep;
        mikmod.md_reverb = &md_reverb;
        mikmod.md_sndfxvolume = &md_sndfxvolume;
        mikmod.md_volume = &md_volume;
    }
    ++mikmod.loaded;

    return 0;
}
void Mix_QuitMOD()
{
    if ( mikmod.loaded == 0 ) {
        return;
    }
    if ( mikmod.loaded == 1 ) {
    }
    --mikmod.loaded;
}
#endif /* MOD_DYNAMIC */

#endif /* MOD_MUSIC */
