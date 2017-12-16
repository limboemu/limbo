include $(top_srcdir)/Makefile.decl

INCLUDES = 				\
	-g 				\
	$(glib_INCLUDES) 		\
	-DSRCDIR=\""$(srcdir)"\"	\
	$(GLIB_DEBUG_FLAGS)

noinst_PROGRAMS = $(TEST_PROGS)
progs_ldadd     = $(top_builddir)/glib/libglib-2.0.la 

TEST_PROGS       += testing
testing_SOURCES	  = testing.c
testing_LDADD	  = $(progs_ldadd)

TEST_PROGS       	 += option-context
option_context_SOURCES	  = option-context.c
option_context_LDADD	  = $(progs_ldadd)

TEST_PROGS     	 += keyfile
keyfile_SOURCES	  = keyfile.c
keyfile_LDADD	  = $(progs_ldadd)

TEST_PROGS        += fileutils
fileutils_SOURCES  = fileutils.c
fileutils_LDADD	   = $(progs_ldadd)

TEST_PROGS        += printf
printf_SOURCES     = printf.c
printf_LDADD       = $(progs_ldadd) -lm

TEST_PROGS        += protocol
protocol_SOURCES   = protocol.c
protocol_LDADD     = $(progs_ldadd)

TEST_PROGS        += rand
rand_SOURCES       = rand.c
rand_LDADD         = $(progs_ldadd) -lm

TEST_PROGS        += strfuncs
strfuncs_SOURCES   = strfuncs.c
strfuncs_LDADD	   = $(progs_ldadd) -lm

TEST_PROGS        += string
string_SOURCES     = string.c
string_LDADD	   = $(progs_ldadd) -lm

TEST_PROGS          += markup-parse
markup_parse_LDADD   = $(progs_ldadd)

TEST_PROGS          += markup-collect
markup_collect_LDADD = $(progs_ldadd)

TEST_PROGS         += markup-escape
markup_escape_LDADD = $(progs_ldadd)

TEST_PROGS               += markup-subparser
markup_subparser_LDADD    = $(progs_ldadd)

TEST_PROGS         += array-test
array_test_LDADD    = $(progs_ldadd)

TEST_PROGS         += hostutils
hostutils_LDADD     = $(progs_ldadd)

TEST_PROGS         += gvariant
gvariant_LDADD      = $(progs_ldadd)

TEST_PROGS         += mem-overflow
mem_overflow_LDADD  = $(progs_ldadd)

TEST_PROGS         += utils
utils_LDADD         = $(progs_ldadd)

TEST_PROGS         += regex
regex_LDADD         = $(progs_ldadd)

TEST_PROGS         += base64
base64_LDADD        = $(progs_ldadd)

TEST_PROGS         += sequence
sequence_LDADD      = $(progs_ldadd)

TEST_PROGS         += scannerapi
scannerapi_LDADD    = $(progs_ldadd)

TEST_PROGS         += shell
shell_LDADD         = $(progs_ldadd)

TEST_PROGS         += collate
collate_LDADD       = $(progs_ldadd)

TEST_PROGS         += utf8-pointer
utf8_pointer_LDADD  = $(progs_ldadd)

TEST_PROGS         += utf8-validate
utf8_validate_LDADD = $(progs_ldadd)

TEST_PROGS         += utf8-misc
utf8_misc_LDADD     = $(progs_ldadd)

TEST_PROGS         += checksum
checksum_LDADD      = $(progs_ldadd)

TEST_PROGS         += hash
hash_LDADD          = $(progs_ldadd)

TEST_PROGS         += date
date_LDADD          = $(progs_ldadd)

TEST_PROGS         += node
node_LDADD          = $(progs_ldadd)

TEST_PROGS         += convert
convert_LDADD       = $(progs_ldadd)

TEST_PROGS         += list
list_LDADD          = $(progs_ldadd)

TEST_PROGS         += slist
slist_LDADD         = $(progs_ldadd)

TEST_PROGS         += queue
queue_LDADD         = $(progs_ldadd)

TEST_PROGS         += tree
tree_LDADD          = $(progs_ldadd)

TEST_PROGS         += uri
uri_LDADD           = $(progs_ldadd)

TEST_PROGS         += dir
dir_LDADD           = $(progs_ldadd)

TEST_PROGS         += pattern
pattern_LDADD       = $(progs_ldadd)

TEST_PROGS         += logging
logging_LDADD       = $(progs_ldadd)

TEST_PROGS         += error
error_LDADD         = $(progs_ldadd)

TEST_PROGS         += bookmarkfile
bookmarkfile_LDADD  = $(progs_ldadd)

TEST_PROGS          += gdatetime
gdatetime_SOURCES    = gdatetime.c
gdatetime_LDADD      = $(progs_ldadd)

if OS_UNIX

# some testing of gtester funcitonality
XMLLINT=xmllint
gtester-xmllint-check: # check testreport xml with xmllint if present
	${GTESTER} -k --quiet -o tmpsample.xml --test-arg=--gtester-selftest ${GTESTER}
	${XMLLINT} --version 2>/dev/null; test "$$?" != 0 || ${XMLLINT} --noout tmpsample.xml
check-am: gtester-xmllint-check

endif

CLEANFILES = \
	tmpsample.xml

EXTRA_DIST += \
	4096-random-bytes	\
	keyfiletest.ini		\
	bookmarks.xbel

dist-hook:
	mkdir $(distdir)/markups;		\
	for f in $(srcdir)/markups/*; do 	\
	  cp $$f $(distdir)/markups; done
	mkdir $(distdir)/bookmarks;		\
	for f in $(srcdir)/bookmarks/* ; do 	\
	  cp $$f $(distdir)/bookmarks; done

