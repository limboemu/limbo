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

#include <string.h>

#include "music_mad.h"

mad_data *
mad_openFileRW(SDL_RWops *src, SDL_AudioSpec *mixer, int freesrc)
{
  mad_data *mp3_mad;

  mp3_mad = (mad_data *)SDL_malloc(sizeof(mad_data));
  if (mp3_mad) {
    mp3_mad->src = src;
    mp3_mad->freesrc = freesrc;
    mad_stream_init(&mp3_mad->stream);
    mad_frame_init(&mp3_mad->frame);
    mad_synth_init(&mp3_mad->synth);
    mp3_mad->frames_read = 0;
    mad_timer_reset(&mp3_mad->next_frame_start);
    mp3_mad->volume = MIX_MAX_VOLUME;
    mp3_mad->status = 0;
    mp3_mad->output_begin = 0;
    mp3_mad->output_end = 0;
    mp3_mad->mixer = *mixer;
  }
  return mp3_mad;
}

void
mad_closeFile(mad_data *mp3_mad)
{
  mad_stream_finish(&mp3_mad->stream);
  mad_frame_finish(&mp3_mad->frame);
  mad_synth_finish(&mp3_mad->synth);

  if (mp3_mad->freesrc) {
    SDL_RWclose(mp3_mad->src);
  }
  SDL_free(mp3_mad);
}

/* Starts the playback. */
void
mad_start(mad_data *mp3_mad) {
  mp3_mad->status |= MS_playing;
}

/* Stops the playback. */
void
mad_stop(mad_data *mp3_mad) {
  mp3_mad->status &= ~MS_playing;
}

/* Returns true if the playing is engaged, false otherwise. */
int
mad_isPlaying(mad_data *mp3_mad) {
  return ((mp3_mad->status & MS_playing) != 0);
}

/* Reads the next frame from the file.  Returns true on success or
   false on failure. */
static int
read_next_frame(mad_data *mp3_mad) {
  if (mp3_mad->stream.buffer == NULL ||
      mp3_mad->stream.error == MAD_ERROR_BUFLEN) {
    size_t read_size;
    size_t remaining;
    unsigned char *read_start;

    /* There might be some bytes in the buffer left over from last
       time.  If so, move them down and read more bytes following
       them. */
    if (mp3_mad->stream.next_frame != NULL) {
      remaining = mp3_mad->stream.bufend - mp3_mad->stream.next_frame;
      memmove(mp3_mad->input_buffer, mp3_mad->stream.next_frame, remaining);
      read_start = mp3_mad->input_buffer + remaining;
      read_size = MAD_INPUT_BUFFER_SIZE - remaining;

    } else {
      read_size = MAD_INPUT_BUFFER_SIZE;
      read_start = mp3_mad->input_buffer;
      remaining = 0;
    }

    /* Now read additional bytes from the input file. */
    read_size = SDL_RWread(mp3_mad->src, read_start, 1, read_size);

    if (read_size <= 0) {
      if ((mp3_mad->status & (MS_input_eof | MS_input_error)) == 0) {
        if (read_size == 0) {
          mp3_mad->status |= MS_input_eof;
        } else {
          mp3_mad->status |= MS_input_error;
        }

        /* At the end of the file, we must stuff MAD_BUFFER_GUARD
           number of 0 bytes. */
        SDL_memset(read_start + read_size, 0, MAD_BUFFER_GUARD);
        read_size += MAD_BUFFER_GUARD;
      }
    }

    /* Now feed those bytes into the libmad stream. */
    mad_stream_buffer(&mp3_mad->stream, mp3_mad->input_buffer,
                      read_size + remaining);
    mp3_mad->stream.error = MAD_ERROR_NONE;
  }

  /* Now ask libmad to extract a frame from the data we just put in
     its buffer. */
  if (mad_frame_decode(&mp3_mad->frame, &mp3_mad->stream)) {
    if (MAD_RECOVERABLE(mp3_mad->stream.error)) {
      return 0;

    } else if (mp3_mad->stream.error == MAD_ERROR_BUFLEN) {
      return 0;

    } else {
      mp3_mad->status |= MS_decode_error;
      return 0;
    }
  }

  mp3_mad->frames_read++;
  mad_timer_add(&mp3_mad->next_frame_start, mp3_mad->frame.header.duration);

  return 1;
}

