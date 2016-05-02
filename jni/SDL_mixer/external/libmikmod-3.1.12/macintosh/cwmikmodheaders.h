
	/* use pre-compiled headers, for speed */
#if __MWERKS__
#define MSL_USE_PRECOMPILED_HEADERS	1
#include <ansi_prefix.mac.h>
#endif

	/* standard headers */
#include <stdio.h>
#include <stdlib.h>

	/* defines */
#define HAVE_CONFIG_H 1

	/* missing function prototypes */
extern char *strdup(const char *);
extern int strcasecmp(const char *,const char *);
