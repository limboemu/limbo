# Assembler section type character
#
ASM_TCHAR	:= @
ASM_TCHAR_OPS	:= @

# Include common x86 headers
#
INCDIRS		+= arch/x86/include

# x86-specific directories containing source files
#
SRCDIRS		+= arch/x86/core
SRCDIRS		+= arch/x86/image
SRCDIRS		+= arch/x86/interface/pcbios
SRCDIRS		+= arch/x86/interface/pxe
SRCDIRS		+= arch/x86/interface/pxeparent
SRCDIRS		+= arch/x86/interface/efi
SRCDIRS		+= arch/x86/interface/vmware
SRCDIRS		+= arch/x86/interface/syslinux
SRCDIRS		+= arch/x86/prefix
SRCDIRS		+= arch/x86/hci/commands
SRCDIRS		+= arch/x86/drivers/xen
SRCDIRS		+= arch/x86/drivers/hyperv
SRCDIRS		+= arch/x86/transitions

# breaks building some of the linux-related objects
CFLAGS		+= -Ulinux

# disable valgrind
CFLAGS		+= -DNVALGRIND

# Define version string for lkrnprefix.S
#
CFLAGS_lkrnprefix	+= -DVERSION="\"$(VERSION)\""

# Include Hyper-V driver in the all-drivers build
#
DRIVERS_hyperv += hyperv
