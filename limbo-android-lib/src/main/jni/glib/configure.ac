# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.62])

dnl ***********************************
dnl *** include special GLib macros ***
dnl ***********************************

m4_define(glib_configure_ac)

#
# The following version number definitions apply to GLib, GModule, GObject,
# GThread and GIO as a whole, so if changes occurred in any of them, they are
# all treated with the same interface and binary age.
#
# Making releases:
#   glib_micro_version += 1;
#   glib_interface_age += 1;
#   glib_binary_age += 1;
# if any functions have been added, set glib_interface_age to 0.
# if backwards compatibility has been broken,
# set glib_binary_age _and_ glib_interface_age to 0.
#
m4_define([glib_major_version], [2])
m4_define([glib_minor_version], [26])
m4_define([glib_micro_version], [0])
m4_define([glib_interface_age], [0])
m4_define([glib_binary_age],
          [m4_eval(100 * glib_minor_version + glib_micro_version)])
m4_define([glib_version],
          [glib_major_version.glib_minor_version.glib_micro_version])

# libtool version related macros
m4_define([glib_lt_release], [glib_major_version.glib_minor_version])
m4_define([glib_lt_current],
          [m4_eval(100 * glib_minor_version + glib_micro_version - glib_interface_age)])
m4_define([glib_lt_revision], [glib_interface_age])
m4_define([glib_lt_age], [m4_eval(glib_binary_age - glib_interface_age)])
m4_define([glib_lt_current_minus_age],
          [m4_eval(glib_lt_current - glib_lt_age)])

# if the minor version number is odd, then we want debugging.  Otherwise
# we only want minimal debugging support.
m4_define([glib_debug_default],
          [m4_if(m4_eval(glib_minor_version % 2), [1], [yes], [minimum])])dnl


AC_INIT(glib, [glib_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=glib])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([glib/glib.h])

# Save this value here, since automake will set cflags later
cflags_set=${CFLAGS+set}

AM_INIT_AUTOMAKE([1.10 no-define])

# Support silent build rules, requires at least automake-1.11. Enable
# by either passing --enable-silent-rules to configure or passing V=0
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])

GLIB_MAJOR_VERSION=glib_major_version
GLIB_MINOR_VERSION=glib_minor_version
GLIB_MICRO_VERSION=glib_micro_version
GLIB_INTERFACE_AGE=glib_interface_age
GLIB_BINARY_AGE=glib_binary_age
GLIB_VERSION=glib_version

AC_SUBST(GLIB_MAJOR_VERSION)
AC_SUBST(GLIB_MINOR_VERSION)
AC_SUBST(GLIB_MICRO_VERSION)
AC_SUBST(GLIB_VERSION)
AC_SUBST(GLIB_INTERFACE_AGE)
AC_SUBST(GLIB_BINARY_AGE)

AC_DEFINE(GLIB_MAJOR_VERSION, [glib_major_version],
	  [Define to the GLIB major version])
AC_DEFINE(GLIB_MINOR_VERSION, [glib_minor_version],
	  [Define to the GLIB minor version])
AC_DEFINE(GLIB_MICRO_VERSION, [glib_micro_version],
	  [Define to the GLIB micro version])
AC_DEFINE(GLIB_INTERFACE_AGE, [glib_interface_age],
	  [Define to the GLIB interface age])
AC_DEFINE(GLIB_BINARY_AGE, [glib_binary_age],
	  [Define to the GLIB binary age])

# libtool versioning
LT_RELEASE=glib_lt_release
LT_CURRENT=glib_lt_current
LT_REVISION=glib_lt_revision
LT_AGE=glib_lt_age
LT_CURRENT_MINUS_AGE=glib_lt_current_minus_age
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP

AC_CANONICAL_HOST

AC_MSG_CHECKING([for the BeOS])
case $host in
  *-*-beos*)
    glib_native_beos="yes"
    ;;
  *)
    glib_native_beos="no"
    ;;
esac
AC_MSG_RESULT([$glib_native_beos])

dnl

AC_MSG_CHECKING([for Win32])
LIB_EXE_MACHINE_FLAG=X86
case "$host" in
  *-*-mingw*)
    glib_native_win32=yes
    glib_pid_type='void *'
    glib_cv_stack_grows=no
    # Unfortunately the mingw implementations of C99-style snprintf and vsnprintf
    # don't seem to be quite good enough, at least not in mingw-runtime-3.14.
    # (Sorry, I don't know exactly what is the problem, but it is related to
    # floating point formatting and decimal point vs. comma.)
    # The simple tests in AC_FUNC_VSNPRINTF_C99 and AC_FUNC_SNPRINTF_C99 aren't
    # rigorous enough to notice, though.
    # So preset the autoconf cache variables.
    ac_cv_func_vsnprintf_c99=no
    ac_cv_func_snprintf_c99=no
    case "$host" in
    x86_64-*-*)
      LIB_EXE_MACHINE_FLAG=X64
      ;;
    esac
    ;;
  *)
    glib_native_win32=no
    glib_pid_type=int
    ;;
esac
case $host in
  *-*-linux*)
    glib_os_linux=yes
    ;;
esac

AC_MSG_RESULT([$glib_native_win32])

AC_SUBST(LIB_EXE_MACHINE_FLAG)

glib_have_carbon=no
AC_MSG_CHECKING([for Mac OS X Carbon support])
AC_TRY_CPP([
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
], glib_have_carbon=yes)

AC_MSG_RESULT([$glib_have_carbon])

AM_CONDITIONAL(OS_WIN32, [test "$glib_native_win32" = "yes"])
AM_CONDITIONAL(OS_WIN32_X64, [test "$LIB_EXE_MACHINE_FLAG" = "X64"])
AM_CONDITIONAL(OS_UNIX, [test "$glib_native_win32" != "yes"])
AM_CONDITIONAL(OS_LINUX, [test "$glib_os_linux" = "yes"])
AM_CONDITIONAL(OS_CARBON, [test "$glib_have_carbon" = "yes"])

if test "$glib_native_win32" = "yes"; then
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test "$WINDRES" = no; then
    AC_MSG_ERROR([*** Could not find an implementation of windres in your PATH.])
  fi
  AC_CHECK_TOOL(NM, nm, no)
  if test "$NM" = no; then
    AC_MSG_ERROR([*** Could not find an implementation of nm in your PATH.])
  fi
  AC_CHECK_TOOL(RANLIB, ranlib, :)
  AC_CHECK_PROG(ms_librarian, [lib.exe], [yes], [no])
fi
AM_CONDITIONAL(MS_LIB_AVAILABLE, [test x$ms_librarian = xyes])

if test "$glib_native_win32" != yes; then
    # libtool option to control which symbols are exported
    # right now, symbols starting with _ are not exported
    LIBTOOL_EXPORT_OPTIONS='-export-symbols-regex "^g.*"'
else
    # We currently use .def files on Windows
    LIBTOOL_EXPORT_OPTIONS=
fi
AC_SUBST(LIBTOOL_EXPORT_OPTIONS)

if test "x$glib_have_carbon" = "xyes"; then
  AC_DEFINE(HAVE_CARBON, 1, [define to 1 if Carbon is available])
  LDFLAGS="$LDFLAGS -framework Carbon"
fi


dnl declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                             [turn on debugging @<:@default=glib_debug_default@:>@]),,
              enable_debug=glib_debug_default)

AC_ARG_ENABLE(gc_friendly,
              [AC_HELP_STRING([--enable-gc-friendly],
                              [turn on garbage collector friendliness [default=no]])],,
              [enable_gc_friendly=no])
AC_ARG_ENABLE(mem_pools,
              [AC_HELP_STRING([--disable-mem-pools],
			      [disable all glib memory pools])],,
	      [disable_mem_pools=no])
AC_ARG_ENABLE(threads,
              [AC_HELP_STRING([--enable-threads],
                              [turn on basic thread support [default=yes]
([=no] will override --with-threads)])],,
              [enable_threads=yes])
AC_ARG_ENABLE(rebuilds,
              [AC_HELP_STRING([--disable-rebuilds],
                              [disable all source autogeneration rules])],,
              [enable_rebuilds=yes])
AC_ARG_ENABLE(visibility,
              [AC_HELP_STRING([--disable-visibility],
                              [don't use ELF visibility attributes])],,
              [enable_visibility=yes])

if test "x$enable_threads" != "xyes"; then
  enable_threads=no
fi

AC_MSG_CHECKING([whether to enable garbage collector friendliness])
if test "x$enable_gc_friendly" = "xyes"; then
  AC_DEFINE(ENABLE_GC_FRIENDLY_DEFAULT, 1, [Whether to enable GC friendliness by default])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether to disable memory pools])
if test "x$disable_mem_pools" = "xno"; then
  AC_MSG_RESULT([no])
else
  AC_DEFINE(DISABLE_MEM_POOLS, [1], [Whether to disable memory pools])
  AC_SUBST(DISABLE_MEM_POOLS)
  AC_MSG_RESULT([yes])
fi

dnl location to install runtime libraries, e.g. ../../lib to install
dnl to /lib if libdir is /usr/lib
AC_ARG_WITH(runtime-libdir,
           [AC_HELP_STRING([--with-runtime-libdir=RELPATH],
                           [Install runtime libraries relative to libdir])],
           [],
	   [with_runtime_libdir=""])
GLIB_RUNTIME_LIBDIR="$with_runtime_libdir"
ABS_GLIB_RUNTIME_LIBDIR="`readlink -f $libdir/$with_runtime_libdir`"
AC_SUBST(GLIB_RUNTIME_LIBDIR)
AC_SUBST(ABS_GLIB_RUNTIME_LIBDIR)
AM_CONDITIONAL(HAVE_GLIB_RUNTIME_LIBDIR, [test "x$with_runtime_libdir" != "x"])

dnl Check for a working C++ compiler, but do not bail out, if none is found.
AC_CHECK_TOOLS(CXX, [$CCC c++ g++ gcc CC cxx cc++ cl], [gcc])
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE(,[class a { int b; } c;], ,CXX=)
AM_CONDITIONAL(HAVE_CXX, [test "$CXX" != ""])
AC_LANG_RESTORE

AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG(0.16)

if test "x$enable_debug" = "xyes"; then
  if test x$cflags_set != xset ; then
      case " $CFLAGS " in
      *[[\ \	]]-g[[\ \	]]*) ;;
      *) CFLAGS="$CFLAGS -g" ;;
      esac
  fi
	
  GLIB_DEBUG_FLAGS="-DG_ENABLE_DEBUG"
else
  GLIB_DEBUG_FLAGS="-DG_DISABLE_CAST_CHECKS"

  if test "x$enable_debug" = "xno"; then
    GLIB_DEBUG_FLAGS="$GLIB_DEBUG_FLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS"
  fi
fi

if test "x$enable_visibility" = "xno"; then
  GLIB_DEBUG_FLAGS="$GLIB_DEBUG_FLAGS -DDISABLE_VISIBILITY"
fi

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
if test x"$glib_native_win32" = xyes; then
  if test x"$GCC" = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
	if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
	  msnative_struct='-fnative-struct'
	fi
	;;
      *)
	if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
	  msnative_struct='-mms-bitfields'
	fi
	;;
    esac
    if test x"$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
fi
GLIB_EXTRA_CFLAGS="${msnative_struct}"
AC_SUBST(GLIB_EXTRA_CFLAGS)

AC_EXEEXT

# define a MAINT-like variable REBUILD which is set if Perl
# and awk are found, so autogenerated sources can be rebuilt
AC_PROG_AWK
AC_CHECK_PROGS(PERL, [perl5 perl])
# We would like indent, but don't require it.
AC_CHECK_PROG(INDENT, indent, indent)
REBUILD=\#
if test "x$enable_rebuilds" = "xyes" && \
     test -n "$PERL" && \
     $PERL -e 'exit !($] >= 5.002)' > /dev/null 2>&1 && \
     test -n "$AWK" ; then
  REBUILD=
fi
AC_SUBST(REBUILD)

# Need full path to Perl for glib-mkenums
#
if test "x$PERL" != x ; then
  AC_PATH_PROG(PERL_PATH, [$PERL])
fi
if test "x$PERL_PATH" = x ; then
  PERL_PATH="/usr/bin/env perl"
fi
AC_SUBST(PERL_PATH)

# Need suitable python path for greport
AM_PATH_PYTHON(2.4,,PYTHON="/usr/bin/env python2.4")


dnl ***********************
dnl *** Tests for iconv ***
dnl ***********************
dnl
dnl We do this before the gettext checks, to avoid distortion

dnl On Windows we use a native implementation

if test x"$glib_native_win32" = xyes; then
  with_libiconv=native
else
  AC_ARG_WITH(libiconv,
	      [AC_HELP_STRING([--with-libiconv=@<:@no/gnu/native@:>@],
			      [use the libiconv library])],,
	      [with_libiconv=maybe])

  found_iconv=no
  case $with_libiconv in
    maybe)
      # Check in the C library first
      AC_CHECK_FUNC(iconv_open, [with_libiconv=no; found_iconv=yes])
      # Check if we have GNU libiconv
      if test $found_iconv = "no"; then
	AC_CHECK_LIB(iconv, libiconv_open, [with_libiconv=gnu; found_iconv=yes])
      fi
      # Check if we have a iconv in -liconv, possibly from vendor
      if test $found_iconv = "no"; then
	AC_CHECK_LIB(iconv, iconv_open, [with_libiconv=native; found_iconv=yes])
      fi
      ;;
    no)
      AC_CHECK_FUNC(iconv_open, [with_libiconv=no; found_iconv=yes])
      ;;
    gnu|yes)
      AC_CHECK_LIB(iconv, libiconv_open, [with_libiconv=gnu; found_iconv=yes])
      ;;
    native)
      AC_CHECK_LIB(iconv, iconv_open, [with_libiconv=native; found_iconv=yes])
      ;;
  esac

  if test "x$found_iconv" = "xno" ; then
     AC_MSG_ERROR([*** No iconv() implementation found in C library or libiconv])
  fi
fi

gl_GLIBC21
AC_ARG_ENABLE(iconv-cache, 
              [AC_HELP_STRING([--enable-iconv-cache=@<:@yes/no/auto@:>@],
                              [cache iconv descriptors [default=auto]])],,
              [enable_iconv_cache=auto])

AC_MSG_CHECKING([Whether to cache iconv descriptors])
case $enable_iconv_cache in
  auto)
    if test $ac_cv_gnu_library_2_1 = yes; then
      enable_iconv_cache=no
    else 
      enable_iconv_cache=yes
    fi
  ;;
  yes|no) 
  ;;
  *) AC_MSG_ERROR([Value given to --enable-iconv-cache must be one of yes, no or auto])
  ;;
esac

if test $enable_iconv_cache = yes; then
   AC_DEFINE(NEED_ICONV_CACHE,1,[Do we cache iconv descriptors])
fi

AC_MSG_RESULT($enable_iconv_cache)


dnl
dnl zlib support
dnl
PKG_CHECK_MODULES([ZLIB], [zlib], [found_zlib=yes], [found_zlib=no])
if test "x$found_zlib" = "xno" ; then
  AC_CHECK_LIB(z, inflate, [AC_CHECK_HEADER(zlib.h, found_zlib=yes)])
  if test "x$found_zlib" = "xno" ; then
    AC_MSG_ERROR([*** Working zlib library and headers not found ***])
  fi
  ZLIB_LIBS='-lz'
  AC_SUBST(ZLIB_LIBS)
fi

dnl
dnl gettext support
dnl

ALL_LINGUAS="`grep -v '^#' "$srcdir/po/LINGUAS" | tr '\n' ' '`"
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/po/LINGUAS'])
GLIB_GNU_GETTEXT

if test "$gt_cv_have_gettext" != "yes" ; then
  AC_MSG_ERROR([
*** You must have either have gettext support in your C library, or use the 
*** GNU gettext library. (http://www.gnu.org/software/gettext/gettext.html
])
fi

LIBS="$INTLLIBS $LIBS"

GETTEXT_PACKAGE=glib20
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], 
  [Define the gettext package to be used])

GLIB_DEFINE_LOCALEDIR(GLIB_LOCALE_DIR)

dnl
dnl Now we are done with gettext checks, figure out ICONV_LIBS
dnl

if test x"$glib_native_win32" != xyes; then
  if test x$with_libiconv != xno ; then
    case " $INTLLIBS " in
    *[[\ \	]]-liconv[[\ \	]]*) ;;
    *) ICONV_LIBS="-liconv" ;;
    esac
  fi
fi
AC_SUBST(ICONV_LIBS)

case $with_libiconv in
  gnu)
    AC_DEFINE(USE_LIBICONV_GNU, 1, [Using GNU libiconv])
    ;;
  native)
    AC_DEFINE(USE_LIBICONV_NATIVE, 1, [Using a native implementation of iconv in a separate library])
    ;;
esac

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll])
dnl when using libtool 2.x create libtool early, because it's used in configure
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])


if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi

CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES"

if test "$glib_native_win32" = "yes"; then
  if test x$enable_static = xyes -a x$enable_shared = xyes; then
    AC_MSG_ERROR([Can not build both shared and static at the same time on Windows.])
  fi
  if test x$enable_static = xyes; then
    glib_win32_static_compilation=yes
    GLIB_WIN32_STATIC_COMPILATION_DEFINE="#define GLIB_STATIC_COMPILATION 1
