## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

SUBDIRS = reference

EXTRA_DIST += debugging.txt macros.txt

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done
