###################################################################
## To: autoconf@gnu.org 
## Subject: socklen_t 
## From: lars brinkhoff <lars@nocrew.org> 
## Date: 26 Mar 1999 11:38:09 +0100 
## 
## Here's an attempt at a check for socklen_t.  AC_CHECK_TYPE doesn't
## work because it doesn't search <sys/socket.h>.  Maybe that macro
## should be changed instead.
## 
AC_DEFUN([AC_TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t], ac_cv_type_socklen_t,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
  #include <sys/socket.h>],
  [socklen_t x;],
  ac_cv_type_socklen_t=yes,
  ac_cv_type_socklen_t=no)
])
  if test $ac_cv_type_socklen_t != yes; then
    AC_DEFINE(socklen_t, int)
  fi
])
