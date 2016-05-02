LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    src/dec/alpha.c \
    src/dec/buffer.c \
    src/dec/frame.c \
    src/dec/idec.c \
    src/dec/io.c \
    src/dec/layer.c \
    src/dec/quant.c \
    src/dec/tree.c \
    src/dec/vp8.c \
    src/dec/vp8l.c \
    src/dec/webp.c \
    src/dsp/cpu.c \
    src/dsp/dec.c \
    src/dsp/dec_sse2.c \
    src/dsp/enc.c \
    src/dsp/enc_sse2.c \
    src/dsp/lossless.c \
    src/dsp/upsampling.c \
    src/dsp/upsampling_sse2.c \
    src/dsp/yuv.c \
    src/enc/alpha.c \
    src/enc/analysis.c \
    src/enc/backward_references.c \
    src/enc/config.c \
    src/enc/cost.c \
    src/enc/filter.c \
    src/enc/frame.c \
    src/enc/histogram.c \
    src/enc/iterator.c \
    src/enc/layer.c \
    src/enc/picture.c \
    src/enc/quant.c \
    src/enc/syntax.c \
    src/enc/token.c \
    src/enc/tree.c \
    src/enc/vp8l.c \
    src/enc/webpenc.c \
    src/utils/bit_reader.c \
    src/utils/bit_writer.c \
    src/utils/color_cache.c \
    src/utils/filters.c \
    src/utils/huffman.c \
    src/utils/huffman_encode.c \
    src/utils/quant_levels.c \
    src/utils/quant_levels_dec.c \
    src/utils/rescaler.c \
    src/utils/thread.c \
    src/utils/utils.c \

LOCAL_CFLAGS := -Wall -DANDROID -DHAVE_MALLOC_H -DHAVE_PTHREAD \
                -DWEBP_USE_THREAD \
                -finline-functions -frename-registers -ffast-math \
                -s -fomit-frame-pointer -Isrc/webp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  # Setting LOCAL_ARM_NEON will enable -mfpu=neon which may cause illegal
  # instructions to be generated for armv7a code. Instead target the neon code
  # specifically.
  LOCAL_SRC_FILES += src/dsp/dec_neon.c.neon
  LOCAL_SRC_FILES += src/dsp/upsampling_neon.c.neon
  LOCAL_SRC_FILES += src/dsp/enc_neon.c.neon
endif
LOCAL_STATIC_LIBRARIES := cpufeatures

LOCAL_MODULE:= webp

include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/cpufeatures)
