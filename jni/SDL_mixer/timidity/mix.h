/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

extern void mix_voice(int32 *buf, int v, int32 c);
extern int recompute_envelope(int v);
extern void apply_envelope_to_amp(int v);
