
#include "mountvol.h"
#include <sys/types.h>

static
VOID
ConFormatMessage(PCON_STREAM Stream, DWORD MessageId, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, MessageId);
    ConMsgPrintfV(Stream,
                  FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  MessageId,
                  LANG_USER_DEFAULT,
                  &arg_ptr);
    va_end(arg_ptr);
}

static
BOOL
QueryAutoMount(PMOUNTMGR_QUERY_AUTO_MOUNT CurrentState)
{
    BOOL Ret;
    HANDLE MountMgrHandle;
    DWORD BytesReturned;

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                                 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Get current auto mount state */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT,
                          NULL, 0,
                          CurrentState, sizeof(*CurrentState), &BytesReturned,
                          NULL);

    CloseHandle(MountMgrHandle);
    return Ret;
}

static
BOOL
IsVolumeOffline(LPCWSTR VolumeName)
{
    BOOL Ret;
    HANDLE MountMgrHandle;
    DWORD BytesReturned;

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(VolumeName,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Get the volume status */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_VOLUME_IS_OFFLINE,
                          NULL, 0,
                          NULL, 0, &BytesReturned,
                          NULL);

    CloseHandle(MountMgrHandle);
    return Ret;
}

static
BOOL
SetAutoMount(MOUNTMGR_AUTO_MOUNT_STATE NewState)
{
    BOOL Ret;
    HANDLE MountMgrHandle;
    DWORD BytesReturned;
    MOUNTMGR_SET_AUTO_MOUNT SetState;

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    SetState.NewState = NewState;

    /* Set auto mount state */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_SET_AUTO_MOUNT,
                          &SetState, sizeof(SetState),
                          NULL, 0, &BytesReturned,
                          NULL);

    CloseHandle(MountMgrHandle);
    return Ret;
}

static
BOOL
GetESPDevice(PSYSTEM_SYSTEM_PARTITION_INFORMATION SystemPartitionInformation, PULONG BufferSize)
{
    /* NOTE: This is done differently (probably by querying registry) on NT 5.x IA-64
     * The method below uses a system information class introduced in Vista */
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemSystemPartitionInformation,
                                      SystemPartitionInformation,
                                      *BufferSize,
                                      BufferSize);
    if (!NT_SUCCESS(Status))
    {
        ConPrintf(StdOut, L"sizeof %d, ReturnLength %d\n", sizeof(*SystemPartitionInformation), *BufferSize);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

static
BOOL
GetESPMountPoint(LPWSTR ESPMountPoint)
{
    PSYSTEM_SYSTEM_PARTITION_INFORMATION SystemPartitionInformation;
    PWSTR TargetPath;
    ULONG SystemInformationLength = sizeof(*SystemPartitionInformation);
    WCHAR MountPoint[4] = {'A', ':', UNICODE_NULL};
    ULONG TargetPathLength = 100;
    DWORD Drives;

    /* First call is to get required buffer size */
    SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(),
                                                 0,
                                                 SystemInformationLength);
    if (!SystemPartitionInformation)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    GetESPDevice(SystemPartitionInformation, &SystemInformationLength);
    RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
    SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(),
                                                 0,
                                                 SystemInformationLength);
    if (!SystemPartitionInformation)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate initial buffer for target path, 50 characters should suffice */
    TargetPath = RtlAllocateHeap(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 TargetPathLength);
    if (!TargetPath)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Get the path of device that is used for system partition */
    if (!GetESPDevice(SystemPartitionInformation, &SystemInformationLength))
    {
        RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
        RtlFreeHeap(GetProcessHeap(), 0, TargetPath);
        return FALSE;
    }

    /* Loop through all drive letters and compare corresponding device paths of
     * occupied letters to the system partition device path */
    Drives = GetLogicalDrives();
    while (Drives != 0)
    {
        if (!(Drives & 1))
        {
            Drives >>= 1;
            MountPoint[0]++;
            continue;
        }

        ConPrintf(StdOut, L"Mountpoint %s TargetPathLength %d\n", MountPoint, TargetPathLength);
        while (!QueryDosDeviceW(MountPoint, TargetPath, TargetPathLength/sizeof(WCHAR)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            ConPrintf(StdOut, L"insufficient buffer\n");
            /* Increase the buffer size */
            RtlFreeHeap(GetProcessHeap(), 0, TargetPath);
            TargetPathLength += TargetPathLength;
            TargetPath = RtlAllocateHeap(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        TargetPathLength);
            if (!TargetPath)
            {
                RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
                ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        }

        if (!wcscmp(SystemPartitionInformation->SystemPartition.Buffer, TargetPath))
            break;

        Drives >>= 1;
        MountPoint[0]++;
    }

    RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
    RtlFreeHeap(GetProcessHeap(), 0, TargetPath);
    if (Drives == 0)
    {
        return FALSE;
    }

    if (ESPMountPoint)
        wcscpy(ESPMountPoint, MountPoint);

    return TRUE;
}

static
BOOL
MountESPVolume(LPCWSTR MountPoint)
{
    PSYSTEM_SYSTEM_PARTITION_INFORMATION SystemPartitionInformation;
    ULONG SystemInformationLength = sizeof(*SystemPartitionInformation);
    WCHAR VolumeName[50];

    /* Ensure that the mount point is not already occupied and that ESP is not already mounted */
    if (GetVolumeNameForVolumeMountPointW(MountPoint, VolumeName, ARRAYSIZE(VolumeName)) ||
        GetESPMountPoint(NULL))
    {
        ConFormatMessage(StdOut, ERROR_DIR_NOT_EMPTY);
        return FALSE;
    }

    /* First call is to get required buffer size */
    SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(),
                                                 0,
                                                 SystemInformationLength);
    if (!SystemPartitionInformation)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    GetESPDevice(SystemPartitionInformation, &SystemInformationLength);
    RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
    SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(),
                                                 0,
                                                 SystemInformationLength);
    if (!SystemPartitionInformation)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* We will try to get the device that is used for ESP, and then map it at desired mount point */
    if (!GetESPDevice(SystemPartitionInformation, &SystemInformationLength) ||
        !DefineDosDeviceW(DDD_RAW_TARGET_PATH,
                          MountPoint,
                          SystemPartitionInformation->SystemPartition.Buffer))
    {
        RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
    return TRUE;
}

