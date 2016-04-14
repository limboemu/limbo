## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

BUILT_SOURCES =
DISTCLEANFILES =
CLEANFILES =

#
# Generate glibconfig.h
#
# The timestamp of the stamp file is used to indicate if glibconfig.h is
# up to date with respect to config.status.  In the usual case, changes
# to config.status will not result in changes to glibconfig.h so we
# avoid touching its timestamp (in order not to rebuild the whole tree).
#
DISTCLEANFILES += glibconfig-stamp glibconfig.h
BUILT_SOURCES += glibconfig-stamp
configexecincludedir = $(libdir)/glib-2.0/include
nodist_configexecinclude_HEADERS = glibconfig.h
glibconfig-stamp: ../config.status
	$(AM_V_GEN) cd $(top_builddir) && \
	  $(SHELL) ./config.status glib/glibconfig.h
	@touch glibconfig-stamp



if HAVE_GOOD_PRINTF
else
PRINTF_SUBDIR = gnulib
printf_la = gnulib/libgnulib.la
endif 

if ENABLE_REGEX
if USE_SYSTEM_PCRE
else
MAYBE_PCRE = pcre 
endif
gregex_c = gregex.c
gregex_h = gregex.h
else
gregex_c =
gregex_h =
endif

if HAVE_GCC_BUILTINS_FOR_ATOMIC_OPERATIONS
gatomic_c = gatomic-gcc.c
else
gatomic_c = gatomic.c
endif

SUBDIRS = libcharset $(PRINTF_SUBDIR) $(MAYBE_PCRE) update-pcre . tests

DIST_SUBDIRS = libcharset gnulib pcre update-pcre tests

AM_CPPFLAGS = 				\
	$(glib_INCLUDES) 		\
	$(pcre_inc) 			\
	-DG_LOG_DOMAIN=\"GLib\" 	\
	$(GLIB_DEBUG_FLAGS) 		\
	-DG_DISABLE_DEPRECATED 		\
	-DGLIB_COMPILATION 		\
	-DPCRE_STATIC

glib.def: glib.symbols
	$(AM_V_GEN) (echo -e EXPORTS; $(CPP) -P -DINCLUDE_VARIABLES -DINCLUDE_INTERNAL_SYMBOLS -DG_OS_WIN32 -DALL_FILES - <$(srcdir)/glib.symbols | sed -e '/^$$/d' -e 's/^/	/' -e 's/G_GNUC_[^ ]*//g') > glib.def

if OS_LINUX
if HAVE_GNUC_VISIBILITY
TESTS_ENVIRONMENT = GLIB_DEBUG_FLAGS="$(GLIB_DEBUG_FLAGS)"
TESTS = abicheck.sh
endif
endif

MIRRORING_TAB_SOURCE = 				\
	glib-mirroring-tab/Makefile 		\
	glib-mirroring-tab/gen-mirroring-tab.c	\
	glib-mirroring-tab/packtab.h		\
	glib-mirroring-tab/packtab.c

# The compilation of GRegex can be disabled, but the source files must
# be distributed.
EXTRA_DIST += 			\
	makefile.msc.in		\
	glib.rc.in		\
	gen-unicode-tables.pl	\
	gen-script-table.pl	\
	glibconfig.h.win32.in	\
	abicheck.sh		\
	glib.symbols		\
	gregex.c		\
	gregex.h		\
	win_iconv.c		\
	libglib-gdb.py.in	\
	$(MIRRORING_TAB_SOURCE)

# These may be in the builddir too
BUILT_EXTRA_DIST = 		\
	makefile.msc		\
	glibconfig.h.win32	\
	glib.rc

lib_LTLIBRARIES = libglib-2.0.la

if OS_WIN32_AND_DLL_COMPILATION
if MS_LIB_AVAILABLE
noinst_DATA = glib-2.0.lib

install_ms_lib_cmd = $(INSTALL) glib-2.0.lib $(DESTDIR)$(libdir)
uninstall_ms_lib_cmd = -rm $(DESTDIR)$(libdir)/glib-2.0.lib
endif
endif

