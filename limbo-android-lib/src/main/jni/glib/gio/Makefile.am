include $(top_srcdir)/Makefile.decl

NULL =

SUBDIRS=

if OS_UNIX
SUBDIRS += libasyncns xdgmime
endif

if OS_WIN32_AND_DLL_COMPILATION
if MS_LIB_AVAILABLE
noinst_DATA = gio-2.0.lib

install_ms_lib_cmd = $(INSTALL) gio-2.0.lib $(DESTDIR)$(libdir)
uninstall_ms_lib_cmd = -rm $(DESTDIR)$(libdir)/gio-2.0.lib
endif
endif

install-ms-lib:
	$(install_ms_lib_cmd)

uninstall-ms-lib:
	$(uninstall_ms_lib_cmd)

gio.def: gio.symbols
	$(AM_V_GEN) (echo -e EXPORTS; $(CPP) -P -DINCLUDE_VARIABLES -DINCLUDE_INTERNAL_SYMBOLS -DG_OS_WIN32 -DALL_FILES - <$(srcdir)/gio.symbols | sed -e '/^$$/d' -e 's/^/	/' -e 's/G_GNUC_[^ ]*//g' | sort) > gio.def.tmp && \
	  mv gio.def.tmp gio.def

if OS_LINUX
if HAVE_GNUC_VISIBILITY
TESTS = abicheck.sh
endif
endif

if HAVE_THREADS
THREAD_FLAGS=-DG_THREADS_MANDATORY
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"GLib-GIO\"			\
	$(gmodule_INCLUDES)				\
	$(GLIB_DEBUG_FLAGS)				\
	$(THREAD_FLAGS)					\
	-DG_DISABLE_DEPRECATED				\
	-DGIO_COMPILATION				\
	-DGIO_MODULE_DIR=\"$(GIO_MODULE_DIR)\"	

lib_LTLIBRARIES = libgio-2.0.la

marshal_sources = \
        gio-marshal.h	\
        gio-marshal.c	\
	$(NULL)

if CROSS_COMPILING
  glib_genmarshal=$(GLIB_GENMARSHAL)
else
  glib_genmarshal=../gobject/glib-genmarshal
endif

gio-marshal.h: gio-marshal.list
	$(AM_V_GEN) $(glib_genmarshal) --prefix=_gio_marshal $(srcdir)/gio-marshal.list --header --internal > $@.tmp && \
	  mv $@.tmp $@

gio-marshal.c: gio-marshal.h gio-marshal.list
	$(AM_V_GEN) (echo "#include \"gio-marshal.h\""; \
	$(glib_genmarshal) --prefix=_gio_marshal $(srcdir)/gio-marshal.list --body --internal) > $@.tmp && \
	  mv $@.tmp $@

gdbus_headers = 			\
	gdbusauthobserver.h		\
	gcredentials.h			\
	gdbusutils.h			\
	gdbuserror.h			\
	gdbusaddress.h			\
	gdbusconnection.h		\
	gdbusmessage.h			\
	gdbusnameowning.h		\
	gdbusnamewatching.h		\
	gdbusproxy.h			\
	gdbusintrospection.h		\
	gdbusmethodinvocation.h		\
	gdbusserver.h			\
	$(NULL)

gdbus_sources = 							\
	gdbusutils.h			gdbusutils.c			\
	gdbusaddress.h			gdbusaddress.c			\
	gdbusauthobserver.h		gdbusauthobserver.c		\
	gdbusauth.h			gdbusauth.c			\
	gdbusauthmechanism.h		gdbusauthmechanism.c		\
	gdbusauthmechanismanon.h	gdbusauthmechanismanon.c	\
	gdbusauthmechanismexternal.h	gdbusauthmechanismexternal.c	\
	gdbusauthmechanismsha1.h	gdbusauthmechanismsha1.c	\
	gdbuserror.h			gdbuserror.c			\
	gdbusconnection.h		gdbusconnection.c		\
	gdbusmessage.h			gdbusmessage.c			\
	gdbusnameowning.h		gdbusnameowning.c		\
	gdbusnamewatching.h		gdbusnamewatching.c		\
	gdbusproxy.h			gdbusproxy.c			\
	gdbusprivate.h			gdbusprivate.c			\
	gdbusintrospection.h		gdbusintrospection.c		\
	gdbusmethodinvocation.h		gdbusmethodinvocation.c		\
	gdbusserver.h			gdbusserver.c			\
	$(NULL)

settings_headers = \
	gsettingsbackend.h		\
	gsettings.h

