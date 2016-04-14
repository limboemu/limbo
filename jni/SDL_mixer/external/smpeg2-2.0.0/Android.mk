LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := smpeg2

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -D_THREAD_SAFE -DTHREADED_AUDIO -DNOCONTROLS

LOCAL_SRC_FILES := \
    smpeg.cpp \
    MPEG.cpp \
    MPEGlist.cpp \
    MPEGring.cpp \
    MPEGstream.cpp \
    MPEGsystem.cpp \
    audio/MPEGaudio.cpp \
    audio/bitwindow.cpp \
    audio/filter.cpp \
    audio/filter_2.cpp \
    audio/hufftable.cpp \
    audio/mpeglayer1.cpp \
    audio/mpeglayer2.cpp \
    audio/mpeglayer3.cpp \
    audio/mpegtable.cpp \
    audio/mpegtoraw.cpp \
    video/MPEGvideo.cpp \
    video/decoders.cpp \
    video/floatdct.cpp \
    video/gdith.cpp \
    video/jrevdct.cpp \
    video/motionvec.cpp \
    video/parseblock.cpp \
    video/readfile.cpp \
    video/util.cpp \
    video/video.cpp

LOCAL_LDLIBS :=
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
