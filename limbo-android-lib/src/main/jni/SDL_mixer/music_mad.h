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

#ifdef MP3_MAD_MUSIC

#include "mad.h"
#include "SDL_rwops.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"

#define MAD_INPUT_BUFFER_SIZE   (5*8192)
#define MAD_OUTPUT_BUFFER_SIZE  8192

enum {
  MS_input_eof    = 0x0001,
  MS_input_error  = 0x0001,
  MS_decode_eof   = 0x0002,
  MS_decode_error = 0x0004,
  MS_error_flags  = 0x000f,

  MS_playing      = 0x0100,
  MS_cvt_decoded  = 0x0200,
};

typedef struct {
  SDL_RWops *src;
  int freesrc;
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;
  int frames_read;
  mad_timer_t next_frame_start;
  int volume;
  int status;
  int output_begin, output_end;
  SDL_AudioSpec mixer;
  SDL_AudioCVT cvt;

  unsigned char input_buffer[MAD_INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
  unsigned char output_buffer[MAD_OUTPUT_BUFFER_SIZE];
} mad_data;

mad_data *mad_openFileRW(SDL_RWops *src, SDL_AudioSpec *mixer, int freesrc);
void mad_closeFile(mad_data *mp3_mad);

void mad_start(mad_data *mp3_mad);
void mad_stop(mad_data *mp3_mad);
int mad_isPlaying(mad_data *mp3_mad);

int mad_getSamples(mad_data *mp3_mad, Uint8 *stream, int len);
void mad_seek(mad_data *mp3_mad, double position);
void mad_setVolume(mad_data *mp3_mad, int volume);

#endif
