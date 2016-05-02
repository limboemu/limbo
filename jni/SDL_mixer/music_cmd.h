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

/* This file supports an external command for playing music */

#ifdef CMD_MUSIC

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#if defined(__linux__) && defined(__arm__)
# include <linux/limits.h>
#endif
typedef struct {
    char *file;
    char *cmd;
    pid_t pid;
} MusicCMD;

/* Unimplemented */
extern void MusicCMD_SetVolume(int volume);

/* Load a music stream from the given file */
extern MusicCMD *MusicCMD_LoadSong(const char *cmd, const char *file);

/* Start playback of a given music stream */
extern void MusicCMD_Start(MusicCMD *music);

/* Stop playback of a stream previously started with MusicCMD_Start() */
extern void MusicCMD_Stop(MusicCMD *music);

/* Pause playback of a given music stream */
extern void MusicCMD_Pause(MusicCMD *music);

/* Resume playback of a given music stream */
extern void MusicCMD_Resume(MusicCMD *music);

/* Close the given music stream */
extern void MusicCMD_FreeSong(MusicCMD *music);

/* Return non-zero if a stream is currently playing */
extern int MusicCMD_Active(MusicCMD *music);

#endif /* CMD_MUSIC */
