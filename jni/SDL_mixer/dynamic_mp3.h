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
#include "smpeg.h"

typedef struct {
    int loaded;
    void *handle;
    void (*SMPEG_actualSpec)( SMPEG *mpeg, SDL_AudioSpec *spec );
    void (*SMPEG_delete)( SMPEG* mpeg );
    void (*SMPEG_enableaudio)( SMPEG* mpeg, int enable );
    void (*SMPEG_enablevideo)( SMPEG* mpeg, int enable );
    SMPEG* (*SMPEG_new_rwops)(SDL_RWops *src, SMPEG_Info* info, int freesrc, int sdl_audio);
    void (*SMPEG_play)( SMPEG* mpeg );
    int (*SMPEG_playAudio)( SMPEG *mpeg, Uint8 *stream, int len );
    void (*SMPEG_rewind)( SMPEG* mpeg );
    void (*SMPEG_setvolume)( SMPEG* mpeg, int volume );
    void (*SMPEG_skip)( SMPEG* mpeg, float seconds );
    SMPEGstatus (*SMPEG_status)( SMPEG* mpeg );
    void (*SMPEG_stop)( SMPEG* mpeg );
} smpeg_loader;

extern smpeg_loader smpeg;

#endif /* MUSIC_MP3 */

extern int Mix_InitMP3();
extern void Mix_QuitMP3();
