/*
 * PROJECT:     Freeldr UEFI Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Generic framebuffer video driver header
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* INCLUDES ******************************************************************/
#include <freeldr.h>

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

typedef enum _GENFB_PIXEL_FORMAT
{
    GENFB_R5G6B5,
    GENFB_R5G5B5A1,
    GENFB_X1R5G5B5,
    GENFB_A1R5G5B5,
    GENFB_R8G8B8,
    GENFB_X8R8G8B8,
    GENFB_A8R8G8B8,
    GENFB_X8B8G8R8,
    GENFB_A8B8G8R8,
    GENFB_X2R10G10B10,
    GENFB_A2R10G10B10,
} GENFB_PIXEL_FORMAT, *PGENFB_PIXEL_FORMAT;

typedef struct _GENERIC_FRAMEBUFFER_CONTEXT
{
    ULONG_PTR           BaseAddress;
    ULONG               BufferSize;
    UINT32              ScreenWidth;
    UINT32              ScreenHeight;
    UINT32              PixelsPerScanLine;
    GENFB_PIXEL_FORMAT  PixelFormat;
} GENERIC_FRAMEBUFFER_CONTEXT, *PGENERIC_FRAMEBUFFER_CONTEXT;

VOID
GenFbInitialize(
    _In_ PGENERIC_FRAMEBUFFER_CONTEXT FramebufferContext);

VOID
GenFbGetFramebufferData(
    _Out_ PGENERIC_FRAMEBUFFER_CONTEXT FramebufferContext);

VOID
GenFbVideoClearScreen(
    _In_ UCHAR Attr);

VOID
GenFbVideoOutputChar(
    _In_ UCHAR Char,
    _In_ unsigned X,
    _In_ unsigned Y,
    _In_ ULONG FgColor,
    _In_ ULONG BgColor);

VOID
GenFbVideoPutChar(
    _In_ int Ch,
    _In_ UCHAR Attr,
    _In_ unsigned X,
    _In_ unsigned Y);

VOID
GenFbVideoGetDisplaySize(
    _In_ PULONG Width,
    _In_ PULONG Height,
    _In_ PULONG Depth);

VIDEODISPLAYMODE
GenFbVideoSetDisplayMode(
    _In_ char *DisplayMode,
    _In_ BOOLEAN Init);

ULONG
GenFbVideoGetBufferSize(VOID);

VOID
GenFbVideoGetFontsFromFirmware(
    _In_ PULONG RomFontPointers);

VOID
GenFbVideoCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer);

VOID
GenFbVideoScrollUp(VOID);

VOID
GenFbVideoSetTextCursorPosition(
    _In_ UCHAR X,
    _In_ UCHAR Y);

VOID
GenFbVideoHideShowTextCursor(
    _In_ BOOLEAN Show);

BOOLEAN
GenFbVideoIsPaletteFixed(VOID);

VOID
GenFbVideoSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red,
    _In_ UCHAR Green,
    _In_ UCHAR Blue);

VOID
GenFbVideoGetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR* Red,
    _In_ UCHAR* Green,
    _In_ UCHAR* Blue);

VOID
GenFbVideoSync(VOID);
