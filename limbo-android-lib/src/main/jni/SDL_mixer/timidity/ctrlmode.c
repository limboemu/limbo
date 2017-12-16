/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

#include "config.h"
#include "ctrlmode.h"

#ifdef SDL
  extern ControlMode sdl_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &sdl_control_mode
# endif
#endif

ControlMode *ctl_list[]={
#ifdef SDL
  &sdl_control_mode,
#endif
  0
};

ControlMode *ctl=DEFAULT_CONTROL_MODE;