settings_sources = \
	gvdb/gvdb-format.h		\
	gvdb/gvdb-reader.h		\
	gvdb/gvdb-reader.c		\
	gdelayedsettingsbackend.h	\
	gdelayedsettingsbackend.c	\
	gkeyfilesettingsbackend.c	\
	gmemorysettingsbackend.h	\
	gmemorysettingsbackend.c	\
	gnullsettingsbackend.h		\
	gnullsettingsbackend.c		\
	gsettingsbackendinternal.h	\
	gsettingsbackend.c		\
	gsettingsschema.h		\
	gsettingsschema.c		\
	gsettings-mapping.h		\
	gsettings-mapping.c		\
	gsettings.c

if OS_WIN32
settings_sources += \
	gregistrysettingsbackend.h	\
	gregistrysettingsbackend.c
endif

local_sources = \
	glocaldirectorymonitor.c 	\
	glocaldirectorymonitor.h 	\
	glocalfile.c 			\
	glocalfile.h 			\
	glocalfileenumerator.c 		\
	glocalfileenumerator.h 		\
	glocalfileinfo.c 		\
	glocalfileinfo.h 		\
	glocalfileinputstream.c 	\
	glocalfileinputstream.h 	\
	glocalfilemonitor.c 		\
	glocalfilemonitor.h 		\
	glocalfileoutputstream.c 	\
	glocalfileoutputstream.h 	\
	glocalfileiostream.c		\
	glocalfileiostream.h		\
	glocalvfs.c 			\
	glocalvfs.h 			\
	gsocks4proxy.c			\
	gsocks4proxy.h			\
	gsocks4aproxy.c			\
	gsocks4aproxy.h			\
	gsocks5proxy.c			\
	gsocks5proxy.h			\
	$(NULL)

platform_libadd =
platform_deps =
appinfo_sources =

if HAVE_INOTIFY
SUBDIRS += inotify
platform_libadd += inotify/libinotify.la
platform_deps += inotify/libinotify.la
endif

if HAVE_FEN
AM_CPPFLAGS += -DHAVE_FEN
SUBDIRS += fen
platform_libadd += fen/libfen.la
platform_deps += fen/libfen.la
endif

if OS_WIN32
SUBDIRS += win32
platform_libadd += win32/libgiowin32.la
platform_deps += win32/libgiowin32.la
endif


SUBDIRS += .

if HAVE_FAM
SUBDIRS += fam
endif

if OS_UNIX
appinfo_sources += gdesktopappinfo.c
platform_libadd += libasyncns/libasyncns.la xdgmime/libxdgmime.la
platform_deps += libasyncns/libasyncns.la xdgmime/libxdgmime.la
unix_sources = \
	gfiledescriptorbased.c  \
	gunixconnection.c	\
	gunixcredentialsmessage.c	\
	gunixfdlist.c		\
	gunixfdmessage.c	\
	gunixmount.c		\
	gunixmount.h		\
	gunixmounts.c 		\
	gunixresolver.c		\
	gunixresolver.h		\
	gunixsocketaddress.c	\
	gunixvolume.c 		\
	gunixvolume.h 		\
	gunixvolumemonitor.c 	\
	gunixvolumemonitor.h 	\
	gunixinputstream.c 	\
	gunixoutputstream.c 	\
	$(NULL)


giounixincludedir=$(includedir)/gio-unix-2.0/gio
giounixinclude_HEADERS = \
	gdesktopappinfo.h	\
	gfiledescriptorbased.h  \
	gunixconnection.h	\
	gunixcredentialsmessage.h	\
	gunixmounts.h 		\
	gunixfdlist.h		\
	gunixfdmessage.h	\
	gunixinputstream.h 	\
	gunixoutputstream.h 	\
	gunixsocketaddress.h	\
	$(NULL)
endif

win32_actual_sources = \
	gwin32mount.c \
	gwin32mount.h \
	gwin32resolver.c \
	gwin32resolver.h \
	gwin32volumemonitor.c \
	gwin32volumemonitor.h \
	gwin32inputstream.c \
	gwin32outputstream.c \
	gwin32outputstream.h \
	$(NULL)

win32_more_sources_for_vcproj = \
	gwin32appinfo.c \
	win32/gwin32directorymonitor.c \
	win32/gwinhttpfile.c \
	win32/gwinhttpfileinputstream.c \
	win32/gwinhttpfileoutputstream.c \
	win32/gwinhttpvfs.c

if OS_WIN32
appinfo_sources += gwin32appinfo.c gwin32appinfo.h
platform_libadd += -lshlwapi -lws2_32 -ldnsapi
win32_sources = $(win32_actual_sources)

