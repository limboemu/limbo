
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

//Limbo: can't find NDK headers so we include them
char *gettext(const char *msgid);

char *dgettext(const char *domainname, const char *msgid);

char *dcgettext(const char *domainname, const char *msgid, int category);

char *ngettext(const char *msgid1, const char *msgid2, unsigned long int n);

char *dngettext(const char *domainname, const char *msgid1, const char *msgid2, unsigned long int n);

char *dcngettext(const char *domainname, const char *msgid1, const char *msgid2, unsigned long int n, int category);

char *textdomain(const char *domainname);

char *bindtextdomain(const char *domainname, const char *dirname);

char *bind_textdomain_codeset(const char *domainname, const char *codeset);
