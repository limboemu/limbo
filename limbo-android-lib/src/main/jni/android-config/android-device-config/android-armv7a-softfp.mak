#Generic defs first
include $(LIMBO_JNI_ROOT)/android-config/android-device-config/android-generic.mak

ARCH_CFLAGS += -D__ANDROID_API__=$(NDK_PLATFORM_API)

#CLANG ONLY
ifeq ($(NDK_TOOLCHAIN_VERSION),clang)
    # ARCH_CLANG_FLAGS += -gcc-toolchain $(TOOLCHAIN_DIR)
    ARCH_CLANG_FLAGS += -target armv7-none-linux-androideabi$(NDK_PLATFORM_API)
    ARCH_CFLAGS += $(ARCH_CLANG_FLAGS) -D__ANDROID_API__=$(NDK_PLATFORM_API)
    # ARCH_CFLAGS += -fno-integrated-as
    ARCH_LD_FLAGS += -Wc,-target -Wc,armv7-none-linux-androideabi$(NDK_PLATFORM_API)
    ARCH_LD_FLAGS += -Wc,-gcc-toolchain -Wc,$(TOOLCHAIN_DIR)
endif

#LINKER SPECIFIC
#ARCH_LD_FLAGS += -Wl,--fix-cortex-a8

#TARGET ARCH
APP_ABI = armeabi-v7a
ARM_MODE=arm

ARCH_CFLAGS += -march=armv7-a

#FLOAT
ARCH_CFLAGS += -mfloat-abi=softfp

# Use VFP (Optional)
#ARCH_CFLAGS += -mfpu=vfpv3-d16
#ARCH_CFLAGS += -mfpu=vfpv3

# Tuning (Optional)
#ARCH_CFLAGS += -mtune=arm7




