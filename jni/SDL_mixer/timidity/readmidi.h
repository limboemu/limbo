/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

typedef struct {
  MidiEvent event;
  void *next;
} MidiEventList;

extern int32 quietchannels;

extern MidiEvent *read_midi_file(SDL_RWops *mrw, int32 *count, int32 *sp);

extern char midi_name[FILENAME_MAX+1];
