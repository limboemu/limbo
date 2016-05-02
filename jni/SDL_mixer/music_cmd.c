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
#include "SDL_config.h"

/* This file supports an external command for playing music */

#ifdef CMD_MUSIC

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "SDL_mixer.h"
#include "music_cmd.h"

/* Unimplemented */
void MusicCMD_SetVolume(int volume)
{
    Mix_SetError("No way to modify external player volume");
}

/* Load a music stream from the given file */
MusicCMD *MusicCMD_LoadSong(const char *cmd, const char *file)
{
    MusicCMD *music;

    /* Allocate and fill the music structure */
    music = (MusicCMD *)SDL_malloc(sizeof *music);
    if ( music == NULL ) {
        Mix_SetError("Out of memory");
        return(NULL);
    }
    music->file = SDL_strdup(file);
    music->cmd = SDL_strdup(cmd);
    music->pid = 0;

    /* We're done */
    return(music);
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
    char *bufp;
    int argc;

    argc = 0;
    for ( bufp = cmdline; *bufp; ) {
        /* Skip leading whitespace */
        while ( isspace(*bufp) ) {
            ++bufp;
        }
        /* Skip over argument */
        if ( *bufp == '"' ) {
            ++bufp;
            if ( *bufp ) {
                if ( argv ) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            while ( *bufp && (*bufp != '"') ) {
                ++bufp;
            }
        } else {
            if ( *bufp ) {
                if ( argv ) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            while ( *bufp && ! isspace(*bufp) ) {
                ++bufp;
            }
        }
        if ( *bufp ) {
            if ( argv ) {
                *bufp = '\0';
            }
            ++bufp;
        }
    }
    if ( argv ) {
        argv[argc] = NULL;
    }
    return(argc);
}

static char **parse_args(char *command, char *last_arg)
{
    int argc;
    char **argv;

    /* Parse the command line */
    argc = ParseCommandLine(command, NULL);
    if ( last_arg ) {
        ++argc;
    }
    argv = (char **)SDL_malloc((argc+1)*(sizeof *argv));
    if ( argv == NULL ) {
        return(NULL);
    }
    argc = ParseCommandLine(command, argv);

    /* Add last command line argument */
    if ( last_arg ) {
        argv[argc++] = last_arg;
    }
    argv[argc] = NULL;

    /* We're ready! */
    return(argv);
}

/* Start playback of a given music stream */
void MusicCMD_Start(MusicCMD *music)
{
#ifdef HAVE_FORK
    music->pid = fork();
#else
    music->pid = vfork();
#endif
    switch(music->pid) {
        /* Failed fork() system call */
        case -1:
        Mix_SetError("fork() failed");
        return;

        /* Child process - executes here */
        case 0: {
            char *command;
            char **argv;

            /* Unblock signals in case we're called from a thread */
            {
            sigset_t mask;
            sigemptyset(&mask);
            sigprocmask(SIG_SETMASK, &mask, NULL);
            }

            /* Execute the command */
            command = SDL_strdup(music->cmd);
            argv = parse_args(command, music->file);
            if ( argv != NULL ) {
            execvp(argv[0], argv);
            }
            SDL_free(command);

            /* exec() failed */
            perror(argv[0]);
            _exit(-1);
        }
        break;

        /* Parent process - executes here */
        default:
        break;
    }
    return;
}

/* Stop playback of a stream previously started with MusicCMD_Start() */
void MusicCMD_Stop(MusicCMD *music)
{
    int status;

    if ( music->pid > 0 ) {
        while ( kill(music->pid, 0) == 0 ) {
            kill(music->pid, SIGTERM);
            sleep(1);
            waitpid(music->pid, &status, WNOHANG);
        }
        music->pid = 0;
    }
}

/* Pause playback of a given music stream */
void MusicCMD_Pause(MusicCMD *music)
{
    if ( music->pid > 0 ) {
        kill(music->pid, SIGSTOP);
    }
}

/* Resume playback of a given music stream */
void MusicCMD_Resume(MusicCMD *music)
{
    if ( music->pid > 0 ) {
        kill(music->pid, SIGCONT);
    }
}

/* Close the given music stream */
void MusicCMD_FreeSong(MusicCMD *music)
{
    SDL_free(music->file);
    SDL_free(music->cmd);
    SDL_free(music);
}

/* Return non-zero if a stream is currently playing */
int MusicCMD_Active(MusicCMD *music)
{
    int status;
    int active;

    active = 0;
    if ( music->pid > 0 ) {
        waitpid(music->pid, &status, WNOHANG);
        if ( kill(music->pid, 0) == 0 ) {
            active = 1;
        }
    }
    return(active);
}

#endif /* CMD_MUSIC */
