include $(top_srcdir)/Makefile.decl

INCLUDES = -g $(gobject_INCLUDES) $(GLIB_DEBUG_FLAGS)

noinst_PROGRAMS  = $(TEST_PROGS)
libgobject_LDADD = ../libgobject-2.0.la $(top_builddir)/gthread/libgthread-2.0.la $(top_builddir)/glib/libglib-2.0.la

TEST_PROGS             += threadtests dynamictests binding properties
threadtests_SOURCES	= threadtests.c
threadtests_LDADD	= $(libgobject_LDADD)
dynamictests_SOURCES	= dynamictests.c
dynamictests_LDADD	= $(libgobject_LDADD)
binding_SOURCES		= binding.c
binding_LDADD		= $(libgobject_LDADD)
properties_SOURCES      = properties.c
properties_LDADD        = $(libgobject_LDADD)
