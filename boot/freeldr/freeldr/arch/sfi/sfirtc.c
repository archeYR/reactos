/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>
#include <arch/sfi/sfitable.h>

/* Dallas DS12C887 Real Time Clock */
#define RTC_ADDRESS_SECONDS           0
#define RTC_ADDRESS_MINUTES           2
#define RTC_ADDRESS_HOURS             4
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7
#define RTC_ADDRESS_MONTH             8
#define RTC_ADDRESS_YEAR              9

/* vRTC YEAR reg contains the offset to 1970 */
#define SfiMrtcYearMin 1970;

/* GLOBALS ********************************************************************/

extern PSFI_TABLE_SIMPLE SfiSystTable;
static ULONG SfiRtcBaseAddr = 0;
PSFI_TIMER_TABLE_ENTRY SfiTimer = NULL;

/* FUNCTIONS ******************************************************************/

TIMEINFO*
SfiGetTime(VOID)
{
	static TIMEINFO TimeInfo;

	if (SfiRtcBaseAddr == 0) return NULL;

	TimeInfo.Second = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_SECONDS * 4);
	TimeInfo.Minute = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_MINUTES * 4);
	TimeInfo.Hour = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_HOURS * 4);
	TimeInfo.Day = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_DAY_OF_THE_MONTH * 4);
	TimeInfo.Month = READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_MONTH * 4);
	TimeInfo.Year = SfiMrtcYearMin + READ_REGISTER_UCHAR(SfiRtcBaseAddr + RTC_ADDRESS_YEAR * 4);

	return &TimeInfo;
}

// Initialize RTC and timer

BOOLEAN
SfiInitTime(VOID)
{
	PSFI_TABLE_SIMPLE SfiTable;
	PSFI_RTC_TABLE_ENTRY RtcTable;

	SfiTable = SfiFindTable(SfiSystTable, SFI_SIG_MRTC);

	if (SfiTable == NULL)
    {
        printf("Critical error: Couldn't get MRTC table'\n");
        return FALSE;
    }

	if (SfiTableEntryCount(SfiTable, sizeof(*RtcTable)) < 1)
    {
        printf("Critical error: MRTC table is not valid\n");
        return FALSE;
    }

	RtcTable = (PSFI_RTC_TABLE_ENTRY)SfiTable->Entry;
	if (RtcTable->PhysicalAddress.HighPart != 0)
    {
        printf("Critical error: Invalid RTC table\n");
        return FALSE;
    }
	SfiRtcBaseAddr = RtcTable->PhysicalAddress.LowPart;

	SfiTable = SfiFindTable(SfiSystTable, SFI_SIG_MTMR);

	//SFI timer is not critical device
	if (SfiTable != NULL)
	{
		if (SfiTableEntryCount(SfiTable, sizeof(SFI_TIMER_TABLE_ENTRY)) > 0)
		{
			SfiTimer = (PSFI_TIMER_TABLE_ENTRY)SfiTable->Entry;
		}
	}
	else
    {
        return FALSE;
    }

	return TRUE;
}