giowin32includedir=$(includedir)/gio-win32-2.0/gio
giowin32include_HEADERS = \
	gwin32inputstream.h \
	gwin32outputstream.h \
	$(NULL)

endif

SUBDIRS += tests

libgio_2_0_la_SOURCES =		\
	gappinfo.c 		\
	gasynchelper.c 		\
	gasynchelper.h 		\
	gasyncinitable.c	\
	gasyncresult.c 		\
	gbufferedinputstream.c 	\
	gbufferedoutputstream.c \
	gcancellable.c 		\
	gcontenttype.c 		\
	gcontenttypeprivate.h 	\
	gcharsetconverter.c	\
	gconverter.c		\
	gconverterinputstream.c	\
	gconverteroutputstream.c	\
	gcredentials.c		\
	gdatainputstream.c 	\
	gdataoutputstream.c 	\
	gdrive.c 		\
	gdummyfile.h 		\
	gdummyfile.c 		\
	gdummyproxyresolver.c	\
	gdummyproxyresolver.h	\
	gemblem.h 		\
	gemblem.c 		\
	gemblemedicon.h		\
	gemblemedicon.c		\
	gfile.c 		\
	gfileattribute.c 	\
	gfileattribute-priv.h 	\
	gfileenumerator.c 	\
	gfileicon.c 		\
	gfileinfo.c 		\
	gfileinfo-priv.h 	\
	gfileinputstream.c 	\
	gfilemonitor.c 		\
	gfilenamecompleter.c 	\
	gfileoutputstream.c 	\
	gfileiostream.c		\
	gfilterinputstream.c 	\
	gfilteroutputstream.c 	\
	gicon.c 		\
	ginetaddress.c		\
	ginetsocketaddress.c	\
	ginitable.c		\
	ginputstream.c 		\
	gioenums.h		\
	gioerror.c 		\
	giomodule.c 		\
	giomodule-priv.h	\
	gioscheduler.c 		\
	giostream.c		\
	gloadableicon.c 	\
	gmount.c 		\
	gmemoryinputstream.c 	\
	gmemoryoutputstream.c 	\
	gmountoperation.c 	\
	gnativevolumemonitor.c 	\
	gnativevolumemonitor.h 	\
	gnetworkaddress.c	\
	gnetworkingprivate.h	\
	gnetworkservice.c	\
	goutputstream.c 	\
	gpermission.c 		\
	gpollfilemonitor.c 	\
	gpollfilemonitor.h 	\
	gproxyresolver.c	\
	gresolver.c		\
	gseekable.c 		\
	gsimpleasyncresult.c 	\
	gsimplepermission.c 	\
	gsocket.c		\
	gsocketaddress.c	\
	gsocketaddressenumerator.c \
	gsocketclient.c		\
	gsocketconnectable.c	\
	gsocketconnection.c	\
	gsocketcontrolmessage.c	\
	gsocketinputstream.c	\
	gsocketinputstream.h	\
	gsocketlistener.c	\
	gsocketoutputstream.c	\
	gsocketoutputstream.h	\
	gproxy.c		\
	gproxyaddress.c         \
	gproxyaddressenumerator.c \
	gproxyconnection.c      \
	gproxyconnection.h      \
	gsocketservice.c	\
	gsrvtarget.c		\
	gtcpconnection.c	\
	gthreadedsocketservice.c\
	gthemedicon.c 		\
	gthreadedresolver.c	\
	gthreadedresolver.h	\
	gunionvolumemonitor.c 	\
	gunionvolumemonitor.h 	\
	gvfs.c 			\
	gvolume.c 		\
	gvolumemonitor.c 	\
	gzlibcompressor.c	\
	gzlibdecompressor.c	\
	gmountprivate.h 	\
	gioenumtypes.h		\
	gioenumtypes.c		\
	$(appinfo_sources) 	\
	$(unix_sources) 	\
	$(win32_sources) 	\
	$(settings_sources) 	\
	$(gdbus_sources) 	\
	$(local_sources) 	\
	$(marshal_sources) 	\
	$(NULL)

EXTRA_DIST += strinfo.c

$(libgio_2_0_la_OBJECTS): $(marshal_sources)

libgio_2_0_la_LIBADD = \
	$(top_builddir)/glib/libglib-2.0.la 		\
	$(top_builddir)/gobject/libgobject-2.0.la 	\
	$(top_builddir)/gmodule/libgmodule-2.0.la 	\
	$(platform_libadd) 				\
	$(ZLIB_LIBS)					\
	$(SELINUX_LIBS) 				\
	$(GLIB_LIBS) 					\
	$(XATTR_LIBS) 					\
	$(NULL)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32_AND_DLL_COMPILATION