#define GOBJECT_STATIC_COMPILATION 1"
    AC_SUBST(GLIB_WIN32_STATIC_COMPILATION_DEFINE)
  fi
fi
AM_CONDITIONAL(OS_WIN32_AND_DLL_COMPILATION, [test x$glib_native_win32 = xyes -a x$glib_win32_static_compilation != xyes])

dnl
dnl DU4 native cc currently needs -std1 for ANSI mode (instead of K&R)
dnl
if test $cross_compiling != yes ; then
    AC_MSG_CHECKING([for extra flags to get ANSI library prototypes])
    glib_save_LIBS=$LIBS
    LIBS="$LIBS -lm"
    AC_TRY_RUN([#include <math.h>
		 int main (void) { return (log(1) != log(1.)); }],
	 AC_MSG_RESULT(none needed),
	 glib_save_CFLAGS=$CFLAGS
	 CFLAGS="$CFLAGS -std1"
	 AC_TRY_RUN([#include <math.h>
		     int main (void) { return (log(1) != log(1.)); }],
	     AC_MSG_RESULT(-std1),
	     AC_MSG_RESULT()
	     CFLAGS=$glib_save_CFLAGS
	     AC_MSG_WARN(
		    [No ANSI prototypes found in library. (-std1 didn't work.)])
	 )
    )
    LIBS=$glib_save_LIBS
fi

dnl NeXTStep cc seems to need this
AC_MSG_CHECKING([for extra flags for POSIX compliance])
AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
  AC_MSG_RESULT(none needed),
  glib_save_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -posix"
  AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
    AC_MSG_RESULT(-posix),
    AC_MSG_RESULT()
    CFLAGS=$glib_save_CFLAGS
    AC_MSG_WARN([Could not determine POSIX flag. (-posix didn't work.)])))

# Checks for header files.
AC_HEADER_STDC

# Checks for library functions.
AC_FUNC_VPRINTF
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(mmap posix_memalign memalign valloc fsync pipe2)
AC_CHECK_FUNCS(atexit on_exit timegm gmtime_r)

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int64)

if test x$ac_cv_sizeof_long = x8 || test x$ac_cv_sizeof_long_long = x8 || test x$ac_cv_sizeof___int64 = x8 ; then
  :
else
  AC_MSG_ERROR([
*** GLib requires a 64 bit type. You might want to consider
*** using the GNU C compiler.
])
fi

if test x$glib_native_win32 != xyes && test x$ac_cv_sizeof_long_long = x8; then
	# long long is a 64 bit integer.
	AC_MSG_CHECKING(for format to printf and scanf a guint64)
	AC_CACHE_VAL(glib_cv_long_long_format,[
		for format in ll q I64; do
		  AC_TRY_RUN([#include <stdio.h>  
			int main()
			{
			  long long b, a = -0x3AFAFAFAFAFAFAFALL;
			  char buffer[1000];
			  sprintf (buffer, "%${format}u", a);
  			  sscanf (buffer, "%${format}u", &b);
			  exit (b!=a);
			}
			],
			[glib_cv_long_long_format=${format}
			break],
			[],[:])
		done])
	if test -n "$glib_cv_long_long_format"; then
	  AC_MSG_RESULT(%${glib_cv_long_long_format}u)
	  AC_DEFINE(HAVE_LONG_LONG_FORMAT,1,[define if system printf can print long long])
	  if test x"$glib_cv_long_long_format" = xI64; then
	    AC_DEFINE(HAVE_INT64_AND_I64,1,[define to support printing 64-bit integers with format I64])
	  fi
        else
	  AC_MSG_RESULT(none)
        fi
elif test x$ac_cv_sizeof___int64 = x8; then
	# __int64 is a 64 bit integer.
	AC_MSG_CHECKING(for format to printf and scanf a guint64)
	# We know this is MSVCRT.DLL, and what the formats are
	glib_cv_long_long_format=I64
	AC_MSG_RESULT(%${glib_cv_long_long_format}u)
        AC_DEFINE(HAVE_LONG_LONG_FORMAT,1,[define if system printf can print long long])
	AC_DEFINE(HAVE_INT64_AND_I64,1,[define to support printing 64-bit integers with format I64])
fi

dnl long doubles were not used, and a portability problem
dnl AC_C_LONG_DOUBLE
AC_C_CONST

dnl ok, here we try to check whether the systems prototypes for
dnl malloc and friends actually match the prototypes provided
dnl by gmem.h (keep in sync). i currently only know how to check
dnl this reliably with gcc (-Werror), improvements for other
dnl compilers are apprechiated.
SANE_MALLOC_PROTOS=no
AC_MSG_CHECKING([if malloc() and friends prototypes are gmem.h compatible])
glib_save_CFLAGS=$CFLAGS
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Werror"
  AC_TRY_COMPILE([#include <stdlib.h>], [
    void* (*my_calloc_p)  (size_t, size_t) = calloc;
    void* (*my_malloc_p)  (size_t)         = malloc;
    void  (*my_free_p)    (void*)          = free;
    void* (*my_realloc_p) (void*, size_t)  = realloc;
    my_calloc_p = 0;
    my_malloc_p = 0;
    my_free_p = 0;
    my_realloc_p = 0;
  ],
    AC_DEFINE(SANE_MALLOC_PROTOS, 1, 
      [Define if you have correct malloc prototypes])
    SANE_MALLOC_PROTOS=yes)
fi
AC_MSG_RESULT($SANE_MALLOC_PROTOS)
CFLAGS=$glib_save_CFLAGS

dnl
dnl check in which direction the stack grows
dnl
AC_CACHE_CHECK([for growing stack pointer],glib_cv_stack_grows,[
        AC_TRY_RUN([
	volatile int *a = 0, *b = 0;
	void foo (void);
	int main () { volatile int y = 7; a = &y; foo (); return b > a; }
	void foo (void) { volatile int x = 5; b = &x; }
       	],
	glib_cv_stack_grows=no
        ,
	glib_cv_stack_grows=yes
        ,)
])

dnl AC_C_INLINE is useless to us since it bails out too early, we need to
dnl truely know which ones of `inline', `__inline' and `__inline__' are
dnl actually supported.
AC_CACHE_CHECK([for __inline],glib_cv_has__inline,[
        AC_COMPILE_IFELSE([
	__inline int foo () { return 0; }
	int main () { return foo (); }
       	],
	glib_cv_has__inline=yes
        ,
	glib_cv_has__inline=no
        ,)
])
case x$glib_cv_has__inline in
xyes) AC_DEFINE(G_HAVE___INLINE,1,[Have __inline keyword])
esac
AC_CACHE_CHECK([for __inline__],glib_cv_has__inline__,[
        AC_COMPILE_IFELSE([
	__inline__ int foo () { return 0; }
	int main () { return foo (); }
       	],
	glib_cv_has__inline__=yes
        ,
	glib_cv_has__inline__=no
        ,)
])
case x$glib_cv_has__inline__ in
xyes) AC_DEFINE(G_HAVE___INLINE__,1,[Have __inline__ keyword])
esac
AC_CACHE_CHECK([for inline], glib_cv_hasinline,[
        AC_COMPILE_IFELSE([
	#undef inline
	inline int foo () { return 0; }
	int main () { return foo (); }
       	],
	glib_cv_hasinline=yes
        ,
	glib_cv_hasinline=no
        ,)
])
case x$glib_cv_hasinline in
xyes) AC_DEFINE(G_HAVE_INLINE,1,[Have inline keyword])
esac

# if we can use inline functions in headers
AC_MSG_CHECKING(if inline functions in headers work)
AC_LINK_IFELSE([
#if defined (G_HAVE_INLINE) && defined (__GNUC__) && defined (__STRICT_ANSI__)
#  undef inline
#  define inline __inline__
#elif !defined (G_HAVE_INLINE)
#  undef inline
#  if defined (G_HAVE___INLINE__)
#    define inline __inline__
#  elif defined (G_HAVE___INLINE)
#    define inline __inline
#  endif
#endif

int glib_test_func2 (int);

static inline int
glib_test_func1 (void) {
  return glib_test_func2 (1);
}

int
main (void) {
  int i = 1;
}],[g_can_inline=yes],[g_can_inline=no])
AC_MSG_RESULT($g_can_inline)

dnl *** check for working do while(0) macros ***
AC_CACHE_CHECK([for working do while(0) macros], g_cv_support_dowhile_macros, [
	AC_TRY_COMPILE([],[
	#define STMT_START do
	#define STMT_END while(0)
	#define STMT_TEST STMT_START { i = 0; } STMT_END
	int main(void) { int i = 1; STMT_TEST; return i; }],
	[g_cv_support_dowhile_macros=yes],
	[g_cv_support_dowhile_macros=no],
	[g_cv_support_dowhile_macros=yes])
])
if test x$g_cv_support_dowhile_macros = xyes; then
  AC_DEFINE(HAVE_DOWHILE_MACROS, 1, [define for working do while(0) macros])
fi

# check for flavours of varargs macros
AC_MSG_CHECKING(for ISO C99 varargs macros in C)
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
],g_have_iso_c_varargs=yes,g_have_iso_c_varargs=no)
AC_MSG_RESULT($g_have_iso_c_varargs)

AC_MSG_CHECKING(for ISO C99 varargs macros in C++)
if test "$CXX" = ""; then
dnl No C++ compiler
  g_have_iso_cxx_varargs=no
else
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
],g_have_iso_cxx_varargs=yes,g_have_iso_cxx_varargs=no)
  AC_LANG_C
fi
AC_MSG_RESULT($g_have_iso_cxx_varargs)

AC_MSG_CHECKING(for GNUC varargs macros)
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(params...) a(1,params)
call_a(2,3);
],g_have_gnuc_varargs=yes,g_have_gnuc_varargs=no)
AC_MSG_RESULT($g_have_gnuc_varargs)

# check for GNUC visibility support
AC_MSG_CHECKING(for GNUC visibility attribute)
GLIB_CHECK_COMPILE_WARNINGS([
void
__attribute__ ((visibility ("hidden")))
     f_hidden (void)
{
}
void
__attribute__ ((visibility ("internal")))
     f_internal (void)
{
}
void
__attribute__ ((visibility ("protected")))
     f_protected (void)
{
}
void
__attribute__ ((visibility ("default")))
     f_default (void)
{
}
int main (int argc, char **argv)
{
	f_hidden();
	f_internal();
	f_protected();
	f_default();
	return 0;
}
],g_have_gnuc_visibility=yes,g_have_gnuc_visibility=no)
AC_MSG_RESULT($g_have_gnuc_visibility)
AM_CONDITIONAL(HAVE_GNUC_VISIBILITY, [test x$g_have_gnuc_visibility = xyes])

AC_MSG_CHECKING([whether using Sun Studio C compiler])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#if defined(__SUNPRO_C) || (__SUNPRO_C >= 0x550)
#else
# include "error: this is not Sun Studio."
#endif
]], [[]])], [ g_have_sunstudio_visibility=yes ], [ g_have_sunstudio_visibility=no ])
AC_MSG_RESULT($g_have_sunstudio_visibility)
AM_CONDITIONAL(HAVE_SUNSTUDIO_VISIBILITY, [test x$g_have_sunstudio_visibility = xyes])

# check for bytesex stuff
AC_C_BIGENDIAN
if test x$ac_cv_c_bigendian = xuniversal ; then
AC_TRY_COMPILE([#include <endian.h>], [#if __BYTE_ORDER == __BIG_ENDIAN
#else
#error Not a big endian. 
#endif],
    ac_cv_c_bigendian=yes
    ,AC_TRY_COMPILE([#include <endian.h>], [#if __BYTE_ORDER == __LITTLE_ENDIAN
#else
#error Not a little endian. 
#endif],
    ac_cv_c_bigendian=no
    ,AC_MSG_WARN([Could not determine endianness.])))
fi


# check for header files
AC_CHECK_HEADERS([dirent.h float.h limits.h pwd.h grp.h sys/param.h sys/poll.h sys/resource.h])
AC_CHECK_HEADERS([sys/time.h sys/times.h sys/wait.h unistd.h values.h])
AC_CHECK_HEADERS([sys/select.h sys/types.h stdint.h inttypes.h sched.h malloc.h])
AC_CHECK_HEADERS([sys/vfs.h sys/mount.h sys/vmount.h sys/statfs.h sys/statvfs.h])
AC_CHECK_HEADERS([mntent.h sys/mnttab.h sys/vfstab.h sys/mntctl.h sys/sysctl.h fstab.h])
AC_CHECK_HEADERS([sys/uio.h])

# check for structure fields
AC_CHECK_MEMBERS([struct stat.st_mtimensec, struct stat.st_mtim.tv_nsec, struct stat.st_atimensec, struct stat.st_atim.tv_nsec, struct stat.st_ctimensec, struct stat.st_ctim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_blksize, struct stat.st_blocks, struct statfs.f_fstypename, struct statfs.f_bavail],,, [#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif])
# struct statvfs.f_basetype is available on Solaris but not for Linux. 
AC_CHECK_MEMBERS([struct statvfs.f_basetype],,, [#include <sys/statvfs.h>])
AC_CHECK_MEMBERS([struct tm.tm_gmtoff])

# Checks for libcharset
AM_LANGINFO_CODESET
gl_GLIBC21
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])
AC_CHECK_FUNCS(setlocale)

# check additional type sizes
AC_CHECK_SIZEOF(size_t)

dnl Try to figure out whether gsize, gssize should be long or int
AC_MSG_CHECKING([for the appropriate definition for size_t])

case $ac_cv_sizeof_size_t in
  $ac_cv_sizeof_short) 
      glib_size_type=short
      ;;
  $ac_cv_sizeof_int) 
      glib_size_type=int
      ;;
  $ac_cv_sizeof_long) 
      glib_size_type=long
      ;;
  $ac_cv_sizeof_long_long)
      glib_size_type='long long'
      ;;
  $ac_cv_sizeof__int64)
      glib_size_type='__int64'
      ;;
  *)  AC_MSG_ERROR([No type matching size_t in size])
      ;;
esac

dnl If int/long are the same size, we see which one produces
dnl warnings when used in the location as size_t. (This matters
dnl on AIX with xlc)
dnl
if test $ac_cv_sizeof_size_t = $ac_cv_sizeof_int &&
   test $ac_cv_sizeof_size_t = $ac_cv_sizeof_long ; then
  GLIB_CHECK_COMPILE_WARNINGS([
#if defined(_AIX) && !defined(__GNUC__)
#pragma options langlvl=stdc89
#endif
#include <stddef.h> 
int main ()
{
  size_t s = 1;
  unsigned int *size_int = &s;
  return (int)*size_int;
}
    ],glib_size_type=int,
      [GLIB_CHECK_COMPILE_WARNINGS([
#if defined(_AIX) && !defined(__GNUC__)
#pragma options langlvl=stdc89
#endif
#include <stddef.h> 
int main ()
{
   size_t s = 1;
   unsigned long *size_long = &s;
   return (int)*size_long;
}
        ],glib_size_type=long)])
fi

AC_MSG_RESULT(unsigned $glib_size_type)

# Check for some functions
AC_CHECK_FUNCS(lstat strerror strsignal memmove vsnprintf stpcpy strcasecmp strncasecmp poll getcwd vasprintf setenv unsetenv getc_unlocked readlink symlink fdwalk memmem)
AC_CHECK_FUNCS(chown lchmod lchown fchmod fchown link statvfs statfs utimes getgrgid getpwuid)
AC_CHECK_FUNCS(getmntent_r setmntent endmntent hasmntopt getmntinfo)
# Check for high-resolution sleep functions
AC_CHECK_FUNCS(nanosleep nsleep)
AC_CHECK_FUNCS(splice)

AC_CHECK_HEADERS(crt_externs.h)
AC_CHECK_FUNCS(_NSGetEnviron)

AC_FUNC_VSNPRINTF_C99
AC_FUNC_PRINTF_UNIX98

