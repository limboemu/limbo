#ifndef _DLFCN_H_
#define	_DLFCN_H_

#define	RTLD_LAZY	1

extern void *dlopen(const char *, int);
extern int dlclose(void *);
extern void *dlsym(void *, const char *);

#endif	/* _DLFCN_H_ */
