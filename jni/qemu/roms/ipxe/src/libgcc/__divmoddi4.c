#include "libgcc.h"

__libgcc int64_t __divmoddi4(int64_t num, int64_t den, int64 *rem_p)
{
  int minus = 0;
  int64_t v;

  if ( num < 0 ) {
    num = -num;
    minus = 1;
  }
  if ( den < 0 ) {
    den = -den;
    minus ^= 1;
  }

  v = __udivmoddi4(num, den, (uint64_t *)rem_p);
  if ( minus ) {
    v = -v;
    if ( rem_p )
      *rem_p = -(*rem_p);
  }

  return v;
}
