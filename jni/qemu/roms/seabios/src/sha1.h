#ifndef __SHA1_H
#define __SHA1_H

#include "types.h" // u32

u32 sha1(const u8 *data, u32 length, u8 *hash);

#endif // sha1.h
