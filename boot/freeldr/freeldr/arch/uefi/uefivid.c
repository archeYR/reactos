/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <genfb.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define LOWEST_SUPPORTED_RES 1

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;

UCHAR MachDefaultTextColor = COLOR_GRAY;
EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/* FUNCTIONS ******************************************************************/

EFI_STATUS
UefiInitializeVideo(VOID)
{
    EFI_STATUS Status;
    GENERIC_FRAMEBUFFER_CONTEXT framebufferData;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    RtlZeroMemory(&framebufferData, sizeof(framebufferData));
    Status = GlobalSystemTable->BootServices->LocateProtocol(&EfiGraphicsOutputProtocol, 0, (void**)&gop);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find GOP with status %d\n", Status);
        return Status;
    }

    /* We don't need high resolutions for freeldr */
    gop->SetMode(gop, LOWEST_SUPPORTED_RES);

    framebufferData.BaseAddress        = (ULONG_PTR)gop->Mode->FrameBufferBase;
    framebufferData.BufferSize         = gop->Mode->FrameBufferSize;
    framebufferData.ScreenWidth        = gop->Mode->Info->HorizontalResolution;
    framebufferData.ScreenHeight       = gop->Mode->Info->VerticalResolution;
    framebufferData.PixelsPerScanLine  = gop->Mode->Info->PixelsPerScanLine;

    if (gop->Mode->Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
        framebufferData.PixelFormat = GENFB_A8R8G8B8;
    else if (gop->Mode->Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
        framebufferData.PixelFormat = GENFB_A8B8G8R8;
    else
        return EFI_UNSUPPORTED;


    GenFbInitialize(&framebufferData);

    return Status;
}
