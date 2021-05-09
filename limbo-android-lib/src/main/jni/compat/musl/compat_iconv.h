#ifndef _COMPAT_ICONV_H_
#define _COMPAT_ICONV_H_

#if __ANDROID_API__ < 28
#include "../musl/include/iconv.h"
#endif /* __ANDROID_API__ < 28 */

#endif
