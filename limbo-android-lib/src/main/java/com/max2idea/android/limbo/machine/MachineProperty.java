/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
package com.max2idea.android.limbo.machine;

public enum MachineProperty {
    MACHINE_NAME, SNAPSHOT_NAME,
    STATUS, PAUSED, LAST_UPDATED,
    UI, MOUSE, ENABLE_USBMOUSE, KEYBOARD,
    ARCH, MACHINETYPE, CPU, CPUNUM, MEMORY, ENABLE_MTTCG, ENABLE_KVM,
    DISABLE_ACPI, DISABLE_HPET, DISABLE_TSC, DISABLE_FD_BOOT_CHK,
    HDA, HDB, HDC, HDD, SHARED_FOLDER, SHARED_FOLDER_MODE, HDCONFIG,
    CDROM, FDA, FDB, SD,
    MEDIA_INTERFACE, HDA_INTERFACE, HDB_INTERFACE, HDC_INTERFACE, HDD_INTERFACE, CDROM_INTERFACE,
    BOOT_CONFIG, KERNEL, INITRD, APPEND,
    VGA, SOUNDCARD, NETCONFIG, HOSTFWD, GUESTFWD, NICCONFIG,
    NON_REMOVABLE_DRIVE, REMOVABLE_DRIVE, DRIVE_ENABLED,
    EXTRA_PARAMS, OTHER
}