# Internet address families
if test $glib_native_win32 = yes; then
  glib_inet_includes=["
#include <winsock2.h>
  "]
else
  glib_inet_includes=["
#include <sys/types.h>
#include <sys/socket.h>
  "]
fi

glib_failed=false
GLIB_CHECK_VALUE(AF_INET, $glib_inet_includes, glib_failed=true)
GLIB_CHECK_VALUE(AF_INET6, $glib_inet_includes, glib_failed=true)
# winsock defines this even though it doesn't support it
GLIB_CHECK_VALUE(AF_UNIX, $glib_inet_includes, glib_failed=true)
if $glib_failed ; then
  AC_MSG_ERROR([Could not determine values for AF_INET* constants])
fi

glib_failed=false
GLIB_CHECK_VALUE(MSG_PEEK, $glib_inet_includes, glib_failed=true)
GLIB_CHECK_VALUE(MSG_OOB, $glib_inet_includes, glib_failed=true)
GLIB_CHECK_VALUE(MSG_DONTROUTE, $glib_inet_includes, glib_failed=true)
if $glib_failed ; then
  AC_MSG_ERROR([Could not determine values for MSG_* constants])
fi

AC_CHECK_FUNCS(getprotobyname_r endservent)
AC_CHECK_HEADERS([netdb.h wspiapi.h])

# For gio/libasyncns
if test $glib_native_win32 = no; then
  AC_CHECK_FUNCS(strndup setresuid setreuid)
  AC_CHECK_HEADERS(sys/prctl.h arpa/nameser_compat.h)

  # We can't just use AC_CHECK_FUNC/AC_CHECK_LIB here. Bug 586150
  AC_MSG_CHECKING([for res_query])
  AC_TRY_LINK([#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>],
              [res_query("test", 0, 0, (void *)0, 0);],
              [AC_MSG_RESULT([yes])],
              [save_libs="$LIBS"
               LIBS="-lresolv $LIBS"
               AC_TRY_LINK([#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>],
                           [res_query("test", 0, 0, (void *)0, 0);],
                           [AC_MSG_RESULT([in -lresolv])
                            LIBASYNCNS_LIBADD="-lresolv"],
                           [LIBS="-lbind $save_libs"
                            AC_TRY_LINK([#include <resolv.h>],
                                        [res_query("test", 0, 0, (void *)0, 0);],
                                        [AC_MSG_RESULT([in -lbind])
                                         LIBASYNCNS_LIBADD="-lbind"],
                                        [AC_MSG_ERROR(not found)])])
               LIBS="$save_libs"])
  AC_SUBST(LIBASYNCNS_LIBADD)
fi

case $host in
  *-*-solaris* )
     AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, Needed to get declarations for msg_control and msg_controllen on Solaris)
     AC_DEFINE(_XOPEN_SOURCE,          2, Needed to get declarations for msg_control and msg_controllen on Solaris)
     AC_DEFINE(__EXTENSIONS__,         1, Needed to get declarations for msg_control and msg_controllen on Solaris)
     ;;
esac

dnl
dnl if statfs() takes 2 arguments (Posix) or 4 (Solaris)
dnl
if test "$ac_cv_func_statfs" = yes ; then
  AC_MSG_CHECKING([number of arguments to statfs()])
  AC_TRY_COMPILE([#include <unistd.h>
  #ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
  #endif
  #ifdef HAVE_SYS_VFS_H
  #include <sys/vfs.h>
  #endif
  #ifdef HAVE_SYS_MOUNT_H
  #include <sys/mount.h>
  #endif
  #ifdef HAVE_SYS_STATFS_H
  #include <sys/statfs.h>
  #endif], [struct statfs st;
  statfs(NULL, &st);],[
    AC_MSG_RESULT([2])
    AC_DEFINE(STATFS_ARGS, 2, [Number of arguments to statfs()])],[
    AC_TRY_COMPILE([#include <unistd.h>
  #ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
  #endif
  #ifdef HAVE_SYS_VFS_H
  #include <sys/vfs.h>
  #endif
  #ifdef HAVE_SYS_MOUNT_H
  #include <sys/mount.h>
  #endif
  #ifdef HAVE_SYS_STATFS_H
  #include <sys/statfs.h>
  #endif], [struct statfs st;
  statfs(NULL, &st, sizeof (st), 0);],[
      AC_MSG_RESULT([4])
      AC_DEFINE(STATFS_ARGS, 4, [Number of arguments to statfs()])],[
      AC_MSG_RESULT(unknown)
      AC_MSG_ERROR([unable to determine number of arguments to statfs()])])])
fi

#
# Check whether to use an included printf
#

AC_ARG_ENABLE(included-printf,
              [AC_HELP_STRING([--enable-included-printf],
                              [use included printf [default=auto]])],
              enable_included_printf="$enableval")

need_included_printf=no
if test "x$enable_included_printf" = "xyes" ; then
  need_included_printf=yes
fi
if test "$ac_cv_func_vsnprintf_c99" != "yes" ; then
  need_included_printf=yes
fi
if test "$ac_cv_func_printf_unix98" != "yes" ; then
  need_included_printf=yes
fi
if test "x$ac_cv_sizeof_long_long" = "x8" &&
   test -z "$glib_cv_long_long_format" ; then
  need_included_printf=yes
fi

if test "x$enable_included_printf" = "xno" && 
   test "x$need_included_printf" = "xyes" ; then
  AC_MSG_ERROR([
*** Your C library's printf doesn't appear to have the features that
*** GLib needs, but you specified --enable-included-printf=no.])
fi

enable_included_printf=$need_included_printf

AM_CONDITIONAL(HAVE_GOOD_PRINTF, test "$enable_included_printf" != "yes")
if test "$enable_included_printf" != "yes" ; then 
  AC_DEFINE(HAVE_GOOD_PRINTF,1,[define to use system printf])
else
  if test -z "$glib_cv_long_long_format" ; then
    glib_cv_long_long_format="ll"
  fi
  AC_DEFINE(HAVE_VASPRINTF,1)
fi

# Checks needed for gnulib vasnprintf
bh_C_SIGNED
jm_AC_TYPE_LONG_LONG
gt_TYPE_LONGDOUBLE
gt_TYPE_WCHAR_T
gt_TYPE_WINT_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ptrdiff_t)
jm_AC_TYPE_INTMAX_T
AC_CHECK_FUNCS([snprintf wcslen])
AC_FUNC_SNPRINTF_C99

# Check if bcopy can be used for overlapping copies, if memmove isn't found.
# The check is borrowed from the PERL Configure script.
if test "$ac_cv_func_memmove" != "yes"; then
  AC_CACHE_CHECK(whether bcopy can handle overlapping copies,
    glib_cv_working_bcopy,[AC_TRY_RUN([
      int main() {
        char buf[128], abc[128], *b;
        int len, off, align;
        bcopy("abcdefghijklmnopqrstuvwxyz0123456789", abc, 36);
        for (align = 7; align >= 0; align--) {
          for (len = 36; len; len--) {
            b = buf+align; bcopy(abc, b, len);
            for (off = 1; off <= len; off++) {
              bcopy(b, b+off, len); bcopy(b+off, b, len);
                if (bcmp(b, abc, len)) return(1);
            }
          }
        }
        return(0);
      }],glib_cv_working_bcopy=yes,glib_cv_working_bcopy=no)])

  GLIB_ASSERT_SET(glib_cv_working_bcopy)
  if test "$glib_cv_working_bcopy" = "yes"; then
    AC_DEFINE(HAVE_WORKING_BCOPY,1,[Have a working bcopy])
  fi
fi

# Check for sys_errlist
AC_MSG_CHECKING(for sys_errlist)
AC_TRY_LINK(, [
extern char *sys_errlist[];
extern int sys_nerr;
sys_errlist[sys_nerr-1][0] = 0;
], glib_ok=yes, glib_ok=no)
AC_MSG_RESULT($glib_ok)
if test "$glib_ok" = "no"; then
    AC_DEFINE(NO_SYS_ERRLIST,1,[global 'sys_errlist' not found])
fi

# Check for sys_siglist
AC_MSG_CHECKING(for sys_siglist)
AC_TRY_LINK(, [
extern char *sys_siglist[];
exit (sys_siglist[0]);
], glib_ok=yes, glib_ok=no)
AC_MSG_RESULT($glib_ok)
if test "$glib_ok" = "no"; then
    AC_DEFINE(NO_SYS_SIGLIST,1,[global 'sys_siglist' not found])
fi

# Check for sys_siglist decl (see Tue Jan 19 00:44:24 1999 in changelog)
AC_MSG_CHECKING(for sys_siglist declaration)
AC_TRY_COMPILE([#include <signal.h>], [
strlen (sys_siglist[0]);
], glib_ok=yes, glib_ok=no)
AC_MSG_RESULT($glib_ok)
if test "$glib_ok" = "no"; then
    AC_DEFINE(NO_SYS_SIGLIST_DECL,1,[global 'sys_siglist' not declared])
fi

# Check if <sys/select.h> needs to be included for fd_set
AC_MSG_CHECKING([for fd_set])
AC_TRY_COMPILE([#include <sys/types.h>],
        [fd_set readMask, writeMask;], gtk_ok=yes, gtk_ok=no)
if test "$gtk_ok" = "yes"; then
    AC_MSG_RESULT([yes, found in sys/types.h])
else
    AC_EGREP_HEADER(fd_set, sys/select.h, gtk_ok=yes)
    if test "$gtk_ok" = "yes"; then
	# *** FIXME: give it a different name
        AC_DEFINE(HAVE_SYS_SELECT_H,1,[found fd_set in sys/select.h])
        AC_MSG_RESULT([yes, found in sys/select.h])
    else
	AC_DEFINE(NO_FD_SET,1,[didn't find fd_set])
	AC_MSG_RESULT(no)
    fi
fi

dnl *** check for sane realloc() ***
AC_CACHE_CHECK([whether realloc (NULL,) will work],glib_cv_sane_realloc,[
        AC_TRY_RUN([#include <stdlib.h>
        int main() {
          return realloc (0, sizeof (int)) == 0;
        }],
        [glib_cv_sane_realloc=yes],
        [glib_cv_sane_realloc=no],
	[glib_cv_sane_realloc=yes])
])
if test x$glib_cv_sane_realloc = xyes; then
  AC_DEFINE(REALLOC_0_WORKS,1,[whether realloc (NULL,) works])
fi

dnl Check for nl_langinfo and CODESET
AC_CACHE_CHECK([for nl_langinfo (CODESET)],glib_cv_langinfo_codeset,[
        AC_TRY_COMPILE([#include <langinfo.h>],
                [char *codeset = nl_langinfo (CODESET);],
                [glib_cv_langinfo_codeset=yes],
                [glib_cv_langinfo_codeset=no])])
if test x$glib_cv_langinfo_codeset = xyes; then
  AC_DEFINE(HAVE_CODESET,1,[Have nl_langinfo (CODESET)])
fi

dnl ****************************************
dnl *** posix_memalign                   ***
dnl ****************************************
AC_MSG_CHECKING(for a compliant posix_memalign() implementation)
AC_CACHE_VAL(glib_cv_compliant_posix_memalign,[
    glib_cv_compliant_posix_memalign=0
    if test "$ac_cv_func_posix_memalign" = "yes" ; then
	AC_TRY_RUN([
                #define _XOPEN_SOURCE 600
                #include <stdlib.h> /* posix_memalign() should be defined here */
                /* some systems break if #include <malloc.h> used */
		static void test_memalign (size_t boundary, size_t size) {
		    void *mem = 0;
		    if (posix_memalign (&mem, boundary, size) != 0 || !mem)
			exit (1);
                    else
                      free (mem);
		}
		int main() {
		    test_memalign (  128,   128 - 2 * sizeof (void*));
		    test_memalign (  256,   256 - 2 * sizeof (void*));
		    test_memalign (  512,   512 - 2 * sizeof (void*));
		    test_memalign ( 1024,  1024 - 2 * sizeof (void*));
		    test_memalign ( 2048,  2048 - 2 * sizeof (void*));
		    test_memalign ( 4096,  4096 - 2 * sizeof (void*));
		    test_memalign ( 8192,  8192 - 2 * sizeof (void*));
		    test_memalign (16384, 16384 - 2 * sizeof (void*));
		    test_memalign (32768, 32768 - 2 * sizeof (void*));
		    exit (0); /* success */
		}
		],
	    [glib_cv_compliant_posix_memalign=1], [], [:])
      :
    fi
    ])
if test "$glib_cv_compliant_posix_memalign" = "1"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(POSIX_MEMALIGN_WITH_COMPLIANT_ALLOCS, 1, [define if posix_memalign() can allocate any size])
else
    AC_MSG_RESULT(no)
fi


dnl ****************************************
dnl *** strlcpy/strlcat                  ***
dnl ****************************************
# Check for strlcpy
AC_CACHE_CHECK([for OpenBSD strlcpy/strlcat],glib_cv_have_strlcpy,[
AC_TRY_RUN([#include <stdlib.h>
#include <string.h>
int main() {
  char p[10];
  (void) strlcpy (p, "hi", 10);
  if (strlcat (p, "bye", 0) != 3) 
    return 1;
  return 0;
}], glib_cv_have_strlcpy=yes, 
    glib_cv_have_strlcpy=no,
    glib_cv_have_strlcpy=no)])
if test "$glib_cv_have_strlcpy" = "yes"; then
    AC_DEFINE(HAVE_STRLCPY,1,[Have functions strlcpy and strlcat])
fi
  

dnl **********************
dnl *** va_copy checks ***
dnl **********************
dnl we currently check for all three va_copy possibilities, so we get
dnl all results in config.log for bug reports.
AC_CACHE_CHECK([for an implementation of va_copy()],glib_cv_va_copy,[
	AC_LINK_IFELSE([#include <stdarg.h>
#include <stdlib.h>
	void f (int i, ...) {
	va_list args1, args2;
	va_start (args1, i);
	va_copy (args2, args1);
	if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
	  exit (1);
	va_end (args1); va_end (args2);
	}
	int main() {
	  f (0, 42);
	  return 0;
	}],
	[glib_cv_va_copy=yes],
	[glib_cv_va_copy=no])
])
AC_CACHE_CHECK([for an implementation of __va_copy()],glib_cv___va_copy,[
	AC_LINK_IFELSE([#include <stdarg.h>
#include <stdlib.h>
	void f (int i, ...) {
	va_list args1, args2;
	va_start (args1, i);
	__va_copy (args2, args1);
	if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
	  exit (1);
	va_end (args1); va_end (args2);
	}
	int main() {
	  f (0, 42);
	  return 0;
	}],
	[glib_cv___va_copy=yes],
	[glib_cv___va_copy=no])
])

if test "x$glib_cv_va_copy" = "xyes"; then
  g_va_copy_func=va_copy
else if test "x$glib_cv___va_copy" = "xyes"; then
  g_va_copy_func=__va_copy
fi
fi

if test -n "$g_va_copy_func"; then
  AC_DEFINE_UNQUOTED(G_VA_COPY,$g_va_copy_func,[A 'va_copy' style function])
fi

AC_CACHE_CHECK([whether va_lists can be copied by value],glib_cv_va_val_copy,[
	AC_TRY_RUN([#include <stdarg.h>
#include <stdlib.h> 
	void f (int i, ...) {
	va_list args1, args2;
	va_start (args1, i);
	args2 = args1;
	if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
	  exit (1);
	va_end (args1); va_end (args2);
	}
	int main() {
	  f (0, 42);
	  return 0;
	}],
	[glib_cv_va_val_copy=yes],
	[glib_cv_va_val_copy=no],
	[glib_cv_va_val_copy=yes])
])

if test "x$glib_cv_va_val_copy" = "xno"; then
  AC_DEFINE(G_VA_COPY_AS_ARRAY,1, ['va_lists' cannot be copies as values])
fi

dnl ***********************
dnl *** g_module checks ***
dnl ***********************
G_MODULE_LIBS=
G_MODULE_LIBS_EXTRA=
G_MODULE_PLUGIN_LIBS=
if test x"$glib_native_win32" = xyes; then
  dnl No use for this on Win32
  G_MODULE_LDFLAGS=
else
  export SED
  G_MODULE_LDFLAGS=`(./libtool --config; echo eval echo \\$export_dynamic_flag_spec) | sh`
fi
dnl G_MODULE_IMPL= don't reset, so cmd-line can override
G_MODULE_NEED_USCORE=0
G_MODULE_BROKEN_RTLD_GLOBAL=0
G_MODULE_HAVE_DLERROR=0
dnl *** force native WIN32 shared lib loader 
if test -z "$G_MODULE_IMPL"; then
  case "$host" in
  *-*-mingw*|*-*-cygwin*) G_MODULE_IMPL=G_MODULE_IMPL_WIN32 ;;
  esac
fi
dnl *** force native AIX library loader
dnl *** dlopen() filepath must be of the form /path/libname.a(libname.so)
if test -z "$G_MODULE_IMPL"; then
  case "$host" in
  *-*-aix*) G_MODULE_IMPL=G_MODULE_IMPL_AR ;;
  esac
fi
dnl *** dlopen() and dlsym() in system libraries
if test -z "$G_MODULE_IMPL"; then
	AC_CHECK_FUNC(dlopen,
		      [AC_CHECK_FUNC(dlsym,
			             [G_MODULE_IMPL=G_MODULE_IMPL_DL],[])],
		      [])
fi
dnl *** load_image (BeOS)
if test -z "$G_MODULE_IMPL" && test "x$glib_native_beos" = "xyes"; then
  AC_CHECK_LIB(root, load_image,
      [G_MODULE_LIBS="-lbe -lroot -lglib-2.0 "
      G_MODULE_LIBS_EXTRA="-L\$(top_builddir_full)/.libs"
      G_MODULE_PLUGIN_LIBS="-L\$(top_builddir_full)/gmodule/.libs -lgmodule"
      G_MODULE_IMPL=G_MODULE_IMPL_BEOS],
      [])
fi   
dnl *** NSLinkModule (dyld) in system libraries (Darwin)
if test -z "$G_MODULE_IMPL"; then
 	AC_CHECK_FUNC(NSLinkModule,
		      [G_MODULE_IMPL=G_MODULE_IMPL_DYLD
		       G_MODULE_NEED_USCORE=1],
		      [])
fi
dnl *** dlopen() and dlsym() in libdl
if test -z "$G_MODULE_IMPL"; then
	AC_CHECK_LIB(dl, dlopen,
		     [AC_CHECK_LIB(dl, dlsym,
			           [G_MODULE_LIBS=-ldl
		                   G_MODULE_IMPL=G_MODULE_IMPL_DL],[])],
		     [])
fi
dnl *** shl_load() in libdld (HP-UX)
if test -z "$G_MODULE_IMPL"; then
	AC_CHECK_LIB(dld, shl_load,
		[G_MODULE_LIBS=-ldld
		G_MODULE_IMPL=G_MODULE_IMPL_DLD],
		[])
fi
dnl *** additional checks for G_MODULE_IMPL_DL
if test "$G_MODULE_IMPL" = "G_MODULE_IMPL_DL"; then
	LIBS_orig="$LIBS"
	LDFLAGS_orig="$LDFLAGS"
	LIBS="$G_MODULE_LIBS $LIBS"
	LDFLAGS="$LDFLAGS $G_MODULE_LDFLAGS"
dnl *** check for OSF1/5.0 RTLD_GLOBAL brokenness
	echo "void glib_plugin_test(void) { }" > plugin.c
	${SHELL} ./libtool --mode=compile ${CC} -shared \
		-export-dynamic -o plugin.o plugin.c 2>&1 >/dev/null
	AC_CACHE_CHECK([for RTLD_GLOBAL brokenness],
		glib_cv_rtldglobal_broken,[
		AC_TRY_RUN([
#include <dlfcn.h>
#ifndef RTLD_GLOBAL
#  define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LAZY
#  define RTLD_LAZY 0
#endif
int glib_plugin_test;
int main () {
    void *handle, *global, *local;
    global = &glib_plugin_test;
    handle = dlopen ("./.libs/plugin.o", RTLD_GLOBAL | RTLD_LAZY);
    if (!handle) return 0;
    local = dlsym (handle, "glib_plugin_test");
    return global == local;
}                       ],
			[glib_cv_rtldglobal_broken=no],
			[glib_cv_rtldglobal_broken=yes],
			[glib_cv_rtldglobal_broken=no])
		rm -f plugin.c plugin.o plugin.lo .libs/plugin.o
		rmdir .libs 2>/dev/null
	])
	if test "x$glib_cv_rtldglobal_broken" = "xyes"; then
  		G_MODULE_BROKEN_RTLD_GLOBAL=1
	else
  		G_MODULE_BROKEN_RTLD_GLOBAL=0
	fi
dnl *** check whether we need preceeding underscores
	AC_CACHE_CHECK([for preceeding underscore in symbols],
		glib_cv_uscore,[
		AC_TRY_RUN([#include <dlfcn.h>
                int glib_underscore_test (void) { return 42; }
		int main() {
		  void *f1 = (void*)0, *f2 = (void*)0, *handle;
		  handle = dlopen ((void*)0, 0);
		  if (handle) {
		    f1 = dlsym (handle, "glib_underscore_test");
		    f2 = dlsym (handle, "_glib_underscore_test");
		  } return (!f2 || f1);
		}],
			[glib_cv_uscore=yes],
			[glib_cv_uscore=no],
			[])
		rm -f plugin.c plugin.$ac_objext plugin.lo
	])
        GLIB_ASSERT_SET(glib_cv_uscore)
	if test "x$glib_cv_uscore" = "xyes"; then
  		G_MODULE_NEED_USCORE=1
	else
  		G_MODULE_NEED_USCORE=0
	fi

	LDFLAGS="$LDFLAGS_orig"
dnl *** check for having dlerror()
	AC_CHECK_FUNC(dlerror,
		[G_MODULE_HAVE_DLERROR=1],
		[G_MODULE_HAVE_DLERROR=0])
	LIBS="$LIBS_orig"
fi
dnl *** done, have we got an implementation?
if test -z "$G_MODULE_IMPL"; then
	G_MODULE_IMPL=0
        G_MODULE_SUPPORTED=false
else
        G_MODULE_SUPPORTED=true
fi

AC_MSG_CHECKING(for the suffix of module shared libraries)
export SED
shrext_cmds=`./libtool --config | grep '^shrext_cmds='`
eval $shrext_cmds
module=yes eval std_shrext=$shrext_cmds
# chop the initial dot
glib_gmodule_suffix=`echo $std_shrext | sed 's/^\.//'`
AC_MSG_RESULT(.$glib_gmodule_suffix)
# any reason it may fail?
if test "x$glib_gmodule_suffix" = x; then
	AC_MSG_ERROR(Cannot determine shared library suffix from libtool)
fi
 
AC_SUBST(G_MODULE_SUPPORTED)
AC_SUBST(G_MODULE_IMPL)
AC_SUBST(G_MODULE_LIBS)
AC_SUBST(G_MODULE_LIBS_EXTRA)
AC_SUBST(G_MODULE_PLUGIN_LIBS)
AC_SUBST(G_MODULE_LDFLAGS)
AC_SUBST(G_MODULE_HAVE_DLERROR)
AC_SUBST(G_MODULE_BROKEN_RTLD_GLOBAL)
AC_SUBST(G_MODULE_NEED_USCORE)
AC_SUBST(GLIB_DEBUG_FLAGS)

dnl **********************
dnl *** g_spawn checks ***
dnl **********************

AC_MSG_CHECKING(for gspawn implementation)
case "$host" in
  *-*-mingw*)
    GSPAWN=gspawn-win32.lo
    ;;
  *)
    GSPAWN=gspawn.lo
    ;;    
esac
AC_MSG_RESULT($GSPAWN)
AC_SUBST(GSPAWN)

dnl *************************
dnl *** GIOChannel checks ***
dnl *************************

AC_MSG_CHECKING(for GIOChannel implementation)
case "$host" in
  *-*-mingw*)
    GIO=giowin32.lo
    ;;
  *)
    GIO=giounix.lo
    ;;    
