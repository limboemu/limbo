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

#ifdef MOD_MUSIC

#include "mikmod.h"

typedef struct {
    int loaded;
    void *handle;

    void (*MikMod_Exit)(void);
    CHAR* (*MikMod_InfoDriver)(void);
    CHAR* (*MikMod_InfoLoader)(void);
    BOOL (*MikMod_Init)(CHAR*);
    void (*MikMod_RegisterAllLoaders)(void);
    void (*MikMod_RegisterDriver)(struct MDRIVER*);
    int* MikMod_errno;
    char* (*MikMod_strerror)(int);
    void (*MikMod_free)(void*);
    BOOL (*Player_Active)(void);
    void (*Player_Free)(MODULE*);
    MODULE* (*Player_LoadGeneric)(MREADER*,int,BOOL);
    void (*Player_SetPosition)(UWORD);
    void (*Player_SetVolume)(SWORD);
    void (*Player_Start)(MODULE*);
    void (*Player_Stop)(void);
    ULONG (*VC_WriteBytes)(SBYTE*,ULONG);
    struct MDRIVER* drv_nos;
    UWORD* md_device;
    UWORD* md_mixfreq;
    UWORD* md_mode;
    UBYTE* md_musicvolume;
    UBYTE* md_pansep;
    UBYTE* md_reverb;
    UBYTE* md_sndfxvolume;
    UBYTE* md_volume;
} mikmod_loader;

extern mikmod_loader mikmod;

#endif /* MOD_MUSIC */

extern int Mix_InitMOD();
extern void Mix_QuitMOD();
