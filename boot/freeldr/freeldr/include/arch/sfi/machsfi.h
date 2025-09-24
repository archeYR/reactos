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

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

BOOLEAN SfiMachDetect(const char *CmdLine);
VOID SfiMachInit(const char *CmdLine);

VOID SfiConsPutChar(int c);
BOOLEAN SfiConsKbHit(VOID);
int SfiConsGetCh(VOID);
VOID SfiVideoInit(VOID);
VOID SfiVideoPrepareForReactOS(VOID);

VOID SfiPrepareForReactOS(VOID);

PFREELDR_MEMORY_DESCRIPTOR SfiMemGetMemoryMap(ULONG *MemoryMapSize);

UCHAR SfiGetFloppyCount(VOID);
BOOLEAN SfiInitializeBootDevices(VOID);
BOOLEAN SfiDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN SfiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG SfiDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* SfiGetTime(VOID);
BOOLEAN SfiInitTime(VOID);

PCONFIGURATION_COMPONENT_DATA SfiHwDetect(
    _In_opt_ PCSTR Options);
VOID SfiHwIdle(VOID);

/* pcmem.c */
extern BIOS_MEMORY_MAP PcBiosMemoryMap[];
extern ULONG PcBiosMapCount;

PFREELDR_MEMORY_DESCRIPTOR Pc98MemGetMemoryMap(ULONG *MemoryMapSize);

/* hwpci.c */
BOOLEAN PcFindPciBios(PPCI_REGISTRY_INFO BusData);

/* Platform-specific boot drive and partition numbers */
extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;

LONG DiskReportError(BOOLEAN bShowError);

/* EOF */
