## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

INCLUDES = $(config_h_INCLUDES)

noinst_LTLIBRARIES = libasyncns.la

libasyncns_la_SOURCES = \
	asyncns.c \
	asyncns.h \
	g-asyncns.h

libasyncns_la_LIBADD = $(LIBASYNCNS_LIBADD)

EXTRA_DIST += README update.sh