esac
AC_MSG_RESULT($GIO)
AC_SUBST(GIO)

dnl *********************************
dnl *** Directory for GIO modules ***
dnl *********************************

AC_ARG_WITH(gio-module-dir,
           [AC_HELP_STRING([--with-gio-module-dir=PATH],
                           [Load gio modules from this directory [LIBDIR/gio/modules]])],
           [],
	   [with_gio_module_dir='${libdir}/gio/modules'])
GIO_MODULE_DIR=$with_gio_module_dir
AC_SUBST(GIO_MODULE_DIR)

dnl **********************************
dnl *** Check for libselinux (GIO) ***
dnl **********************************
AC_ARG_ENABLE(selinux, [  --disable-selinux       build without selinux support])
msg_selinux=no
SELINUX_LIBS=
if test "x$enable_selinux" != "xno"; then

 AC_CHECK_LIB(selinux, is_selinux_enabled,
   [AC_CHECK_HEADERS(selinux/selinux.h,
     [AC_CHECK_LIB(selinux, lgetfilecon_raw, 
       [AC_DEFINE(HAVE_SELINUX, 1, [Define to 1 if libselinux is available])
        SELINUX_LIBS="-lselinux"
        msg_selinux=yes])
     ])
   ])
fi
AC_SUBST(SELINUX_LIBS)

dnl *****************************
dnl ** Check for inotify (GIO) **
dnl *****************************
inotify_support=no
AC_CHECK_HEADERS([sys/inotify.h],
[
	inotify_support=yes
	AC_CHECK_FUNCS(inotify_init1)
])

AM_CONDITIONAL(HAVE_INOTIFY, [test "$inotify_support" = "yes"])

dnl *********************************
dnl ** Check for Solaris FEN (GIO) **
dnl *********************************
fen_support=no
AC_COMPILE_IFELSE([ 
#include <port.h> 
#ifndef PORT_SOURCE_FILE 
#error "Please upgrade to Nevada 72 or above to suppoert FEN" 
#endif 
int main() { return 0; } ],
[
	fen_support=yes
],)

AM_CONDITIONAL(HAVE_FEN, [test "$fen_support" = "yes"])

dnl ****************************
dnl *** Checks for FAM (GIO) ***
dnl ****************************

should_disable_fam=no

AC_ARG_ENABLE(fam, [  --disable-fam          build without enabling fam for file system monitoring],
                         [
                                if test "x$enable_fam" = "xno"; then
                                        should_disable_fam=yes
                                        echo "Not building FAM support"
                                fi
                         ]
                         )
fam_support=no
FAM_LIBS=
if test "x$should_disable_fam" = "xno"; then
AC_CHECK_LIB(fam, FAMOpen,
  [AC_CHECK_HEADERS(fam.h,
    [AC_DEFINE(HAVE_FAM, [], [Define if we have FAM])
     AC_CHECK_LIB(fam, FAMNoExists,
     		  AC_DEFINE(HAVE_FAM_NO_EXISTS, [], [Define if we have FAMNoExists in fam]))
     FAM_LIBS="-lfam"]
     fam_support=yes,
    AC_MSG_WARN(*** FAM support will not be built (header files not found) ***))],
  AC_MSG_WARN(*** FAM support will not be built (FAM library not found) ***))
AC_SUBST(FAM_LIBS)
fi
AM_CONDITIONAL(HAVE_FAM, [test "$fam_support" = "yes"])


dnl *****************************
dnl *** Check for xattr (GIO) ***
dnl *****************************
AC_ARG_ENABLE(xattr, [  --disable-xattr           build without xattr support])
msg_xattr=no
XATTR_LIBS=
if test "x$enable_xattr" != "xno"; then

dnl either glibc or libattr can provide xattr support

dnl for both of them, we check for getxattr being in
dnl the library and a valid xattr header.

dnl try glibc
 AC_CHECK_LIB(c, getxattr,
   [AC_CHECK_HEADERS(sys/xattr.h,
     [AC_DEFINE(HAVE_XATTR, 1, [Define to 1 if xattr is available])
      msg_xattr=yes])
   ])

  if test "x$msg_xattr" != "xyes"; then
dnl   failure. try libattr
   AC_CHECK_LIB(attr, getxattr,
      [AC_CHECK_HEADERS(attr/xattr.h,
       [AC_DEFINE(HAVE_XATTR, 1, [Define to 1 if xattr is available])
        XATTR_LIBS="-lattr"
        msg_xattr=yes])
      ])
  fi

  if test "x$msg_xattr" = "xyes"; then
    AC_MSG_CHECKING([for XATTR_NOFOLLOW])
    AC_TRY_COMPILE([
      #include <stdio.h>
      #ifdef HAVE_SYS_TYPES_H
      #include <sys/types.h>
      #endif
      #ifdef HAVE_SYS_XATTR_H
      #include <sys/xattr.h>
      #elif HAVE_ATTR_XATTR_H
      #include <attr/xattr.h>
      #endif
    ],
    [ssize_t len = getxattr("", "", NULL, 0, 0, XATTR_NOFOLLOW);],
    [
      AC_DEFINE([HAVE_XATTR_NOFOLLOW], [1], [Define to 1 if xattr API uses XATTR_NOFOLLOW])
      AC_MSG_RESULT([yes])
    ],
    [AC_MSG_RESULT([no])]
    )
  fi

fi
AC_SUBST(XATTR_LIBS)

dnl ****************************************
dnl *** platform dependent source checks ***
dnl ****************************************

AC_MSG_CHECKING(for platform-dependent source)
case "$host" in
  *-*-cygwin*|*-*-mingw*)
    PLATFORMDEP=gwin32.lo
    ;;
  *)
    PLATFORMDEP=
    ;;    
esac
AC_MSG_RESULT($PLATFORMDEP)
AC_SUBST(PLATFORMDEP)

AC_MSG_CHECKING([whether to compile timeloop])
case "$host" in
  *-*-cygwin*|*-*-mingw*|*-*-minix)
    enable_timeloop=no
    ;;
  *)
    enable_timeloop=yes
    ;;    
esac
AC_MSG_RESULT($enable_timeloop)
AM_CONDITIONAL(ENABLE_TIMELOOP, test x$enable_timeloop = xyes)

