
#ifndef LIMBO_COMPAT_SIGNALS_H
#define LIMBO_COMPAT_SIGNALS_H

struct timespec;

#ifdef __cplusplus
extern "C" {
#endif
int __rt_sigtimedwait(const sigset_t*, siginfo_t*, const timespec*, size_t);
int sigtimedwait(const sigset_t* set, siginfo_t* info, const timespec* timeout);
#ifdef __cplusplus
}
#endif

#endif