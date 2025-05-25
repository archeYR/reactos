/*
 *  FreeLoader
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

#include <drivers/xbox/xgpu.h>

extern UCHAR BitmapFont8x16[256 * 16];

VOID XboxConsPutChar(int Ch);
BOOLEAN XboxConsKbHit(VOID);
int XboxConsGetCh(VOID);

VOID XboxVideoInit(VOID);
VOID XboxVideoPrepareForReactOS(VOID);
VOID XboxVideoScrollUp(VOID);
VOID XboxPrepareForReactOS(VOID);

VOID XboxMemInit(VOID);
PFREELDR_MEMORY_DESCRIPTOR XboxMemGetMemoryMap(ULONG *MemoryMapSize);

BOOLEAN XboxDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN XboxDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG XboxDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* XboxGetTime(VOID);

PCONFIGURATION_COMPONENT_DATA
XboxHwDetect(
    _In_opt_ PCSTR Options);

VOID XboxHwIdle(VOID);

VOID XboxSetLED(PCSTR Pattern);

/* EOF */
