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

#include <ntdef.h>

/* Table signatures reserved by the SFI specification */
#define SFI_SIG_SYST		'TSYS'
#define SFI_SIG_FREQ		'QERF'
#define SFI_SIG_IDLE		'ELDI'
#define SFI_SIG_CPUS		'SUPC'
#define SFI_SIG_MTMR		'RMTM'
#define SFI_SIG_MRTC		'CTRM'
#define SFI_SIG_MMAP		'PAMM'
#define SFI_SIG_APIC		'CIPA'
#define SFI_SIG_XSDT		'TDSX'
#define SFI_SIG_WAKE		'EKAW'
#define SFI_SIG_SPIB		'BIPS'
#define SFI_SIG_I2CB		'BC2I'
#define SFI_SIG_GPEM		'MEPG'

#define SFI_SIG_XSDT_MCFG	'GFCM'

#define SFI_ACPI_TABLE		(1 << 0)
#define SFI_NORMAL_TABLE	(1 << 1)

#define SFI_SIGNATURE_SIZE	4
#define SFI_OEM_ID_SIZE		6
#define SFI_OEM_TABLE_ID_SIZE	8

#define SFI_SYST_SEARCH_BEGIN		0x000E0000
#define SFI_SYST_SEARCH_END		0x000FFFFF

#define SFI_GET_NUM_ENTRIES(ptable, entry_type) \
	((ptable->header.length - sizeof(struct sfi_table_header)) / \
	(sizeof(entry_type)))


/*
 * Table structures must be byte-packed to match the SFI specification,
 * as they are provided by the BIOS.
 */
#include <pshpack1.h>
typedef struct _SFI_TABLE_HEADER {
    TCHAR Signature[SFI_SIGNATURE_SIZE];
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    TCHAR OemId[SFI_OEM_ID_SIZE];
    TCHAR OemTableId[SFI_OEM_TABLE_ID_SIZE];
} SFI_TABLE_HEADER, *PSFI_TABLE_HEADER;

typedef struct _SFI_TABLE_SIMPLE {
    SFI_TABLE_HEADER Header;
    PHYSICAL_ADDRESS Entry[1];
} SFI_TABLE_SIMPLE, *PSFI_TABLE_SIMPLE;

typedef struct _SFI_MEM_ENTRY {
    ULONG Type;
    PHYSICAL_ADDRESS PhysicalMemStart;
    ULONG64	VirtualMemStart;
    ULONG64	Pages;
    ULONG64	Attributes;
} SFI_MEM_ENTRY, *PSFI_MEM_ENTRY;

typedef struct _SFI_CPU_TABLE_ENTRY {
    ULONG ApicId;
} SFI_CPU_TABLE_ENTRY, *PSFI_CPU_TABLE_ENTRY;

typedef struct _SFI_CSTATE_TABLE_ENTRY {
    ULONG Hint;
    ULONG Latency;
} SFI_CSTATE_TABLE_ENTRY, *PSFI_CSTATE_TABLE_ENTRY;

typedef struct _SFI_APIC_TABLE_ENTRY {
    PHYSICAL_ADDRESS PhysicalAddress;
} SFI_APIC_TABLE_ENTRY, *PSFI_APIC_TABLE_ENTRY;

typedef struct _SFI_FREQ_TABLE_ENTRY {
    ULONG Frequency;
    ULONG Latency;
    ULONG ControlValue;
} SFI_FREQ_TABLE_ENTRY, *PSFI_FREQ_TABLE_ENTRY;

typedef struct _SFI_WAKE_TABLE_ENTRY {
    PHYSICAL_ADDRESS PhysicalAddress;
} SFI_WAKE_TABLE_ENTRY, *PSFI_WAKE_TABLE_ENTRY;

typedef struct _SFI_TIMER_TABLE_ENTRY {
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Frequency;
    ULONG Irq;
} SFI_TIMER_TABLE_ENTRY, *PSFI_TIMER_TABLE_ENTRY;

typedef struct _SFI_RTC_TABLE_ENTRY {
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Irq;
} SFI_RTC_TABLE_ENTRY, *PSFI_RTC_TABLE_ENTRY;

typedef struct _SFI_SPI_TABLE_ENTRY {
    USHORT HostNumber;
    USHORT ChipSelect;
    USHORT IrqInfo;
    TCHAR DeviceName[16];
    UCHAR DeviceInfo[10];
} SFI_SPI_TABLE_ENTRY, *PSFI_SPI_TABLE_ENTRY;

typedef struct _SFI_I2C_TABLE_ENTRY {
    USHORT HostNumber;
    USHORT Address;
    USHORT IrqInfo;
    TCHAR DeviceName[16];
    UCHAR DeviceInfo[10];
} SFI_I2C_TABLE_ENTRY, *PSFI_I2C_TABLE_ENTRY;

typedef struct _SFI_GPE_TABLE_ENTRY {
    USHORT LogicalId;
    USHORT PhysicalId;
} SFI_GPE_TABLE_ENTRY, *PSFI_GPE_TABLE_ENTRY;
#include <poppack.h>
