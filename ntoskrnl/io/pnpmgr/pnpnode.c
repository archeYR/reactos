/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device node handle code
 * COPYRIGHT:   2018 Vadim Galyant
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

extern ERESOURCE PiEngineLock;
extern ERESOURCE PiDeviceTreeLock;


VOID
NTAPI
PpDevNodeLockTree(_In_ ULONG LockLevel)
{
    ULONG SharedCount;
    ULONG ix;

    PAGED_CODE();
    DPRINT("PpDevNodeLockTree: LockLevel - %X\n", LockLevel);

    KeEnterCriticalRegion();

    switch (LockLevel)
    {
        case 0:
        {
            ExAcquireSharedWaitForExclusive(&PiDeviceTreeLock, TRUE);
            break;
        }
        case 1:
        {
            ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
            ExAcquireSharedWaitForExclusive(&PiDeviceTreeLock, TRUE);
            break;
        }
        case 2:
        {
            ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
            ExAcquireResourceExclusiveLite(&PiDeviceTreeLock, TRUE);
            break;
        }
        case 3:
        {
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            ASSERT(ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock) &&
                  (!ExIsResourceAcquiredExclusiveLite(&PiDeviceTreeLock)));

            SharedCount = ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock);

            for (ix = 0; ix < SharedCount; ix++)
            {
                ExReleaseResourceLite(&PiDeviceTreeLock);
            }

            for (ix = 0; ix < SharedCount; ix++)
            {
                ExAcquireResourceExclusiveLite(&PiDeviceTreeLock, TRUE);
            }
            break;
        }
        default:
        {
            ASSERT(LockLevel <= 3);
            break;
        }
    }
}

VOID
NTAPI
PpDevNodeUnlockTree(_In_ ULONG LockLevel)
{
    PAGED_CODE();
    DPRINT("PpDevNodeUnlockTree: LockLevel - %X\n", LockLevel);

    if (LockLevel == 0)
    {
        ExReleaseResourceLite(&PiDeviceTreeLock);
    }
    else if (LockLevel == 1 || LockLevel == 2)
    {
        ExReleaseResourceLite(&PiDeviceTreeLock);
        ExReleaseResourceLite(&PiEngineLock);
    }
    else if (LockLevel == 3)
    {
        ASSERT(ExIsResourceAcquiredExclusiveLite(&PiDeviceTreeLock));
        ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
        ExConvertExclusiveToSharedLite(&PiDeviceTreeLock);
    }
    else
    {
        ASSERT(LockLevel <= 3);
    }

    KeLeaveCriticalRegion();
    DPRINT("PpDevNodeUnlockTree: UnLocked\n");
}
