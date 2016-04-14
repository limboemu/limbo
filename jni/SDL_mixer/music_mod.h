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

/* $Id: music_mod.h 4211 2008-12-08 00:27:32Z slouken $ */

#ifdef MOD_MUSIC

/* This file supports MOD tracker music streams */

struct MODULE;

/* Initialize the Ogg Vorbis player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
extern int MOD_init(SDL_AudioSpec *mixer);

/* Uninitialize the music players */
extern void MOD_exit(void);

/* Set the volume for a MOD stream */
extern void MOD_setvolume(struct MODULE *music, int volume);

/* Load a MOD stream from an SDL_RWops object */
extern struct MODULE *MOD_new_RW(SDL_RWops *rw, int freerw);

/* Start playback of a given MOD stream */
extern void MOD_play(struct MODULE *music);

/* Return non-zero if a stream is currently playing */
extern int MOD_playing(struct MODULE *music);

/* Play some of a stream previously started with MOD_play() */
extern int MOD_playAudio(struct MODULE *music, Uint8 *stream, int len);

/* Stop playback of a stream previously started with MOD_play() */
extern void MOD_stop(struct MODULE *music);

/* Close the given MOD stream */
extern void MOD_delete(struct MODULE *music);

/* Jump (seek) to a given position (time is in seconds) */
extern void MOD_jump_to_time(struct MODULE *music, double time);

#endif /* MOD_MUSIC */
