# generic
ARCH_CFLAGS += -lc -lm -llog

#Standard C 99
ARCH_CFLAGS += -std=gnu99

# Suppress some warnings
#ARCH_CFLAGS += -Wno-psabi

# Smaller code generation for shared libraries, usually faster
# if doesn't work use -fPIC
ARCH_CFLAGS += -fpic

#Standard Android NDK flags
ARCH_CFLAGS += -ffunction-sections -funwind-tables -no-canonical-prefixes -fomit-frame-pointer
ARCH_CFLAGS += -DANDROID -DGL_GLEXT_PROTOTYPES -Wa,--noexecstack -Wformat -Werror=format-security  -Wno-format-security


################## OPTIMIZATION
# Overriding any of these values might
# have an adverse effect do this only if
# you know what you're doing
ifeq ($(USE_OPTIMIZATION),true)

    # QEMU is using O2 by default
    #ARCH_CFLAGS += -O2

    # Loop optimization might be safe?
    #ARCH_CFLAGS += -fstrength-reduce
    #ARCH_CFLAGS += -fforce-addr

    # Faster math might not be safe?
    #ARCH_CFLAGS += -ffast-math

    # Fast optimizations but maybe crashing apps?
    #ARCH_CFLAGS += -funsafe-math-optimizations

    #Inline functions limit
    #ARCH_CFLAGS += -finline-limit=99999

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
