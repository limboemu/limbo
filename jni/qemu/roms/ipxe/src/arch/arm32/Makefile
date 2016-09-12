# ARM32-specific directories containing source files
#
SRCDIRS		+= arch/arm32/core
SRCDIRS		+= arch/arm32/libgcc

# ARM32-specific flags
#
CFLAGS		+= -mthumb -mcpu=cortex-a15 -mabi=aapcs -mfloat-abi=soft
CFLAGS		+= -mword-relocations
ASFLAGS		+= -mthumb -mcpu=cortex-a15

# EFI requires -fshort-wchar, and nothing else currently uses wchar_t
#
CFLAGS		+= -fshort-wchar

# Include common ARM Makefile
MAKEDEPS	+= arch/arm/Makefile
include arch/arm/Makefile

# Include platform-specific Makefile
#
MAKEDEPS	+= arch/arm32/Makefile.$(PLATFORM)
include arch/arm32/Makefile.$(PLATFORM)
