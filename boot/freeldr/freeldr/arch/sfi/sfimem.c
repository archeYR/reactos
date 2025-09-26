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
 *  Note: much of this code was based on knowledge and/or code developed
 *  by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>
#include <arch/sfi/sfitable.h>
#include <debug.h>
#include <genfb.h>

DBG_DEFAULT_CHANNEL(MEMORY);

extern PSFI_TABLE_SIMPLE SfiSystTable;
extern multiboot_info_t * MultibootInfoPtr;

extern VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType);

extern VOID
ReserveMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType,
    PCHAR Usage);

extern ULONG
PcMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

static
TYPE_OF_MEMORY
SfiConvertToFreeldrDesc(SFI_MEM_TYPE SfiMemoryType)
{
    switch (SfiMemoryType)
    {
        case SFI_MEM_RESERVED:
            return LoaderReserve;
        case SFI_LOADER_CODE:
            return LoaderLoadedProgram;
        case SFI_LOADER_DATA:
            return LoaderLoadedProgram;
        case SFI_BOOT_SERVICE_CODE:
            return LoaderFirmwareTemporary;
        case SFI_BOOT_SERVICE_DATA:
            return LoaderFirmwareTemporary;
        case SFI_RUNTIME_SERVICE_CODE:
            return LoaderFirmwarePermanent;
        case SFI_RUNTIME_SERVICE_DATA:
            return LoaderFirmwarePermanent;
        case SFI_MEM_CONV:
            return LoaderFree;
        case SFI_MEM_UNUSABLE:
            return LoaderBad;
        case SFI_ACPI_RECLAIM:
            return LoaderFirmwareTemporary;
        case SFI_ACPI_NVS:
            return LoaderReserve;
        case SFI_MEM_MMIO:
            return LoaderReserve;
        case SFI_MEM_IOPORT:
            return LoaderReserve;
        case SFI_PAL_CODE:
            return LoaderReserve;
        default:
            break;
    }
    return LoaderReserve;
}

BOOLEAN
SfiGetFirmwareMemoryMap(PFREELDR_MEMORY_DESCRIPTOR MemMap)
{
    PSFI_TABLE_SIMPLE SfiTable;
    PSFI_MEM_ENTRY MemEntry;
    ULONG EntryNum, i;
    SfiTable = SfiFindTable(SfiSystTable, SFI_SIG_MMAP);

    if (SfiTable == NULL)
    {
        ERR("Couldn't get SFI MMAP table\n");
        return FALSE;
    }

    if ((EntryNum = SfiTableEntryCount(SfiTable, sizeof(SFI_MEM_ENTRY))) < 1)
    {
        ERR("SFI MMAP table is not valid\n");
        return FALSE;
    }

    MemEntry = (PSFI_MEM_ENTRY)SfiTable->Entry;

    for (i = 0; i < EntryNum; i++, MemEntry++)
    {
        TRACE("i = %d, PhysicalMemStart = 0x%p, Size = 0x%llx", i, MemEntry->PhysicalMemStart, (MemEntry->Pages << PAGE_SHIFT));

        SetMemory(MemMap,
                MemEntry->PhysicalMemStart.QuadPart,
                MemEntry->Pages << PAGE_SHIFT,
                SfiConvertToFreeldrDesc(MemEntry->Type));
    }

    return TRUE;

}

memory_map_t *
SfiGetMultibootMemoryMap(INT * Count)
{
    memory_map_t * MemoryMap;

    if (!MultibootInfoPtr)
    {
        ERR("Multiboot info structure not found!\n");
        return NULL;
    }

    if (!(MultibootInfoPtr->flags & MB_INFO_FLAG_MEMORY_MAP))
    {
        ERR("Multiboot memory map is not passed!\n");
        return NULL;
    }

    MemoryMap = (memory_map_t *)MultibootInfoPtr->mmap_addr;

    if (!MemoryMap ||
        MultibootInfoPtr->mmap_length == 0 ||
        MultibootInfoPtr->mmap_length % sizeof(memory_map_t) != 0)
    {
        ERR("Multiboot memory map structure is malformed!\n");
        return NULL;
    }

    *Count = MultibootInfoPtr->mmap_length / sizeof(memory_map_t);
    return MemoryMap;
}

TYPE_OF_MEMORY
SfiMultibootMemoryType(ULONG Type)
{
    switch (Type)
    {
        case 0: // Video RAM
            return LoaderFirmwarePermanent;
        case 1: // Available RAM
            return LoaderFree;
        case 3: // ACPI area
            return LoaderFirmwareTemporary;
        case 4: // Hibernation area
            return LoaderSpecialMemory;
        case 5: // Reserved or invalid memory
            return LoaderSpecialMemory;
        default:
            return LoaderFirmwarePermanent;
    }
}

FREELDR_MEMORY_DESCRIPTOR SfiMemoryMap[128];

PFREELDR_MEMORY_DESCRIPTOR
SfiMemGetMemoryMap(ULONG *MemoryMapSize)
{
    GENERIC_FRAMEBUFFER_CONTEXT FramebufferData;
    memory_map_t * MbMap;
    INT Count, i;
    multiboot_module_t *RamDiskInfo;

    TRACE("SfiMemGetMemoryMap()\n");

    /* First try SFI MMAP table */
    if (SfiGetFirmwareMemoryMap(SfiMemoryMap))
        goto finalize;

    ERR("Could not get memory map from SFI. Falling back to Multiboot map!\n");

    MbMap = SfiGetMultibootMemoryMap(&Count);
    if (MbMap)
    {
        /* Obtain memory map via multiboot spec */

        for (i = 0; i < Count; i++, MbMap++)
        {
            TRACE("i = %d, base_addr_low = 0x%p, length_low = 0x%p\n", i, MbMap->base_addr_low, MbMap->length_low);

            if (MbMap->base_addr_high > 0 || MbMap->length_high > 0)
            {
                ERR("Memory descriptor base or size is greater than 4 GB, should not happen on Xbox!\n");
                //TRACE("i = %d, base_addr_high = 0x%p, length_high = 0x%p\n", i, MbMap->base_addr_high, MbMap->length_high);
                ASSERT(FALSE);
            }

            SetMemory(SfiMemoryMap,
                      MbMap->base_addr_low,
                      MbMap->length_low,
                      SfiMultibootMemoryType(MbMap->type));
        }
    }

finalize:
    RamDiskInfo = (multiboot_module_t *)(MultibootInfoPtr->mods_addr + sizeof(multiboot_module_t));
    TRACE("Reserving memory for RAM disk: Base = 0x%x, Size = 0x%x\n", RamDiskInfo->mod_start, (RamDiskInfo->mod_end - RamDiskInfo->mod_start));
    /* Initial RAM disk */
    ReserveMemory(SfiMemoryMap,
                RamDiskInfo->mod_start,
                (RamDiskInfo->mod_end - RamDiskInfo->mod_start),
                LoaderFirmwareTemporary,
                "Initial RAM disk");
    *MemoryMapSize = PcMemFinalizeMemoryMap(SfiMemoryMap);
    return SfiMemoryMap;
}

/* EOF */
