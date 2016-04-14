
## Makefile.am for the smpeg library

bin_SCRIPTS = smpeg2-config

CCLD = $(CXXLD)

# The smpeg library target
lib_LTLIBRARIES = libsmpeg2.la

libsmpeg2_la_SOURCES =		\
	MPEG.cpp		\
	MPEGring.cpp		\
	MPEGlist.cpp		\
	MPEGstream.cpp		\
	MPEGsystem.cpp		\
	smpeg.cpp		\
	audio/MPEGaudio.cpp	\
	audio/bitwindow.cpp	\
	audio/filter.cpp	\
	audio/filter_2.cpp	\
	audio/hufftable.cpp	\
	audio/mpeglayer1.cpp	\
	audio/mpeglayer2.cpp	\
	audio/mpeglayer3.cpp	\
	audio/mpegtable.cpp	\
	audio/mpegtoraw.cpp	\
	video/MPEGvideo.cpp	\
	video/decoders.cpp	\
	video/decoders.h	\
	video/dither.h		\
	video/floatdct.cpp	\
	video/gdith.cpp		\
	video/jrevdct.cpp	\
	video/motionvec.cpp	\
	video/parseblock.cpp	\
	video/proto.h		\
	video/readfile.cpp	\
	video/util.cpp		\
	video/util.h		\
	video/video.cpp		\
	video/video.h		\
	video/mmxflags_asm.S	\
	video/mmxidct_asm.S

libsmpeg2includedir = $(includedir)/smpeg2
libsmpeg2include_HEADERS =	\
	MPEG.h			\
	MPEGaction.h		\
	MPEGaudio.h		\
	MPEGerror.h		\
	MPEGframe.h		\
	MPEGring.h		\
	MPEGlist.h		\
	MPEGstream.h		\
	MPEGsystem.h		\
	MPEGvideo.h		\
	smpeg.h

libsmpeg2_la_LDFLAGS = 		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

EXTRA_DIST =			\
	CHANGES			\
	COPYING			\
	TODO			\
	README			\
	README.SDL_mixer 	\
	plaympeg.1		\
	Android.mk		\
	VisualC.zip		\
	smpeg2.m4		\
	smpeg2.spec		\
	smpeg2.spec.in		\
	gcc-fat.sh		\
	g++-fat.sh		\
	acinclude		\
	autogen.sh		\
	audio/AUTHORS		\
	audio/COPYING.LIB	\
	audio/README		\
	audio/README.LIB	\
	video/README		\
	video/COPYRIGHT


video/16bit_mmx.lo: video/16bit_mmx.cpp
	$(CXX) -c -o $@ $^ $(CXXFLAGS)
video/32bit_mmx.lo: video/32bit_mmx.cpp
	$(CXX) -c -o $@ $^ $(CXXFLAGS)

bin_PROGRAMS = plaympeg

# Sources for plaympeg
man_MANS = plaympeg.1
plaympeg_SOURCES = plaympeg.c
plaympeg_LDADD = libsmpeg2.la

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = smpeg2.m4

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba smpeg2.spec

