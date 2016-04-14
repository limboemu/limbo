include $(top_srcdir)/Makefile.decl

INCLUDES =					\
	$(gmodule_INCLUDES)			\
	$(GLIB_DEBUG_FLAGS)

libglib = $(top_builddir)/glib/libglib-2.0.la
libgthread = $(top_builddir)/gthread/libgthread-2.0.la
libgmodule = $(top_builddir)/gmodule/libgmodule-2.0.la
libgobject = $(top_builddir)/gobject/libgobject-2.0.la


# libtool dependency tracking seems broken.  this is currently
# required to get the tests to dynamic link against the in-tree
# libglib instead of the system one
libgobject += $(libglib)

########################################################################

noinst_LTLIBRARIES = libtestgobject.la

libtestgobject_la_SOURCES =	\
	testcommon.h		\
	testmarshal.h		\
	testmarshal.c		\
	testmodule.c		\
	testmodule.h

if CROSS_COMPILING
  glib_genmarshal=$(GLIB_GENMARSHAL)
else
  glib_genmarshal=$(top_builddir)/gobject/glib-genmarshal
endif

testmarshal.h: stamp-testmarshal.h
	@true
stamp-testmarshal.h: @REBUILD@ testmarshal.list $(glib_genmarshal)
	$(AM_V_GEN) $(glib_genmarshal) --prefix=test_marshal $(srcdir)/testmarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh testmarshal.h 2>/dev/null || cp xgen-gmh testmarshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $@
testmarshal.c: @REBUILD@ testmarshal.list $(glib_genmarshal)
	$(AM_V_GEN) $(glib_genmarshal) --prefix=test_marshal $(srcdir)/testmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc testmarshal.c \
	&& rm -f xgen-gmc xgen-gmc~

BUILT_SOURCES = testmarshal.h testmarshal.c
CLEANFILES = stamp-testmarshal.h

########################################################################

LDADD = libtestgobject.la $(libgobject)

test_programs =					\
	deftype					\
	gvalue-test				\
	paramspec-test				\
	accumulator				\
	defaultiface				\
	dynamictype				\
	ifacecheck				\
	ifaceinit				\
	ifaceinherit				\
	ifaceproperties				\
	override				\
	performance				\
	performance-threaded			\
	singleton				\
	references

performance_LDADD = $(libgobject) $(libgthread)
performance_threaded_LDADD = $(libgobject) $(libgthread)
check_PROGRAMS = $(test_programs)

TESTS = $(test_programs)
TESTS_ENVIRONMENT = srcdir=$(srcdir) \
	LIBCHARSET_ALIAS_DIR=$(top_builddir)/glib/libcharset \
	MALLOC_CHECK_=2	\
	MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256))

########################################################################

EXTRA_DIST += 		  \
	testmarshal.list

BUILT_EXTRA_DIST =				\
	testmarshal.h				\
	testmarshal.c

dist-hook: $(BUILT_EXTRA_DIST)
	files='$(BUILT_EXTRA_DIST)';				\
	for f in $$files; do					\
	  if test -f $$f; then d=.; else d=$(srcdir); fi;	\
	  cp $$d/$$f $(distdir) || exit 1; done

distclean-local:
	if test $(srcdir) = .; then :; else	\
	    rm -f $(BUILT_EXTRA_DIST);		\
	fi
