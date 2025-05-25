/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Machine Setup
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <genfb.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;

/* FUNCTIONS ******************************************************************/

VOID
MachInit(const char *CmdLine)
{
    RtlZeroMemory(&MachVtbl, sizeof(MachVtbl));

    MachVtbl.ConsPutChar = UefiConsPutChar;
    MachVtbl.ConsKbHit = UefiConsKbHit;
    MachVtbl.ConsGetCh = UefiConsGetCh;
    MachVtbl.VideoClearScreen = GenFbVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = GenFbVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = GenFbVideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = GenFbVideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = GenFbVideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = GenFbVideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = GenFbVideoHideShowTextCursor;
    MachVtbl.VideoPutChar = GenFbVideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = GenFbVideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = GenFbVideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = GenFbVideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = GenFbVideoGetPaletteColor;
    MachVtbl.VideoSync = GenFbVideoSync;
    MachVtbl.Beep = UefiPcBeep;
    MachVtbl.PrepareForReactOS = UefiPrepareForReactOS;
    MachVtbl.GetMemoryMap = UefiMemGetMemoryMap;
    MachVtbl.GetExtendedBIOSData = UefiGetExtendedBIOSData;
    MachVtbl.GetFloppyCount = UefiGetFloppyCount;
    MachVtbl.DiskReadLogicalSectors = UefiDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = UefiDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = UefiDiskGetCacheableBlockCount;
    MachVtbl.GetTime = UefiGetTime;
    MachVtbl.InitializeBootDevices = UefiInitializeBootDevices;
    MachVtbl.HwDetect = UefiHwDetect;
    MachVtbl.HwIdle = UefiHwIdle;

    /* Setup GOP */
    if (UefiInitializeVideo() != EFI_SUCCESS)
    {
        ERR("Failed to setup GOP\n");
    }
}
