AC_INIT([libwebp], [0.3.0],
        [http://code.google.com/p/webp/issues],,
        [http://developers.google.com/speed/webp])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

dnl === automake >= 1.12 requires this for 'unusual archivers' support.
dnl === it must occur before LT_INIT (AC_PROG_LIBTOOL).
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_LIBTOOL
AM_PROG_CC_C_O

dnl === Enable less verbose output when building.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl === SET_IF_UNSET(shell_var, value)
dnl ===   Set the shell variable 'shell_var' to 'value' if it is unset.
AC_DEFUN([SET_IF_UNSET], [test "${$1+set}" = "set" || $1=$2])

AC_ARG_ENABLE([everything],
              AS_HELP_STRING([--enable-everything],
                             [Enable all optional targets. These can still be
                              disabled with --disable-target]),
              [SET_IF_UNSET([enable_libwebpdecoder], [$enableval])
               SET_IF_UNSET([enable_libwebpdemux], [$enableval])
               SET_IF_UNSET([enable_libwebpmux], [$enableval])])

AC_ARG_WITH([pkgconfigdir], AS_HELP_STRING([--with-pkgconfigdir=DIR],
            [Path to the pkgconfig directory @<:@LIBDIR/pkgconfig@:>@]),
            [pkgconfigdir="$withval"], [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])

dnl === TEST_AND_ADD_CFLAGS(flag)
dnl ===   Checks whether $CC supports 'flag' and adds it to AM_CFLAGS on success.
AC_DEFUN([TEST_AND_ADD_CFLAGS],
         [SAVED_CFLAGS="$CFLAGS"
          CFLAGS="-Werror $1"
          AC_MSG_CHECKING([whether $CC supports $1])
          dnl Note AC_LANG_PROGRAM([]) uses an old-style main definition.
          AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main(void) { return 0; }])],
                            [AC_MSG_RESULT([yes])]
                            [AS_VAR_APPEND([AM_CFLAGS], [" $1"])],
                            [AC_MSG_RESULT([no])])
          CFLAGS="$SAVED_CFLAGS"])
TEST_AND_ADD_CFLAGS([-Wall])
TEST_AND_ADD_CFLAGS([-Wdeclaration-after-statement])
TEST_AND_ADD_CFLAGS([-Wextra])
TEST_AND_ADD_CFLAGS([-Wmissing-declarations])
TEST_AND_ADD_CFLAGS([-Wmissing-prototypes])
TEST_AND_ADD_CFLAGS([-Wold-style-definition])
TEST_AND_ADD_CFLAGS([-Wshadow])
TEST_AND_ADD_CFLAGS([-Wunused-but-set-variable])
TEST_AND_ADD_CFLAGS([-Wunused])
TEST_AND_ADD_CFLAGS([-Wvla])
AC_SUBST([AM_CFLAGS])

dnl === CLEAR_LIBVARS([var_pfx])
dnl ===   Clears <var_pfx>_{INCLUDES,LIBS}.
AC_DEFUN([CLEAR_LIBVARS], [$1_INCLUDES=""; $1_LIBS=""])

dnl === WITHLIB_OPTION([opt_pfx], [outvar_pfx])
dnl ===   Defines --with-<opt_pfx>{include,lib}dir options which set
dnl ===   the variables <outvar_pfx>_{INCLUDES,LIBS}.
AC_DEFUN([WITHLIB_OPTION],
  [AC_ARG_WITH([$1includedir],
               AS_HELP_STRING([--with-$1includedir=DIR],
                              [use $2 includes from DIR]),
               $2_INCLUDES="-I$withval")
   AC_ARG_WITH([$1libdir],
               AS_HELP_STRING([--with-$1libdir=DIR],
                              [use $2 libraries from DIR]),
               [$2_LIBS="-L$withval"])])

dnl === LIBCHECK_PROLOGUE([var_pfx])
dnl ===   Caches the current values of CPPFLAGS/LIBS in SAVED_* then
dnl ===   prepends the current values with <var_pfx>_{INCLUDES,LIBS}.
AC_DEFUN([LIBCHECK_PROLOGUE],
         [SAVED_CPPFLAGS=$CPPFLAGS
          SAVED_LIBS=$LIBS
          CPPFLAGS="$$1_INCLUDES $CPPFLAGS"
          LIBS="$$1_LIBS $LIBS"])

dnl === LIBCHECK_EPILOGUE([var_pfx])
dnl ===   Restores the values of CPPFLAGS/LIBS from SAVED_* and exports
dnl ===   <var_pfx>_{INCLUDES,LIBS} with AC_SUBST.
AC_DEFUN([LIBCHECK_EPILOGUE],
         [AC_SUBST($1_LIBS)
          AC_SUBST($1_INCLUDES)
          CPPFLAGS=$SAVED_CPPFLAGS
          LIBS=$SAVED_LIBS])

