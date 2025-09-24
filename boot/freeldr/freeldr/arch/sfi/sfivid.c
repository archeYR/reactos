/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>

#include <genfb.h>

//#include <debug.h>
//DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

UCHAR MachDefaultTextColor = COLOR_GRAY;

/* FUNCTIONS ******************************************************************/

VOID
SfiVideoInit(VOID)
{
#if 0 // We may try to read framebuffer info from cmdline or maybe even graphics hardware at some point
	multiboot_module_t * MbModule;
	CHAR * ModCmdLine;
	ULONG i;
	ULONG ModRelocAddr, CmdArgOffs;

	/* SFI device may not have VGA-compatible BIOS, but this BIOS may be available
	 * on another devices with the same graphics controller.
	 * User can pass VGA BIOS image as multiboot module, tagged with "VgaBios" or "VgaBios=xxx"
	 * module command line string, where xxx is base address (hex).
	 */
	if (MultibootInfoPtr != NULL)
	{
		MbModule = (multiboot_module_t *)MultibootInfoPtr->mods_addr;

		if ((MultibootInfoPtr->flags & MULTIBOOT_INFO_MODS) && (MbModule != 0))
		{
			for (i = 0; i < MultibootInfoPtr->mods_count; i++)
			{
				ModCmdLine = (CHAR *)MbModule[i].cmdline;
				if (ModCmdLine != NULL)
				{
					CmdArgOffs = strlen(MultibootModuleVgaBiosCmd);
				}
			}
		}
	}
#endif
    /* HACK: Hardcoded for P89 mini for now */
    GENERIC_FRAMEBUFFER_CONTEXT framebufferData;
    RtlZeroMemory(&framebufferData, sizeof(framebufferData));

    framebufferData.BaseAddress        = 0x3f000000;
    framebufferData.BufferSize         = 0x300000;
    framebufferData.ScreenWidth        = 768;
    framebufferData.ScreenHeight       = 1024;
    framebufferData.PixelsPerScanLine  = 768;
    framebufferData.PixelFormat = GENFB_A8R8G8B8;

    GenFbInitialize(&framebufferData);
    GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
}

VOID
SfiVideoPrepareForReactOS(VOID)
{
    GenFbVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
    GenFbVideoHideShowTextCursor(FALSE);
}
