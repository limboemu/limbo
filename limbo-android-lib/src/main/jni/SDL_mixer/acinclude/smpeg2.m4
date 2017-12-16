# Configure paths for SMPEG
# Nicolas Vignal 11/19/2000
# stolen from Sam Lantinga
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SMPEG2([MINIMUM-VERSION, [ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Test for SMPEG, and define SMPEG_CFLAGS and SMPEG_LIBS
dnl
AC_DEFUN([AM_PATH_SMPEG2],
[dnl
dnl Get the cflags and libraries from the smpeg2-config script
dnl
AC_ARG_WITH([smpeg-prefix],
            AS_HELP_STRING([--with-smpeg-prefix=PFX],
                           [Prefix where SMPEG is installed (optional)]),
            [smpeg_prefix="$withval"], [smpeg_prefix=""])
AC_ARG_WITH([smpeg-exec-prefix],
            AS_HELP_STRING([--with-smpeg-exec-prefix=PFX],
                           [Exec prefix where SMPEG is installed (optional)]),
            [smpeg_exec_prefix="$withval"], [smpeg_exec_prefix=""])
AC_ARG_ENABLE([smpegtest],
              AS_HELP_STRING([--disable-smpegtest],
                             [Do not try to compile and run a test SMPEG program]),
              [], [enable_smpegtest=yes])

  if test x$smpeg_exec_prefix != x ; then
     smpeg_args="$smpeg_args --exec-prefix=$smpeg_exec_prefix"
     if test x${SMPEG_CONFIG+set} != xset ; then
        SMPEG_CONFIG=$smpeg_exec_prefix/bin/smpeg2-config
     fi
  fi
  if test x$smpeg_prefix != x ; then
     smpeg_args="$smpeg_args --prefix=$smpeg_prefix"
     if test x${SMPEG_CONFIG+set} != xset ; then
        SMPEG_CONFIG=$smpeg_prefix/bin/smpeg2-config
     fi
  fi

  AC_PATH_PROG([SMPEG_CONFIG], [smpeg2-config], [no])
  min_smpeg_version=ifelse([$1], [], [2.0.0], [$1])
  AC_MSG_CHECKING([for SMPEG - version >= $min_smpeg_version])
  no_smpeg=""
  if test "$SMPEG_CONFIG" = "no" ; then
    no_smpeg=yes
  else
    SMPEG_CFLAGS=`$SMPEG_CONFIG $smpegconf_args --cflags`
    SMPEG_LIBS=`$SMPEG_CONFIG $smpegconf_args --libs`

    smpeg_major_version=`$SMPEG_CONFIG $smpeg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    smpeg_minor_version=`$SMPEG_CONFIG $smpeg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    smpeg_micro_version=`$SMPEG_CONFIG $smpeg_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_smpegtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SMPEG_CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SMPEG_LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SMPEG is sufficiently new. (Also sanity
dnl checks the results of smpeg2-config to some extent
dnl
      rm -f conf.smpegtest
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smpeg.h"

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.smpegtest");
  */
  { FILE *fp = fopen("conf.smpegtest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_smpeg_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_smpeg_version");
     exit(1);
   }

   if (($smpeg_major_version > major) ||
      (($smpeg_major_version == major) && ($smpeg_minor_version > minor)) ||
      (($smpeg_major_version == major) && ($smpeg_minor_version == minor) && ($smpeg_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'smpeg2-config --version' returned %d.%d.%d, but the minimum version\n", $smpeg_major_version, $smpeg_minor_version, $smpeg_micro_version);
      printf("*** of SMPEG required is %d.%d.%d. If smpeg2-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If smpeg2-config was wrong, set the environment variable SMPEG_CONFIG\n");
      printf("*** to point to the correct copy of smpeg2-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

]])],[],[no_smpeg=yes], [echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_smpeg" = x ; then
     AC_MSG_RESULT([yes])
     ifelse([$2], [], [:], [$2])
  else
     AC_MSG_RESULT([no])
     if test "$SMPEG_CONFIG" = "no" ; then
       echo "*** The smpeg2-config script installed by SMPEG could not be found"
       echo "*** If SMPEG was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SMPEG_CONFIG environment variable to the"
       echo "*** full path to smpeg2-config."
     else
       if test -f conf.smpegtest ; then
        :
       else
          echo "*** Could not run SMPEG test program, checking why..."
          CFLAGS="$CFLAGS $SMPEG_CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SMPEG_LIBS $SDL_LIBS"
          AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include "smpeg.h"
]],
        [[ return 0; ]])],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SMPEG or finding the wrong"
          echo "*** version of SMPEG. If it is not finding SMPEG, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SMPEG was incorrectly installed"
          echo "*** or that you have moved SMPEG since it was installed. In the latter case, you"
          echo "*** may want to edit the smpeg2-config script: $SMPEG_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SMPEG_CFLAGS=""
     SMPEG_LIBS=""
     ifelse([$3], [], [:], [$3])
  fi
  AC_SUBST([SMPEG_CFLAGS])
  AC_SUBST([SMPEG_LIBS])
  rm -f conf.smpegtest
])
