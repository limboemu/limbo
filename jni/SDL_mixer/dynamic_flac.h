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

  The following file defines all of the functions/objects used to dynamically
  link to the libFLAC library.
    ~ Austen Dicken (admin@cvpcs.org)
*/

#ifdef FLAC_MUSIC

#include <FLAC/stream_decoder.h>

typedef struct {
    int loaded;
    void *handle;
    FLAC__StreamDecoder *(*FLAC__stream_decoder_new)();
    void (*FLAC__stream_decoder_delete)(FLAC__StreamDecoder *decoder);
    FLAC__StreamDecoderInitStatus (*FLAC__stream_decoder_init_stream)(
                        FLAC__StreamDecoder *decoder,
                        FLAC__StreamDecoderReadCallback read_callback,
                        FLAC__StreamDecoderSeekCallback seek_callback,
                        FLAC__StreamDecoderTellCallback tell_callback,
                        FLAC__StreamDecoderLengthCallback length_callback,
                        FLAC__StreamDecoderEofCallback eof_callback,
                        FLAC__StreamDecoderWriteCallback write_callback,
                        FLAC__StreamDecoderMetadataCallback metadata_callback,
                        FLAC__StreamDecoderErrorCallback error_callback,
                        void *client_data);
    FLAC__bool (*FLAC__stream_decoder_finish)(FLAC__StreamDecoder *decoder);
    FLAC__bool (*FLAC__stream_decoder_flush)(FLAC__StreamDecoder *decoder);
    FLAC__bool (*FLAC__stream_decoder_process_single)(
                        FLAC__StreamDecoder *decoder);
    FLAC__bool (*FLAC__stream_decoder_process_until_end_of_metadata)(
                        FLAC__StreamDecoder *decoder);
    FLAC__bool (*FLAC__stream_decoder_process_until_end_of_stream)(
                        FLAC__StreamDecoder *decoder);
    FLAC__bool (*FLAC__stream_decoder_seek_absolute)(
                        FLAC__StreamDecoder *decoder,
                        FLAC__uint64 sample);
    FLAC__StreamDecoderState (*FLAC__stream_decoder_get_state)(
                        const FLAC__StreamDecoder *decoder);
} flac_loader;

extern flac_loader flac;

#endif /* FLAC_MUSIC */

extern int Mix_InitFLAC();
extern void Mix_QuitFLAC();
