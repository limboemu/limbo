include $(top_srcdir)/Makefile.decl

INCLUDES = -g $(gthread_INCLUDES) $(GLIB_DEBUG_FLAGS)

noinst_PROGRAMS = $(TEST_PROGS)
progs_ldadd     = $(top_builddir)/glib/libglib-2.0.la \
		  $(top_builddir)/gthread/libgthread-2.0.la

TEST_PROGS	 += 1bit-mutex
1bit_mutex_LDADD  = $(progs_ldadd) $(top_builddir)/gthread/libgthread-2.0.la

TEST_PROGS	      += 1bit-emufutex
1bit_emufutex_SOURCES  = 1bit-mutex.c
1bit_emufutex_CFLAGS   = -DTEST_EMULATED_FUTEX
1bit_emufutex_LDADD    = $(progs_ldadd) $(top_builddir)/gthread/libgthread-2.0.la
