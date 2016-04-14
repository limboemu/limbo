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

#ifdef MODPLUG_MUSIC

#include "SDL_loadso.h"
#include "SDL_mixer.h"
#include "dynamic_modplug.h"

modplug_loader modplug = {
    0, NULL
};

#ifdef MODPLUG_DYNAMIC
int Mix_InitModPlug()
{
    if ( modplug.loaded == 0 ) {
        modplug.handle = SDL_LoadObject(MODPLUG_DYNAMIC);
        if ( modplug.handle == NULL ) {
            return -1;
        }

        modplug.ModPlug_Load =
            (ModPlugFile* (*)(const void* data, int size))
            SDL_LoadFunction(modplug.handle, "ModPlug_Load");

        modplug.ModPlug_Unload =
            (void (*)(ModPlugFile* file))
            SDL_LoadFunction(modplug.handle, "ModPlug_Unload");

        modplug.ModPlug_Read =
            (int  (*)(ModPlugFile* file, void* buffer, int size))
            SDL_LoadFunction(modplug.handle, "ModPlug_Read");

        modplug.ModPlug_Seek =
            (void (*)(ModPlugFile* file, int millisecond))
            SDL_LoadFunction(modplug.handle, "ModPlug_Seek");

        modplug.ModPlug_GetSettings =
            (void (*)(ModPlug_Settings* settings))
            SDL_LoadFunction(modplug.handle, "ModPlug_GetSettings");

        modplug.ModPlug_SetSettings =
            (void (*)(const ModPlug_Settings* settings))
            SDL_LoadFunction(modplug.handle, "ModPlug_SetSettings");

        modplug.ModPlug_SetMasterVolume =
            (void (*)(ModPlugFile* file,unsigned int cvol))
            SDL_LoadFunction(modplug.handle, "ModPlug_SetMasterVolume");
    }
    ++modplug.loaded;

    return 0;
}

void Mix_QuitModPlug()
{
    if ( modplug.loaded == 0 ) {
        return;
    }
    if ( modplug.loaded == 1 ) {
        SDL_UnloadObject(modplug.handle);
    }
    --modplug.loaded;
}
#else
int Mix_InitModPlug()
{
    if ( modplug.loaded == 0 ) {
#ifdef __MACOSX__
        extern ModPlugFile* ModPlug_Load(const void* data, int size) __attribute__((weak_import));
        if ( ModPlug_Load == NULL )
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing modplug.framework");
            return -1;
        }
#endif // __MACOSX__

        modplug.ModPlug_Load = ModPlug_Load;
        modplug.ModPlug_Unload = ModPlug_Unload;
        modplug.ModPlug_Read = ModPlug_Read;
        modplug.ModPlug_Seek = ModPlug_Seek;
        modplug.ModPlug_GetSettings = ModPlug_GetSettings;
        modplug.ModPlug_SetSettings = ModPlug_SetSettings;
        modplug.ModPlug_SetMasterVolume = ModPlug_SetMasterVolume;
    }
    ++modplug.loaded;

    return 0;
}
void Mix_QuitModPlug()
{
    if ( modplug.loaded == 0 ) {
        return;
    }
    if ( modplug.loaded == 1 ) {
    }
    --modplug.loaded;
}
#endif /* MODPLUG_DYNAMIC */

#endif /* MODPLUG_MUSIC */