install-ms-lib:
	$(install_ms_lib_cmd)

uninstall-ms-lib:
	$(uninstall_ms_lib_cmd)

libglib_2_0_la_SOURCES = 	\
	glib_probes.d		\
	garray.c		\
	gasyncqueue.c		\
	$(gatomic_c)		\
	gbacktrace.c		\
	gbase64.c		\
	gbitlock.c		\
	gbookmarkfile.c 	\
	gbsearcharray.h		\
	gbuffer.c		\
	gbuffer.h		\
	gcache.c		\
	gchecksum.c		\
	gcompletion.c		\
	gconvert.c		\
	gdataset.c		\
	gdatasetprivate.h	\
	gdate.c	 		\
	gdatetime.c	 	\
	gdir.c			\
	gerror.c		\
	gfileutils.c		\
	ghash.c			\
	ghook.c			\
	ghostutils.c		\
	giochannel.c    	\
	gkeyfile.c		\
	glibintl.h		\
	glib_trace.h		\
	glist.c			\
	gmain.c	 		\
	gmappedfile.c		\
	gmarkup.c		\
	gmem.c			\
	gmessages.c		\
	gmirroringtable.h	\
	gnode.c			\
	goption.c		\
	gpattern.c		\
	gpoll.c			\
	gprimes.c		\
	gqsort.c		\
	gqueue.c		\
	grel.c			\
	grand.c			\
	$(gregex_c)		\
	gscanner.c		\
	gscripttable.h		\
	gsequence.c		\
	gshell.c		\
	gslice.c		\
	gslist.c		\
	gstdio.c		\
	gstrfuncs.c		\
	gstring.c		\
	gtestutils.c		\
	gthread.c       	\
	gthreadprivate.h	\
	gthreadpool.c   	\
	gtimer.c		\
	gtimezoneprivate.h 	\
	gtimezone.c	 	\
	gtree.c			\
	guniprop.c		\
	gutf8.c			\
	gunibreak.h		\
	gunibreak.c		\
	gunichartables.h	\
	gunicollate.c		\
	gunicomp.h		\
	gunidecomp.h		\
	gunidecomp.c		\
	gunicodeprivate.h	\
	gurifuncs.c 		\
	gutils.c		\
	gvariant.h		\
	gvariant.c		\
	gvariant-core.h		\
	gvariant-core.c		\
	gvariant-internal.h	\
	gvariant-parser.c	\
	gvariant-serialiser.h	\
	gvariant-serialiser.c	\
	gvarianttypeinfo.h	\
	gvarianttypeinfo.c	\
	gvarianttype.c		\
	gdebug.h		\
	gprintf.c		\
	gprintfint.h


EXTRA_libglib_2_0_la_SOURCES = \
	giounix.c	\
	giowin32.c	\
	gspawn.c	\
	gspawn-win32.c	\
	gwin32.c

glibincludedir=$(includedir)/glib-2.0
glibinclude_HEADERS =   \
	glib-object.h	\
	glib.h

glibsubincludedir=$(includedir)/glib-2.0/glib
glibsubinclude_HEADERS =   \
	galloca.h	\
	garray.h	\
	gasyncqueue.h	\
	gatomic.h	\
	gbacktrace.h	\
	gbase64.h	\
	gbitlock.h	\
	gbookmarkfile.h \
	gcache.h	\
	gchecksum.h	\
	gcompletion.h	\
	gconvert.h	\
	gdataset.h	\
	gdate.h		\
	gdatetime.h     \
	gdir.h		\
	gerror.h	\
	gfileutils.h	\
	ghash.h		\
	ghook.h		\
	ghostutils.h	\
	gi18n.h		\
	gi18n-lib.h	\
	giochannel.h	\
	gkeyfile.h 	\
	glist.h		\
	gmacros.h	\
	gmain.h		\
	gmappedfile.h	\
	gmarkup.h	\
	gmem.h		\
	gmessages.h	\
	gnode.h		\
	goption.h	\
	gpattern.h	\
	gpoll.h		\
	gprimes.h	\
	gqsort.h	\
	gquark.h	\
	gqueue.h	\
	grand.h		\
	$(gregex_h)	\
	grel.h		\
	gscanner.h	\
	gsequence.h	\
	gshell.h	\
	gslice.h	\
	gslist.h	\
	gspawn.h	\
	gstdio.h	\
	gstrfuncs.h	\
	gtestutils.h	\
	gstring.h	\
	gthread.h	\
	gthreadpool.h	\
	gtimer.h	\
	gtimezone.h	\
	gtree.h		\
	gtypes.h	\
	gunicode.h	\
	gurifuncs.h 	\
	gutils.h	\
	gvarianttype.h	\
	gvariant.h	\
	gwin32.h	\
	gprintf.h