AC_MSG_CHECKING([if building for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT($platform_win32)
AM_CONDITIONAL(PLATFORM_WIN32, test x$platform_win32 = xyes)

dnl ***********************
dnl *** g_thread checks ***
dnl ***********************

AC_ARG_WITH(threads,
           [AC_HELP_STRING([--with-threads=@<:@none/posix/dce/win32@:>@],
                           [specify a thread implementation to use])],
           [if test "x$with_threads" = x; then
		want_threads=yes
	    else
		want_threads=$with_threads
	    fi],
	   [want_threads=yes])
if test "x$enable_threads" = "xno"; then
        want_threads=no
fi

dnl error and warning message
dnl *************************

THREAD_NO_IMPLEMENTATION="You do not have any known thread system on your
                computer. GLib will not have a default thread implementation."

FLAG_DOES_NOT_WORK="I can't find the MACRO to enable thread safety on your
                platform (normally it's "_REENTRANT"). I'll not use any flag on
                compilation now, but then your programs might not work.
                Please provide information on how it is done on your system."

LIBS_NOT_FOUND_1="I can't find the libraries for the thread implementation
		"

LIBS_NOT_FOUND_2=". Please choose another thread implementation or
		provide information on your thread implementation.
		You can also run 'configure --disable-threads' 
		to compile without thread support."

FUNC_NO_GETPWUID_R="the 'g_get_(user_name|real_name|home_dir|tmp_dir)'
		functions will not be MT-safe during their first call because
		there is no working 'getpwuid_r' on your system."

FUNC_NO_LOCALTIME_R="the 'g_date_set_time' function will not be MT-safe
		because there is no 'localtime_r' on your system."

POSIX_NO_YIELD="I can not find a yield functions for your platform. A rather
		crude surrogate will be used. If you happen to know a 
		yield function for your system, please inform the GLib 
		developers."

POSIX_NO_PRIORITIES="I can not find the minimal and maximal priorities for 
		threads on your system. Thus threads can only have the default 
		priority. If you happen to know these main/max
		priorities, please inform the GLib developers."

AIX_COMPILE_INFO="AIX's C compiler needs to be called by a different name, when
		linking threaded applications. As GLib cannot do that 
		automatically, you will get an linkg error everytime you are 
		not using the right compiler. In that case you have to relink 
		with the right compiler. Ususally just '_r' is appended 
		to the compiler name."

dnl determination of thread implementation
dnl ***************************************

# have_threads=no   means no thread support
# have_threads=none means no default thread implementation

have_threads=no
if test "x$want_threads" = xyes || test "x$want_threads" = xposix \
				|| test "x$want_threads" = xdce; then
	# -D_POSIX4_DRAFT_SOURCE -D_POSIX4A_DRAFT10_SOURCE is for DG/UX
	# -U_OSF_SOURCE is for Digital UNIX 4.0d
	GTHREAD_COMPILE_IMPL_DEFINES="-D_POSIX4_DRAFT_SOURCE -D_POSIX4A_DRAFT10_SOURCE -U_OSF_SOURCE"
	glib_save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $GTHREAD_COMPILE_IMPL_DEFINES"
        if test "x$have_threads" = xno; then
                AC_TRY_COMPILE([#include <pthread.h>],
			[pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;],
			have_threads=posix)
        fi
        if test "x$have_threads" = xno; then
                AC_TRY_COMPILE([#include <pthread.h>],
			[pthread_mutex_t m; 
                         pthread_mutex_init (&m, pthread_mutexattr_default);],
			have_threads=dce)
        fi
	# Tru64Unix requires -pthread to find pthread.h. See #103020
	CPPFLAGS="$CPPFLAGS -pthread"
	if test "x$have_threads" = xno; then
	AC_TRY_COMPILE([#include <pthread.h>],
		       [pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;],
		       have_threads=posix)
        fi
	CPPFLAGS="$glib_save_CPPFLAGS"
fi
if test "x$want_threads" = xyes || test "x$want_threads" = xwin32; then
       	case $host in
               	*-*-mingw*)
		have_threads=win32
		;;
	esac
fi
if test "x$want_threads" = xnone; then
	have_threads=none
fi

AC_MSG_CHECKING(for thread implementation)

if test "x$have_threads" = xno && test "x$want_threads" != xno; then
	AC_MSG_RESULT(none available)
        AC_MSG_WARN($THREAD_NO_IMPLEMENTATION)
else
	AC_MSG_RESULT($have_threads)
fi


dnl determination of G_THREAD_CFLAGS
dnl ********************************

G_THREAD_LIBS=
G_THREAD_LIBS_EXTRA=
G_THREAD_CFLAGS=

dnl
dnl Test program for basic POSIX threads functionality
dnl
m4_define([glib_thread_test],[
#include <pthread.h> 
int check_me = 0;
void* func(void* data) {check_me = 42; return &check_me;}
int main()
 { pthread_t t; 
   void *ret;
   pthread_create (&t, $1, func, 0);
   pthread_join (t, &ret);
   return (check_me != 42 || ret != &check_me);
}])

dnl
dnl Test program for sched_get_priority_min()
dnl
m4_define([glib_sched_priority_test],[
#include <sched.h>
#include <errno.h>
int main() {
    errno = 0;
    return sched_get_priority_min(SCHED_OTHER)==-1
 	   && errno != 0;
}])

if test x"$have_threads" != xno; then

  if test x"$have_threads" = xposix; then
    # First we test for posix, whether -pthread or -pthreads do the trick as 
    # both CPPFLAG and LIBS. 
    # One of them does for most gcc versions and some other platforms/compilers
    # too and could be considered as the canonical way to go. 
    case $host in
      *-*-cygwin*|*-*-darwin*)
         # skip cygwin and darwin -pthread or -pthreads test
         ;;
      *-solaris*)
        # These compiler/linker flags work with both Sun Studio and gcc
	# Sun Studio expands -mt to -D_REENTRANT and -lthread
	# gcc expands -pthreads to -D_REENTRANT -D_PTHREADS -lpthread
        G_THREAD_CFLAGS="-D_REENTRANT -D_PTHREADS"
        G_THREAD_LIBS="-lpthread -lthread"
        ;;
      *)
        for flag in pthread pthreads mt; do
          glib_save_CFLAGS="$CFLAGS"
          CFLAGS="$CFLAGS -$flag"
          AC_TRY_RUN(glib_thread_test(0),
                     glib_flag_works=yes,
                     glib_flag_works=no,
                     [AC_LINK_IFELSE(glib_thread_test(0),
                                     glib_flag_works=yes,
                                     glib_flag_works=no)])
          CFLAGS="$glib_save_CFLAGS"
          if test $glib_flag_works = yes ; then
             G_THREAD_CFLAGS=-$flag
	     G_THREAD_LIBS=-$flag
	     break;
          fi
        done
         ;;
    esac 
  fi

  if test x"$G_THREAD_CFLAGS" = x; then

    # The canonical -pthread[s] does not work. Try something different.

    case $host in
	*-aix*)
		if test x"$GCC" = xyes; then
			# GCC 3.0 and above needs -pthread. 
			# Should be coverd by the case above.
			# GCC 2.x and below needs -mthreads
			G_THREAD_CFLAGS="-mthreads"		
			G_THREAD_LIBS=$G_THREAD_CFLAGS
		else 
			# We are probably using the aix compiler. Normaly a 
			# program would have to be compiled with the _r variant
			# of the corresponding compiler, but we as GLib cannot 
			# do that: but the good news is that for compiling the
			# only difference is the added -D_THREAD_SAFE compile 
			# option. This is according to the "C for AIX User's 
			# Guide".
			G_THREAD_CFLAGS="-D_THREAD_SAFE"
		fi
		;;
	*-dg-dgux*)  # DG/UX
		G_THREAD_CFLAGS="-D_REENTRANT -D_POSIX4A_DRAFT10_SOURCE"
		;;
	*-osf*)
		# So we are using dce threads. posix threads are already 
		# catched above.
		G_THREAD_CFLAGS="-threads"
		G_THREAD_LIBS=$G_THREAD_CFLAGS
		;;
	*-sysv5uw7*) # UnixWare 7 
		# We are not using gcc with -pthread. Catched above.
		G_THREAD_CFLAGS="-Kthread"
		G_THREAD_LIBS=$G_THREAD_CFLAGS
		;;
	*-mingw*)
		# No flag needed when using MSVCRT.DLL
		G_THREAD_CFLAGS=""
		;;
	*)
		G_THREAD_CFLAGS="-D_REENTRANT" # good default guess otherwise
		;;
    esac
 
  fi

    # if we are not finding the localtime_r function, then we probably are
    # not using the proper multithread flag

    glib_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $G_THREAD_CFLAGS"

    # First we test, whether localtime_r is declared in time.h
    # directly. Then we test whether a macro localtime_r exists, in
    # which case localtime_r in the test program is replaced and thus
    # if we still find localtime_r in the output, it is not defined as 
    # a macro.

    AC_EGREP_CPP([[^a-zA-Z1-9_]localtime_r[^a-zA-Z1-9_]], [#include <time.h>], ,
      [AC_EGREP_CPP([[^a-zA-Z1-9_]localtime_r[^a-zA-Z1-9_]], [#include <time.h> 
							   localtime_r(a,b)],
      		   AC_MSG_WARN($FLAG_DOES_NOT_WORK))])

    CPPFLAGS="$glib_save_CPPFLAGS"

    AC_MSG_CHECKING(thread related cflags)
    AC_MSG_RESULT($G_THREAD_CFLAGS)
    CPPFLAGS="$CPPFLAGS $G_THREAD_CFLAGS"
fi

dnl determination of G_THREAD_LIBS
dnl ******************************

mutex_has_default=no
case $have_threads in
        posix|dce)
	  glib_save_CPPFLAGS="$CPPFLAGS"
	  CPPFLAGS="$CPPFLAGS $GTHREAD_COMPILE_IMPL_DEFINES"
          if test x"$G_THREAD_LIBS" = x; then
            case $host in
              *-aix*)
                # We are not using gcc (would have set G_THREAD_LIBS) and thus 
                # probably using the aix compiler.
		AC_MSG_WARN($AIX_COMPILE_INFO)
                ;;
              *)
                G_THREAD_LIBS=error
	        glib_save_LIBS="$LIBS"
	        for thread_lib in "" pthread pthread32 pthreads thread dce; do
			if test x"$thread_lib" = x; then
				add_thread_lib=""
				IN=""
			else
				add_thread_lib="-l$thread_lib"
				IN=" in -l$thread_lib"
			fi
			if test x"$have_threads" = xposix; then
				defattr=0
			else
				defattr=pthread_attr_default
			fi
			
			LIBS="$add_thread_lib $glib_save_LIBS"
			
			AC_MSG_CHECKING(for pthread_create/pthread_join$IN)
			AC_TRY_RUN(glib_thread_test($defattr),
                                   glib_result=yes,
                                   glib_result=no,
                                   [AC_LINK_IFELSE(glib_thread_test($defattr),
                                                   glib_result=yes,
                                                   glib_result=no)])
                        AC_MSG_RESULT($glib_result)
			
                        if test "$glib_result" = "yes" ; then
			  G_THREAD_LIBS="$add_thread_lib"
                          break
                        fi
		done
		if test "x$G_THREAD_LIBS" = xerror; then
		  AC_MSG_ERROR($LIBS_NOT_FOUND_1$have_threads$LIBS_NOT_FOUND_2)
		fi 
		LIBS="$glib_save_LIBS"
                ;;
            esac
          fi

          glib_save_LIBS="$LIBS"
	  for thread_lib in "" rt rte; do
	    if test x"$thread_lib" = x; then
	      add_thread_lib=""
	      IN=""
	    else
	      add_thread_lib="-l$thread_lib"
	      IN=" in -l$thread_lib"
	    fi
	    LIBS="$add_thread_lib $glib_save_LIBS"
	    
            AC_MSG_CHECKING(for sched_get_priority_min$IN)
	    AC_TRY_RUN(glib_sched_priority_test,
                       glib_result=yes,
                       glib_result=no,
                       [AC_LINK_IFELSE(glib_sched_priority_test,
                                       glib_result=yes,
                                       glib_result=no)])
	    AC_MSG_RESULT($glib_result)

	    if test "$glib_result" = "yes" ; then	    
 	       G_THREAD_LIBS="$G_THREAD_LIBS $add_thread_lib"
	       posix_priority_min="sched_get_priority_min(SCHED_OTHER)"
	       posix_priority_max="sched_get_priority_max(SCHED_OTHER)"
	       break
            fi
	  done
	  LIBS="$glib_save_LIBS"
          mutex_has_default=yes
          mutex_default_type='pthread_mutex_t'
          mutex_default_init='PTHREAD_MUTEX_INITIALIZER'
          mutex_header_file='pthread.h'
	  if test "x$have_threads" = "xposix"; then
	    g_threads_impl="POSIX"
	  else
	    g_threads_impl="DCE"
	    have_threads="posix"
	  fi
	  AC_SUBST(GTHREAD_COMPILE_IMPL_DEFINES)
          CPPFLAGS="$glib_save_CPPFLAGS"
          ;;
	win32)
	   g_threads_impl="WIN32"
	   ;;
        none|no)
	   g_threads_impl="NONE"
           ;;
        *)
	   g_threads_impl="NONE"
           G_THREAD_LIBS=error
           ;;
esac

if test "x$G_THREAD_LIBS" = xerror; then
        AC_MSG_ERROR($LIBS_NOT_FOUND_1$have_threads$LIBS_NOT_FOUND_2)
fi

case $host in
  *-*-beos*)
    G_THREAD_LIBS="-lbe -lroot -lglib-2.0 "
    G_THREAD_LIBS_EXTRA="-L\$(top_builddir_full)/.libs"
    ;;
  *)
    ;;
esac

AC_MSG_CHECKING(thread related libraries)
AC_MSG_RESULT($G_THREAD_LIBS)

dnl check for mt safe function variants and some posix functions
dnl ************************************************************

if test x"$have_threads" != xno; then
	glib_save_LIBS="$LIBS"
	# we are not doing the following for now, as this might require glib 
	# to always be linked with the thread libs on some platforms. 
	# LIBS="$LIBS $G_THREAD_LIBS"
	AC_CHECK_FUNCS(localtime_r gmtime_r)
	if test "$ac_cv_header_pwd_h" = "yes"; then
	 	AC_CACHE_CHECK([for posix getpwuid_r],
			ac_cv_func_posix_getpwuid_r,
			[AC_TRY_RUN([
#include <errno.h>
#include <pwd.h>
int main () { 
    char buffer[10000];
    struct passwd pwd, *pwptr = &pwd;
    int error;
    errno = 0;
    error = getpwuid_r (0, &pwd, buffer, 
                        sizeof (buffer), &pwptr);
   return (error < 0 && errno == ENOSYS) 
	   || error == ENOSYS; 
}                               ],
				[ac_cv_func_posix_getpwuid_r=yes],
				[ac_cv_func_posix_getpwuid_r=no])])
		GLIB_ASSERT_SET(ac_cv_func_posix_getpwuid_r)
		if test "$ac_cv_func_posix_getpwuid_r" = yes; then
			AC_DEFINE(HAVE_POSIX_GETPWUID_R,1,
				[Have POSIX function getpwuid_r])
		else
	 		AC_CACHE_CHECK([for nonposix getpwuid_r],
				ac_cv_func_nonposix_getpwuid_r,
				[AC_TRY_LINK([#include <pwd.h>],
                                	[char buffer[10000];
                                	struct passwd pwd;
                                	getpwuid_r (0, &pwd, buffer, 
                                        		sizeof (buffer));],
					[ac_cv_func_nonposix_getpwuid_r=yes],
					[ac_cv_func_nonposix_getpwuid_r=no])])
			GLIB_ASSERT_SET(ac_cv_func_nonposix_getpwuid_r)
			if test "$ac_cv_func_nonposix_getpwuid_r" = yes; then
				AC_DEFINE(HAVE_NONPOSIX_GETPWUID_R,1,
					[Have non-POSIX function getpwuid_r])
			fi
		fi
	fi
	if test "$ac_cv_header_grp_h" = "yes"; then
	   	AC_CACHE_CHECK([for posix getgrgid_r],
			ac_cv_func_posix_getgrgid_r,
			[AC_TRY_RUN([
#include <errno.h>
#include <grp.h>
int main () { 
    char buffer[10000];
    struct group grp, *grpptr = &grp;
    int error;
    errno = 0;
    error = getgrgid_r (0, &grp, buffer, 
                        sizeof (buffer), &grpptr);
   return (error < 0 && errno == ENOSYS) 
	   || error == ENOSYS; 
}                              ],
			       [ac_cv_func_posix_getgrgid_r=yes],
			       [ac_cv_func_posix_getgrgid_r=no])])
		GLIB_ASSERT_SET(ac_cv_func_posix_getgrgid_r)
		if test "$ac_cv_func_posix_getgrgid_r" = yes; then
		   	AC_DEFINE(HAVE_POSIX_GETGRGID_R,1,
				[Have POSIX function getgrgid_r])
		else
			AC_CACHE_CHECK([for nonposix getgrgid_r],
				ac_cv_func_nonposix_getgrgid_r,
				[AC_TRY_LINK([#include <grp.h>],
                               		[char buffer[10000];
					struct group grp;	
					getgrgid_r (0, &grp, buffer, 
                                       	sizeof (buffer));],
				[ac_cv_func_nonposix_getgrgid_r=yes],
				[ac_cv_func_nonposix_getgrgid_r=no])])
			GLIB_ASSERT_SET(ac_cv_func_nonposix_getgrgid_r)
			if test "$ac_cv_func_nonposix_getgrgid_r" = yes; then
			   	AC_DEFINE(HAVE_NONPOSIX_GETGRGID_R,1,
					[Have non-POSIX function getgrgid_r])
			fi
		fi
	fi
	LIBS="$G_THREAD_LIBS $LIBS"
	if test x"$have_threads" = xposix; then
		glib_save_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $GTHREAD_COMPILE_IMPL_DEFINES"
		dnl we might grow sizeof(pthread_t) later on, so use a dummy name here
		GLIB_SIZEOF([#include <pthread.h>], pthread_t, system_thread)
		# This is not AC_CHECK_FUNC to also work with function
		# name mangling in header files.
		AC_MSG_CHECKING(for pthread_attr_setstacksize)
		AC_TRY_LINK([#include <pthread.h>],
			[pthread_attr_t t; pthread_attr_setstacksize(&t,0)],
			[AC_MSG_RESULT(yes)
			AC_DEFINE(HAVE_PTHREAD_ATTR_SETSTACKSIZE,1,
				  [Have function pthread_attr_setstacksize])],
			[AC_MSG_RESULT(no)])
		AC_MSG_CHECKING(for minimal/maximal thread priority)
		if test x"$posix_priority_min" = x; then
			AC_EGREP_CPP(PX_PRIO_MIN,[#include <pthread.h>
				PX_PRIO_MIN],,[
				posix_priority_min=PX_PRIO_MIN
				posix_priority_max=PX_PRIO_MAX])
		fi
		if test x"$posix_priority_min" = x; then
			# AIX
			AC_EGREP_CPP(PTHREAD_PRIO_MIN,[#include <pthread.h>
				PTHREAD_PRIO_MIN],,[
				posix_priority_min=PTHREAD_PRIO_MIN
				posix_priority_max=PTHREAD_PRIO_MAX])
		fi
		if test x"$posix_priority_min" = x; then
			AC_EGREP_CPP(PRI_OTHER_MIN,[#include <pthread.h>
				PRI_OTHER_MIN],,[
				posix_priority_min=PRI_OTHER_MIN	
				posix_priority_max=PRI_OTHER_MAX])
		fi
		if test x"$posix_priority_min" = x; then
			AC_MSG_RESULT(none found)
			AC_MSG_WARN($POSIX_NO_PRIORITIES)
	                posix_priority_min=-1
			posix_priority_max=-1
		else
			AC_MSG_RESULT($posix_priority_min/$posix_priority_max)
			AC_MSG_CHECKING(for pthread_setschedparam)
			AC_TRY_LINK([#include <pthread.h>],
		          [pthread_t t; pthread_setschedparam(t, 0, NULL)],
			  [AC_MSG_RESULT(yes)
			AC_DEFINE_UNQUOTED(POSIX_MIN_PRIORITY,$posix_priority_min,[Minimum POSIX RT priority])
			   AC_DEFINE_UNQUOTED(POSIX_MAX_PRIORITY,$posix_priority_max,[Maximum POSIX RT priority])],
                          [AC_MSG_RESULT(no)
                           AC_MSG_WARN($POSIX_NO_PRIORITIES)])
		fi
		posix_yield_func=none
		AC_MSG_CHECKING(for posix yield function)
		for yield_func in sched_yield pthread_yield_np pthread_yield \
							thr_yield; do
			AC_TRY_LINK([#include <pthread.h>],
				[$yield_func()],
				[posix_yield_func="$yield_func"
				break])
		done		
		if test x"$posix_yield_func" = xnone; then
			AC_MSG_RESULT(none found)
			AC_MSG_WARN($POSIX_NO_YIELD)
	                posix_yield_func="g_usleep(1000)"
		else
			AC_MSG_RESULT($posix_yield_func)
			posix_yield_func="$posix_yield_func()"
		fi
		AC_DEFINE_UNQUOTED(POSIX_YIELD_FUNC,$posix_yield_func,[The POSIX RT yield function])
		CPPFLAGS="$glib_save_CPPFLAGS"
           
	elif test x"$have_threads" = xwin32; then
		# It's a pointer to a private struct
		GLIB_SIZEOF(,struct _GThreadData *, system_thread)
	fi

	LIBS="$glib_save_LIBS"

	# now spit out all the warnings.
	if test "$ac_cv_func_posix_getpwuid_r" != "yes" && 
	   test "$ac_cv_func_nonposix_getpwuid_r" != "yes"; then
		AC_MSG_WARN($FUNC_NO_GETPWUID_R)
	fi
	if test "$ac_cv_func_localtime_r" != "yes"; then
		AC_MSG_WARN($FUNC_NO_LOCALTIME_R)
	fi
fi	

if test x"$glib_cv_sizeof_system_thread" = x; then
   # use a pointer as a fallback.
   GLIB_SIZEOF(,void *, system_thread)
fi

#
# Hack to deal with:
# 
#  a) GCC < 3.3 for Linux doesn't include -lpthread when
#     building shared libraries with linux.
#  b) FreeBSD doesn't do this either.
#
case $host in
  *-*-freebsd*|*-*-linux*)
    G_THREAD_LIBS_FOR_GTHREAD="`echo $G_THREAD_LIBS | sed s/-pthread/-lpthread/`"
    ;;
  *) 	 
    G_THREAD_LIBS_FOR_GTHREAD="$G_THREAD_LIBS" 	 
    ;;
esac

AM_CONDITIONAL(HAVE_THREADS, [test "$have_threads" != "none"])
AC_DEFINE_UNQUOTED(G_THREAD_SOURCE,"gthread-$have_threads.c",
		   [Source file containing theread implementation])
AC_SUBST(G_THREAD_CFLAGS)
AC_SUBST(G_THREAD_LIBS)
AC_SUBST(G_THREAD_LIBS_FOR_GTHREAD)
AC_SUBST(G_THREAD_LIBS_EXTRA)

dnl **********************************************
dnl *** GDefaultMutex setup and initialization ***
dnl **********************************************
dnl
dnl if mutex_has_default = yes, we also got
dnl mutex_default_type, mutex_default_init and mutex_header_file
if test $mutex_has_default = yes ; then
	glib_save_CPPFLAGS="$CPPFLAGS"
	glib_save_LIBS="$LIBS"
	LIBS="$G_THREAD_LIBS $LIBS"
	CPPFLAGS="$CPPFLAGS $GTHREAD_COMPILE_IMPL_DEFINES"
	GLIB_SIZEOF([#include <$mutex_header_file>],
                    $mutex_default_type,
                    gmutex)
	GLIB_BYTE_CONTENTS([#include <$mutex_header_file>],
			   $mutex_default_type,
			   gmutex,
			   $glib_cv_sizeof_gmutex,
			   $mutex_default_init)
	if test x"$glib_cv_byte_contents_gmutex" = xno; then
		mutex_has_default=no
	fi
	CPPFLAGS="$glib_save_CPPFLAGS"
	LIBS="$glib_save_LIBS"
fi

AC_CHECK_FUNCS(clock_gettime, [], [
  AC_CHECK_LIB(rt, clock_gettime, [
    AC_DEFINE(HAVE_CLOCK_GETTIME, 1)
    G_THREAD_LIBS="$G_THREAD_LIBS -lrt"
    G_THREAD_LIBS_FOR_GTHREAD="$G_THREAD_LIBS_FOR_GTHREAD -lrt"
  ])
])

AC_CACHE_CHECK(for monotonic clocks,
    glib_cv_monotonic_clock,AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <time.h>
#include <unistd.h>
int main() {
#if !(defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0 && defined(CLOCK_MONOTONIC))
        #error No monotonic clock
#endif
	return 0;
}
]])],glib_cv_monotonic_clock=yes,glib_cv_monotonic_clock=no))
if test "$glib_cv_monotonic_clock" = "yes"; then
  AC_DEFINE(HAVE_MONOTONIC_CLOCK,1,[Have a monotonic clock])
fi


dnl ********************************
dnl *** g_atomic_* tests for gcc ***
dnl ********************************

AC_MSG_CHECKING([whether to use assembler code for atomic operations])

glib_cv_gcc_has_builtin_atomic_operations=no
if test x"$GCC" = xyes; then
  AC_MSG_CHECKING([whether GCC supports build-in atomic intrinsics])
  AC_TRY_LINK([],
	      [int i;
	       __sync_synchronize ();
	       __sync_bool_compare_and_swap (&i, 0, 1);
	       __sync_fetch_and_add (&i, 1);
	      ],
	      [glib_cv_gcc_has_builtin_atomic_operations=yes],
	      [glib_cv_gcc_has_builtin_atomic_operations=no])

  AC_MSG_RESULT($glib_cv_gcc_has_builtin_atomic_operations)
  if test $glib_cv_gcc_has_builtin_atomic_operations = yes; then
    glib_memory_barrier_needed=yes
  else
    case $host_cpu in
      i386)
        AC_MSG_RESULT([none])
        glib_memory_barrier_needed=no
        ;;
      i?86)
        AC_MSG_RESULT([i486])
        AC_DEFINE_UNQUOTED(G_ATOMIC_I486, 1,
			   [i486 atomic implementation])
        glib_memory_barrier_needed=no
        ;;
      sparc*)
        SPARCV9_WARNING="Try to rerun configure with CFLAGS='-mcpu=v9',
			 when you are using a sparc with v9 instruction set (most
			 sparcs nowadays). This will make the code for atomic
			 operations much faster. The resulting code will not run
			 on very old sparcs though."

        AC_LINK_IFELSE([[
          main ()
          {
            int tmp1, tmp2, tmp3;
            __asm__ __volatile__("casx [%2], %0, %1"
                                 : "=&r" (tmp1), "=&r" (tmp2) : "r" (&tmp3));
          }]],
          AC_MSG_RESULT([sparcv9])
          AC_DEFINE_UNQUOTED(G_ATOMIC_SPARCV9, 1,
			     [sparcv9 atomic implementation]),
          AC_MSG_RESULT([no])
          AC_MSG_WARN([[$SPARCV9_WARNING]]))
        glib_memory_barrier_needed=yes
        ;;
      alpha*)
        AC_MSG_RESULT([alpha])
        AC_DEFINE_UNQUOTED(G_ATOMIC_ALPHA, 1,
			   [alpha atomic implementation])
        glib_memory_barrier_needed=yes
        ;;
      x86_64)
        AC_MSG_RESULT([x86_64])
        AC_DEFINE_UNQUOTED(G_ATOMIC_X86_64, 1,
			   [x86_64 atomic implementation])
        glib_memory_barrier_needed=no
       ;;
      powerpc*)
        AC_MSG_RESULT([powerpc])
        AC_DEFINE_UNQUOTED(G_ATOMIC_POWERPC, 1,
			   [powerpc atomic implementation])
        glib_memory_barrier_needed=yes
        AC_MSG_CHECKING([whether asm supports numbered local labels])
        AC_TRY_COMPILE(
		       ,[
		       __asm__ __volatile__ ("1:       nop\n"
			       "         bne-    1b")
		       ],[
		       AC_DEFINE_UNQUOTED(ASM_NUMERIC_LABELS, 1, [define if asm blocks can use numeric local labels])
		       AC_MSG_RESULT([yes])
		       ],[
		       AC_MSG_RESULT([no])
		       ])
        ;;
      ia64)
        AC_MSG_RESULT([ia64])
        AC_DEFINE_UNQUOTED(G_ATOMIC_IA64, 1,
			   [ia64 atomic implementation])
        glib_memory_barrier_needed=yes
        ;;
      s390|s390x)
        AC_MSG_RESULT([s390])
        AC_DEFINE_UNQUOTED(G_ATOMIC_S390, 1,
			   [s390 atomic implementation])
        glib_memory_barrier_needed=no
        ;;
      arm*)
        AC_MSG_RESULT([arm])
        AC_DEFINE_UNQUOTED(G_ATOMIC_ARM, 1,
			   [arm atomic implementation])
        glib_memory_barrier_needed=no
        ;;
      crisv32*|etraxfs*)
        AC_MSG_RESULT([crisv32])
        AC_DEFINE_UNQUOTED(G_ATOMIC_CRISV32, 1,
			   [crisv32 atomic implementation])
        glib_memory_barrier_needed=no
        ;;
      cris*|etrax*)
        AC_MSG_RESULT([cris])
        AC_DEFINE_UNQUOTED(G_ATOMIC_CRIS, 1,
			   [cris atomic implementation])
        glib_memory_barrier_needed=no
        ;;
      *)
        AC_MSG_RESULT([none])
        glib_memory_barrier_needed=yes
        ;;
    esac
  fi
