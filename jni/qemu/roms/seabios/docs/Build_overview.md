The SeaBIOS code can be built using standard GNU tools. A recent Linux
distribution should be able to build SeaBIOS using the standard
compiler tools.

Building SeaBIOS
================

First, [obtain the code](Download). SeaBIOS can be compiled for
several different build targets. It is also possible to configure
additional compile time options - run **make menuconfig** to do this.

Build for QEMU (along with KVM, Xen, and Bochs)
-----------------------------------------------

To build for QEMU (and similar), one should be able to run "make" in
the main directory. The resulting file "out/bios.bin" contains the
processed bios image.

One can use the resulting binary with QEMU by using QEMU's "-bios"
option. For example:

`qemu -bios out/bios.bin -fda myfdimage.img`

One can also use the resulting binary with Bochs. For example:

`bochs -q 'floppya: 1_44=myfdimage.img' 'romimage: file=out/bios.bin'`

Build for coreboot
------------------

To build for coreboot please see the coreboot build instructions at:
<http://www.coreboot.org/SeaBIOS>

Build as a UEFI Compatibility Support Module (CSM)
--------------------------------------------------

To build as a CSM, first run kconfig (make menuconfig) and enable
CONFIG_CSM. Then build SeaBIOS (make) - the resulting binary will be
in "out/Csm16.bin".

This binary may be used with the OMVF/EDK-II UEFI firmware. It will
provide "legacy" BIOS services for booting non-EFI operating systems
and will also allow OVMF to display on otherwise unsupported video
hardware by using the traditional VGA BIOS. (Windows 2008r2 is known
to use INT 10h BIOS calls even when booted via EFI, and the presence
of a CSM makes this work as expected too.)

Having built SeaBIOS with CONFIG_CSM, one should be able to drop the
result (out/Csm16.bin) into an OVMF build tree at
OvmfPkg/Csm/Csm16/Csm16.bin and then build OVMF with 'build -D
CSM_ENABLE'. The SeaBIOS binary will be included as a discrete file
within the 'Flash Volume' which is created, and there are tools which
will extract it and allow it to be replaced.

Distribution builds
===================

If one is building a binary version of SeaBIOS as part of a package
(such as an rpm) or for wide distribution, please provide the
EXTRAVERSION field during the build. For example:

`make EXTRAVERSION="-${RPM_PACKAGE_RELEASE}"`

The EXTRAVERSION field should provide the package version (if
applicable) and the name of the distribution (if that's not already
obvious from the package version). This string will be appended to the
main SeaBIOS version. The above information helps SeaBIOS developers
correlate defect reports to the source code and build environment.

If one is building a binary in a build environment that does not have
access to the git tool or does not have the full SeaBIOS git repo
available, then please use an official SeaBIOS release tar file as
source. If building from a snapshot (where there is no official
SeaBIOS tar) then one should generate a snapshot tar file on a machine
that does support git using the scripts/tarball.sh tool. For example:

`scripts/tarball.sh`

The tarball.sh script encodes version information in the resulting tar
file which the build can extract and include in the final binary. The
above EXTRAVERSION field should still be set when building from a tar.

Overview of files in the repository
===================================

The **src/** directory contains the main bios source code. The
**src/hw/** directory contains source code specific to hardware
drivers. The **src/fw/** directory contains source code for platform
firmware initialization. The **src/std/** directory contains header
files describing standard bios, firmware, and hardware interfaces.

The **vgasrc/** directory contains code for [SeaVGABIOS](SeaVGABIOS).

The **scripts/** directory contains helper utilities for manipulating
and building the final roms.

The **out/** directory is created by the build process - it contains
all intermediate and final files.

When reading the C code be aware that code that runs in 16bit mode can
not arbitrarily access non-stack memory - see [Memory Model](Memory
Model) for more details. For information on the major C code functions
and where code execution starts see [Execution and code
flow](Execution and code flow).