dnl === Check for pthread support
AC_ARG_ENABLE([threading],
              AS_HELP_STRING([--disable-threading],
                             [Disable detection of thread support]),,
              [enable_threading=yes])
if test "$enable_threading" = "yes"; then
  AC_MSG_NOTICE([checking for threading support...])
  AX_PTHREAD([AC_DEFINE([WEBP_USE_THREAD], [1],
                        [Undefine this to disable thread support.])
              LIBS="$PTHREAD_LIBS $LIBS"
              CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
              CC="$PTHREAD_CC"
             ],
             [enable_threading=no])
fi
AC_MSG_NOTICE([checking if threading is enabled... ${enable_threading-no}])

dnl === check for OpenGL/GLUT support ===
CLEAR_LIBVARS([GL])
WITHLIB_OPTION([gl], [GL])

LIBCHECK_PROLOGUE([GL])

glut_cflags="none"
glut_ldflags="none"
case $host_os in
  darwin*)
    # Special case for OSX builds. Append these to give the user a chance to
    # override with --with-gl*
    glut_cflags="$glut_cflags|-framework GLUT -framework OpenGL"
    glut_ldflags="$glut_ldflags|-framework GLUT -framework OpenGL"
    ;;
esac

GLUT_SAVED_CPPFLAGS="$CPPFLAGS"
SAVED_IFS="$IFS"
IFS="|"
for flag in $glut_cflags; do
  # restore IFS immediately as the autoconf macros may need the default.
  IFS="$SAVED_IFS"
  unset ac_cv_header_GL_glut_h
  unset ac_cv_header_OpenGL_glut_h

  case $flag in
    none) ;;
    *) CPPFLAGS="$flag $CPPFLAGS";;
  esac
  AC_CHECK_HEADERS([GL/glut.h GLUT/glut.h OpenGL/glut.h],
                   [glut_headers=yes;
                    test "$flag" = "none" || GL_INCLUDES="$CPPFLAGS";
                    break])
  CPPFLAGS="$GLUT_SAVED_CPPFLAGS"
  test "$glut_headers" = "yes" && break
done
IFS="$SAVED_IFS"

if test "$glut_headers" = "yes"; then
  AC_LANG_PUSH([C])
  GLUT_SAVED_LDFLAGS="$LDFLAGS"
  SAVED_IFS="$IFS"
  IFS="|"
  for flag in $glut_ldflags; do
    # restore IFS immediately as the autoconf macros may need the default.
    IFS="$SAVED_IFS"
    unset ac_cv_search_glBegin

    case $flag in
      none) ;;
      *) LDFLAGS="$flag $LDFLAGS";;
    esac

    # find libGL
    GL_SAVED_LIBS="$LIBS"
    AC_SEARCH_LIBS([glBegin], [GL OpenGL])
    LIBS="$GL_SAVED_LIBS"

    # A direct link to libGL may not be necessary on e.g., linux.
    for lib in "" $ac_cv_search_glBegin; do
      unset ac_cv_search_glutMainLoop
      AC_SEARCH_LIBS([glutMainLoop], [glut], [glut_support=yes], [], [$lib])
      if test "$glut_support" = "yes"; then
        GL_LIBS="$LDFLAGS"
        if test "$ac_cv_search_glutMainLoop" != "none required"; then
          GL_LIBS="$GL_LIBS $ac_cv_search_glutMainLoop"
        fi
        GL_LIBS="$GL_LIBS $lib"
        break
      fi
    done
    LDFLAGS="$GLUT_SAVED_LDFLAGS"
    test "$glut_support" = "yes" && break
  done
  IFS="$SAVED_IFS"
  AC_LANG_POP
fi

LIBCHECK_EPILOGUE([GL])

if test "$glut_support" = "yes" -a "$enable_libwebpdemux" = "yes"; then
  build_vwebp=yes
fi
AM_CONDITIONAL([BUILD_VWEBP], [test "$build_vwebp" = "yes"])

dnl === check for PNG support ===

CLEAR_LIBVARS([PNG])
AC_PATH_PROGS(LIBPNG_CONFIG,
              [libpng-config libpng15-config libpng14-config libpng12-config])
if test -n "$LIBPNG_CONFIG"; then
  PNG_INCLUDES=`$LIBPNG_CONFIG --cflags`
  PNG_PREFIX=`$LIBPNG_CONFIG --prefix`
  if test "${PNG_PREFIX}/lib" != "/usr/lib" ; then
    PNG_LIBS="-L${PNG_PREFIX}/lib"
  fi
  PNG_LIBS="$PNG_LIBS `$LIBPNG_CONFIG --libs`"
