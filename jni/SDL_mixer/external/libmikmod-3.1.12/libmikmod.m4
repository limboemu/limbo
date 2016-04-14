# Configure paths for libmikmod
#
# Derived from glib.m4 (Owen Taylor 97-11-3)
# Improved by Chris Butler
#

dnl AM_PATH_LIBMIKMOD([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libmikmod, and define LIBMIKMOD_CFLAGS, LIBMIKMOD_LIBS and
dnl LIBMIKMOD_LDADD
dnl
AC_DEFUN([AM_PATH_LIBMIKMOD],
[dnl 
dnl Get the cflags and libraries from the libmikmod-config script
dnl
AC_ARG_WITH(libmikmod-prefix,[  --with-libmikmod-prefix=PFX   Prefix where libmikmod is installed (optional)],
            libmikmod_config_prefix="$withval", libmikmod_config_prefix="")
AC_ARG_WITH(libmikmod-exec-prefix,[  --with-libmikmod-exec-prefix=PFX Exec prefix where libmikmod is installed (optional)],
            libmikmod_config_exec_prefix="$withval", libmikmod_config_exec_prefix="")
AC_ARG_ENABLE(libmikmodtest, [  --disable-libmikmodtest       Do not try to compile and run a test libmikmod program],
		    , enable_libmikmodtest=yes)

  if test x$libmikmod_config_exec_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --exec-prefix=$libmikmod_config_exec_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_exec_prefix/bin/libmikmod-config
     fi
  fi
  if test x$libmikmod_config_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --prefix=$libmikmod_config_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_prefix/bin/libmikmod-config
     fi
  fi

  AC_PATH_PROG(LIBMIKMOD_CONFIG, libmikmod-config, no)
  min_libmikmod_version=ifelse([$1], ,3.1.5,$1)
  AC_MSG_CHECKING(for libmikmod - version >= $min_libmikmod_version)
  no_libmikmod=""
  if test "$LIBMIKMOD_CONFIG" = "no" ; then
    no_libmikmod=yes
  else
    LIBMIKMOD_CFLAGS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --cflags`
    LIBMIKMOD_LIBS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --libs`
    LIBMIKMOD_LDADD=`$LIBMIKMOD_CONFIG $libmikmod_config_args --ldadd`
    libmikmod_config_major_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    libmikmod_config_minor_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    libmikmod_config_micro_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`
    if test "x$enable_libmikmodtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
	  AC_LANG_SAVE
	  AC_LANG_C
      CFLAGS="$CFLAGS $LIBMIKMOD_CFLAGS $LIBMIKMOD_LDADD"
      LIBS="$LIBMIKMOD_LIBS $LIBS"
dnl
dnl Now check if the installed libmikmod is sufficiently new. (Also sanity
dnl checks the results of libmikmod-config to some extent
dnl
      rm -f conf.mikmodtest
      AC_TRY_RUN([
#include <mikmod.h>
#include <stdio.h>
#include <stdlib.h>

char* my_strdup (char *str)
{
  char *new_str;

  if (str) {
    new_str = malloc ((strlen (str) + 1) * sizeof(char));
    strcpy (new_str, str);
  } else
    new_str = NULL;

  return new_str;
}

int main()
{
  int major,minor,micro;
  int libmikmod_major_version,libmikmod_minor_version,libmikmod_micro_version;
  char *tmp_version;

  system("touch conf.mikmodtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_libmikmod_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libmikmod_version");
     exit(1);
   }

  libmikmod_major_version=(MikMod_GetVersion() >> 16) & 255;
  libmikmod_minor_version=(MikMod_GetVersion() >>  8) & 255;
  libmikmod_micro_version=(MikMod_GetVersion()      ) & 255;

  if ((libmikmod_major_version != $libmikmod_config_major_version) ||
      (libmikmod_minor_version != $libmikmod_config_minor_version) ||
      (libmikmod_micro_version != $libmikmod_config_micro_version))
    {
      printf("\n*** 'libmikmod-config --version' returned %d.%d.%d, but libmikmod (%d.%d.%d)\n", 
             $libmikmod_config_major_version, $libmikmod_config_minor_version, $libmikmod_config_micro_version,
             libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
      printf ("*** was found! If libmikmod-config was correct, then it is best\n");
      printf ("*** to remove the old version of libmikmod. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If libmikmod-config was wrong, set the environment variable LIBMIKMOD_CONFIG\n");
      printf("*** to point to the correct copy of libmikmod-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((libmikmod_major_version != LIBMIKMOD_VERSION_MAJOR) ||
	   (libmikmod_minor_version != LIBMIKMOD_VERSION_MINOR) ||
           (libmikmod_micro_version != LIBMIKMOD_REVISION))
    {
      printf("*** libmikmod header files (version %d.%d.%d) do not match\n",
	     LIBMIKMOD_VERSION_MAJOR, LIBMIKMOD_VERSION_MINOR, LIBMIKMOD_REVISION);
      printf("*** library (version %d.%d.%d)\n",
	     libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
    }
  else
    {
      if ((libmikmod_major_version > major) ||
        ((libmikmod_major_version == major) && (libmikmod_minor_version > minor)) ||
        ((libmikmod_major_version == major) && (libmikmod_minor_version == minor) && (libmikmod_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libmikmod (%d.%d.%d) was found.\n",
               libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
        printf("*** You need a version of libmikmod newer than %d.%d.%d.\n",
	       major, minor, micro);
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the libmikmod-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of libmikmod, but you can also set the LIBMIKMOD_CONFIG environment to point to the\n");
        printf("*** correct copy of libmikmod-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_libmikmod=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
	   AC_LANG_RESTORE
     fi
  fi
  if test "x$no_libmikmod" = x ; then
     AC_MSG_RESULT([yes, `$LIBMIKMOD_CONFIG --version`])
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBMIKMOD_CONFIG" = "no" ; then
       echo "*** The libmikmod-config script installed by libmikmod could not be found"
       echo "*** If libmikmod was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBMIKMOD_CONFIG environment variable to the"
       echo "*** full path to libmikmod-config."
     else
       if test -f conf.mikmodtest ; then
        :
       else
          echo "*** Could not run libmikmod test program, checking why..."
          CFLAGS="$CFLAGS $LIBMIKMOD_CFLAGS"
          LIBS="$LIBS $LIBMIKMOD_LIBS"
		  AC_LANG_SAVE
		  AC_LANG_C
          AC_TRY_LINK([
#include <mikmod.h>
#include <stdio.h>
],      [ return (MikMod_GetVersion()!=0); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding libmikmod or finding the wrong"
          echo "*** version of libmikmod. If it is not finding libmikmod, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location. Also, make sure you have run ldconfig if that"
          echo "*** is required on your system."
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means libmikmod was incorrectly installed"
          echo "*** or that you have moved libmikmod since it was installed. In the latter case, you"
          echo "*** may want to edit the libmikmod-config script: $LIBMIKMOD_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
		  AC_LANG_RESTORE
       fi
     fi
     LIBMIKMOD_CFLAGS=""
     LIBMIKMOD_LIBS=""
     LIBMIKMOD_LDADD=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBMIKMOD_CFLAGS)
  AC_SUBST(LIBMIKMOD_LIBS)
  AC_SUBST(LIBMIKMOD_LDADD)
  rm -f conf.mikmodtest
])
