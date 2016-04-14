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

#ifdef OGG_MUSIC
#ifdef OGG_USE_TREMOR
#include <ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

typedef struct {
    int loaded;
    void *handle;
    int (*ov_clear)(OggVorbis_File *vf);
    vorbis_info *(*ov_info)(OggVorbis_File *vf,int link);
    int (*ov_open_callbacks)(void *datasource, OggVorbis_File *vf, const char *initial, long ibytes, ov_callbacks callbacks);
    ogg_int64_t (*ov_pcm_total)(OggVorbis_File *vf,int i);
#ifdef OGG_USE_TREMOR
    long (*ov_read)(OggVorbis_File *vf,char *buffer,int length, int *bitstream);
#else
    long (*ov_read)(OggVorbis_File *vf,char *buffer,int length, int bigendianp,int word,int sgned,int *bitstream);
#endif
#ifdef OGG_USE_TREMOR
    int (*ov_time_seek)(OggVorbis_File *vf,ogg_int64_t pos);
#else
    int (*ov_time_seek)(OggVorbis_File *vf,double pos);
#endif
} vorbis_loader;

extern vorbis_loader vorbis;

#endif /* OGG_MUSIC */

extern int Mix_InitOgg();
extern void Mix_QuitOgg();