install-data-local: install-ms-lib install-def-file
	@if test -f $(glibincludedir)/glist.h ; then					\
	  echo "*** Old headers found in $(glibincludedir). You should remove the" ;	\
	  echo "*** contents of this directory and type 'make install' again." ;	\
	  false ;									\
	fi

uninstall-local: uninstall-ms-lib uninstall-def-file

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32_AND_DLL_COMPILATION
export_symbols = -export-symbols glib.def

glib_win32_res = glib-win32-res.o
glib_win32_res_ldflag = -Wl,$(glib_win32_res)

glib_def = glib.def

install-def-file:
	$(INSTALL) glib.def $(DESTDIR)$(libdir)/glib-2.0.def

uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/glib-2.0.def
else
install-def-file:
uninstall-def-file:

export_symbols = $(LIBTOOL_EXPORT_OPTIONS)
endif

if ENABLE_REGEX
if USE_SYSTEM_PCRE
pcre_lib = $(PCRE_LIBS)
pcre_inc = $(PCRE_CFLAGS)
else
pcre_lib = pcre/libpcre.la
pcre_inc =
endif
else
pcre_lib =
pcre_inc =
endif

libglib_2_0_la_LIBADD = libcharset/libcharset.la $(printf_la) @GIO@ @GSPAWN@ @PLATFORMDEP@ @ICONV_LIBS@ @G_LIBS_EXTRA@ $(pcre_lib)
libglib_2_0_la_DEPENDENCIES = libcharset/libcharset.la $(printf_la) @GIO@ @GSPAWN@ @PLATFORMDEP@ $(glib_win32_res) $(glib_def)

libglib_2_0_la_LDFLAGS = $(GLIB_LINK_FLAGS) \
	 $(glib_win32_res_ldflag) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic $(no_undefined) $(export_symbols)

INSTALL_PROGS=

if ENABLE_DTRACE
glib_probes.h: glib_probes.d Makefile
	$(DTRACE) -C -h -s $< -o $@.tmp
	sed -e "s,define STAP_HAS_SEMAPHORES 1,undef STAP_HAS_SEMAPHORES," < $@.tmp > $@ && rm -f $@.tmp
glib_probes.o: glib_probes.d Makefile
	$(DTRACE) -G -s $< -o $@
BUILT_SOURCES += glib_probes.h glib_probes.o
CLEANFILES += glib_probes.h glib_probes.h.tmp
libglib_2_0_la_LIBADD += glib_probes.o
endif

if ENABLE_SYSTEMTAP
tapset_in_files = glib.stp.in
tapsetdir   = @ABS_TAPSET_DIR@
tapset_DATA = $(tapset_in_files:.stp.in=.stp)
EXTRA_DIST += $(tapset_in_files)
endif

gspawn-win32-helper-console.c:
	echo '#define HELPER_CONSOLE' >$@
	echo '#include "gspawn-win32-helper.c"' >>$@

gspawn-win64-helper.c:
	echo '#include "gspawn-win32-helper.c"' >$@

gspawn-win64-helper-console.c:
	echo '#define HELPER_CONSOLE' >$@
	echo '#include "gspawn-win32-helper.c"' >>$@