static
BOOL
PrintESPMountPoint()
{
    WCHAR MountPoint[4];

    /* Get the ESP mount point */
    if (!GetESPMountPoint(MountPoint))
        return FALSE;

    /* Append backlash for display on console output */
    wcscat(MountPoint, L"\\");
    ConResPrintf(StdOut, STRING_MOUNTVOL_ESPMOUNTPOINT, MountPoint);
    return TRUE;
}

static
BOOL
IsEFI()
{
    SYSTEM_BOOT_ENVIRONMENT_INFORMATION SystemBootInfo = {0};
    ULONG SystemInformationLength = sizeof(SystemBootInfo);
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBootEnvironmentInformation,
                                      &SystemBootInfo,
                                      SystemInformationLength,
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Assume non-(U)EFI system */
        ConPrintf(StdOut, L"NTSTATUS %d\n", Status);
        return FALSE;
    }
    ConPrintf(StdOut, L"Firmware Type %d\n", SystemBootInfo.FirmwareType);
    return (SystemBootInfo.FirmwareType == FirmwareTypeUefi) ? TRUE : FALSE;
}

static
BOOL
PrintVolumeList()
{
    BOOL Ret;
    WCHAR VolumeName[50];
    LPWCH VolumePathNames = NULL;
    HANDLE Volume;
    DWORD ReturnLength = MAX_PATH, PathOffset;
    MOUNTMGR_QUERY_AUTO_MOUNT AutoMountState = {0};

    /* Loop through all volumes */
    Volume = FindFirstVolumeW((LPWSTR)VolumeName, ARRAYSIZE(VolumeName));

    if (Volume == INVALID_HANDLE_VALUE)
        goto Fail;
    
    VolumePathNames = RtlAllocateHeap(GetProcessHeap(),
                                      0,
                                      ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    if (!VolumePathNames)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    do
    {
        /* Get volume mount points */
        Ret = GetVolumePathNamesForVolumeNameW(VolumeName,
                                               VolumePathNames,
                                               ReturnLength + sizeof(UNICODE_NULL),
                                               &ReturnLength);
        
        if (GetLastError() == ERROR_MORE_DATA)
        {
            /* We need more heap */
            RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
            VolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, ReturnLength * sizeof(WCHAR));
            if (!VolumePathNames)
            {
                ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            Ret = GetVolumePathNamesForVolumeNameW(VolumeName,
                                                   VolumePathNames,
                                                   ReturnLength,
                                                   &ReturnLength);
        }

        /* Print volume name */
        ConPrintf(StdOut, L"%*s%s\n", 4, "", VolumeName);

        if (!Ret)
        {
            ConFormatMessage(StdOut, GetLastError());
            continue;
        }

        PathOffset = 0;

        if (ReturnLength > 1)
        {
            /* Print all paths found in multiline string */
            while (PathOffset < ReturnLength - 1)
            {
                ConPrintf(StdOut, L"%*s%s\n", 8, "", VolumePathNames + PathOffset);
                PathOffset += wcslen(VolumePathNames + PathOffset) + 1;
            }
            ConPuts(StdOut, L"\n");
        }
        else
        {
            /* No mount points, determine if the volume is mountable */
            VolumeName[wcslen(VolumeName) - 1] = UNICODE_NULL;
            IsVolumeOffline(VolumeName) ? ConResPuts(StdOut, STRING_MOUNTVOL_NOTMOUNTABLE) :
            ConResPuts(StdOut, STRING_MOUNTVOL_NOPOINTS);
            wcscat(VolumeName, L"\\");
        }

    } while (FindNextVolumeW(Volume, VolumeName, ARRAYSIZE(VolumeName)) == TRUE);

    RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
    FindVolumeClose(Volume);

    if (GetLastError() != ERROR_NO_MORE_FILES)
        goto Fail;

    /* If automount is disabled, inform the user */
    if (!QueryAutoMount(&AutoMountState))
        return FALSE;

    if (AutoMountState.CurrentState == Disabled)
        ConResPrintf(StdOut, STRING_MOUNTVOL_NOAUTOMOUNT);

    /* If running on (U)EFI system, print the ESP mount point if there is any */
    if (IsEFI())
        PrintESPMountPoint();

    return TRUE;

Fail:
    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
}

static
BOOL
RemoveMountPoints()
{
    BOOL Ret;
    DWORD BytesReturned;
    WCHAR VolumeName[50], VolumeNameFolder[50];
    LPWSTR VolumeMountPointName, VolumeMountPointPath;
    DWORD VolumeMountPointPathLength = MAX_PATH * sizeof(WCHAR);
    HANDLE Volume, VolumeMountPoint, VolumeHandle, MountMgrHandle;

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
        goto Fail;

    /* Invoke mount manager registry scrubbing */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_SCRUB_REGISTRY,
                          NULL, 0,
                          NULL, 0, &BytesReturned,
                          NULL);
    
    CloseHandle(MountMgrHandle);
    if (!Ret)
        goto Fail;

    VolumeMountPointName = RtlAllocateHeap(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           VolumeMountPointPathLength);
    VolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           VolumeMountPointPathLength + sizeof(VolumeNameFolder));

    if (!VolumeMountPointName || !VolumeMountPointPath)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Loop through all volumes */
    Volume = FindFirstVolumeW(VolumeName, ARRAYSIZE(VolumeName));

    if (Volume == INVALID_HANDLE_VALUE)
        goto Fail;

    do
    {
        /* Loop through all folder mount points on this volume */
        VolumeMountPoint = FindFirstVolumeMountPointW(VolumeName,
                                                      VolumeMountPointName,
                                                      VolumeMountPointPathLength/sizeof(WCHAR));
  
        while (VolumeMountPoint == INVALID_HANDLE_VALUE && GetLastError() == ERROR_MORE_DATA)
        {
            RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointName);
            RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointPath);
            VolumeMountPointPathLength += VolumeMountPointPathLength;
            VolumeMountPointName = RtlAllocateHeap(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   VolumeMountPointPathLength);
            VolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   VolumeMountPointPathLength + sizeof(VolumeName));
            if (!VolumeMountPointName || !VolumeMountPointPath)
            {
                ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            FindFirstVolumeMountPointW(VolumeName,
                                       VolumeMountPointName,
                                       VolumeMountPointPathLength/sizeof(WCHAR));
        }
    
        if (VolumeMountPoint == INVALID_HANDLE_VALUE)
        {
            ConPrintf(StdOut, L"FindFirstVolumeMountPointW not found %d\n", GetLastError());
            continue;
        }

        /* Assemble a full mounted folder path */
        wcscpy(VolumeMountPointPath, VolumeName);
        wcscat(VolumeMountPointPath, VolumeMountPointName);
        do
        {
            ConPrintf(StdOut, L"szVolumeMountPoint %s\n", VolumeMountPointName);
            /* Get the volume name from mounted folder */
            GetVolumeNameForVolumeMountPointW(VolumeMountPointPath,
                                              (LPWSTR)VolumeNameFolder,
                                              ARRAYSIZE(VolumeNameFolder));

            /* Trim trailing backslash */
            VolumeNameFolder[wcslen(VolumeNameFolder) - 1] = UNICODE_NULL;

            /* Try to open the volume */
            VolumeHandle = CreateFileW(VolumeNameFolder, 0, 0, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       INVALID_HANDLE_VALUE);
            ConPrintf(StdOut, L"VolumeNameFolder %s\n", VolumeNameFolder);
            if (VolumeHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                ConPrintf(StdOut, L"RemoveDirectory\n");
                /* Mounted fodler appears to be not used, remove it */
                RemoveDirectoryW(VolumeMountPointPath);
            }
            else if (VolumeHandle != INVALID_HANDLE_VALUE)
            {
                ConPrintf(StdOut, L"CloseHandle %d\n", GetLastError());
                /* Mounted fodler is used, don't touch it */
                CloseHandle(VolumeHandle);
            }
            else
            {
                ConFormatMessage(StdOut, GetLastError());
            }

            Ret = FindNextVolumeMountPointW(VolumeMountPoint,
                                            VolumeMountPointName,
                                            VolumeMountPointPathLength/sizeof(WCHAR));
            while (Ret == FALSE && GetLastError() == ERROR_MORE_DATA)
            {
                RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointName);
                RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointPath);
                VolumeMountPointPathLength += VolumeMountPointPathLength;
                VolumeMountPointName = RtlAllocateHeap(GetProcessHeap(),
                                                       HEAP_ZERO_MEMORY,
                                                       VolumeMountPointPathLength);
                VolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(),
                                                       HEAP_ZERO_MEMORY,
                                                       VolumeMountPointPathLength +
                                                       sizeof(VolumeName));
                if (!VolumeMountPointName || !VolumeMountPointPath)
                {
                    FindVolumeMountPointClose(VolumeMountPoint);
                    FindVolumeClose(Volume);
                    ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
                Ret = FindNextVolumeMountPointW(VolumeMountPoint,
                                                VolumeMountPointName,
                                                VolumeMountPointPathLength/sizeof(WCHAR));
            }

            ZeroMemory(VolumeMountPointPath, VolumeMountPointPathLength);
            wcscpy(VolumeMountPointPath, VolumeName);
            wcscat(VolumeMountPointPath, VolumeMountPointName);

        } while (Ret == TRUE);

        FindVolumeMountPointClose(VolumeMountPoint);
        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointName);
            RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointPath);
            FindVolumeClose(Volume);
            goto Fail;
        }

    } while (FindNextVolumeW(Volume, VolumeName, ARRAYSIZE(VolumeName)) == TRUE);

    RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointName);
    RtlFreeHeap(GetProcessHeap(), 0, VolumeMountPointPath);
    FindVolumeClose(Volume);
    if (GetLastError() != ERROR_NO_MORE_FILES)
        goto Fail;

    return TRUE;

