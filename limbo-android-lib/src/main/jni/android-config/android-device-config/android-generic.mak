# generic defs

## Do not comment this, always specify a gcc base version
GCC_TOOLCHAIN_VERSION=4.9

# override the log functions 
ARCH_EXTRA_CFLAGS += -include $(LOGUTILS)

# choose gcc or clang
ifneq ($(USE_GCC),true)
    NDK_TOOLCHAIN_VERSION=clang
else
    NDK_TOOLCHAIN_VERSION=$(GCC_TOOLCHAIN_VERSION)
endif

ifneq ($(NDK_TOOLCHAIN_VERSION),clang)
    ARCH_CFLAGS += -std=gnu99
else
    ARCH_LD_CLANG_FLAGS += -Wc,-shared
endif

ARCH_CFLAGS += -Wno-macro-redefined
    
#libs
ARCH_LD_FLAGS += -lc -lm -llog

# add aaudio
ifeq ($(USE_AAUDIO),true)
	ARCH_CFLAGS += -D__ENABLE_AAUDIO__
endif

# Suppress some warnings
#ARCH_CFLAGS += -Wno-psabi
ARCH_CFLAGS += -Wno-error=declaration-after-statement -Wno-unused-variable
ifneq ($(NDK_TOOLCHAIN_VERSION),clang)
	ARCH_CFLAGS += -Wno-unused-but-set-variable -Wno-unused-function
endif

# Smaller code generation for shared libraries, usually faster
# if doesn't work use -fPIC
ARCH_CFLAGS += -fpic

#Standard Android NDK flags
ARCH_CFLAGS += -ffunction-sections -funwind-tables -no-canonical-prefixes -fomit-frame-pointer

ARCH_CFLAGS += -DANDROID
ARCH_CFLAGS += -DGL_GLEXT_PROTOTYPES
ARCH_CFLAGS += -Wa,--noexecstack -Wformat -Werror=format-security  -Wno-format-security
ARCH_LD_CFLAGS += -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--warn-shared-textrel -Wl,--fatal-warnings

################## OPTIMIZATION
# Overriding any of these values might
# have an adverse effect do this only if
# you know what you're doing
ifeq ($(USE_OPTIMIZATION),true)
        #ARCH_CFLAGS += -O2
        # Below optimizations might not be safe
        ARCH_CFLAGS += -Ofast
        # might not be supported by clang
        #ARCH_CFLAGS += -fforce-addr
        #ARCH_CFLAGS += -ffast-math
        #ARCH_CFLAGS += -finline-limit=99999
        #ARCH_CFLAGS += -fstrength-reduce
else
    # we disable optimization make things easier when debugging
    ARCH_CFLAGS += -O0
endif

ifeq ($(USE_SECURITY),true)
        # Hardening security but slow performance is the best option for now
        ARCH_CFLAGS += -D_FORTIFY_SOURCE=2
        ARCH_CFLAGS += -fstack-protector-strong
        # extra hardening
        #ARCH_CFLAGS += -fstack-protector-all
        
        # add sanitize options (they should be off by default)
        #CLANG_SANITIZE_FLAGS = -fsanitize=safe-stack
        #ARCH_CFLAGS += $(CLANG_SANITIZE_FLAGS)
        #ARCH_LD_FLAGS += $(CLANG_SANITIZE_FLAGS)
else
        # Uncomment to get a slight performance boost but with less security
        ARCH_CFLAGS += -U_FORTIFY_SOURCE
        ARCH_CFLAGS += -fno-stack-protector
        
        # remove sanitize options (they should be off by default)
        #CLANG_SANITIZE_FLAGS += -fno-sanitize=safe-stack
        #ARCH_CFLAGS += $(CLANG_SANITIZE_FLAGS)
        #ARCH_LD_FLAGS += $(CLANG_SANITIZE_FLAGS)
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