/* Scale a MAD sample to 16 bits for output. */
static signed int
scale(mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/* Once the frame has been read, copies its samples into the output
   buffer. */
static void
decode_frame(mad_data *mp3_mad) {
  struct mad_pcm *pcm;
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
  unsigned char *out;
  int ret;

  mad_synth_frame(&mp3_mad->synth, &mp3_mad->frame);
  pcm = &mp3_mad->synth.pcm;
  out = mp3_mad->output_buffer + mp3_mad->output_end;

  if ((mp3_mad->status & MS_cvt_decoded) == 0) {
    mp3_mad->status |= MS_cvt_decoded;

    /* The first frame determines some key properties of the stream.
       In particular, it tells us enough to set up the convert
       structure now. */
    SDL_BuildAudioCVT(&mp3_mad->cvt, AUDIO_S16, pcm->channels, mp3_mad->frame.header.samplerate, mp3_mad->mixer.format, mp3_mad->mixer.channels, mp3_mad->mixer.freq);
  }

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    *out++ = ((sample >> 0) & 0xff);
    *out++ = ((sample >> 8) & 0xff);

    if (nchannels == 2) {
      sample = scale(*right_ch++);
      *out++ = ((sample >> 0) & 0xff);
      *out++ = ((sample >> 8) & 0xff);
    }
  }

  mp3_mad->output_end = out - mp3_mad->output_buffer;
  /*assert(mp3_mad->output_end <= MAD_OUTPUT_BUFFER_SIZE);*/
}

int
mad_getSamples(mad_data *mp3_mad, Uint8 *stream, int len) {
  int bytes_remaining;
  int num_bytes;
  Uint8 *out;

  if ((mp3_mad->status & MS_playing) == 0) {
    /* We're not supposed to be playing, so send silence instead. */
    SDL_memset(stream, 0, len);
    return 0;
  }

  out = stream;
  bytes_remaining = len;
  while (bytes_remaining > 0) {
    if (mp3_mad->output_end == mp3_mad->output_begin) {
      /* We need to get a new frame. */
      mp3_mad->output_begin = 0;
      mp3_mad->output_end = 0;
      if (!read_next_frame(mp3_mad)) {
        if ((mp3_mad->status & MS_error_flags) != 0) {
          /* Couldn't read a frame; either an error condition or
             end-of-file.  Stop. */
          SDL_memset(out, 0, bytes_remaining);
          mp3_mad->status &= ~MS_playing;
          return bytes_remaining;
        }
      } else {
        decode_frame(mp3_mad);

        /* Now convert the frame data to the appropriate format for
           output. */
        mp3_mad->cvt.buf = mp3_mad->output_buffer;
        mp3_mad->cvt.len = mp3_mad->output_end;

        mp3_mad->output_end = (int)(mp3_mad->output_end * mp3_mad->cvt.len_ratio);
        /*assert(mp3_mad->output_end <= MAD_OUTPUT_BUFFER_SIZE);*/
        SDL_ConvertAudio(&mp3_mad->cvt);
      }
    }

    num_bytes = mp3_mad->output_end - mp3_mad->output_begin;
    if (bytes_remaining < num_bytes) {
      num_bytes = bytes_remaining;
    }

    if (mp3_mad->volume == MIX_MAX_VOLUME) {
      SDL_memcpy(out, mp3_mad->output_buffer + mp3_mad->output_begin, num_bytes);
    } else {
      SDL_MixAudio(out, mp3_mad->output_buffer + mp3_mad->output_begin,
                   num_bytes, mp3_mad->volume);
    }
    out += num_bytes;
    mp3_mad->output_begin += num_bytes;
    bytes_remaining -= num_bytes;
  }
  return 0;
}

void
mad_seek(mad_data *mp3_mad, double position) {
  mad_timer_t target;
  int int_part;

  int_part = (int)position;
  mad_timer_set(&target, int_part,
                (int)((position - int_part) * 1000000), 1000000);

  if (mad_timer_compare(mp3_mad->next_frame_start, target) > 0) {
    /* In order to seek backwards in a VBR file, we have to rewind and
       start again from the beginning.  This isn't necessary if the
       file happens to be CBR, of course; in that case we could seek
       directly to the frame we want.  But I leave that little
       optimization for the future developer who discovers she really
       needs it. */
    mp3_mad->frames_read = 0;
    mad_timer_reset(&mp3_mad->next_frame_start);
    mp3_mad->status &= ~MS_error_flags;
    mp3_mad->output_begin = 0;
    mp3_mad->output_end = 0;

    SDL_RWseek(mp3_mad->src, 0, RW_SEEK_SET);
  }

  /* Now we have to skip frames until we come to the right one.
     Again, only truly necessary if the file is VBR. */
  while (mad_timer_compare(mp3_mad->next_frame_start, target) < 0) {
    if (!read_next_frame(mp3_mad)) {
      if ((mp3_mad->status & MS_error_flags) != 0) {
        /* Couldn't read a frame; either an error condition or
           end-of-file.  Stop. */
        mp3_mad->status &= ~MS_playing;
        return;
      }
    }
  }

  /* Here we are, at the beginning of the frame that contains the
     target time.  Ehh, I say that's close enough.  If we wanted to,
     we could get more precise by decoding the frame now and counting
     the appropriate number of samples out of it. */
}

void
mad_setVolume(mad_data *mp3_mad, int volume) {
  mp3_mad->volume = volume;
}


#endif  /* MP3_MAD_MUSIC */
