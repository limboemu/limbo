#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>

int strcasecmp(const char *__s1, const char *__s2)
{
	register unsigned char c1, c2;

	while (*__s1 && *__s2) {
		c1 = tolower(*__s1);
		c2 = tolower(*__s2);
		if (c1 != c2)
			return (int)(c1 - c2);
		__s1++;
		__s2++;
	}

	return (int)(*__s1 - *__s2);
}