else
  if test $glib_native_win32 = yes; then
    # For Windows but not using gcc. No barriers needed then either.
    glib_memory_barrier_needed=no
  fi
fi

AM_CONDITIONAL(HAVE_GCC_BUILTINS_FOR_ATOMIC_OPERATIONS,
	       [test $glib_cv_gcc_has_builtin_atomic_operations = yes])

dnl ************************
dnl ** Check for futex(2) **
dnl ************************
AC_MSG_CHECKING([for futex(2) system call])
AC_COMPILE_IFELSE([ 
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>

int
main (void)
{
  /* it's not like this actually runs or anything... */
  syscall (SYS_futex, NULL, FUTEX_WAKE, FUTEX_WAIT);
  return 0;
}
],
[
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_FUTEX, [test "$have_futex" = "yes"],
            [we have the futex(2) system call])
],
[
  AC_MSG_RESULT(no)
])

dnl ****************************************
dnl *** GLib POLL* compatibility defines ***
dnl ****************************************

glib_poll_includes=["
#include <sys/types.h>
#include <sys/poll.h>
"]

if test $ac_cv_header_sys_types_h = yes &&
   test $ac_cv_header_sys_poll_h = yes ; then
  glib_failed=false
  GLIB_CHECK_VALUE(POLLIN, $glib_poll_includes, glib_failed=true)
  GLIB_CHECK_VALUE(POLLOUT, $glib_poll_includes, glib_failed=true)
  GLIB_CHECK_VALUE(POLLPRI, $glib_poll_includes, glib_failed=true)
  GLIB_CHECK_VALUE(POLLERR, $glib_poll_includes, glib_failed=true)
  GLIB_CHECK_VALUE(POLLHUP, $glib_poll_includes, glib_failed=true)
  GLIB_CHECK_VALUE(POLLNVAL, $glib_poll_includes, glib_failed=true)
  if $glib_failed ; then
    AC_MSG_ERROR([Could not determine values for POLL* constants])
  fi
else
  glib_cv_value_POLLIN=1
  glib_cv_value_POLLOUT=4
  glib_cv_value_POLLPRI=2
  glib_cv_value_POLLERR=8
  glib_cv_value_POLLHUP=16
  glib_cv_value_POLLNVAL=32
fi

AC_MSG_CHECKING([for broken poll])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
	#include <stdlib.h>
	#include <fcntl.h>
	#include <poll.h>
	#ifdef HAVE_SYS_POLL_H
	#include <sys/poll.h>
	#endif
	int main(void) {
	  struct pollfd fds[1];
	  int fd;
	  fd = open("/dev/null", 1);
	  fds[0].fd = fd;
	  fds[0].events = POLLIN;
	  fds[0].revents = 0;
	  if (poll(fds, 1, 0) < 0 || (fds[0].revents & POLLNVAL) != 0) {
		exit(1);  /* Does not work for devices -- fail */
	  }
	  exit(0);
	}]])],
  [broken_poll=no],
  [broken_poll=yes
   AC_DEFINE(BROKEN_POLL,1,[poll doesn't work on devices])],
  [broken_poll="no (cross compiling)"])
AC_MSG_RESULT($broken_poll)

dnl *********************
dnl *** GRegex checks ***
dnl *********************
PCRE_REQUIRED_VERSION=7.2

# Check if we should compile GRegex
AC_ARG_ENABLE(regex, AC_HELP_STRING([--disable-regex],
              [disable the compilation of GRegex]),
[case "${enableval}" in
  yes) enable_regex=true ;;
  no)  enable_regex=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-regex) ;;
esac],
[enable_regex=true])

AM_CONDITIONAL(ENABLE_REGEX, $enable_regex)

if test x$enable_regex = xtrue; then
  AC_DEFINE(ENABLE_REGEX, [], [include GRegex])
  # Check if we should use the internal or the system-supplied pcre
  AC_ARG_WITH(pcre,
             [AC_HELP_STRING([--with-pcre=@<:@internal/system@:>@],
                             [specify whether to use the internal or the
                              system-supplied PCRE library])])

  AM_CONDITIONAL(USE_SYSTEM_PCRE, [test "x$with_pcre" = xsystem])

  if test "x$with_pcre" = xsystem; then
    PKG_CHECK_MODULES(PCRE,
                      libpcre >= $PCRE_REQUIRED_VERSION)
    AC_CACHE_CHECK([for Unicode support in PCRE],glib_cv_pcre_has_unicode,[
                   glib_save_CFLAGS="$CFLAGS"
                   glib_save_LIBS="$LIBS"
                   CFLAGS="$CFLAGS $PCRE_CFLAGS" LIBS="$PCRE_LIBS"
                   AC_TRY_RUN([#include <pcre.h>
                               int main () {
                                 int support;
                                 pcre_config (PCRE_CONFIG_UTF8, &support);
                                 if (!support)
                                   return 1;
                                 pcre_config (PCRE_CONFIG_UNICODE_PROPERTIES, &support);
                                 if (!support)
                                   return 1;
                                 return 0;
                               }],
                   glib_cv_pcre_has_unicode=yes,
                   glib_cv_pcre_has_unicode=no,
                   glib_cv_pcre_has_unicode=yes)
                   CFLAGS="$glib_save_CFLAGS"
                   LIBS="$glib_save_LIBS"
	])
    if test "$glib_cv_pcre_has_unicode" = "no"; then
      AC_MSG_ERROR([*** The system-supplied PCRE does not support Unicode properties or UTF-8.])
    fi
    AC_SUBST(PCRE_CFLAGS)
    AC_SUBST(PCRE_LIBS)
    AC_DEFINE(USE_SYSTEM_PCRE, [], [using the system-supplied PCRE library])
    PCRE_REQUIRES=libpcre
    AC_SUBST(PCRE_REQUIRES)
  else
    # If using gcc 4 pass -Wno-pointer-sign when compiling the internal PCRE
    if test x"$GCC" = xyes; then
      AC_MSG_CHECKING([whether compiler understands -Wno-pointer-sign])
      save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Wno-pointer-sign"
      AC_TRY_COMPILE([],[],[PCRE_WARN_CFLAGS="$PCRE_WARN_CFLAGS -Wno-pointer-sign"
			    AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
      CFLAGS="$save_CFLAGS"
    fi
  fi
  AC_SUBST(PCRE_WARN_CFLAGS)
else
  AM_CONDITIONAL(USE_SYSTEM_PCRE, false])
fi

dnl **********************
dnl *** Win32 API libs ***
dnl **********************

case $host in
  *-*-cygwin*)
	G_LIBS_EXTRA="-luser32 -lkernel32"
    ;;
  *-*-mingw*)
	G_LIBS_EXTRA="-lws2_32 -lole32"
    ;;
  *)
	G_LIBS_EXTRA=""
    ;;
esac
AC_SUBST(G_LIBS_EXTRA)

dnl If the system doesn't define EILSEQ, we should define EILSEQ ourselves
dnl since we need it for g_iconv()

AC_MSG_CHECKING([for EILSEQ])
AC_TRY_COMPILE([
#include <errno.h>
],
[
int error = EILSEQ;
], have_eilseq=yes, have_eilseq=no);
AC_MSG_RESULT($have_eilseq)

dnl ******************************************************************
dnl *** If we are cross-compiling, look for glib-genmarshal and    ***
dnl *** glib-compile-schemas in PATH                               ***
dnl ******************************************************************

AM_CONDITIONAL(CROSS_COMPILING, test $cross_compiling = yes)

if test $cross_compiling = yes; then
  AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
  if test x$GLIB_GENMARSHAL = xno; then
    AC_MSG_ERROR(Could not find a glib-genmarshal in your PATH)
  fi

  AC_PATH_PROG(GLIB_COMPILE_SCHEMAS, glib-compile-schemas, no)
  if test x$GLIB_COMPILE_SCHEMAS = xno; then
    AC_MSG_ERROR(Could not find a glib-compile-schemas in your PATH)
  fi
fi

dnl **************************
dnl *** Checks for gtk-doc ***
dnl **************************

GTK_DOC_CHECK([1.15])

AC_ARG_ENABLE(man,
              [AC_HELP_STRING([--enable-man],
                              [regenerate man pages from Docbook [default=no]])],enable_man=yes,
              enable_man=no)

if test "${enable_man}" != no; then
dnl
dnl Check for xsltproc
dnl
AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man=no
  fi

  dnl check for DocBook DTD and stylesheets in the local catalog.
  JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
     [DocBook XML DTD V4.1.2],,enable_man=no)
  JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
     [DocBook XSL Stylesheets],,enable_man=no)
fi

AM_CONDITIONAL(ENABLE_MAN, test x$enable_man != xno)

dnl
dnl Tracing
dnl

AC_ARG_ENABLE([dtrace],
             [AS_HELP_STRING([--enable-dtrace],
                              [Enable inclusion of dtrace trace support])])
have_dtrace=no
AC_MSG_CHECKING([whether to include dtrace tracing support])
if test "x$enable_dtrace" != xno; then
  if test x$glib_have_carbon = xyes; then
    AC_MSG_RESULT([no (not yet compatible with MacOS dtrace)])
  else 
    AC_MSG_RESULT([yes])
    AC_CHECK_PROGS(DTRACE, dtrace)
    if test -z "$DTRACE"; then
      if test "x$enable_dtrace" = xyes; then
        AC_MSG_ERROR([dtrace not found])
      fi
    fi
    AC_CHECK_HEADER([sys/sdt.h],have_dtrace=yes,
                    [if test "x$enable_dtrace" = xyes; then
                      AC_MSG_ERROR([dtrace support needs sys/sdt.h header])
                     fi])
  fi
else
  AC_MSG_RESULT([no])
fi
if test "x$have_dtrace" = xyes; then
  AC_DEFINE([HAVE_DTRACE], [1], [Define to 1 if using dtrace probes.])
fi
AM_CONDITIONAL([ENABLE_DTRACE], [test x$have_dtrace = xyes ])

