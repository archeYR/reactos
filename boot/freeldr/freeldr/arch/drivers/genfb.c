/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Generic framebuffer video driver
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>

#include <genfb.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

extern UCHAR BitmapFont8x16[256 * 16];

GENERIC_FRAMEBUFFER_CONTEXT FbContext;
static ULONG BytesPerPixel = 0;

/* FUNCTIONS ******************************************************************/

VOID
GenFbInitialize(
    _In_ PGENERIC_FRAMEBUFFER_CONTEXT FramebufferContext)
{
    RtlZeroMemory(&FbContext, sizeof(FbContext));
    RtlCopyMemory(&FbContext, FramebufferContext, sizeof(FbContext));

    /* FIXME: Implement proper pixel format handling */
    switch (FbContext.PixelFormat)
    {
        case GENFB_R5G6B5:
        case GENFB_R5G5B5A1:
        case GENFB_X1R5G5B5:
        case GENFB_A1R5G5B5:
            BytesPerPixel = 2;
            break;
        case GENFB_R8G8B8:
            BytesPerPixel = 3;
        case GENFB_X8R8G8B8:
        case GENFB_A8R8G8B8:
        case GENFB_X8B8G8R8:
        case GENFB_A8B8G8R8:
        case GENFB_X2R10G10B10:
        case GENFB_A2R10G10B10:
        default:
            BytesPerPixel = 4;
    }

    TRACE("Framebuffer BaseAddress       : %X\n", FbContext.BaseAddress);
    TRACE("Framebuffer BufferSize        : %X\n", FbContext.BufferSize);
    TRACE("Framebuffer ScreenWidth       : %d\n", FbContext.ScreenWidth);
    TRACE("Framebuffer ScreenHeight      : %d\n", FbContext.ScreenHeight);
    TRACE("Framebuffer PixelsPerScanLine : %d\n", FbContext.PixelsPerScanLine);
    //TRACE("Framebuffer PixelFormat     : %d\n", FbContext.PixelFormat);
    TRACE("Framebuffer BytesPerPixel     : %d\n", BytesPerPixel);
}

VOID
GenFbGetFramebufferData(
    _Out_ PGENERIC_FRAMEBUFFER_CONTEXT FramebufferContext)
{
    /* If there is any framebuffer bpp should be set */
    if (!BytesPerPixel)
        return;

    RtlCopyMemory(FramebufferContext, &FbContext, sizeof(*FramebufferContext));
}

static
ULONG
GenFbVideoAttrToSingleColor(
    _In_ UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static
VOID
GenFbVideoAttrToColors(
    _In_ UCHAR Attr,
    _In_ ULONG *FgColor,
    _In_ ULONG *BgColor)
{
    *FgColor = GenFbVideoAttrToSingleColor(Attr & 0xf);
    *BgColor = GenFbVideoAttrToSingleColor((Attr >> 4) & 0xf);
}


static
VOID
GenFbVideoClearScreenColor(
    _In_ ULONG Color)
{
    ULONG Delta;
    ULONG Line, Col;
    PULONG p;

    Delta = (FbContext.PixelsPerScanLine * BytesPerPixel + 3) & ~ 0x3;
    for (Line = 0; Line < FbContext.ScreenHeight; Line++)
    {
        p = (PULONG) ((char *) FbContext.BaseAddress + Line * Delta);
        for (Col = 0; Col < FbContext.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

VOID
GenFbVideoClearScreen(
    _In_ UCHAR Attr)
{
    ULONG FgColor, BgColor;

    GenFbVideoAttrToColors(Attr, &FgColor, &BgColor);
    GenFbVideoClearScreenColor(BgColor);
}

VOID
GenFbVideoOutputChar(
    _In_ UCHAR Char,
    _In_ unsigned X,
    _In_ unsigned Y,
    _In_ ULONG FgColor,
    _In_ ULONG BgColor)
{
    PUCHAR FontPtr;
    PULONG Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;
    ULONG Delta;
    Delta = (FbContext.PixelsPerScanLine * BytesPerPixel + 3) & ~ 0x3;
    FontPtr = BitmapFont8x16 + Char * 16;
    Pixel = (PULONG) ((char *) FbContext.BaseAddress +
            (Y * CHAR_HEIGHT) *  Delta + X * CHAR_WIDTH * BytesPerPixel);

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (PULONG) ((char *) Pixel + Delta);
    }
}

VOID
GenFbVideoPutChar(
    _In_ int Ch,
    _In_ UCHAR Attr,
    _In_ unsigned X,
    _In_ unsigned Y)
{
    ULONG FgColor = 0;
    ULONG BgColor = 0;
    if (Ch != 0)
    {
        GenFbVideoAttrToColors(Attr, &FgColor, &BgColor);
        GenFbVideoOutputChar(Ch, X, Y, FgColor, BgColor);
    }
}

VOID
GenFbVideoGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    *Width =  FbContext.ScreenWidth / CHAR_WIDTH;
    *Height = (FbContext.ScreenHeight) / CHAR_HEIGHT;
    *Depth =  0;
}

VIDEODISPLAYMODE
GenFbVideoSetDisplayMode(
    _In_ char *DisplayMode,
    _In_ BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

ULONG
GenFbVideoGetBufferSize(VOID)
{
    return (FbContext.ScreenHeight / CHAR_HEIGHT * (FbContext.ScreenWidth / CHAR_WIDTH) * 2);
}

VOID
GenFbVideoGetFontsFromFirmware(
    _Out_ PULONG RomFontPointers)
{

}

VOID
GenFbVideoCopyOffScreenBufferToVRAM(
    _Out_ PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;

    ULONG Col, Line;
    for (Line = 0; Line < (FbContext.ScreenHeight) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < FbContext.ScreenWidth / CHAR_WIDTH; Col++)
        {
            GenFbVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += 2;
        }
    }
}

VOID
GenFbVideoScrollUp(VOID)
{
    ULONG BgColor, Dummy;
    ULONG Delta;
    Delta = (FbContext.PixelsPerScanLine * BytesPerPixel + 3) & ~ 0x3;
    ULONG PixelCount = FbContext.ScreenWidth * CHAR_HEIGHT *
                       (((FbContext.ScreenHeight) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)FbContext.BaseAddress + CHAR_HEIGHT * Delta);
    PULONG Dst = (PULONG)((PUCHAR)FbContext.BaseAddress);

    GenFbVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < FbContext.ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

VOID
GenFbVideoSetTextCursorPosition(
    _In_ UCHAR X,
    _In_ UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
GenFbVideoHideShowTextCursor(
    _In_ BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
GenFbVideoIsPaletteFixed(VOID)
{
    return 0;
}

VOID
GenFbVideoSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red,
    _In_ UCHAR Green,
    _In_ UCHAR Blue)
{
    /* Not supported */
}

VOID
GenFbVideoGetPaletteColor(
    _In_ UCHAR Color,
    _Out_ UCHAR* Red,
    _Out_ UCHAR* Green,
    _Out_ UCHAR* Blue)
{
    /* Not supported */
}

VOID
GenFbVideoSync(VOID)
{
    /* Not supported */
}
