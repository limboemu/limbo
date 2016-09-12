/*
 * arch/i386/libgcc/__moddi3.c
 */

#include "libgcc.h"

__libgcc int64_t __moddi3(int64_t num, int64_t den)
{
  int64_t v;

  (void) __divmoddi4(num, den, &v);
  return v;
}