fi

WITHLIB_OPTION([png], [PNG])

LIBCHECK_PROLOGUE([PNG])
AC_CHECK_HEADER(png.h,
  AC_SEARCH_LIBS(png_get_libpng_ver, [png],
                 [test "$ac_cv_search_png_get_libpng_ver" = "none required" \
                    || PNG_LIBS="$PNG_LIBS $ac_cv_search_png_get_libpng_ver"
                  PNG_INCLUDES="$PNG_INCLUDES -DWEBP_HAVE_PNG"
                  AC_DEFINE(WEBP_HAVE_PNG, [1],
                            [Set to 1 if PNG library is installed])
                  png_support=yes
                 ],
                 [AC_MSG_WARN(Optional png library not found)
                  PNG_LIBS=""
                  PNG_INCLUDES=""
                 ],
                 [$MATH_LIBS]),
  [AC_MSG_WARN(png library not available - no png.h)
   PNG_LIBS=""
   PNG_INCLUDES=""
  ],
)
LIBCHECK_EPILOGUE([PNG])

dnl === check for JPEG support ===

CLEAR_LIBVARS([JPEG])
WITHLIB_OPTION([jpeg], [JPEG])

LIBCHECK_PROLOGUE([JPEG])
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_set_defaults,
               [JPEG_LIBS="$JPEG_LIBS -ljpeg"
                JPEG_INCLUDES="$JPEG_INCLUDES -DWEBP_HAVE_JPEG"
                AC_DEFINE(WEBP_HAVE_JPEG, [1],
                          [Set to 1 if JPEG library is installed])
                jpeg_support=yes
               ],
               AC_MSG_WARN(Optional jpeg library not found),
               [$MATH_LIBS]),
  AC_MSG_WARN(jpeg library not available - no jpeglib.h)
)
LIBCHECK_EPILOGUE([JPEG])

dnl === check for TIFF support ===

CLEAR_LIBVARS([TIFF])
WITHLIB_OPTION([tiff], [TIFF])

LIBCHECK_PROLOGUE([TIFF])
AC_CHECK_HEADER(tiffio.h,
  AC_CHECK_LIB(tiff, TIFFGetVersion,
               [TIFF_LIBS="$TIFF_LIBS -ltiff"
                TIFF_INCLUDES="$TIFF_INCLUDES -DWEBP_HAVE_TIFF"
                AC_DEFINE(WEBP_HAVE_TIFF, [1],
                          [Set to 1 if TIFF library is installed])
                tiff_support=yes
               ],
               AC_MSG_WARN(Optional tiff library not found),
               [$MATH_LIBS]),
  AC_MSG_WARN(tiff library not available - no tiffio.h)
)
LIBCHECK_EPILOGUE([TIFF])

dnl === check for GIF support ===

CLEAR_LIBVARS([GIF])
WITHLIB_OPTION([gif], [GIF])

LIBCHECK_PROLOGUE([GIF])
AC_CHECK_HEADER(gif_lib.h,
  AC_CHECK_LIB([gif], [DGifOpenFileHandle],
               [GIF_LIBS="$GIF_LIBS -lgif"
                gif_support=yes
               ],
               AC_MSG_WARN(Optional gif library not found),
               [$MATH_LIBS]),
  AC_MSG_WARN(gif library not available - no gif_lib.h)
)
LIBCHECK_EPILOGUE([GIF])

if test "$gif_support" = "yes" -a \
        "$enable_libwebpmux" = "yes"; then
  build_gif2webp=yes
fi
AM_CONDITIONAL([BUILD_GIF2WEBP], [test "${build_gif2webp}" = "yes"])

dnl === check for WIC support ===

if test "$target_os" = "mingw32"; then
  AC_CHECK_HEADERS([wincodec.h shlwapi.h windows.h])
  if test "$ac_cv_header_wincodec_h" = "yes"; then
    AC_MSG_CHECKING(for Windows Imaging Component support)
    SAVED_LIBS=$LIBS
    LIBS="-lshlwapi -lole32 $LIBS"
    # match include structure from [cd]webp.c
    wic_headers="
      #define INITGUID
      #define CINTERFACE
      #define COBJMACROS
      #define _WIN32_IE 0x500

      #include <shlwapi.h>
      #include <windows.h>
      #include <wincodec.h>
      "
    # test for functions from each lib and the GUID is created properly
    wic_main="
      int main(void) {
        CLSID_WICImagingFactory;
        CoInitialize(NULL);
        SHCreateStreamOnFile(NULL, 0, NULL);
        return 0;
      }
      "
    AC_LANG_PUSH(C)
    AC_LINK_IFELSE(
      [AC_LANG_SOURCE([
         $wic_headers
         $wic_main])],
      [wic_support=yes],
      [wic_support=no]
    )
    AC_LANG_POP

    test "$wic_support" = "yes" || LIBS=$SAVED_LIBS
    AC_MSG_RESULT(${wic_support-no})
  fi
