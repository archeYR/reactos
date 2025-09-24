/*
 *  FreeLoader SFI support
 *  Copyright (C) 2021  Xen (https://gitlab.com/XenRE)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(DISK);

extern multiboot_info_t * MultibootInfoPtr;

UCHAR
SfiGetFloppyCount(VOID)
{
	/* No floppy drives on SFI systems */

	return 0;
}

VOID __cdecl DiskStopFloppyMotor(VOID)
{
    /* No floppy controller on SFI */
}

VOID __cdecl ChainLoadBiosBootSectorCode(
    IN UCHAR BootDrive OPTIONAL,
    IN ULONG BootPartition OPTIONAL)
{
    /* No boot sectors on SFI */
}

BOOLEAN
SfiInitializeBootDevices(VOID)
{
    /* Read the initial RAM disk info from Multiboot if it wasn't specified in cmdline */
    if(!gInitRamDiskBase || !gInitRamDiskSize)
    {
        multiboot_module_t *RamDiskInfo;
        if (!MultibootInfoPtr)
        {
            ERR("Multiboot info structure not found!\n");
            return FALSE;
        }

        /* RAM disk info is passed as a Multiboot module. */
        if (!(MultibootInfoPtr->flags &  MB_INFO_FLAG_MODULES) ||
            (MultibootInfoPtr->mods_count < 2))
        {
            ERR("Multiboot RAM disk info is not passed!\n");
            return FALSE;
        }

        RamDiskInfo = (multiboot_module_t *)(MultibootInfoPtr->mods_addr + sizeof(multiboot_module_t));
        gInitRamDiskBase = (PULONG)(RamDiskInfo->mod_start);
        gInitRamDiskSize = (RamDiskInfo->mod_end - RamDiskInfo->mod_start);
        TRACE("RAM disk start address: 0x%x, RAM disk size: %d\n", gInitRamDiskBase, gInitRamDiskSize);
    }

    /* Initialize the RAMDISK Device */
    RamDiskInitialize(TRUE, NULL, NULL);

    /* Fill out the ARC disk block */
    AddReactOSArcDiskInfo("ramdisk(0)", 0xBADAB00F, 0xDEADBABE, TRUE);

    /* Set the boot path to RAM disk. This is where we will load FreeLDR modules from */
    RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath), "ramdisk(%u)", 0);

	return TRUE;
}

BOOLEAN
SfiDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
	/* Unused for SFI systems */
	return FALSE;
}

BOOLEAN
SfiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
	/* Unused for SFI systems */
	return FALSE;
}

ULONG
SfiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
	/* Unused for SFI systems */
	return 0;
}
