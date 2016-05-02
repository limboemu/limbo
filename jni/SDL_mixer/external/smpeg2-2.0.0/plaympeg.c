/*
   plaympeg - Sample MPEG player using the SMPEG library
   Copyright (C) 1999 Loki Entertainment Software
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef unix
#include <unistd.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define NET_SUPPORT  /* General network support */
#define RAW_SUPPORT  /* Raw data transport support */
#define HTTP_SUPPORT /* HTTP support */
#define FTP_SUPPORT  /* FTP support */
#ifdef linux
#define VCD_SUPPORT  /* Video CD support */
#endif
#endif

#ifdef NET_SUPPORT
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#ifdef VCD_SUPPORT
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/cdrom.h>
#endif

#include "smpeg.h"


void usage(char *argv0)
{
    printf(
"Usage: %s [options] file ...\n"
"Where the options are one of:\n"
"        --noaudio             Don't play audio stream\n"
"        --novideo             Don't play video stream\n"
"        --fullscreen             Play MPEG in fullscreen mode\n"
"        --double or -2             Play MPEG at double size\n"
"        --loop or -l             Play MPEG over and over\n"
"        --bilinear             Use bilinear filtering\n"
"        --volume N or -v N   Set audio volume to N (0-100)\n"
"        --title T or -t T  Set window's title to T\n"
"        --scale wxh or -s wxh  Play MPEG at given resolution\n"
"        --seek N or -S N     Skip N bytes\n"
#ifdef USE_SYSTEM_TIMESTAMP
"        --skip N or -k N     Skip N seconds\n"
#endif
"        --help or -h\n"
"        --version or -V\n"
"Specifying - as filename will use stdin for input\n", argv0);
}

#ifdef NET_SUPPORT
int is_address_multicast(unsigned long address)
{
  if((address & 255) >= 224 && (address & 255) <= 239) return(1);
  return(0);
}

int tcp_open(char * address, int port)
{
  struct sockaddr_in stAddr;
  struct hostent * host;
  int sock;
  struct linger l;

  memset(&stAddr,0,sizeof(stAddr));
  stAddr.sin_family = AF_INET ;
  stAddr.sin_port = htons(port);

  if((host = gethostbyname(address)) == NULL) return(0);

  stAddr.sin_addr = *((struct in_addr *) host->h_addr_list[0]) ;

  if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return(0);

  l.l_onoff = 1; l.l_linger = 5;
  if(setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*) &l, sizeof(l)) < 0) return(0);

  if(connect(sock, (struct sockaddr *) &stAddr, sizeof(stAddr)) < 0) return(0);

  return(sock);
}

int udp_open(char * address, int port)
{
  int enable = 1L;
  struct sockaddr_in stAddr;
  struct sockaddr_in stLclAddr;
  struct ip_mreq stMreq;
  struct hostent * host;
  int sock;

  stAddr.sin_family = AF_INET; 
  stAddr.sin_port = htons(port);

  if((host = gethostbyname(address)) == NULL) return(0);

  stAddr.sin_addr = *((struct in_addr *) host->h_addr_list[0]) ;

  /* Create a UDP socket */
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return(0);
            
  /* Allow multiple instance of the client to share the same address and port */
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &enable, sizeof(unsigned long int)) < 0) return(0);
  
  /* If the address is multicast, register to the multicast group */
  if(is_address_multicast(stAddr.sin_addr.s_addr))
  {
    /* Bind the socket to port */
    stLclAddr.sin_family      = AF_INET;
    stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stLclAddr.sin_port        = stAddr.sin_port;
    if(bind(sock, (struct sockaddr*) & stLclAddr, sizeof(stLclAddr)) < 0) return(0);
      
    /* Register to a multicast address */
    stMreq.imr_multiaddr.s_addr = stAddr.sin_addr.s_addr;
    stMreq.imr_interface.s_addr = INADDR_ANY;
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) & stMreq, sizeof(stMreq)) < 0) return(0);
  }
  else
  {
    /* Bind the socket to port */
    stLclAddr.sin_family      = AF_INET;
    stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stLclAddr.sin_port        = htons(0);
    if(bind(sock, (struct sockaddr*) & stLclAddr, sizeof(stLclAddr)) < 0) return(0);
  }
  
  return(sock);
}

