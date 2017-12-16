include $(top_srcdir)/Makefile.decl

INCLUDES = \
	-DG_LOG_DOMAIN=\"GLib-GRegex\" \
	-DSUPPORT_UCP \
	-DSUPPORT_UTF8 \
	-DNEWLINE=-1 \
	-DMATCH_LIMIT=10000000 \
	-DMATCH_LIMIT_RECURSION=8192 \
	-DMAX_NAME_SIZE=32 \
	-DMAX_NAME_COUNT=10000 \
	-DMAX_DUPLENGTH=30000 \
	-DLINK_SIZE=2 \
	-DPOSIX_MALLOC_THRESHOLD=10 \
	-DPCRE_STATIC \
	$(glib_INCLUDES) \
	@GLIB_DEBUG_FLAGS@ \
	-DG_DISABLE_DEPRECATED \
	-DGLIB_COMPILATION \
	$(DEPRECATED_FLAGS)\
	$(WARN_CFLAGS) \
	$(PCRE_WARN_CFLAGS) \
	$(DEP_CFLAGS)

noinst_LTLIBRARIES = libpcre.la

libpcre_headers =

libpcre_la_SOURCES = \
	pcre_compile.c \
	pcre_chartables.c \
	pcre_config.c \
	pcre_dfa_exec.c \
	pcre_exec.c \
	pcre_fullinfo.c \
	pcre_get.c \
	pcre_globals.c \
	pcre_newline.c \
	pcre_ord2utf8.c \
	pcre_study.c \
	pcre_tables.c \
	pcre_try_flipped.c \
	pcre_ucp_searchfuncs.c \
	pcre_xclass.c \
	pcre.h \
	pcre_internal.h \
	ucp.h \
	$(libpcre_headers)

libpcre_la_LIBADD = $(DEP_LIBS)

libpcre_la_LDFLAGS = -no-undefined

EXTRA_DIST +=				\
		COPYING			\
		makefile.msc