AC_MSG_CHECKING([whether to include systemtap tracing support])
AC_ARG_ENABLE([systemtap],
             [AS_HELP_STRING([--enable-systemtap],
                              [Enable inclusion of systemtap trace support])])
have_systemtap=no
if test "x$enable_systemtap" != xno -a "x$have_dtrace" = xyes; then
  have_systemtap=yes
fi
AC_MSG_RESULT(${have_systemtap})

AM_CONDITIONAL([ENABLE_SYSTEMTAP], [test x$have_systemtap = xyes])

AC_ARG_WITH([tapset-install-dir],
            [AS_HELP_STRING([--with-tapset-install-dir],
            [The absolute path where the systemtap tapsets will be installed])],
            [if test "x${withval}" = x; then
	       ABS_TAPSET_DIR="\$(datadir)/systemtap/tapset"
	     else
	       ABS_TAPSET_DIR="${withval}"
	      fi],
	    [ABS_TAPSET_DIR="\$(datadir)/systemtap/tapset"])
AC_SUBST(ABS_TAPSET_DIR)

dnl ************************************
dnl *** Enable lcov coverage reports ***
dnl ************************************

AC_ARG_ENABLE(gcov,
  AS_HELP_STRING([--enable-gcov],
		 [Enable gcov]),
  [use_gcov=$enableval], [use_gcov=no])

if test "x$use_gcov" = "xyes"; then
  dnl we need gcc:
  if test "$GCC" != "yes"; then
    AC_MSG_ERROR([GCC is required for --enable-gcov])
  fi

  dnl Check if ccache is being used
  AC_CHECK_PROG(SHTOOL, shtool, shtool)
  case `$SHTOOL path $CC` in
    *ccache*[)] gcc_ccache=yes;;
    *[)] gcc_ccache=no;;
  esac

  if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
    AC_MSG_ERROR([ccache must be disabled when --enable-gcov option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
  fi

  ltp_version_list="1.6 1.7 1.8"
  AC_CHECK_PROG(LTP, lcov, lcov)
  AC_CHECK_PROG(LTP_GENHTML, genhtml, genhtml)

  if test "$LTP"; then
    AC_CACHE_CHECK([for ltp version], glib_cv_ltp_version, [
      glib_cv_ltp_version=invalid
      ltp_version=`$LTP -v 2>/dev/null | $SED -e 's/^.* //'`
      for ltp_check_version in $ltp_version_list; do
        if test "$ltp_version" = "$ltp_check_version"; then
          glib_cv_ltp_version="$ltp_check_version (ok)"
        fi
      done
    ])
  else
    ltp_msg="To enable code coverage reporting you must have one of the following LTP versions installed: $ltp_version_list"
    AC_MSG_ERROR([$ltp_msg])
  fi

  case $glib_cv_ltp_version in
    ""|invalid[)]
      ltp_msg="You must have one of the following versions of LTP: $ltp_version_list (found: $ltp_version)."
      AC_MSG_ERROR([$ltp_msg])
      LTP="exit 0;"
      ;;
  esac

  if test -z "$LTP_GENHTML"; then
    AC_MSG_ERROR([Could not find genhtml from the LTP package])
  fi

  AC_DEFINE(HAVE_GCOV, 1, [Whether you have gcov])

  dnl Remove all optimization flags from CFLAGS
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9]*//g'`
  changequote([,])

  dnl Add the special gcc flags
  CFLAGS="$CFLAGS -O0 -fprofile-arcs -ftest-coverage"
  LDFLAGS="$LDFLAGS -lgcov"
fi

dnl ******************************
dnl *** output the whole stuff ***
dnl ******************************

dnl this section will only be run if config.status is invoked with no
dnl arguments, or with "glib/glibconfig.h" as an argument.
AC_CONFIG_COMMANDS([glib/glibconfig.h],
[
	outfile=glib/glibconfig.h-tmp
	cat > $outfile <<\_______EOF
/* glibconfig.h
 *
 * This is a generated file.  Please modify 'configure.ac'
 */

#ifndef __G_LIBCONFIG_H__
#define __G_LIBCONFIG_H__

#include <glib/gmacros.h>

_______EOF

	if test x$glib_limits_h = xyes; then
	  echo '#include <limits.h>' >> $outfile
	fi
	if test x$glib_float_h = xyes; then
	  echo '#include <float.h>' >> $outfile
	fi
	if test x$glib_values_h = xyes; then
	  echo '#include <values.h>' >> $outfile
	fi
	if test "$glib_header_alloca_h" = "yes"; then
	  echo '#define GLIB_HAVE_ALLOCA_H' >> $outfile
	fi
	if test x$glib_sys_poll_h = xyes; then
	  echo '#define GLIB_HAVE_SYS_POLL_H' >> $outfile
	fi
	if test x$glib_included_printf != xyes; then
          echo "
/* Specifies that GLib's g_print*() functions wrap the
 * system printf functions.  This is useful to know, for example,
 * when using glibc's register_printf_function().
 */" >> $outfile
	  echo '#define GLIB_USING_SYSTEM_PRINTF' >> $outfile
	fi

	cat >> $outfile <<_______EOF

G_BEGIN_DECLS

#define G_MINFLOAT	$glib_mf
#define G_MAXFLOAT	$glib_Mf
#define G_MINDOUBLE	$glib_md
#define G_MAXDOUBLE	$glib_Md
#define G_MINSHORT	$glib_ms
#define G_MAXSHORT	$glib_Ms
#define G_MAXUSHORT	$glib_Mus
#define G_MININT	$glib_mi
#define G_MAXINT	$glib_Mi
#define G_MAXUINT	$glib_Mui
#define G_MINLONG	$glib_ml
#define G_MAXLONG	$glib_Ml
#define G_MAXULONG	$glib_Mul

_______EOF


	### this should always be true in a modern C/C++ compiler
	cat >>$outfile <<_______EOF
typedef signed char gint8;
typedef unsigned char guint8;
_______EOF


	if test -n "$gint16"; then
	  cat >>$outfile <<_______EOF
typedef signed $gint16 gint16;
typedef unsigned $gint16 guint16;
#define G_GINT16_MODIFIER $gint16_modifier
#define G_GINT16_FORMAT $gint16_format
#define G_GUINT16_FORMAT $guint16_format
_______EOF
	fi


	if test -n "$gint32"; then
	  cat >>$outfile <<_______EOF
typedef signed $gint32 gint32;
typedef unsigned $gint32 guint32;
#define G_GINT32_MODIFIER $gint32_modifier
#define G_GINT32_FORMAT $gint32_format
#define G_GUINT32_FORMAT $guint32_format
_______EOF
	fi

	cat >>$outfile <<_______EOF
#define G_HAVE_GINT64 1          /* deprecated, always true */

${glib_extension}typedef signed $gint64 gint64;
${glib_extension}typedef unsigned $gint64 guint64;

#define G_GINT64_CONSTANT(val)	$gint64_constant
#define G_GUINT64_CONSTANT(val)	$guint64_constant
_______EOF

	if test x$gint64_format != x ; then
	  cat >>$outfile <<_______EOF
#define G_GINT64_MODIFIER $gint64_modifier
#define G_GINT64_FORMAT $gint64_format
#define G_GUINT64_FORMAT $guint64_format
_______EOF
        else
	  cat >>$outfile <<_______EOF
#undef G_GINT64_MODIFIER
#undef G_GINT64_FORMAT
#undef G_GUINT64_FORMAT
_______EOF
        fi           

        cat >>$outfile <<_______EOF

#define GLIB_SIZEOF_VOID_P $glib_void_p
#define GLIB_SIZEOF_LONG   $glib_long
#define GLIB_SIZEOF_SIZE_T $glib_size_t

_______EOF

        cat >>$outfile <<_______EOF
typedef signed $glib_size_type_define gssize;
typedef unsigned $glib_size_type_define gsize;
#define G_GSIZE_MODIFIER $gsize_modifier
#define G_GSSIZE_FORMAT $gssize_format
#define G_GSIZE_FORMAT $gsize_format

#define G_MAXSIZE	G_MAXU$glib_msize_type
#define G_MINSSIZE	G_MIN$glib_msize_type
#define G_MAXSSIZE	G_MAX$glib_msize_type

typedef gint64 goffset;
#define G_MINOFFSET	G_MININT64
#define G_MAXOFFSET	G_MAXINT64

#define G_GOFFSET_MODIFIER      G_GINT64_MODIFIER
#define G_GOFFSET_FORMAT        G_GINT64_FORMAT
#define G_GOFFSET_CONSTANT(val) G_GINT64_CONSTANT(val)

_______EOF

	if test -z "$glib_unknown_void_p"; then
	  cat >>$outfile <<_______EOF

#define GPOINTER_TO_INT(p)	((gint)  ${glib_gpi_cast} (p))
#define GPOINTER_TO_UINT(p)	((guint) ${glib_gpui_cast} (p))

#define GINT_TO_POINTER(i)	((gpointer) ${glib_gpi_cast} (i))
#define GUINT_TO_POINTER(u)	((gpointer) ${glib_gpui_cast} (u))

typedef signed $glib_intptr_type_define gintptr;
typedef unsigned $glib_intptr_type_define guintptr;

#define G_GINTPTR_MODIFIER      $gintptr_modifier
#define G_GINTPTR_FORMAT        $gintptr_format
#define G_GUINTPTR_FORMAT       $guintptr_format
_______EOF
	else
	  echo '#error SIZEOF_VOID_P unknown - This should never happen' >>$outfile
	fi



	cat >>$outfile <<_______EOF
$glib_atexit
$glib_memmove
$glib_defines
$glib_os
$glib_static_compilation

$glib_vacopy

#ifdef	__cplusplus
#define	G_HAVE_INLINE	1
#else	/* !__cplusplus */
$glib_inline
#endif	/* !__cplusplus */

#ifdef	__cplusplus
#define G_CAN_INLINE	1
_______EOF

	if test x$g_can_inline = xyes ; then
		cat >>$outfile <<_______EOF
#else	/* !__cplusplus */
#define G_CAN_INLINE	1
_______EOF
	fi

	cat >>$outfile <<_______EOF
#endif

_______EOF

	if test x$g_have_iso_c_varargs = xyes ; then
		cat >>$outfile <<_______EOF
#ifndef __cplusplus
# define G_HAVE_ISO_VARARGS 1
#endif
_______EOF
	fi
	if test x$g_have_iso_cxx_varargs = xyes ; then
		cat >>$outfile <<_______EOF
#ifdef __cplusplus
# define G_HAVE_ISO_VARARGS 1
#endif
_______EOF
	fi
	if test x$g_have_gnuc_varargs = xyes ; then
		cat >>$outfile <<_______EOF

/* gcc-2.95.x supports both gnu style and ISO varargs, but if -ansi
 * is passed ISO vararg support is turned off, and there is no work
 * around to turn it on, so we unconditionally turn it off.
 */
#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
#  undef G_HAVE_ISO_VARARGS
#endif

#define G_HAVE_GNUC_VARARGS 1
_______EOF
	fi

	case x$g_stack_grows in
	xyes) echo "#define G_HAVE_GROWING_STACK 1" >>$outfile ;;
	*)    echo "#define G_HAVE_GROWING_STACK 0" >>$outfile ;;
	esac


	echo >>$outfile
	if test x$g_have_eilseq = xno; then
		cat >>$outfile <<_______EOF
#ifndef EILSEQ
/* On some systems, like SunOS and NetBSD, EILSEQ is not defined.
 * The correspondence between this and the corresponding definition
 * in libiconv is essential.
 */
#  define EILSEQ ENOENT
#endif
_______EOF

	fi

	if test x$g_have_gnuc_visibility = xyes; then
		cat >>$outfile <<_______EOF
#define G_HAVE_GNUC_VISIBILITY 1
_______EOF
	fi
		cat >>$outfile <<_______EOF
#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define G_GNUC_INTERNAL __attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define G_GNUC_INTERNAL __hidden
#elif defined (__GNUC__) && defined (G_HAVE_GNUC_VISIBILITY)
#define G_GNUC_INTERNAL __attribute__((visibility("hidden")))
#else
#define G_GNUC_INTERNAL
#endif 
_______EOF


	echo >>$outfile
	if test x$g_mutex_has_default = xyes; then
		cat >>$outfile <<_______EOF
$g_enable_threads_def G_THREADS_ENABLED
#define G_THREADS_IMPL_$g_threads_impl_def
typedef struct _GStaticMutex GStaticMutex;
struct _GStaticMutex
{
  struct _GMutex *runtime_mutex;
  union {
    char   pad[[$g_mutex_sizeof]];
    double dummy_double;
    void  *dummy_pointer;
    long   dummy_long;
  } static_mutex;
};
#define	G_STATIC_MUTEX_INIT	{ NULL, { { $g_mutex_contents} } }
#define	g_static_mutex_get_mutex(mutex) \\
  (g_thread_use_default_impl ? ((GMutex*)(gpointer) ((mutex)->static_mutex.pad)) : \\
   g_static_mutex_get_mutex_impl_shortcut (&((mutex)->runtime_mutex)))
_______EOF
	else
		cat >>$outfile <<_______EOF
$g_enable_threads_def G_THREADS_ENABLED
#define G_THREADS_IMPL_$g_threads_impl_def
typedef struct _GMutex* GStaticMutex;
#define G_STATIC_MUTEX_INIT NULL
#define g_static_mutex_get_mutex(mutex) \\
  (g_static_mutex_get_mutex_impl_shortcut (mutex))
_______EOF
	fi

	cat >>$outfile <<_______EOF
/* This represents a system thread as used by the implementation. An
 * alien implementaion, as loaded by g_thread_init can only count on
 * "sizeof (gpointer)" bytes to store their info. We however need more
 * for some of our native implementations. */
typedef union _GSystemThread GSystemThread;
union _GSystemThread
{
  char   data[[$g_system_thread_sizeof]];
  double dummy_double;
  void  *dummy_pointer;
  long   dummy_long;
};
_______EOF
	if test x"$g_memory_barrier_needed" != xno; then
	  echo >>$outfile
	  echo "#define G_ATOMIC_OP_MEMORY_BARRIER_NEEDED 1" >>$outfile
	fi

	echo >>$outfile
	g_bit_sizes="16 32 64"
	for bits in $g_bit_sizes; do
	  cat >>$outfile <<_______EOF
#define GINT${bits}_TO_${g_bs_native}(val)	((gint${bits}) (val))
#define GUINT${bits}_TO_${g_bs_native}(val)	((guint${bits}) (val))
#define GINT${bits}_TO_${g_bs_alien}(val)	((gint${bits}) GUINT${bits}_SWAP_LE_BE (val))
#define GUINT${bits}_TO_${g_bs_alien}(val)	(GUINT${bits}_SWAP_LE_BE (val))
_______EOF
	done

	cat >>$outfile <<_______EOF
#define GLONG_TO_LE(val)	((glong) GINT${glongbits}_TO_LE (val))
#define GULONG_TO_LE(val)	((gulong) GUINT${glongbits}_TO_LE (val))
#define GLONG_TO_BE(val)	((glong) GINT${glongbits}_TO_BE (val))
#define GULONG_TO_BE(val)	((gulong) GUINT${glongbits}_TO_BE (val))
#define GINT_TO_LE(val)		((gint) GINT${gintbits}_TO_LE (val))
#define GUINT_TO_LE(val)	((guint) GUINT${gintbits}_TO_LE (val))
#define GINT_TO_BE(val)		((gint) GINT${gintbits}_TO_BE (val))
#define GUINT_TO_BE(val)	((guint) GUINT${gintbits}_TO_BE (val))
#define GSIZE_TO_LE(val)	((gsize) GUINT${gsizebits}_TO_LE (val))
#define GSSIZE_TO_LE(val)	((gssize) GINT${gsizebits}_TO_LE (val))
#define GSIZE_TO_BE(val)	((gsize) GUINT${gsizebits}_TO_BE (val))
#define GSSIZE_TO_BE(val)	((gssize) GINT${gsizebits}_TO_BE (val))
#define G_BYTE_ORDER $g_byte_order

#define GLIB_SYSDEF_POLLIN =$g_pollin
#define GLIB_SYSDEF_POLLOUT =$g_pollout
#define GLIB_SYSDEF_POLLPRI =$g_pollpri
#define GLIB_SYSDEF_POLLHUP =$g_pollhup
#define GLIB_SYSDEF_POLLERR =$g_pollerr
#define GLIB_SYSDEF_POLLNVAL =$g_pollnval

#define G_MODULE_SUFFIX "$g_module_suffix"

/* A GPid is an abstraction for a process "handle". It is *not* an
 * abstraction for a process identifier in general. GPid is used in
 * GLib only for descendant processes spawned with the g_spawn*
 * functions. On POSIX there is no "process handle" concept as such,
 * but on Windows a GPid is a handle to a process, a kind of pointer,
 * not a process identifier.
 */
typedef $g_pid_type GPid;

#define GLIB_SYSDEF_AF_UNIX $g_af_unix
#define GLIB_SYSDEF_AF_INET $g_af_inet
#define GLIB_SYSDEF_AF_INET6 $g_af_inet6

#define GLIB_SYSDEF_MSG_OOB $g_msg_oob
#define GLIB_SYSDEF_MSG_PEEK $g_msg_peek
#define GLIB_SYSDEF_MSG_DONTROUTE $g_msg_dontroute