#ifdef RAW_SUPPORT
int raw_open(char * arg)
{
  char * host;
  int port;
  int sock;

  /* Check for URL syntax */
  if(strncmp(arg, "raw://", strlen("raw://"))) return(0);

  /* Parse URL */
  port = 0;
  host = arg + strlen("raw://");
  if(strchr(host, ':') != NULL) /* port is specified */
  {
    port = atoi(strchr(host, ':') + 1);
    *strchr(host, ':') = 0;
  }

  /* Open a UDP socket */
  if(!(sock = udp_open(host, port)))
    perror("raw_open");
  
  return(sock);
}
#endif

#ifdef HTTP_SUPPORT
int http_open(char * arg)
{
  char * host;
  int port;
  char * request;
  int tcp_sock;
  char http_request[1024];
  char c;

  /* Check for URL syntax */
  if(strncmp(arg, "http://", strlen("http://"))) return(0);

  /* Parse URL */
  port = 80;
  host = arg + strlen("http://");
  if((request = strchr(host, '/')) == NULL) return(0);
  *request++ = 0;
  if(strchr(host, ':') != NULL) /* port is specified */
  {
    port = atoi(strchr(host, ':') + 1);
    *strchr(host, ':') = 0;
  } 

  /* Open a TCP socket */
  if(!(tcp_sock = tcp_open(host, port)))
  {
    perror("http_open");
    return(0);
  }

  /* Send HTTP GET request */
  sprintf(http_request, 
          "GET /%s HTTP/1.0\r\n"
          "User-Agent: Mozilla/2.0 (Win95; I)\r\n"
          "Pragma: no-cache\r\n"
          "Host: %s\r\n"
          "Accept: */*\r\n"
          "\r\n",
          request, host);
  send(tcp_sock, http_request, strlen(http_request), 0);
  
  /* Parse server reply */
  do read(tcp_sock, &c, sizeof(char)); while(c != ' ');
  read(tcp_sock, http_request, 4*sizeof(char));
  http_request[4] = 0;
  if(strcmp(http_request, "200 "))
  {
    fprintf(stderr, "http_open: ");
    do { 
      read(tcp_sock, &c, sizeof(char));
      fprintf(stderr, "%c", c); 
    }
    while(c != '\r');
    fprintf(stderr, "\n");
    return(0);
  }
  
  return(tcp_sock);
}
#endif
#ifdef FTP_SUPPORT
int ftp_get_reply(int tcp_sock)
{
  int i;
  char c;
  char answer[1024];

  do {
    /* Read a line */
    for(i = 0, c = 0; i < 1024 && c != '\n'; i++)
    {
      read(tcp_sock, &c, sizeof(char));
      answer[i] = c;
    }
    answer[i] = 0;
    fprintf(stderr, "%s", answer + 4);
  }
  while(answer[3] == '-');

  answer[3] = 0;

  return(atoi(answer));
}

