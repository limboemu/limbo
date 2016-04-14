#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

char* strdup(const char *__s)
{
	char *charptr;

	if (!__s)
		return NULL;

	charptr=(char *)malloc(sizeof(char) * (strlen(__s) + 1));
	if (charptr) 
		strcpy(charptr, __s);

	return charptr;
}
