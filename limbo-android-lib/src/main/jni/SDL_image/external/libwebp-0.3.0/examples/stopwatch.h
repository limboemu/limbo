// Copyright 2011 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//  Helper functions to measure elapsed time.
//
// Author: Mikolaj Zalewski (mikolajz@google.com)

#ifndef WEBP_EXAMPLES_STOPWATCH_H_
#define WEBP_EXAMPLES_STOPWATCH_H_

#if defined _WIN32 && !defined __GNUC__
#include <windows.h>

typedef LARGE_INTEGER Stopwatch;

static WEBP_INLINE double StopwatchReadAndReset(Stopwatch* watch) {
  const LARGE_INTEGER old_value = *watch;
  LARGE_INTEGER freq;
  if (!QueryPerformanceCounter(watch))
    return 0.0;
  if (!QueryPerformanceFrequency(&freq))
    return 0.0;
  if (freq.QuadPart == 0)
    return 0.0;
  return (watch->QuadPart - old_value.QuadPart) / (double)freq.QuadPart;
}


#else    /* !_WIN32 */
#include <sys/time.h>

typedef struct timeval Stopwatch;

static WEBP_INLINE double StopwatchReadAndReset(Stopwatch* watch) {
  const struct timeval old_value = *watch;
  gettimeofday(watch, NULL);
  return watch->tv_sec - old_value.tv_sec +
      (watch->tv_usec - old_value.tv_usec) / 1000000.0;
}

#endif   /* _WIN32 */

#endif  /* WEBP_EXAMPLES_STOPWATCH_H_ */
