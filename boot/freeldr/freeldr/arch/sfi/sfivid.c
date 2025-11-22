/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>

#include <genfb.h>

/* GLOBALS ********************************************************************/

extern multiboot_info_t * MultibootInfoPtr;
UCHAR MachDefaultTextColor = COLOR_GRAY;

/* FUNCTIONS ******************************************************************/
static
GENFB_PIXEL_FORMAT
MbColorInfoToPixelFormat(color_rgb_mode_t MbColorInfo)
{
    /* We only handle GMA RGB pixel formats here.
     * Assume pixel format based on green mask size. */
    switch (MbColorInfo.framebuffer_green_mask_size)
    {
        case 5:
            return GENFB_X1R5G5B5;
        case 6:
            return GENFB_R5G6B5;
        case 8:
            /* A8R8G8B8 supposedly is possible too, but there's no way to tell with Multuboot info */
            return GENFB_X8R8G8B8;
        case 10:
            return GENFB_X2R10G10B10;
        default:
            return GENFB_R5G6B5;
    }
}

VOID
SfiVideoInit(VOID)
{
    /* SFI devices do not feature a VGA compatible controller, even though Clovertrail+ and older
     * do advertise it as VGA compatible (via a PCI class). There's no standard graphics protocol
     * (such as GOP or UGA on (U)EFI) that we could leverage either. Most of SFI devices, however,
     * should have a panel and framebuffer already left initialized by the firmware. Therefore
     * we can achieve basic graphics support by writing to the framebuffer memory. We can fetch
     * the framebuffer information from Multiboot compliant bootloader, or appropriate GMA registers
     * which luckily remained the same across all generations of SFI devices.
     * TODO: Currently only the Multiboot method is implemented.
     */
    if (MultibootInfoPtr != NULL &&
        (MultibootInfoPtr->flags & MB_INFO_FLAG_FRAMEBUFFER_TABLE) &&
        (MultibootInfoPtr->framebuffer_type == MB_FRAMEBUFFER_RGB))
    {
        GENERIC_FRAMEBUFFER_CONTEXT framebufferData;
        RtlZeroMemory(&framebufferData, sizeof(framebufferData));

        framebufferData.BaseAddress = MultibootInfoPtr->framebuffer_addr;
        framebufferData.BufferSize = MultibootInfoPtr->framebuffer_pitch *
                                        MultibootInfoPtr->framebuffer_height;
        framebufferData.ScreenWidth = MultibootInfoPtr->framebuffer_width;
        framebufferData.ScreenHeight = MultibootInfoPtr->framebuffer_height;
        framebufferData.PixelsPerScanLine = MultibootInfoPtr->framebuffer_pitch /
        (MultibootInfoPtr->framebuffer_bpp / 8);
        framebufferData.PixelFormat = GENFB_X8R8G8B8;
        MbColorInfoToPixelFormat(MultibootInfoPtr->color_info.color_rgb);

        GenFbInitialize(&framebufferData);
        GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
    }
}

VOID
SfiVideoPrepareForReactOS(VOID)
{
    GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
    GenFbVideoHideShowTextCursor(FALSE);
}
