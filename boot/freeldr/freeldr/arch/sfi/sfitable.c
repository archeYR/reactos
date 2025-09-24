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

#include <arch/sfi/sfitable.h>

BOOLEAN SfiCheckTableSum(PSFI_TABLE_SIMPLE Table)
{
	PUCHAR TableBuffer;
	ULONG i;
	UCHAR Sum;

	TableBuffer = (PUCHAR)Table;
	Sum = 0;
	for (i = 0; i < Table->Header.Length; i++) {
		Sum += TableBuffer[i];
	}
	return (Sum == 0);
}

PSFI_TABLE_SIMPLE SfiFindSyst(VOID)
{
    ULONG PhysicalAddress;
    PSFI_TABLE_SIMPLE SfiTable;

	/* SFI spec defines the SYST starts at a 16-byte boundary */
	for (PhysicalAddress = SFI_SYST_SEARCH_BEGIN; PhysicalAddress < SFI_SYST_SEARCH_END; PhysicalAddress += 16) {
		SfiTable = (PSFI_TABLE_SIMPLE)PhysicalAddress;
		if (*((PULONG)SfiTable) == SFI_SIG_SYST) {
			if (!SfiCheckTableSum(SfiTable)) {
				return NULL;
			}
			return SfiTable;
		}
	}
	return NULL;
}

PSFI_TABLE_SIMPLE SfiFindTable(PSFI_TABLE_SIMPLE SystTable, ULONG Signature)
{
	LONG i, TableCount;
	PHYSICAL_ADDRESS *PhysicalEntry;
	PSFI_TABLE_SIMPLE SfiTable;

	TableCount = (SystTable->Header.Length - sizeof(SFI_TABLE_HEADER)) / sizeof(ULONG64);
	PhysicalEntry = SystTable->Entry;

	/* walk through the syst to search the table */
	for (i = 0; i < TableCount; i++) {
		SfiTable = (PSFI_TABLE_SIMPLE)PhysicalEntry->LowPart;
		if (SfiTable == NULL) {
			return NULL;
		}
		if (*((PULONG)SfiTable) == Signature) {
			if (!SfiCheckTableSum(SfiTable)) {
				return NULL;
			}
			return SfiTable;
		}
		PhysicalEntry++;
	}
	return NULL;
}

ULONG SfiTableEntryCount(PSFI_TABLE_SIMPLE Table, ULONG EntrySize)
{
	if (Table->Header.Length < sizeof(SFI_TABLE_HEADER))
        return 0;
	return (Table->Header.Length - sizeof(SFI_TABLE_HEADER)) / EntrySize;
}
#if 0
EFI_ACPI_DESCRIPTION_HEADER * AcpiFindTableInXSDT (XSDT_TABLE *Xsdt, UINT32 Signature)
{
	UINT32 Index;
	UINT32 EntryCount;
	UINT64 * XsdtEntrys;
	EFI_ACPI_DESCRIPTION_HEADER *Table;

	EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);

	XsdtEntrys = (UINT64 *)(&(Xsdt->Entry));
	for (Index = 0; Index < EntryCount; Index ++) {
		Table = (EFI_ACPI_DESCRIPTION_HEADER*)(unsigned int)(XsdtEntrys[Index]);
		if (Table->Signature == Signature) {
			return Table;
			break;
		}
	}

	return NULL;
}
#endif
