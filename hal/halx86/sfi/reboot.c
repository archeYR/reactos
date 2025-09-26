/*
 * PROJECT:         Simple Firmware Interface HAL
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         SFI systems reboot functions
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* PRIVATE FUNCTIONS *********************************************************/

static
VOID
HalpTripleFault()
{
	asm volatile("null_idt:;"
				 ".word 0;"
				 ".word 0;"
				 "lidt null_idt;"
				 "int3;");
}

/* PUBLIC FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
/*
 * @implemented
 */
VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    /* Check what kind of action this is */
    switch (Action)
    {
        /* All recognized actions */
        case HalHaltRoutine:
        case HalPowerDownRoutine:
        {
            /* The only way to shut down is through IPC */
            DbgPrint("Shutdown is not implemented yet!\n");
            DbgBreakPoint();
        }
        case HalRestartRoutine:
        case HalRebootRoutine:
        {
            /* Reboot can be achieved either through IPC or CPU triple fault */
            HalpTripleFault();
        }

        /* Anything else */
        default:
        {
            /* Print message and break */
            DbgPrint("HalReturnToFirmware(%d) called!\n", Action);
            DbgBreakPoint();
        }
    }
}
#endif // _MINIHAL_

/* EOF */
