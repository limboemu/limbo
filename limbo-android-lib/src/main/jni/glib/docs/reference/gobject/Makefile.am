## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

AUTOMAKE_OPTIONS = 1.6

# The name of the module.
DOC_MODULE=gobject

# The top-level SGML file.
DOC_MAIN_SGML_FILE=gobject-docs.sgml

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=../../../gobject

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS=--deprecated-guards="G_DISABLE_DEPRECATED" \
	--ignore-decorators=G_GNUC_INTERNAL

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml --name-space=g

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/gobject/*.h
CFILE_GLOB=$(top_srcdir)/gobject/*.c

# Headers to ignore
IGNORE_HFILES = \
	gobject_trace.h			\
	gatomicarray.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES = \
	-I$(srcdir) 			\
	$(gobject_INCLUDES) 		\
	$(GLIB_DEBUG_FLAGS)
GTKDOC_LIBS = \
	$(top_builddir)/glib/libglib-2.0.la	\
	$(top_builddir)/gobject/libgobject-2.0.la

# Images to copy into HTML directory
HTML_IMAGES =  \
	images/glue.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = version.xml 		\
		glib-mkenums.xml 	\
		glib-genmarshal.xml	\
		gobject-query.xml \
		tut_gobject.xml \
		tut_gsignal.xml \
		tut_gtype.xml \
		tut_howto.xml \
		tut_intro.xml \
		tut_tools.xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=--extra-dir=$(srcdir)/../glib/html

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += 		\
	gobject.cI	\
	version.xml.in

########################################################################

man_MANS = 			\
	glib-mkenums.1 		\
	glib-genmarshal.1 	\
	gobject-query.1

if ENABLE_MAN

.xml.1:
	@XSLTPROC@ -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

dist-local-check-mans-enabled:
	if grep "Man generation disabled" $(man_MANS) >/dev/null; then $(RM) $(man_MANS); fi

else

$(man_MANS):
	echo Man generation disabled.  Creating dummy $@.  Configure with --enable-man to enable it.
	echo Man generation disabled.  Remove this file, configure with --enable-man, and rebuild > $@

dist-local-check-mans-enabled:
	echo "*** --enable-man must be used in order to make dist"
	false

endif

MAINTAINERCLEANFILES = $(man_MANS) $(BUILT_SOURCES)

EXTRA_DIST += $(man_MANS)

dist-hook-local: dist-local-check-mans-enabled gobject-docs-clean all-local

gobject-docs-clean: clean
	cd $(srcdir) && rm -rf xml html