export_symbols = -export-symbols gio.def
gio_def = gio.def

gio_win32_res = gio-win32-res.o
gio_win32_res_ldflag = -Wl,$(gio_win32_res)

install-def-file:
	$(INSTALL) gio.def $(DESTDIR)$(libdir)/gio-2.0.def

uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/gio-2.0.def
else
install-def-file:
uninstall-def-file:

export_symbols = -export-symbols-regex '^g_.*'
endif

install-data-local: install-ms-lib install-def-file
	$(mkinstalldirs) $(DESTDIR)$(GIO_MODULE_DIR)

uninstall-local: uninstall-ms-lib uninstall-def-file

libgio_2_0_la_LDFLAGS = $(GLIB_LINK_FLAGS) \
	$(gio_win32_res_ldflag) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic $(no_undefined) $(export_symbols)

libgio_2_0_la_DEPENDENCIES = $(gio_win32_res) $(gio_def) $(platform_deps)

gio-win32-res.o: gio.rc
	$(WINDRES) gio.rc $@

gio_headers =			\
	gappinfo.h 		\
	gasyncinitable.h	\
	gasyncresult.h 		\
	gbufferedinputstream.h 	\
	gbufferedoutputstream.h \
	gcancellable.h 		\
	gcontenttype.h 		\
	gcharsetconverter.h	\
	gconverter.h		\
	gconverterinputstream.h	\
	gconverteroutputstream.h	\
	gdatainputstream.h 	\
	gdataoutputstream.h 	\
	gdrive.h 		\
	gemblem.h 		\
	gemblemedicon.h		\
	gfile.h 		\
	gfileattribute.h 	\
	gfileenumerator.h 	\
	gfileicon.h 		\
	gfileinfo.h 		\
	gfileinputstream.h 	\
	gfilemonitor.h 		\
	gfilenamecompleter.h 	\
	gfileoutputstream.h 	\
	gfileiostream.h		\
	gfilterinputstream.h 	\
	gfilteroutputstream.h 	\
	gicon.h 		\
	ginetaddress.h		\
	ginetsocketaddress.h	\
	ginputstream.h 		\
	ginitable.h		\
	gio.h			\
	giotypes.h		\
	gioenums.h		\
	gioerror.h 		\
	giomodule.h 		\
	gioscheduler.h 		\
	giostream.h		\
	gloadableicon.h 	\
	gmount.h 		\
	gmemoryinputstream.h 	\
	gmemoryoutputstream.h 	\
	gmountoperation.h 	\
	gnativevolumemonitor.h 	\
	gnetworkaddress.h	\
	gnetworkservice.h	\
	goutputstream.h 	\
	gpermission.h 		\
	gproxyaddress.h         \
	gproxy.h		\
	gproxyaddressenumerator.h \
	gproxyresolver.h	\
	gresolver.h		\
	gseekable.h 		\
	gsimpleasyncresult.h 	\
	gsimplepermission.h 	\
	gsocket.h		\
	gsocketaddress.h	\
	gsocketaddressenumerator.h \
	gsocketclient.h		\
	gsocketconnectable.h	\
	gsocketconnection.h	\
	gsocketcontrolmessage.h	\
	gsocketlistener.h	\
	gsocketservice.h	\
	gsrvtarget.h		\
	gtcpconnection.h	\
	gthreadedsocketservice.h\
	gthemedicon.h 		\
	gvfs.h 			\
	gvolume.h 		\
	gvolumemonitor.h 	\
	gzlibcompressor.h	\
	gzlibdecompressor.h	\
	$(settings_headers)	\
	$(gdbus_headers)	\
	$(NULL)

gioincludedir=$(includedir)/glib-2.0/gio/
gioinclude_HEADERS = 		\
	$(gio_headers)		\
	gioenumtypes.h

# these sources (also mentioned above) are generated.
BUILT_SOURCES = 		\
	gio-marshal.h 		\
	gio-marshal.c 		\
	gioenumtypes.h		\
	gioenumtypes.c		\
	$(NULL)

EXTRA_DIST += 			\
	gio-marshal.list	\
	gio.symbols		\
	gioenumtypes.h.template	\
	gioenumtypes.c.template	\
	makefile.msc		\
	abicheck.sh		\
	gio.rc.in		\
	gschema.dtd		\
	$(NULL)

