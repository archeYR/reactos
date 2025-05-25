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
 *
 * Note: much of this code was based on knowledge and/or code developed
 * by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>

#include <genfb.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

ULONG NvBase = 0xFD000000;

extern multiboot_info_t * MultibootInfoPtr;

UCHAR MachDefaultTextColor = COLOR_GRAY;

#define FB_SIZE_MB 4

UCHAR
NvGetCrtc(UCHAR Index)
{
    WRITE_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_INDEX, Index);
    return READ_REGISTER_UCHAR(NvBase + NV2A_CRTC_REGISTER_VALUE);
}

ULONG
XboxGetFramebufferSize(ULONG_PTR Offset)
{
    memory_map_t * MemoryMap;
    INT Count, i;

    if (!MultibootInfoPtr)
    {
        return 0;
    }

    if (!(MultibootInfoPtr->flags & MB_INFO_FLAG_MEMORY_MAP))
    {
        return 0;
    }

    MemoryMap = (memory_map_t *)MultibootInfoPtr->mmap_addr;

    if (!MemoryMap ||
        MultibootInfoPtr->mmap_length == 0 ||
        MultibootInfoPtr->mmap_length % sizeof(memory_map_t) != 0)
    {
        return 0;
    }

    Count = MultibootInfoPtr->mmap_length / sizeof(memory_map_t);
    for (i = 0; i < Count; i++, MemoryMap++)
    {
        TRACE("i = %d, base_addr_low = 0x%p, MemoryMap->length_low = 0x%p\n", i, MemoryMap->base_addr_low, MemoryMap->length_low);

        /* Framebuffer address offset value is coming from the GPU within
         * memory mapped I/O address space, so we're comparing only low
         * 28 bits of the address within actual RAM address space */
        if (MemoryMap->base_addr_low == (Offset & 0x0FFFFFFF) && MemoryMap->base_addr_high == 0)
        {
            TRACE("Video memory found\n");
            return MemoryMap->length_low;
        }
    }
    ERR("Video memory not found!\n");
    return 0;
}

VOID
XboxVideoInit(VOID)
{
    ULONG BytesPerPixel;
    GENERIC_FRAMEBUFFER_CONTEXT FrameBuffer;

    RtlZeroMemory(&FrameBuffer, sizeof(FrameBuffer));

    /* Reuse framebuffer that was set up by firmware */
    FrameBuffer.BaseAddress = READ_REGISTER_ULONG(NvBase + NV2A_CRTC_FRAMEBUFFER_START);
    /* Verify that framebuffer address is page-aligned */
    ASSERT(FrameBuffer.BaseAddress % PAGE_SIZE == 0);

    /* Obtain framebuffer memory size from multiboot memory map */
    if ((FrameBuffer.BufferSize = XboxGetFramebufferSize(FrameBuffer.BaseAddress)) == 0)
    {
        /* Fallback to Cromwell standard which reserves high 4 MB of RAM */
        FrameBuffer.BufferSize = 4 * 1024 * 1024;
        WARN("Could not detect framebuffer memory size, fallback to 4 MB\n");
    }

    FrameBuffer.ScreenWidth = FrameBuffer.PixelsPerScanLine = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_HVALID_END) + 1;
    FrameBuffer.ScreenHeight = READ_REGISTER_ULONG(NvBase + NV2A_RAMDAC_FP_VVALID_END) + 1;
    /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
    BytesPerPixel = 8 * (((NvGetCrtc(0x19) & 0xE0) << 3) | (NvGetCrtc(0x13) & 0xFF)) / FrameBuffer.ScreenWidth;
    if (BytesPerPixel == 4)
    {
        ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel - 1);
        FrameBuffer.PixelFormat = GENFB_A8R8G8B8;
    }
    else if (BytesPerPixel == 3)
    {
        ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel);
        FrameBuffer.PixelFormat = GENFB_R8G8B8;
    }
    else
    {
        ASSERT((NvGetCrtc(0x28) & 0xF) == BytesPerPixel);
        FrameBuffer.PixelFormat = GENFB_R5G6B5;
    }

    /* Verify screen resolution */
    ASSERT(FrameBuffer.ScreenWidth > 1);
    ASSERT(FrameBuffer.ScreenHeight > 1);
    ASSERT(BytesPerPixel >= 2 && BytesPerPixel <= 4);
    /* Verify that screen fits framebuffer size */
    ASSERT(FrameBuffer.ScreenWidth * FrameBuffer.ScreenHeight * BytesPerPixel <= FrameBuffer.BufferSize);
    GenFbInitialize(&FrameBuffer);

    GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
}

VOID
XboxVideoPrepareForReactOS(VOID)
{
    GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
    GenFbVideoHideShowTextCursor(FALSE);
}

/* EOF */
