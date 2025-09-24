/*
 * PROJECT:     FreeLoader SFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Console output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>

#include <genfb.h>

/* GLOBALS ********************************************************************/

static unsigned CurrentCursorX = 0;
static unsigned CurrentCursorY = 0;
static unsigned CurrentAttr = 0x0f;

/* FUNCTIONS ******************************************************************/

VOID
SfiConsPutChar(int c)
{
    ULONG Width, Height, Unused;
    BOOLEAN NeedScroll;

    GenFbVideoGetDisplaySize(&Width, &Height, &Unused);

    NeedScroll = (CurrentCursorY >= Height);
    if (NeedScroll)
    {
        GenFbVideoScrollUp();
        --CurrentCursorY;
    }
    if (c == '\r')
    {
        CurrentCursorX = 0;
    }
    else if (c == '\n')
    {
        CurrentCursorX = 0;

        if (!NeedScroll)
            ++CurrentCursorY;
    }
    else if (c == '\t')
    {
        CurrentCursorX = (CurrentCursorX + 8) & ~7;
    }
    else
    {
        GenFbVideoPutChar(c, CurrentAttr, CurrentCursorX, CurrentCursorY);
        CurrentCursorX++;
    }
    if (CurrentCursorX >= Width)
    {
        CurrentCursorX = 0;
        CurrentCursorY++;
    }
}

//Keyboard input functions will be set during hardware detections by its drivers
BOOLEAN
SfiConsKbHit(VOID)
{
#if 0
	if (SfiKbHitFunc == NULL) return FALSE;
	return SfiKbHitFunc();
#endif
    return FALSE;
}

int
SfiConsGetCh(VOID)
{
#if 0
	if (SfiConsGetChFunc == NULL) return 0;
	return SfiConsGetChFunc();
#endif
    return FALSE;
}
