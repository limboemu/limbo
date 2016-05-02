#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

char *strstr(const char *haystack, const char *needle)
{
	const char *scan;
	size_t len;
	char firstc;

	firstc = *needle;
	len = strlen(needle);
	for (scan = haystack; *scan != firstc || strncmp(scan, needle, len); )
		if (!*scan++)
			return NULL;
	return (char *)scan;
}
