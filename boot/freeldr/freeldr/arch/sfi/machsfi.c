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
#include <multiboot.h>

#include <arch/sfi/sfitable.h>

#include <genfb.h>

#include <debug.h>

DBG_DEFAULT_CHANNEL(HWDETECT);

PSFI_TABLE_SIMPLE SfiSystTable = NULL;

static BOOLEAN SfiHwInited = FALSE;

extern multiboot_info_t * MultibootInfoPtr;

extern FREELDR_MEMORY_DESCRIPTOR PcMemoryMap[MAX_BIOS_DESCRIPTORS + 1];
extern ULONG PcMapCount;

extern BOOLEAN SfiCalibrateStallExecution(VOID);

extern VOID
SetMemory(
	PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
	ULONG_PTR BaseAddress,
	SIZE_T Size,
	TYPE_OF_MEMORY MemoryType);

extern ULONG
PcMemFinalizeMemoryMap(
	PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

VOID WaitForAnyKey()
{
	printf("Press any key to continue...\n");
	while (!MachConsKbHit());
	MachConsGetCh();
}

#if 0
MCFG_STRUCTURE * SfiGetMcfg (VOID)
{
	XSDT_TABLE * XsdtTable;
	EFI_ACPI_DESCRIPTION_HEADER * McfgTable;

	XsdtTable = (XSDT_TABLE *)SfiFindTable(SfiSystTable, SFI_SIG_XSDT);

	if (XsdtTable == NULL) return NULL;
	if (XsdtTable->Header.Length < sizeof (XSDT_TABLE)) return NULL;

	McfgTable = AcpiFindTableInXSDT(XsdtTable, SFI_SIG_XSDT_MCFG);

	if (McfgTable == NULL) return NULL;
	if (McfgTable->Length < (sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(MCFG_STRUCTURE))) return NULL;

	return (MCFG_STRUCTURE *)((UINT8 *)McfgTable + sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT64));
}
#endif

BOOLEAN SfiInitHardware(VOID)
{
#if 0
	MCFG_STRUCTURE * Mcfg;

	if (SfiHwInited) return FALSE;

	if (SfiMcfgPciDriverInterface.RegisterInterface(&SdMmcHciDriverInterface)) return FALSE;
	if (SfiMcfgPciDriverInterface.RegisterInterface(&MrstKeypadDriverInterface)) return FALSE;
	if (SdMmcHciDriverInterface.RegisterInterface(&SdDriverInterface)) return FALSE;
	if (SdMmcHciDriverInterface.RegisterInterface(&EmmcDriverInterface)) return FALSE;
	if (SdDriverInterface.RegisterInterface(&BlockIoDriverInterface)) return FALSE;
	if (EmmcDriverInterface.RegisterInterface(&BlockIoDriverInterface)) return FALSE;

	Mcfg = SfiGetMcfg();

	if (Mcfg == NULL) {
		printf("Unable to locate MCFG table\n");
		return FALSE;
	}

	SfiMcfgData = *Mcfg;

	if (SfiMcfgPciDriverInterface.Start(&SfiMcfgData, &SfiMcfgPciContext)) return FALSE;
#endif
	SfiHwInited = TRUE;
	return TRUE;
}

BOOLEAN SfiFreeHardware(VOID)
{
#if 0
	if (!SfiHwInited) return FALSE;

	if (SfiMcfgPciDriverInterface.Stop(&SfiMcfgData, SfiMcfgPciContext)) return FALSE;

	if (EmmcDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SdDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SdMmcHciDriverInterface.UnregisterInterfaces()) return FALSE;
	if (SfiMcfgPciDriverInterface.UnregisterInterfaces()) return FALSE;
#endif
	SfiHwInited = FALSE;
	return TRUE;
}

VOID
SfiGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
	/* Unused for SFI systems */
	*ExtendedBIOSDataArea = 0;
	*ExtendedBIOSDataSize = 0;
}

PCONFIGURATION_COMPONENT_DATA
SfiHwDetect(
    _In_opt_ PCSTR Options)
{
	PCONFIGURATION_COMPONENT_DATA SystemKey;

	TRACE("DetectHardware()\n");

	/* Create the 'System' key */
	FldrCreateSystemKey(&SystemKey, "Intel SFI compatible");

	//nothing to do here for SFI systems

	TRACE("DetectHardware() Done\n");
	return SystemKey;
}

VOID
SfiHwIdle(VOID)
{
	/* UNIMPLEMENTED */
}

VOID
FrLdrCheckCpuCompatibility(VOID)
{
	/* SFI system cannot have incompatible CPU */
}

VOID SfiBeep(VOID)
{
	/* Unused for SFI systems */
}

/******************************************************************************/

BOOLEAN
SfiMachDetect(const char *CmdLine)
{
	SfiSystTable = SfiFindSyst();
	return (SfiSystTable != NULL);
}

VOID
MachInit(const char *CmdLine)
{

	/* SfiMachDetect must be called to obtain SYST table */
	if (!SfiMachDetect(CmdLine)) return;

	/* Setup vtbl */
	MachVtbl.ConsPutChar = SfiConsPutChar;
	MachVtbl.ConsKbHit = SfiConsKbHit;
	MachVtbl.ConsGetCh = SfiConsGetCh;
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
	MachVtbl.Beep = SfiBeep;
	MachVtbl.PrepareForReactOS = SfiPrepareForReactOS;
	MachVtbl.GetMemoryMap = SfiMemGetMemoryMap;
	MachVtbl.GetExtendedBIOSData = SfiGetExtendedBIOSData;
	MachVtbl.GetFloppyCount = SfiGetFloppyCount;
	MachVtbl.DiskReadLogicalSectors = SfiDiskReadLogicalSectors;
	MachVtbl.DiskGetDriveGeometry = SfiDiskGetDriveGeometry;
	MachVtbl.DiskGetCacheableBlockCount = SfiDiskGetCacheableBlockCount;
	MachVtbl.GetTime = SfiGetTime;
	MachVtbl.InitializeBootDevices = SfiInitializeBootDevices;
	MachVtbl.HwDetect = SfiHwDetect;
	MachVtbl.HwIdle = SfiHwIdle;

    SfiVideoInit();

	if (!SfiInitTime())
	{
		printf("Critical error: SfiInitTime failed\n");
		WaitForAnyKey();
		return;
	}

	if (!SfiCalibrateStallExecution())
	{
		printf("CalibrateStallExecution failed - leaving default DelayCount value\n");
		WaitForAnyKey();
		return;
	}
}

VOID
SfiPrepareForReactOS(VOID)
{
	/* On SFI, prepare video */
	if (!SfiFreeHardware())
	{
		UiMessageBoxCritical("Error: SfiFreeHardware failed\n");
	}

	SfiVideoPrepareForReactOS();
}

/* EOF */