BUILT_EXTRA_DIST = 		\
	gio.rc

CLEANFILES = 			\
	$(marshal_sources)	\
	$(NULL)

gioenumtypes.h: $(gio_headers) gioenumtypes.h.template
	$(AM_V_GEN) ( top_builddir=`cd $(top_builddir) && pwd`; \
	  cd $(srcdir) && $$top_builddir/gobject/glib-mkenums --template gioenumtypes.h.template $(gio_headers) ) > \
	    gioenumtypes.h.tmp && mv gioenumtypes.h.tmp gioenumtypes.h

gioenumtypes.c: $(gio_headers) gioenumtypes.c.template
	$(AM_V_GEN) ( top_builddir=`cd $(top_builddir) && pwd`; \
	  cd $(srcdir) && $$top_builddir/gobject/glib-mkenums --template gioenumtypes.c.template $(gio_headers) ) > \
	    gioenumtypes.c.tmp && mv gioenumtypes.c.tmp gioenumtypes.c

gio-2.0.lib: libgio-2.0.la gio.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libgio-2.0-$(LT_CURRENT_MINUS_AGE).dll -def:gio.def -out:$@

bin_PROGRAMS = gio-querymodules glib-compile-schemas gsettings

gio_querymodules_SOURCES = gio-querymodules.c
gio_querymodules_LDADD	 = \
	$(top_builddir)/glib/libglib-2.0.la		\
	$(top_builddir)/gobject/libgobject-2.0.la	\
	$(top_builddir)/gmodule/libgmodule-2.0.la	\
	libgio-2.0.la					\
	$(NULL)

glib_compile_schemas_LDADD = $(top_builddir)/glib/libglib-2.0.la
glib_compile_schemas_SOURCES = \
	gvdb/gvdb-format.h		\
	gvdb/gvdb-builder.h		\
	gvdb/gvdb-builder.c		\
	glib-compile-schemas.c

gsettings_LDADD = \
	$(top_builddir)/glib/libglib-2.0.la		\
	$(top_builddir)/gobject/libgobject-2.0.la	\
	libgio-2.0.la
gsettings_SOURCES = gsettings-tool.c

schemadir = $(datadir)/glib-2.0/schemas
dist_schema_DATA = gschema.dtd

# ------------------------------------------------------------------------
# gdbus(1) tool

bin_PROGRAMS += gdbus
gdbus_SOURCES = gdbus-tool.c
gdbus_LDADD = libgio-2.0.la \
	$(top_builddir)/glib/libglib-2.0.la 		\
	$(top_builddir)/gobject/libgobject-2.0.la

completiondir = $(sysconfdir)/bash_completion.d
completion_SCRIPTS = \
	gdbus-bash-completion.sh		\
	gsettings-bash-completion.sh
EXTRA_DIST += $(completion_SCRIPTS)

# ------------------------------------------------------------------------

dist-hook: $(BUILT_EXTRA_DIST) ../build/win32/vs9/gio.vcproj
	files='$(BUILT_EXTRA_DIST)'; \
	for f in $$files; do \
	  if test -f $$f; then d=.; else d=$(srcdir); fi; \
	  cp $$d/$$f $(distdir) || exit 1; done

../build/win32/vs9/gio.vcproj: $(top_srcdir)/build/win32/vs9/gio.vcprojin
	for F in `echo $(libgio_2_0_la_SOURCES) $(win32_actual_sources) $(win32_actual_more_sources_for_vcproj) | tr '/' '\\'`; do \
		case $$F in \
		gunix*.c|gdesktopappinfo.c) ;; \
		*.c) echo '   <File RelativePath="..\..\..\gio\'$$F'" />' \
		     ;; \
		esac; \
	done | sort -u >libgio.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/gio.vcprojin >$@
	rm libgio.sourcefiles

if HAVE_GLIB_RUNTIME_LIBDIR
install-data-hook:
	mkdir -p $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	mv $(DESTDIR)$(libdir)/libgio-2.0.so.0 $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	mv $(DESTDIR)$(libdir)/libgio-2.0.so.0.$(LT_CURRENT).$(LT_REVISION) $(DESTDIR)$(libdir)/$(GLIB_RUNTIME_LIBDIR)
	rm -f $(DESTDIR)$(libdir)/libgio-2.0.so
	ln -s $(GLIB_RUNTIME_LIBDIR)/libgio-2.0.so.0.$(LT_CURRENT).$(LT_REVISION) $(DESTDIR)$(libdir)/libgio-2.0.so
endif
