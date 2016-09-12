/*
 * arch/i386/libgcc/__divdi3.c
 */

#include "libgcc.h"

__libgcc int64_t __divdi3(int64_t num, int64_t den)
{
  return __divmoddi4(num, den, NULL);
}
