/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

/* Data format encoding bits */

#define PE_MONO 	0x01  /* versus stereo */
#define PE_SIGNED	0x02  /* versus unsigned */
#define PE_16BIT 	0x04  /* versus 8-bit */
#define PE_ULAW 	0x08  /* versus linear */
#define PE_BYTESWAP	0x10  /* versus the other way */

typedef struct {
  int32 rate, encoding;
  char *id_name;
} PlayMode;

extern PlayMode *play_mode_list[], *play_mode;
extern int init_buffers(int kbytes);

/* Conversion functions -- These overwrite the int32 data in *lp with
   data in another format */

/* The size of the output buffers */
extern int AUDIO_BUFFER_SIZE;

/* Actual copy function */
extern void (*s32tobuf)(void *dp, int32 *lp, int32 c);

/* 8-bit signed and unsigned*/
extern void s32tos8(void *dp, int32 *lp, int32 c);
extern void s32tou8(void *dp, int32 *lp, int32 c);

/* 16-bit */
extern void s32tos16(void *dp, int32 *lp, int32 c);
extern void s32tou16(void *dp, int32 *lp, int32 c);

/* byte-exchanged 16-bit */
extern void s32tos16x(void *dp, int32 *lp, int32 c);
extern void s32tou16x(void *dp, int32 *lp, int32 c);

/* uLaw (8 bits) */
extern void s32toulaw(void *dp, int32 *lp, int32 c);

/* little-endian and big-endian specific */
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define s32tou16l s32tou16
#define s32tou16b s32tou16x
#define s32tos16l s32tos16
#define s32tos16b s32tos16x
#else
#define s32tou16l s32tou16x
#define s32tou16b s32tou16
#define s32tos16l s32tos16x
#define s32tos16b s32tos16
#endif