G_END_DECLS

#endif /* GLIBCONFIG_H */
_______EOF


	if cmp -s $outfile glib/glibconfig.h; then
	  AC_MSG_NOTICE([glib/glibconfig.h is unchanged])
	  rm -f $outfile
	else
	  mv $outfile glib/glibconfig.h
	fi
],[

# Note that if two cases are the same, case goes with the first one.
# Note also that this is inside an AC_OUTPUT_COMMAND.  We do not depend
# on variable expansion in case labels.  Look at the generated config.status
# for a hint.

if test "x${ac_cv_working_alloca_h+set}" = xset ; then
  glib_header_alloca_h="$ac_cv_working_alloca_h"
else
  glib_header_alloca_h="$ac_cv_header_alloca_h"
fi

case xyes in
x$ac_cv_header_float_h)
  glib_float_h=yes
  glib_mf=FLT_MIN glib_Mf=FLT_MAX
  glib_md=DBL_MIN glib_Md=DBL_MAX
  ;;
x$ac_cv_header_values_h)
  glib_values_h=yes
  glib_mf=MINFLOAT  glib_Mf=MAXFLOAT
  glib_md=MINDOUBLE glib_Md=MAXDOUBLE
  ;;
esac

case xyes in
x$ac_cv_header_limits_h)
  glib_limits_h=yes
  glib_ms=SHRT_MIN glib_Ms=SHRT_MAX glib_Mus=USHRT_MAX
  glib_mi=INT_MIN  glib_Mi=INT_MAX  glib_Mui=UINT_MAX
  glib_ml=LONG_MIN glib_Ml=LONG_MAX glib_Mul=ULONG_MAX
  ;;
x$ac_cv_header_values_h)
  glib_values_h=yes
  glib_ms=MINSHORT glib_Ms=MAXSHORT glib_Mus="(((gushort)G_MAXSHORT)*2+1)"
  glib_mi=MININT   glib_Mi=MAXINT   glib_Mui="(((guint)G_MAXINT)*2+1)"
  glib_ml=MINLONG  glib_Ml=MAXLONG  glib_Mul="(((gulong)G_MAXLONG)*2+1)"
  ;;
esac

if test x$ac_cv_header_sys_poll_h = xyes ; then
  glib_sys_poll_h=yes
fi

if test x$enable_included_printf = xyes ; then
  glib_included_printf=yes
fi

case 2 in
$ac_cv_sizeof_short)		
  gint16=short
  gint16_modifier='"h"'
  gint16_format='"hi"'
  guint16_format='"hu"'
  ;;
$ac_cv_sizeof_int)		
  gint16=int
  gint16_modifier='""'
  gint16_format='"i"'
  guint16_format='"u"'
  ;;
esac
case 4 in
$ac_cv_sizeof_short)		
  gint32=short
  gint32_modifier='"h"'
  gint32_format='"hi"'
  guint32_format='"hu"'
  ;;
$ac_cv_sizeof_int)		
  gint32=int
  gint32_modifier='""'
  gint32_format='"i"'
  guint32_format='"u"'
  ;;
$ac_cv_sizeof_long)		
  gint32=long
  gint32_modifier='"l"'
  gint32_format='"li"'
  guint32_format='"lu"'
  ;;
esac
case 8 in
$ac_cv_sizeof_int)
  gint64=int
  gint64_modifier='""'
  gint64_format='"i"'
  guint64_format='"u"'
  glib_extension=
  gint64_constant='(val)'
  guint64_constant='(val)'
  ;;
$ac_cv_sizeof_long)
  gint64=long
  gint64_modifier='"l"'
  gint64_format='"li"'
  guint64_format='"lu"'
  glib_extension=
  gint64_constant='(val##L)'
  guint64_constant='(val##UL)'
  ;;
$ac_cv_sizeof_long_long)
  gint64='long long'
  if test -n "$glib_cv_long_long_format"; then
    gint64_modifier='"'$glib_cv_long_long_format'"'
    gint64_format='"'$glib_cv_long_long_format'i"'
    guint64_format='"'$glib_cv_long_long_format'u"'
  fi
  glib_extension='G_GNUC_EXTENSION '
  gint64_constant='(G_GNUC_EXTENSION (val##LL))'
  guint64_constant='(G_GNUC_EXTENSION (val##ULL))'
  ;;
$ac_cv_sizeof___int64)
  gint64='__int64'
  if test -n "$glib_cv_long_long_format"; then
    gint64_modifier='"'$glib_cv_long_long_format'"'
    gint64_format='"'$glib_cv_long_long_format'i"'
    guint64_format='"'$glib_cv_long_long_format'u"'
  fi
  glib_extension=
  gint64_constant='(val##i64)'
  guint64_constant='(val##ui64)'
  ;;
esac
glib_size_t=$ac_cv_sizeof_size_t
glib_size_type_define="$glib_size_type"
glib_void_p=$ac_cv_sizeof_void_p
glib_long=$ac_cv_sizeof_long

case "$glib_size_type" in
short)
  gsize_modifier='"h"'
  gsize_format='"hu"'
  gssize_format='"hi"'
  glib_msize_type='SHRT'
  ;;
int)
  gsize_modifier='""'
  gsize_format='"u"'
  gssize_format='"i"'
  glib_msize_type='INT'
  ;;
long)
  gsize_modifier='"l"'
  gsize_format='"lu"'
  gssize_format='"li"'
  glib_msize_type='LONG'
  ;;
"long long"|__int64)
  gsize_modifier='"I64"'
  gsize_format='"I64u"'
  gssize_format='"I64i"'
  glib_msize_type='INT64'
  ;;
esac

gintbits=`expr $ac_cv_sizeof_int \* 8`
glongbits=`expr $ac_cv_sizeof_long \* 8`
gsizebits=`expr $ac_cv_sizeof_size_t \* 8`

case $ac_cv_sizeof_void_p in
$ac_cv_sizeof_int)
  glib_intptr_type_define=int
  gintptr_modifier='""'
  gintptr_format='"i"'
  guintptr_format='"u"'
  glib_gpi_cast=''
  glib_gpui_cast=''
  ;;
$ac_cv_sizeof_long)
  glib_intptr_type_define=long
  gintptr_modifier='"l"'
  gintptr_format='"li"'
  guintptr_format='"lu"'
  glib_gpi_cast='(glong)'
  glib_gpui_cast='(gulong)'
  ;;
$ac_cv_sizeof_long_long)
  glib_intptr_type_define='long long'
  gintptr_modifier='"I64"'
  gintptr_format='"I64i"'
  guintptr_format='"I64u"'
  glib_gpi_cast='(gint64)'
  glib_gpui_cast='(guint64)'
  ;;
$ac_cv_sizeof___int64)
  glib_intptr_type_define=__int64
  gintptr_modifier='"I64"'
  gintptr_format='"I64i"'
  guintptr_format='"I64u"'
  glib_gpi_cast='(gint64)'
  glib_gpui_cast='(guint64)'
  ;;
*)
  glib_unknown_void_p=yes
  ;;
esac


case xyes in
x$ac_cv_func_atexit)
  glib_atexit="
#ifdef NeXT /* @#%@! NeXTStep */
# define g_ATEXIT(proc)	(!atexit (proc))
#else
# define g_ATEXIT(proc)	(atexit (proc))
#endif"
  ;;
x$ac_cv_func_on_exit)
  glib_atexit="
#define g_ATEXIT(proc)	(on_exit ((void (*)(int, void*))(proc), NULL))"
  ;;
esac

case xyes in
x$ac_cv_func_memmove)
  glib_memmove='
#define g_memmove(dest,src,len) G_STMT_START { memmove ((dest), (src), (len)); } G_STMT_END'
  ;;
x$glib_cv_working_bcopy)
  glib_memmove="
/* memmove isn't available, but bcopy can copy overlapping memory regions */
#define g_memmove(d,s,n) G_STMT_START { bcopy ((s), (d), (n)); } G_STMT_END"
  ;;
*)  
  glib_memmove="
/* memmove isn't found and bcopy can't copy overlapping memory regions, 
 * so we have to roll our own copy routine. */
void g_memmove (void* dest, const void * src, unsigned long len);"
  ;;
esac

glib_defines="
#define GLIB_MAJOR_VERSION $GLIB_MAJOR_VERSION
#define GLIB_MINOR_VERSION $GLIB_MINOR_VERSION
#define GLIB_MICRO_VERSION $GLIB_MICRO_VERSION
"

case xyes in
x$glib_cv_va_copy)	glib_vacopy='#define G_VA_COPY	va_copy' ;;
x$glib_cv___va_copy)	glib_vacopy='#define G_VA_COPY	__va_copy' ;;
*)			glib_vacopy=''
esac

if test x$glib_cv_va_val_copy = xno; then
  glib_vacopy="\$glib_vacopy
#define G_VA_COPY_AS_ARRAY 1"
fi

if test x$glib_cv_hasinline = xyes; then
    glib_inline='#define G_HAVE_INLINE 1'
fi
if test x$glib_cv_has__inline = xyes; then
    glib_inline="\$glib_inline
#define G_HAVE___INLINE 1"
fi
if test x$glib_cv_has__inline__ = xyes; then
    glib_inline="\$glib_inline
#define G_HAVE___INLINE__ 1"
fi

g_have_gnuc_varargs=$g_have_gnuc_varargs
g_have_iso_c_varargs=$g_have_iso_c_varargs
g_have_iso_cxx_varargs=$g_have_iso_cxx_varargs

g_can_inline=$g_can_inline
g_have_gnuc_visibility=$g_have_gnuc_visibility
g_have_sunstudio_visibility=$g_have_sunstudio_visibility

if test x$ac_cv_c_bigendian = xyes; then
  g_byte_order=G_BIG_ENDIAN
  g_bs_native=BE
  g_bs_alien=LE
else
  g_byte_order=G_LITTLE_ENDIAN
  g_bs_native=LE
  g_bs_alien=BE
fi

g_pollin=$glib_cv_value_POLLIN
g_pollout=$glib_cv_value_POLLOUT
g_pollpri=$glib_cv_value_POLLPRI
g_pollhup=$glib_cv_value_POLLHUP
g_pollerr=$glib_cv_value_POLLERR
g_pollnval=$glib_cv_value_POLLNVAL

g_af_unix=$glib_cv_value_AF_UNIX
g_af_inet=$glib_cv_value_AF_INET
g_af_inet6=$glib_cv_value_AF_INET6

g_msg_peek=$glib_cv_value_MSG_PEEK
g_msg_oob=$glib_cv_value_MSG_OOB
g_msg_dontroute=$glib_cv_value_MSG_DONTROUTE

g_stack_grows=$glib_cv_stack_grows

g_have_eilseq=$have_eilseq

case x$have_threads in
xno)	g_enable_threads_def="#undef";;
*)	g_enable_threads_def="#define";;
esac

g_threads_impl_def=$g_threads_impl

g_mutex_has_default="$mutex_has_default"
g_mutex_sizeof="$glib_cv_sizeof_gmutex"
g_system_thread_sizeof="$glib_cv_sizeof_system_thread"
g_mutex_contents="$glib_cv_byte_contents_gmutex"

g_memory_barrier_needed="$glib_memory_barrier_needed"

g_module_suffix="$glib_gmodule_suffix"

g_pid_type="$glib_pid_type"
case $host in
  *-*-beos*)
    glib_os="#define G_OS_BEOS"
    ;;
  *-*-cygwin*)
    glib_os="#define G_OS_UNIX
#define G_PLATFORM_WIN32
#define G_WITH_CYGWIN"
    ;;
  *-*-mingw*)
    glib_os="#define G_OS_WIN32
#define G_PLATFORM_WIN32"
    ;;
  *)
    glib_os="#define G_OS_UNIX"
    ;;
esac
glib_static_compilation=""
if test x$glib_win32_static_compilation = xyes; then
  glib_static_compilation="#define GLIB_STATIC_COMPILATION 1
#define GOBJECT_STATIC_COMPILATION 1"
fi
])

# Check for libdbus1 - Optional - is only used in the GDBus test cases
#
# 1.2.14 required for dbus_message_set_serial
PKG_CHECK_MODULES(DBUS1,
                  dbus-1 >= 1.2.14,
                  [AC_DEFINE(HAVE_DBUS1, 1, [Define if dbus-1 is available]) have_dbus1=yes],
                  have_dbus1=no)
AC_SUBST(DBUS1_CFLAGS)
AC_SUBST(DBUS1_LIBS)
AM_CONDITIONAL(HAVE_DBUS1, [test "x$have_dbus1" = "xyes"])

dnl
dnl Check for -Bsymbolic-functions linker flag used to avoid
dnl intra-library PLT jumps, if available.
dnl

AC_ARG_ENABLE(Bsymbolic,
              [AC_HELP_STRING([--disable-Bsymbolic],
                              [avoid linking with -Bsymbolic])],,
              [SAVED_LDFLAGS="${LDFLAGS}"
               AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
               LDFLAGS=-Wl,-Bsymbolic-functions
               AC_TRY_LINK([], [int main (void) { return 0; }],
                           AC_MSG_RESULT(yes)
                           enable_Bsymbolic=yes,
                           AC_MSG_RESULT(no)
                           enable_Bsymbolic=no)
               LDFLAGS="${SAVED_LDFLAGS}"])

if test "x${enable_Bsymbolic}" == "xyes"; then
  GLIB_LINK_FLAGS=-Wl,-Bsymbolic-functions
fi

AC_SUBST(GLIB_LINK_FLAGS)

#
# Define variables corresponding to the correct include paths to use for
# in-tree building.
#

# for config.h:
config_h_INCLUDES='-I$(top_builddir)'
AC_SUBST(config_h_INCLUDES)

# glib:
#   config.h
#   $(top_builddir)/glib: for glibconfig.h
#   $(top_srcdir)/glib:   for glib.h
#   $(top_srcdir):        for everything
glib_INCLUDES='$(config_h_INCLUDES) -I$(top_builddir)/glib -I$(top_srcdir)/glib -I$(top_srcdir)'
AC_SUBST(glib_INCLUDES)

# gthread:
#   same as glib
gthread_INCLUDES='$(glib_INCLUDES)'
AC_SUBST(gthread_INCLUDES)

# gobject:
#   same as gthread
gobject_INCLUDES='$(gthread_INCLUDES)'
AC_SUBST(gobject_INCLUDES)

# gmodule:
#   glib includes
#   $(top_srcdir)/gmodule: for gmodule.h
gmodule_INCLUDES='$(glib_INCLUDES) -I$(top_srcdir)/gmodule'
AC_SUBST(gmodule_INCLUDES)

# gio:
#   same as gmodule
gio_INCLUDES='$(gmodule_INCLUDES)'
AC_SUBST(gio_INCLUDES)


AC_CONFIG_FILES([
glib-2.0.pc
glib-2.0-uninstalled.pc
gmodule-2.0.pc
gmodule-export-2.0.pc
gmodule-no-export-2.0.pc
gmodule-2.0-uninstalled.pc
gmodule-no-export-2.0-uninstalled.pc
gthread-2.0.pc
gthread-2.0-uninstalled.pc
gobject-2.0.pc
gobject-2.0-uninstalled.pc
gio-2.0.pc
gio-unix-2.0.pc
gio-windows-2.0.pc
gio-2.0-uninstalled.pc
gio-unix-2.0-uninstalled.pc
glib-zip
glib-gettextize
Makefile
build/Makefile
build/win32/Makefile
build/win32/dirent/Makefile
build/win32/vs9/Makefile
glib/Makefile
glib/glib.stp
glib/libcharset/Makefile
glib/gnulib/Makefile
glib/pcre/Makefile
glib/update-pcre/Makefile
glib/tests/Makefile
gmodule/Makefile
gmodule/gmoduleconf.h
gobject/Makefile
gobject/gobject.stp
gobject/glib-mkenums
gobject/tests/Makefile
gthread/Makefile
gthread/tests/Makefile
gio/Makefile
gio/xdgmime/Makefile
gio/inotify/Makefile
gio/libasyncns/Makefile
gio/fen/Makefile
gio/fam/Makefile
gio/win32/Makefile
gio/tests/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/glib/Makefile
docs/reference/glib/version.xml
docs/reference/gobject/Makefile
docs/reference/gobject/version.xml
docs/reference/gio/Makefile
docs/reference/gio/version.xml
tests/Makefile
tests/gobject/Makefile
tests/refcount/Makefile
m4macros/Makefile
])

AC_CONFIG_COMMANDS([chmod-scripts],
[chmod 0755 glib-zip
chmod 0755 glib-gettextize
chmod 0755 gobject/glib-mkenums])

# we want to invoke this macro solely so that the config.status script
# and automake generated makefiles know about these generated files.
# They are only needed to distcheck the package
if false; then
  AC_CONFIG_FILES([
    INSTALL
    README
    config.h.win32
    glib/glibconfig.h.win32
    glib/makefile.msc
    glib/glib.rc
    gmodule/makefile.msc
    gmodule/gmodule.rc
    gobject/makefile.msc
    gobject/gobject.rc
    gthread/makefile.msc
    gthread/gthread.rc
    gio/gio.rc
    tests/makefile.msc
  ])
fi

AC_OUTPUT