if OS_WIN32
if OS_WIN32_X64
INSTALL_PROGS += gspawn-win64-helper gspawn-win64-helper-console
gspawn_win64_helper_LDADD = libglib-2.0.la
gspawn_win64_helper_LDFLAGS = -mwindows
gspawn_win64_helper_console_LDADD = libglib-2.0.la
else
INSTALL_PROGS += gspawn-win32-helper gspawn-win32-helper-console
gspawn_win32_helper_LDADD = libglib-2.0.la
gspawn_win32_helper_LDFLAGS = -mwindows
gspawn_win32_helper_console_LDADD = libglib-2.0.la
endif
endif

glib-win32-res.o: glib.rc
	$(WINDRES) glib.rc $@

bin_PROGRAMS	 = ${INSTALL_PROGS}

if OS_UNIX

INSTALL_PROGS	+= gtester
gtester_SOURCES	 = gtester.c
gtester_LDADD	 = libglib-2.0.la 

auto_config_binscripts = gtester-report
bin_SCRIPTS = ${auto_config_binscripts}
EXTRA_DIST += ${auto_config_binscripts}

CONFIGVARS = \
	"bindir"    	: "${bindir}",		\
	"glib-version"  : "${GLIB_VERSION}"

install-exec-hook:
	for sf in ${auto_config_binscripts} ; do \
	  mv -f "$(DESTDIR)$(bindir)/$$sf" "$(DESTDIR)$(bindir)/$$sf".tmp \
	  && sed < "$(DESTDIR)$(bindir)/$$sf".tmp > "$(DESTDIR)$(bindir)/$$sf" \
	    -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGVARS}|' \
	    -e '1,1s|#! /usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  || exit $$? ; \
	  chmod a+x $(DESTDIR)$(bindir)/$$sf ; \
	  rm -f "$(DESTDIR)$(bindir)/$$sf".tmp ; \
	done

endif

glib-2.0.lib: libglib-2.0.la glib.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libglib-2.0-$(LT_CURRENT_MINUS_AGE).dll -def:glib.def -out:$@

dist-hook: $(BUILT_EXTRA_DIST) ../build/win32/vs9/glib.vcproj
	files='$(BUILT_EXTRA_DIST)'; \
	for f in $$files; do \
	  if test -f $$f; then d=.; else d=$(srcdir); fi; \
	  cp $$d/$$f $(distdir) || exit 1; done

../build/win32/vs9/glib.vcproj: $(top_srcdir)/build/win32/vs9/glib.vcprojin
	for F in $(libglib_2_0_la_SOURCES); do \
		case $$F in \
		*-gcc.c) \
			;; \
		*.c)	echo '   <File RelativePath="..\..\..\glib\'$$F'" />' \
			;; \
		esac; \
	done >libglib.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/glib.vcprojin >$@

distclean-local:
	if test $(srcdir) = .; then :; else \
	    rm -f libglib-gdb.py; \
	fi

# install gdb scripts
gdbdir = $(datadir)/glib-2.0/gdb
dist_gdb_SCRIPTS = glib.py

libglib-gdb.py: libglib-gdb.py.in
	$(AM_V_GEN) $(SED) -e "s|\@datadir\@|$(datadir)|" $(srcdir)/libglib-gdb.py.in > libglib-gdb.py


install-data-hook: libglib-gdb.py
	mkdir -p $(DESTDIR)$(datadir)/gdb/auto-load$(ABS_GLIB_RUNTIME_LIBDIR)
	$(INSTALL) libglib-gdb.py $(DESTDIR)$(datadir)/gdb/auto-load$(ABS_GLIB_RUNTIME_LIBDIR)/libglib-2.0.so.0.$(LT_CURRENT).$(LT_REVISION)-gdb.py
if HAVE_GLIB_RUNTIME_LIBDIR
	mkdir -p $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	mv $(DESTDIR)$(libdir)/libglib-2.0.so.0 $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	mv $(DESTDIR)$(libdir)/libglib-2.0.so.0.$(LT_CURRENT).$(LT_REVISION) $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	rm -f $(DESTDIR)$(libdir)/libglib-2.0.so
	ln -s $(GLIB_RUNTIME_LIBDIR)/libglib-2.0.so.0.$(LT_CURRENT).$(LT_REVISION) $(DESTDIR)$(libdir)/libglib-2.0.so
endif
