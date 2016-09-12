# ARM64-specific directories containing source files
#
SRCDIRS		+= arch/arm64/core

# ARM64-specific flags
#
CFLAGS		+= -mabi=lp64 -mlittle-endian -mcmodel=small
CFLAGS		+= -fomit-frame-pointer
ASFLAGS		+= -mabi=lp64 -EL

# EFI requires -fshort-wchar, and nothing else currently uses wchar_t
#
CFLAGS		+= -fshort-wchar

# Include common ARM Makefile
MAKEDEPS	+= arch/arm/Makefile
include arch/arm/Makefile

# Include platform-specific Makefile
#
MAKEDEPS	+= arch/arm64/Makefile.$(PLATFORM)
include arch/arm64/Makefile.$(PLATFORM)