int ftp_open(char * arg)
{
  char * host;
  int port;
  char * dir;
  char * file;
  int tcp_sock;
  int data_sock;
  char ftp_request[1024];
  struct sockaddr_in stLclAddr;
  socklen_t namelen;
  int i;

  /* Check for URL syntax */
  if(strncmp(arg, "ftp://", strlen("ftp://"))) return(0);

  /* Parse URL */
  port = 21;
  host = arg + strlen("ftp://");
  if((dir = strchr(host, '/')) == NULL) return(0);
  *dir++ = 0;
  if((file = strrchr(dir, '/')) == NULL) {
    file = dir;
    dir = NULL;
  } else
    *file++ = 0;

  if(strchr(host, ':') != NULL) /* port is specified */
  {
    port = atoi(strchr(host, ':') + 1);
    *strchr(host, ':') = 0;
  }

  /* Open a TCP socket */
  if(!(tcp_sock = tcp_open(host, port)))
  {
    perror("ftp_open");
    return(0);
  }

  /* Send FTP USER and PASS request */
  ftp_get_reply(tcp_sock);
  sprintf(ftp_request, "USER anonymous\r\n");
  send(tcp_sock, ftp_request, strlen(ftp_request), 0);
  if(ftp_get_reply(tcp_sock) != 331) return(0);
  sprintf(ftp_request, "PASS smpeguser@\r\n");
  send(tcp_sock, ftp_request, strlen(ftp_request), 0);
  if(ftp_get_reply(tcp_sock) != 230) return(0);
  sprintf(ftp_request, "TYPE I\r\n");
  send(tcp_sock, ftp_request, strlen(ftp_request), 0);
  if(ftp_get_reply(tcp_sock) != 200) return(0);
  if(dir != NULL)
  {
    sprintf(ftp_request, "CWD %s\r\n", dir);
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if(ftp_get_reply(tcp_sock) != 250) return(0);
  }
    
  /* Get interface address */
  namelen = sizeof(stLclAddr);
  if(getsockname(tcp_sock, (struct sockaddr *) &stLclAddr, &namelen) < 0)
    return(0);

  /* Open data socket */
  if ((data_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) return(0);

  stLclAddr.sin_family = AF_INET;

  /* Get the first free port */
  for(i = 0; i < 0xC000; i++) {
    stLclAddr.sin_port = htons(0x4000 + i);
    if(bind(data_sock, (struct sockaddr *) &stLclAddr, sizeof(stLclAddr)) >= 0) break;
  }
  port = 0x4000 + i;

  if(listen(data_sock, 1) < 0) return(0);

  i = ntohl(stLclAddr.sin_addr.s_addr);
  sprintf(ftp_request, "PORT %d,%d,%d,%d,%d,%d\r\n",
            (i >> 24) & 0xFF, (i >> 16) & 0xFF,
            (i >> 8) & 0xFF, i & 0xFF,
            (port >> 8) & 0xFF, port & 0xFF);
  send(tcp_sock, ftp_request, strlen(ftp_request), 0);
  if(ftp_get_reply(tcp_sock) != 200) return(0);

  sprintf(ftp_request, "RETR %s\r\n", file);
  send(tcp_sock, ftp_request, strlen(ftp_request), 0);
  if(ftp_get_reply(tcp_sock) != 150) return(0);

  return(accept(data_sock, NULL, NULL));
}
#endif
#endif

#ifdef VCD_SUPPORT
int vcd_read(int fd, int lba, unsigned char *buf)
{
    struct cdrom_msf *msf;

    msf = (struct cdrom_msf*) buf;
    msf->cdmsf_min0   = (lba + CD_MSF_OFFSET) / CD_FRAMES / CD_SECS; 
    msf->cdmsf_sec0   = (lba + CD_MSF_OFFSET) / CD_FRAMES % CD_SECS;
    msf->cdmsf_frame0 = (lba + CD_MSF_OFFSET) % CD_FRAMES;
    return(ioctl(fd, CDROMREADMODE2, buf));
}

int vcd_open(char * arg)
{
  struct stat buf;
  struct cdrom_tocentry toc;
  char *pip;
  int track;
  int pipe_fd[2];
  int fd;
  int pid, parent;
  unsigned char * buffer;
  
  /* Track defaults to 02, unless requested otherwise */
  track = 02;
  pip = strrchr(arg, ':');
  if ( pip ) {
    *pip = '\0';
    track = atoi(pip+1) + 1;
  }

  /* See if the CD-ROM device file exists */
  if ( (stat(arg, &buf) < 0) || !S_ISBLK(buf.st_mode) ) {
    if ( pip ) {
      *pip = ':';
    }
    return(0);
  }

  fd = open(arg, O_RDONLY, 0);
  if ( fd < 0 ) {
    if ( pip ) {
      *pip = ':';
    }
    return(0);
  }

  /* Track 02 (changed to 'track') contains MPEG data */
  if ( track < 2 ) {
    printf("Warning: VCD data normally starts on track 2\n");
  }
  toc.cdte_track  = track; 
  toc.cdte_format = CDROM_LBA;
  if(ioctl(fd, CDROMREADTOCENTRY, &toc) < 0) return(0);

  if(pipe(pipe_fd) < 0) return(0);

  parent = getpid();
  pid = fork();

  if(pid < 0) return(0);

  if(!pid)
  {
    /* Child process fills the pipe */
    int pos;
    struct timeval timeout;
    fd_set fdset;

    buffer = (unsigned char *) malloc(CD_FRAMESIZE_RAW0);

    for(pos = toc.cdte_addr.lba; vcd_read(fd, pos, buffer) >= 0; pos ++)
    {
      if(kill(parent, 0) < 0) break;

      FD_ZERO(&fdset);
      FD_SET(pipe_fd[1], &fdset);
      timeout.tv_sec = 10;
      timeout.tv_usec = 0;
      if(select(pipe_fd[1]+1, NULL, &fdset, NULL, &timeout) <= 0) break;
      if(write(pipe_fd[1], buffer, CD_FRAMESIZE_RAW0) < 0) break;
    }

    free(buffer);
    exit(0);
  }
  
  return(pipe_fd[0]);
}
#endif

typedef struct
{
    SMPEG_Frame *frame;
    int frameCount;
    SDL_mutex *lock;
} update_context;

void update(void *data, SMPEG_Frame *frame)
{
    update_context *context = (update_context *)data;
    context->frame = frame;
    ++context->frameCount;
}

/* Flag telling the UI that the movie or song should be skipped */
int done;

void next_movie(int sig)
{
        done = 1;
}

int main(int argc, char *argv[])
{
    int use_audio, use_video;
    int fullscreen;
    int scalesize;
    int scale_width, scale_height;
    int loop_play;
    int i, pause;
    int volume;
    Uint32 seek;
    float skip;
    int filtering;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    int texture_width;
    int texture_height;
    update_context context;
    int frameCount = 0;
    SMPEG *mpeg;
    SMPEG_Info info;
    char *basefile;
    const char *title = NULL;
    SDL_version sdlver;
    SMPEG_version smpegver;
    int fd;
    int status;

    /* Get the command line options */
    use_audio = 1;
    use_video = 1;
    fullscreen = 0;
    scalesize = 1;
    scale_width = 0;
    scale_height = 0;
    loop_play = 0;
    volume = 100;
    seek = 0;
    skip = 0;
    filtering = 0;
    fd = 0;
    for ( i=1; argv[i] && (argv[i][0] == '-') && (argv[i][1] != 0); ++i ) {
        if ( (strcmp(argv[i], "--noaudio") == 0) ||
             (strcmp(argv[i], "--nosound") == 0) ) {
            use_audio = 0;
        } else
        if ( strcmp(argv[i], "--novideo") == 0 ) {
            use_video = 0;
        } else
        if ( strcmp(argv[i], "--fullscreen") == 0 ) {
            fullscreen = 1;
        } else
        if ((strcmp(argv[i], "--double") == 0)||(strcmp(argv[i], "-2") == 0)) {
            scalesize = 2;
        } else
        if ((strcmp(argv[i], "--loop") == 0) || (strcmp(argv[i], "-l") == 0)) {
            loop_play = 1;
        } else
        if ( strcmp(argv[i], "--bilinear") == 0 ) {
            filtering = 1;
        } else
        if ((strcmp(argv[i], "--seek") == 0)||(strcmp(argv[i], "-S") == 0)) {
            ++i;
            if ( argv[i] ) {
                seek = atol(argv[i]);
            }
        } else
        if ((strcmp(argv[i], "--skip") == 0)||(strcmp(argv[i], "-k") == 0)) {
            ++i;
            if ( argv[i] ) {
                skip = (float)atof(argv[i]);
            }
        } else
        if ((strcmp(argv[i], "--volume") == 0)||(strcmp(argv[i], "-v") == 0)) {
            ++i;
            if (i >= argc)
              {
                fprintf(stderr, "Please specify volume when using --volume or -v\n");
                return(1);
              }
            if ( argv[i] ) {
                volume = atoi(argv[i]);
            }
            if ( ( volume < 0 ) || ( volume > 100 ) ) {
              fprintf(stderr, "Volume must be between 0 and 100\n");
              volume = 100;
            }
        } else
        if ((strcmp(argv[i], "--title") == 0)||(strcmp(argv[i], "-t") == 0)) {
            ++i;
            if (i >= argc)
              {
                fprintf(stderr, "Please specify title when using --title or -t\n");
                return(1);
              }
            if ( argv[i] ) {
                title = argv[i];
            }
        } else
        if ((strcmp(argv[i], "--version") == 0) ||
            (strcmp(argv[i], "-V") == 0)) {
            SDL_GetVersion(&sdlver);
            SMPEG_VERSION(&smpegver);
            printf("SDL version: %d.%d.%d\n"
                   "SMPEG version: %d.%d.%d\n",
                   sdlver.major, sdlver.minor, sdlver.patch,
                   smpegver.major, smpegver.minor, smpegver.patch);
            return(0);
        } else
        if ((strcmp(argv[i], "--scale") == 0)||(strcmp(argv[i], "-s") == 0)) {
            ++i;
            if ( argv[i] ) {
                sscanf(argv[i], "%dx%d", &scale_width, &scale_height);
            }
        } else
        if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
            usage(argv[0]);
            return(0);
        } else {
            fprintf(stderr, "Warning: Unknown option: %s\n", argv[i]);
        }
    }
    /* If there were no arguments just print the usage */
    if (argc == 1) {
        usage(argv[0]);
        return(0);
    }

#if defined(linux) || defined(__FreeBSD__) /* Plaympeg doesn't need a mouse */
    putenv("SDL_NOMOUSE=1");
#endif

    context.lock = SDL_CreateMutex();

    /* Play the mpeg files! */
    status = 0;
    for ( ; argv[i]; ++i ) {
        /* Initialize SDL */
        if ( use_video ) {
          if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "Warning: Couldn't init SDL video: %s\n",
                    SDL_GetError());
            fprintf(stderr, "Will ignore video stream\n");
            use_video = 0;
          }
        }
        
        if ( use_audio ) {
          if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "Warning: Couldn't init SDL audio: %s\n",
                    SDL_GetError());
            fprintf(stderr, "Will ignore audio stream\n");
            use_audio = 0;
          }
        }

        /* Allow Ctrl-C when there's no video output */
        signal(SIGINT, next_movie);
        
        /* Create the MPEG stream */
