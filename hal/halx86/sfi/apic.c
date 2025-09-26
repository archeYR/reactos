/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source File for MPS specific functions on SFI systems
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HALP_APIC_INFO_TABLE HalpApicInfoTable;
static PROCESSOR_IDENTITY HalpStaticProcessorIdentity[MAXIMUM_PROCESSORS];
const PPROCESSOR_IDENTITY HalpProcessorIdentity = HalpStaticProcessorIdentity;
PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;
PVOID HalpLowStub;

/* FUNCTIONS ******************************************************************/

VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNREFERENCED_PARAMETER(LoaderBlock);
    UNIMPLEMENTED;
}

VOID
HalpPrintApicTables(VOID)
{
    UNIMPLEMENTED;
}
