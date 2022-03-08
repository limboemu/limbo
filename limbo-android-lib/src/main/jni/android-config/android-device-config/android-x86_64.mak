#Generic defs first
include $(LIMBO_JNI_ROOT)/android-config/android-device-config/android-generic.mak

ARCH_CFLAGS += -D__ANDROID_API__=$(NDK_PLATFORM_API)
ARCH_LD_FLAGS += -latomic

#CLANG ONLY
ifeq ($(NDK_TOOLCHAIN_VERSION),clang)
    TARGET_PREFIX = x86_64-none-linux-android
    # ARCH_CLANG_FLAGS += -gcc-toolchain $(TOOLCHAIN_DIR)
    ARCH_CLANG_FLAGS += -target $(TARGET_PREFIX)$(NDK_PLATFORM_API)
    ARCH_CFLAGS += $(ARCH_CLANG_FLAGS) -D__ANDROID_API__=$(NDK_PLATFORM_API)
    # ARCH_CFLAGS += -fno-integrated-as
    ARCH_LD_FLAGS += -Wc,-target -Wc,x86_64-none-linux-android$(NDK_PLATFORM_API)
    ARCH_LD_FLAGS += -Wc,-gcc-toolchain -Wc,$(TOOLCHAIN_DIR)
endif

#TARGET ARCH
APP_ABI = x86_64

# Use 64bit
ARCH_CFLAGS += -m64