#ifdef NET_SUPPORT
#ifdef RAW_SUPPORT
        /* Check if source is an IP address and port*/
        if((fd = raw_open(argv[i])) != 0)
          mpeg = SMPEG_new_descr(fd, &info, use_audio);
        else
#endif
#ifdef HTTP_SUPPORT
        /* Check if source is an http URL */
        if((fd = http_open(argv[i])) != 0)
          mpeg = SMPEG_new_descr(fd, &info, use_audio);
        else
#endif
#ifdef FTP_SUPPORT
        /* Check if source is an http URL */
        if((fd = ftp_open(argv[i])) != 0)
          mpeg = SMPEG_new_descr(fd, &info, use_audio);
        else
#endif
#endif
#ifdef VCD_SUPPORT
        /* Check if source is a CDROM device */
        if((fd = vcd_open(argv[i])) != 0)
          mpeg = SMPEG_new_descr(fd, &info, use_audio);
        else
#endif
        {
          if(strcmp(argv[i], "-") == 0) /* Use stdin for input */
            mpeg = SMPEG_new_descr(0, &info, use_audio);
          else
            mpeg = SMPEG_new(argv[i], &info, use_audio);
        }

        if ( SMPEG_error(mpeg) ) {
            fprintf(stderr, "%s: %s\n", argv[i], SMPEG_error(mpeg));
            SMPEG_delete(mpeg);
            status = -1;
            continue;
        }
        SMPEG_enableaudio(mpeg, use_audio);
        SMPEG_enablevideo(mpeg, use_video);
        SMPEG_setvolume(mpeg, volume);

        /* Print information about the video */
        basefile = strrchr(argv[i], '/');
        if ( basefile ) {
            ++basefile;
        } else {
            basefile = argv[i];
        }
        if ( info.has_audio && info.has_video ) {
            printf("%s: MPEG system stream (audio/video)\n", basefile);
        } else if ( info.has_audio ) {
            printf("%s: MPEG audio stream\n", basefile);
        } else if ( info.has_video ) {
            printf("%s: MPEG video stream\n", basefile);
        }
        if ( info.has_video ) {
            printf("\tVideo %dx%d resolution\n", info.width, info.height);
        }
        if ( info.has_audio ) {
            printf("\tAudio %s\n", info.audio_string);
        }
        if ( info.total_size ) {
            printf("\tSize: %d\n", info.total_size);
        }
        if ( info.total_time ) {
            printf("\tTotal time: %f\n", info.total_time);
        }

        /* Set up video display if needed */
        if ( info.has_video && use_video ) {
            Uint32 window_flags;
            int width, height;

            if ( scale_width ) {
                width = scale_width;
            } else {
                width = info.width;
            }
            width *= scalesize;
            if ( scale_height ) {
                height = scale_height;
            } else {
                height = info.height;
            }
            height *= scalesize;
            window_flags = SDL_WINDOW_RESIZABLE;
            if ( fullscreen ) {
                window_flags |= SDL_WINDOW_FULLSCREEN;
            }
            if (window) {
                SDL_SetWindowSize(window, width, height);
                SDL_DestroyTexture(texture);
            } else {
                window = SDL_CreateWindow(title ? title : argv[i],
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          width, height, window_flags);
                if ( window == NULL ) {
                    fprintf(stderr, "Unable to create %dx%d window: %s\n",
                                            width, height, SDL_GetError());
                    continue;
                }
                renderer = SDL_CreateRenderer(window, -1, 0);
            }
            if ( fullscreen ) {
                SDL_ShowCursor(0);
            }

            if (filtering) {
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            } else {
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
            }

            texture_width = (info.width + 15) & ~15;
            texture_height = (info.height + 15) & ~15;
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, texture_width, texture_height);

            context.frameCount = frameCount = 0;
            SMPEG_setdisplay(mpeg, update, &context, context.lock);
        } else {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            use_video = 0;
        }

        /* Set any special playback parameters */
        if ( loop_play ) {
            SMPEG_loop(mpeg, 1);
        }

        /* Seek starting position */
        if(seek) SMPEG_seek(mpeg, seek);

        /* Skip seconds to starting position */
        if(skip) SMPEG_skip(mpeg, skip);
        
        /* Play it, and wait for playback to complete */
        SMPEG_play(mpeg);
        done = 0;
        pause = 0;
        while ( ! done && ( pause || (SMPEG_status(mpeg) == SMPEG_PLAYING) ) ) {
            SDL_Event event;

            while ( use_video && SDL_PollEvent(&event) ) {
                switch (event.type) {
                    case SDL_WINDOWEVENT:
                        if ( event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ) {
                            SDL_RenderSetViewport(renderer, NULL);
                        }
                        break;
                    case SDL_KEYDOWN:
                        if ( (event.key.keysym.sym == SDLK_ESCAPE) || (event.key.keysym.sym == SDLK_q) ) {
                          // Quit
                          done = 1;
                        } else if ( event.key.keysym.sym == SDLK_RETURN ) {
                          // toggle fullscreen
                          if ( event.key.keysym.mod & KMOD_ALT ) {
                            fullscreen = !fullscreen;
                            if (SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0) < 0) {
                                fullscreen = !fullscreen;
                            }
                            SDL_ShowCursor(!fullscreen); 
                          }
                        } else if ( event.key.keysym.sym == SDLK_UP ) {
                          // Volume up
                          if ( volume < 100 ) {
                            if ( event.key.keysym.mod & KMOD_SHIFT ) {   // 10+
                              volume += 10;
                            } else if ( event.key.keysym.mod & KMOD_CTRL ) { // 100+
                              volume = 100;
                            } else {                                     // 1+
                              volume++;
                            }
                            if ( volume > 100 ) 
                              volume = 100;
                            SMPEG_setvolume(mpeg, volume);
                          }
                        } else if ( event.key.keysym.sym == SDLK_DOWN ) {
                          // Volume down
                          if ( volume > 0 ) {
                            if ( event.key.keysym.mod & KMOD_SHIFT ) {
                              volume -= 10;
                            } else if ( event.key.keysym.mod & KMOD_CTRL ) {
                              volume = 0;
                            } else {
                              volume--;
                            }
                            if ( volume < 0 ) 
                              volume = 0;
                            SMPEG_setvolume(mpeg, volume);
                          }
                        } else if ( event.key.keysym.sym == SDLK_PAGEUP ) {
                          // Full volume
                          volume = 100;
                          SMPEG_setvolume(mpeg, volume);
                        } else if ( event.key.keysym.sym == SDLK_PAGEDOWN ) {
                          // Volume off
                          volume = 0;
                          SMPEG_setvolume(mpeg, volume);
                        } else if ( event.key.keysym.sym == SDLK_SPACE ) {
                          // Toggle play / pause
                          if ( SMPEG_status(mpeg) == SMPEG_PLAYING ) {
                            SMPEG_pause(mpeg);
                            pause = 1;
                          } else {
                            SMPEG_play(mpeg);
                            pause = 0;
                          }
                        } else if ( event.key.keysym.sym == SDLK_RIGHT ) {
                          // Forward
                          if ( event.key.keysym.mod & KMOD_SHIFT ) {
                            SMPEG_skip(mpeg, 100);
                          } else if ( event.key.keysym.mod & KMOD_CTRL ) {
                            SMPEG_skip(mpeg, 50);
                          } else {
                            SMPEG_skip(mpeg, 5);
                          }
                        } else if ( event.key.keysym.sym == SDLK_LEFT ) {
                          // Reverse
                          if ( event.key.keysym.mod & KMOD_SHIFT ) {

                          } else if ( event.key.keysym.mod & KMOD_CTRL ) {

                          } else {

                          }
                        } else if ( event.key.keysym.sym == SDLK_KP_MINUS ) {
                          // Scale minus
                          if ( scalesize > 1 ) {
                            scalesize--;
                          }
                        } else if ( event.key.keysym.sym == SDLK_KP_PLUS ) {
                          // Scale plus
                          scalesize++;
                        }
                        break;
                    case SDL_QUIT:
                        done = 1;
                        break;
                    default:
                        break;
                }
            }

            if (use_video && context.frameCount > frameCount) {
                SDL_Rect src;

                SDL_mutexP(context.lock);
                SDL_assert(context.frame->image_width == texture_width);
                SDL_assert(context.frame->image_height == texture_height);
                SDL_UpdateTexture(texture, NULL, context.frame->image, context.frame->image_width);
                frameCount = context.frameCount;
                SDL_mutexV(context.lock);

                src.x = 0;
                src.y = 0;
                src.w = info.width;
                src.h = info.height;
                SDL_RenderCopy(renderer, texture, &src, NULL);

                SDL_RenderPresent(renderer);
            } else {
                SDL_Delay(0);
            }
        }

        SMPEG_delete(mpeg);
    }
    SDL_DestroyMutex(context.lock);
    SDL_Quit();

#if defined(RAW_SUPPORT) || defined(HTTP_SUPPORT) || defined(FTP_SUPPORT) || \
    defined(VCD_SUPPORT)
    if(fd) close(fd);
#endif

    return(status);
}
