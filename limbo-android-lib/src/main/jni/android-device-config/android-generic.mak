# generic defs

# Version
GCC_TOOLCHAIN_VERSION=4.9

########## Use GCC
#NDK_TOOLCHAIN_VERSION=$(GCC_TOOLCHAIN_VERSION)
#ARCH_CFLAGS += -std=gnu99

########## or CLANG
NDK_TOOLCHAIN_VERSION=clang
ARCH_LD_CLANG_FLAGS += -Wc,-shared

#libs
ARCH_LD_FLAGS += -lc -lm -llog

# Suppress some warnings
#ARCH_CFLAGS += -Wno-psabi

# Smaller code generation for shared libraries, usually faster
# if doesn't work use -fPIC
ARCH_CFLAGS += -fpic

#Standard Android NDK flags
ARCH_CFLAGS += -ffunction-sections -funwind-tables -no-canonical-prefixes -fomit-frame-pointer

ARCH_CFLAGS += -DANDROID
ARCH_CFLAGS += -DGL_GLEXT_PROTOTYPES
ARCH_CFLAGS += -Wa,--noexecstack -Wformat -Werror=format-security  -Wno-format-security


################## OPTIMIZATION
# Overriding any of these values might
# have an adverse effect do this only if
# you know what you're doing
ifeq ($(USE_OPTIMIZATION),true)


ifneq ($(NDK_TOOLCHAIN_VERSION),clang)
    # Loop optimization might not be safe, only for GCC, not for clang
    ARCH_CFLAGS += -fstrength-reduce
    ARCH_CFLAGS += -fforce-addr
    ARCH_CFLAGS += -ffast-math
    ARCH_CFLAGS += -finline-limit=99999
    #ARCH_CFLAGS += -funsafe-math-optimizations

else
    # clang we can increase
    ARCH_CFLAGS += -O3
endif

#hardening security
ARCH_CFLAGS += -D_FORTIFY_SOURCE=2
ARCH_CFLAGS += -fstack-protector-strong -fstack-protector-all

# Or faster code but less security
#ARCH_CFLAGS += -U_FORTIFY_SOURCE
#ARCH_CFLAGS += -fno-stack-protector

else
    # we disable optimization make things easier when debugging
    ARCH_CFLAGS += -O0
endif

###################### DEBUGGING
ifeq ($(NDK_DEBUG),1)
	ARCH_CFLAGS += -g
	# for Debugging only
	# this is set by the compiler automatically for ndk libs
	# For qemu we might need this
	# This also produces compilation errors in libffi so we suppress it there
	ARCH_CFLAGS += -funwind-tables
endif
