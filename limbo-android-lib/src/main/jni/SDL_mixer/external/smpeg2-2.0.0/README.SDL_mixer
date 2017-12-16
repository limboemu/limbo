
There is no longer explicit support for the SDL_mixer library.

You can have the SDL mixer library mix audio from a movie by hooking into
the SDL mixer music hooks:

#include "smpeg.h"
#include "SDL_mixer.h"

        .. set up the mixer audio ...

        /* Note the last parameter is zero! */
        mpeg = SMPEG_new("file.mpg", &info, 0);

        /* Play the movie, using SDL_mixer for audio */
        SMPEG_enableaudio(mpeg, 0);
        if ( play_audio ) {
                SDL_AudioSpec audiofmt;
                Uint16 format;
                int freq, channels;

                /* Tell SMPEG what the audio format is */
                Mix_QuerySpec(&freq, &format, &channels);
                audiofmt.format = format;
                audiofmt.freq = freq;
                audiofmt.channels = channels;
                SMPEG_actualSpec(mpeg, &audiofmt);

                /* Hook in the MPEG music mixer */
                Mix_HookMusic(SMPEG_playAudioSDL, mpeg);
                SMPEG_enableaudio(mpeg, 1);
        }
        SMPEG_play(mpeg);

        /* Stop the movie and unhook SMPEG from the mixer */
        SMPEG_stop(mpeg);
        Mix_HookMusic(NULL, NULL);

