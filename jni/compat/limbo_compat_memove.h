
#ifndef LIMBO_COMPAT_MEMMOVE_H
#define LIMBO_COMPAT_MEMMOVE_H

#include "string.h"

#ifdef memcpy
#undef memcpy
#endif
void*  memcpy(void * destination, const void * source, size_t length);

#endif
