#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

int memcmp(const void *__s1, const void *__s2, size_t __n)
{
	const char *scan1, *scan2;
	size_t n;

	scan1 = __s1;
	scan2 = __s2;
	for (n = __n; n > 0; n--)
		if (*scan1 == *scan2) {
			scan1++;
			scan2++;
		} else
			return *scan1 - *scan2;

	return 0;
}