Fail:
    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
}

static
BOOL
PrintVolumeNameForMountPoint(LPCWSTR MountPoint)
{
    WCHAR VolumeName[50];

    /* Print the volume name for requested mount point */
    if (GetVolumeNameForVolumeMountPointW(MountPoint, VolumeName, ARRAYSIZE(VolumeName)))
    {
         ConPrintf(StdOut, L"%*s%s\n", 4, "", VolumeName);
         return TRUE;
    }

    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
}

static
BOOL
DismountVolume(LPCWSTR MountPoint)
{
    BOOL Ret;
    DWORD BytesReturned;
    WCHAR VolumeName[50];
    LPWCH VolumePathNames = NULL;
    HANDLE Volume;
    DWORD ReturnLength = MAX_PATH, PathOffset = 0, PathNum = 0;

    /* Get the volume name */
    if (!GetVolumeNameForVolumeMountPointW(MountPoint, VolumeName, ARRAYSIZE(VolumeName)))
    {
        goto Fail;
    }

    /* Get volume mount points for volume */
    VolumePathNames = RtlAllocateHeap(GetProcessHeap(),
                                      0,
                                      ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!VolumePathNames)
    {
        ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Ret = GetVolumePathNamesForVolumeNameW(VolumeName,
                                           VolumePathNames,
                                           ReturnLength + sizeof(UNICODE_NULL),
                                           &ReturnLength);
    if (GetLastError() == ERROR_MORE_DATA)
    {
        /* We need more heap */
        RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
        VolumePathNames = RtlAllocateHeap(GetProcessHeap(),
                                          0,
                                          ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
        if (!VolumePathNames)
        {
            ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        Ret = GetVolumePathNamesForVolumeNameW(VolumeName,
                                               VolumePathNames,
                                               ReturnLength + sizeof(UNICODE_NULL),
                                               &ReturnLength);
    }

    if (!Ret)
    {
        RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
        goto Fail;
    }

    /* Verify there are no more than one path in multiline string */
    if (ReturnLength > 1)
    {
        while (PathOffset < ReturnLength - 1 && PathNum < 2)
        {
            PathOffset += wcslen(VolumePathNames + PathOffset) + 1;
            PathNum++;
        }

        /* We cannot dismount the volume if it has multiple mount points */
        if (PathNum > 1)
        {
            RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
            ConResPuts(StdOut, STRING_MOUNTVOL_TOOMANYPOINTS);
            return FALSE;
        }
    }

    RtlFreeHeap(GetProcessHeap(), 0, VolumePathNames);
    VolumeName[wcslen(VolumeName) - 1] = UNICODE_NULL;

    /* Open a handle to the volume */
    Volume = CreateFileW(VolumeName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (Volume == INVALID_HANDLE_VALUE)
        goto Fail;

    /* Verify the volume in question can be offline */
    if (!DeviceIoControl(Volume,
                        IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        ConResPuts(StdOut, STRING_MOUNTVOL_UNSUPPORTEDOPERATION);
        CloseHandle(Volume);
        return FALSE;
    }

    /* Try locking the volume first */
    if (!DeviceIoControl(Volume,
                        FSCTL_LOCK_VOLUME,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        /* We couldn't lock the volume before dismounting, inform the user about that */
        ConResPuts(StdOut, STRING_MOUNTVOL_VOLUMESTILLINUSE);
    }

    /* Dismount the vloume */
    if (!DeviceIoControl(Volume,
                        FSCTL_DISMOUNT_VOLUME,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        CloseHandle(Volume);
        goto Fail;
    }

    /* Offline the volume */
    if (!DeviceIoControl(Volume,
                        IOCTL_VOLUME_OFFLINE,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        CloseHandle(Volume);
        goto Fail;
    }

    CloseHandle(Volume);

    /* Finally delte the mount point */
    DeleteVolumeMountPointW(MountPoint);
    return TRUE;

Fail:
    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
}

static
VOID
PrintHelp()
{
    /* Print (U)EFI specific strings only when running on such system */
    ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
    IsEFI() ? ConResPuts(StdOut, STRING_MOUNTVOL_ESPMOUNTUSAGE) : ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, STRING_MOUNTVOL_HELP);
    IsEFI() ? ConResPuts(StdOut, STRING_MOUNTVOL_ESPMOUNTHELP) : ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, STRING_MOUNTVOL_LIST);
    PrintVolumeList();
}

int wmain(int argc, WCHAR *argv[])
{
    int Ret = 0;
    PWSTR DosPath = NULL;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Print help */
    if (argc == 1 || (argc > 1 && wcscmp(argv[1], L"/?") == 0))
    {
        PrintHelp();
        return 0;
    }

    /* Argument should at least have two characters */
    if (wcslen(argv[1]) < 2)
    {
        PrintHelp();
        return 1;
    }

    /* Check if first argument is a mount point */
    if (argv[1][1] == L':')
    {
        /* If we got more or less than 2 arguments here, print help */
        if (argc != 3)
        {
            PrintHelp();
            return 1;
        }

        /* Allocate another buffer for the path string so we can manipulate it as needed */
        DosPath = RtlAllocateHeap(GetProcessHeap(),
                                  0,
                                  (wcslen(argv[1]) * sizeof(WCHAR)) +
                                  sizeof(WCHAR) + sizeof(UNICODE_NULL));

        if (DosPath == NULL)
        {
            ConFormatMessage(StdOut, ERROR_NOT_ENOUGH_MEMORY);
            return 1;
        }

        wcscpy(DosPath, argv[1]);

        /* Append a backslash if needed */
        if (DosPath[wcslen(DosPath) - 1] != L'\\')
        {
            wcscat(DosPath, L"\\");
        }

        /* Check if second argument looks like a switch */
        if ((wcslen(argv[2]) == 2) && argv[2][0] == L'/')
        {
            /* Look for supported switches for managing the mount points */
            switch (towupper(argv[2][1]))
            {
                case L'D':
                    /* Delete the mount point */
                    Ret = !DeleteVolumeMountPointW(DosPath);
                    if (Ret && GetLastError() == ERROR_INVALID_PARAMETER)
                    {
                        /* It could be a mount point without a volume name (ESP for example) */
                        DosPath[wcslen(DosPath) - 1] = UNICODE_NULL;
                        Ret = !DefineDosDeviceW(DDD_REMOVE_DEFINITION, DosPath, NULL);
                        if (Ret)
                        {
                            /* We report the original error code */
                            ConFormatMessage(StdOut, ERROR_INVALID_PARAMETER);
                        }
                    }
                    else if (Ret)
                    {
                        ConFormatMessage(StdOut, GetLastError());
                    }

                    goto Exit;
                case L'L':
                    /* Print volume name for the mount point */
                    Ret = !PrintVolumeNameForMountPoint(DosPath);
                    goto Exit;
                case L'P':
                    /* Delete the mount point and dismount the volume */
                    Ret = !DismountVolume(DosPath);
                    goto Exit;
                case L'S':
                    /* ESP mount is requested, mount point should be a root path */
                    if (wcslen(argv[1]) > 3)
                    {
                        Ret = 1;
                        ConFormatMessage(StdOut, ERROR_INVALID_PARAMETER);
                        goto Exit;
                    }

                    /* Mount ESP partition at requested mount point */
                    DosPath[wcslen(DosPath) - 1] = UNICODE_NULL;
                    Ret = !MountESPVolume(DosPath);
                    goto Exit;
               default:
                    /* Unsupported switch, print help */
                    PrintHelp();
                    goto Exit;
            }    
        }

        /* Not a switch, pass it as a volume name then */
        Ret = !SetVolumeMountPointW(DosPath, argv[2]);
        if (Ret)
            ConFormatMessage(StdOut, GetLastError());
        goto Exit;
    }

    /* Maybe we just got a switch as an argument, don't allow more than one argument anymore */
    if (argc > 2)
    {
        PrintHelp();
        return 0;
    }

    if ((wcslen(argv[1]) == 2) && (argv[1][0] != L'/'))
    {
        /* Not a switch either, print help */
        PrintHelp();
        return 0;
    }

    /* Check for supported switches for general volume management */
    switch (towupper(argv[1][1]))
    {
        case L'R':
            /* Remove mount manager entries and mount points for volumes that are not in system */
            return !RemoveMountPoints();
        case L'N':
            /* Disable automatic mounting of new volumes */
            return !SetAutoMount(Disabled);
        case L'E':
            /* Enable automatic mounting of new volumes */
            return !SetAutoMount(Enabled);
        default:
            /* Unsupported switch, print help */
            PrintHelp();
            return 0;
    }

Exit:
    RtlFreeHeap(GetProcessHeap(), 0, DosPath);
    return Ret;
}

/* EOF */