fi

dnl === If --enable-swap-16bit-csp is defined, add -DWEBP_SWAP_16BIT_CSP

USE_SWAP_16BIT_CSP=""
AC_MSG_CHECKING(if --enable-swap-16bit-csp option is specified)
AC_ARG_ENABLE([swap-16bit-csp],
              AS_HELP_STRING([--enable-swap-16bit-csp],
                             [Enable byte swap for 16 bit colorspaces]))
if test "$enable_swap_16bit_csp" = "yes"; then
  USE_SWAP_16BIT_CSP="-DWEBP_SWAP_16BIT_CSP"
fi
AC_MSG_RESULT(${enable_swap_16bit_csp-no})
AC_SUBST(USE_SWAP_16BIT_CSP)

dnl === If --enable-experimental is defined, add -DWEBP_EXPERIMENTAL_FEATURES

USE_EXPERIMENTAL_CODE=""
AC_MSG_CHECKING(if --enable-experimental option is specified)
AC_ARG_ENABLE([experimental], AS_HELP_STRING([--enable-experimental],
                                             [Activate experimental features]))
if test "$enable_experimental" = "yes"; then
  AC_DEFINE(WEBP_EXPERIMENTAL_FEATURES, [1], [Enable experimental code])
  USE_EXPERIMENTAL_CODE="-DWEBP_EXPERIMENTAL_FEATURES"
fi
AC_MSG_RESULT(${enable_experimental-no})
AC_SUBST(USE_EXPERIMENTAL_CODE)

dnl === Check whether libwebpmux should be built
AC_MSG_CHECKING(whether libwebpmux is to be built)
AC_ARG_ENABLE([libwebpmux],
              AS_HELP_STRING([--enable-libwebpmux],
                             [Build libwebpmux @<:@default=no@:>@]))
AC_MSG_RESULT(${enable_libwebpmux-no})
AM_CONDITIONAL([WANT_MUX], [test "$enable_libwebpmux" = "yes"])

dnl === Check whether libwebpdemux should be built
AC_MSG_CHECKING(whether libwebpdemux is to be built)
AC_ARG_ENABLE([libwebpdemux],
              AS_HELP_STRING([--enable-libwebpdemux],
                             [Build libwebpdemux @<:@default=no@:>@]))
AC_MSG_RESULT(${enable_libwebpdemux-no})
AM_CONDITIONAL([WANT_DEMUX], [test "$enable_libwebpdemux" = "yes"])

dnl === Check whether decoder library should be built.
AC_MSG_CHECKING(whether decoder library is to be built)
AC_ARG_ENABLE([libwebpdecoder],
              AS_HELP_STRING([--enable-libwebpdecoder],
                             [Build libwebpdecoder @<:@default=no@:>@]))
AC_MSG_RESULT(${enable_libwebpdecoder-no})
AM_CONDITIONAL([BUILD_LIBWEBPDECODER], [test "$enable_libwebpdecoder" = "yes"])

dnl =========================

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile man/Makefile \
                 examples/Makefile src/dec/Makefile \
                 src/enc/Makefile src/dsp/Makefile \
                 src/demux/Makefile src/mux/Makefile \
                 src/utils/Makefile \
                 src/libwebp.pc src/libwebpdecoder.pc \
                 src/demux/libwebpdemux.pc src/mux/libwebpmux.pc])


AC_OUTPUT

AC_MSG_NOTICE([
WebP Configuration Summary
--------------------------

Shared libraries: ${enable_shared}
Static libraries: ${enable_static}
Threaded decode: ${enable_threading-no}
libwebp: yes
libwebpdecoder: ${enable_libwebpdecoder-no}
libwebpdemux: ${enable_libwebpdemux-no}
libwebpmux: ${enable_libwebpmux-no}

Tools:
cwebp : yes
  Input format support
  ====================
  JPEG : ${jpeg_support-no}
  PNG  : ${png_support-no}
  TIFF : ${tiff_support-no}
  WIC  : ${wic_support-no}
dwebp : yes
  Output format support
  =====================
  PNG  : ${png_support-no}
  WIC  : ${wic_support-no}
gif2webp : ${build_gif2webp-no}
webpmux  : ${enable_libwebpmux-no}
vwebp    : ${build_vwebp-no}
])
