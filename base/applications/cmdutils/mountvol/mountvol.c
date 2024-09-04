
#include "mountvol.h"

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
                                 GENERIC_READ | GENERIC_WRITE,
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
IsVolumeOffline(LPCWSTR lpVolumeName)
{
    BOOL Ret;
    HANDLE hVolume;
    DWORD BytesReturned;

    /* Open a handle to the mount manager */
    hVolume = CreateFileW(lpVolumeName,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Get current auto mount state */
    Ret = DeviceIoControl(hVolume,
                          IOCTL_VOLUME_IS_OFFLINE,
                          NULL, 0,
                          NULL, 0, &BytesReturned,
                          NULL);

    CloseHandle(hVolume);
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
GetESPDevice(LPWSTR lpMountPoint)
{
    /* NOTE: This is done differently (probably by querying registry) on NT 5.x IA-64.
     * The method below uses a system information class introduced in Vista
     * FIXME: Enable when ReactOS supports ESPs and exposes necessary SystemInformation classes */

#if !defined(__REACTOS__) && (NTDDI_VERSION >= NTDDI_LONGHORN)
    PSYSTEM_SYSTEM_PARTITION_INFORMATION SystemPartitionInformation;
    ULONG SystemInformationLength = sizeof(*SystemPartitionInformation);
    ULONG ReturnLength = 0;
    NTSTATUS Status;

    SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(), 0, SystemInformationLength);
    Status = NtQuerySystemInformation(SystemSystemPartitionInformation, SystemPartitionInformation, SystemInformationLength, &ReturnLength);

    while (Status == STATUS_BUFFER_TOO_SMALL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
        SystemInformationLength = ReturnLength;
        SystemPartitionInformation = RtlAllocateHeap(GetProcessHeap(), 0, SystemInformationLength);
        if (!SystemPartitionInformation)
        {
            lpMountPoint[0] = UNICODE_NULL;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ConFormatMessage(StdOut, GetLastError());
            return FALSE;
        }
        Status = NtQuerySystemInformation(SystemSystemPartitionInformation, SystemPartitionInformation, SystemInformationLength, &ReturnLength);
    }

    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
        lpMountPoint[0] = UNICODE_NULL;
        ConFormatMessage(StdOut, RtlNtStatusToDosError(Status));
        return FALSE;
    }

    wcscpy(lpMountPoint, SystemPartitionInformation->SystemPartition.Buffer);

    RtlFreeHeap(GetProcessHeap(), 0, SystemPartitionInformation);
    return TRUE;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
#endif
}

static
BOOL
MountESPVolume(LPCWSTR lpMountPoint)
{
    WCHAR szDeviceName[50];

    if (!GetESPDevice(szDeviceName) || !DefineDosDeviceW(DDD_RAW_TARGET_PATH, lpMountPoint, szDeviceName))
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
BOOL
PrintESPMountPoint()
{
    WCHAR szDeviceName[50];
    WCHAR szTargetPath[50];
    WCHAR szMountPoint[5];
    ULONG Letter;

    if (!GetESPDevice(szDeviceName))
    {
        return FALSE;
    }

    szMountPoint[1] = ':';
    szMountPoint[2] = UNICODE_NULL;
    for (Letter = 65; Letter <= 90; Letter++)
    {
        szMountPoint[0] = Letter;
        QueryDosDeviceW(szMountPoint, szTargetPath, ARRAYSIZE(szTargetPath));
        if (!wcscmp(szDeviceName, szTargetPath))
            break;
    }

    if (Letter > 90)
    {
        return FALSE;
    }

    szMountPoint[2] = '\\';
    szMountPoint[3] = UNICODE_NULL;
    ConResPrintf(StdOut, STRING_MOUNTVOL_ESPMOUNTPOINT, szMountPoint);
    return TRUE;
}

static
BOOL
IsEFI()
{
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    SYSTEM_BOOT_ENVIRONMENT_INFORMATION SystemBootInfo = {0};
    ULONG SystemInformationLength = sizeof(SystemBootInfo);
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBootEnvironmentInformation, &SystemBootInfo, SystemInformationLength, NULL);

    if (!NT_SUCCESS(Status))
    {
        ConFormatMessage(StdOut, RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return SystemBootInfo.FirmwareType ? FirmwareTypeUefi : FirmwareTypeBios;
#else
    return FALSE;
#endif
}

static
BOOL
PrintVolumeList()
{
    BOOL Ret;
    WCHAR szVolumeName[60];
    LPWCH lpszVolumePathNames = NULL;
    HANDLE hVolume;
    DWORD cchReturnLength = MAX_PATH, PathOffset;
    MOUNTMGR_QUERY_AUTO_MOUNT AutoMountState = {0};

    /* Loop through all volumes */
    hVolume = FindFirstVolumeW((LPWSTR)szVolumeName, ARRAYSIZE(szVolumeName));

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    
    lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    if (!lpszVolumePathNames)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    do
    {
        szVolumeName[wcslen(szVolumeName) - 1] = UNICODE_NULL;
        if (IsVolumeOffline(szVolumeName))
        {
            ConPrintf(StdOut, L"    %s\n", szVolumeName);
            ConResPuts(StdOut, STRING_MOUNTVOL_NOTMOUNTABLE);
            continue;
        }
        szVolumeName[wcslen(szVolumeName)] = '\\';
        szVolumeName[wcslen(szVolumeName) + 1] = UNICODE_NULL;

        /* Get volume mount points */
        Ret = GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength + sizeof(UNICODE_NULL), &cchReturnLength);
        
        if (GetLastError() == ERROR_MORE_DATA)
        {
            /* We need more heap */
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumePathNames);
            lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            if (!lpszVolumePathNames)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ConFormatMessage(StdOut, GetLastError());
                return FALSE;
            }
            Ret = GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength + sizeof(UNICODE_NULL), &cchReturnLength);
        }

        ConPrintf(StdOut, L"    %s\n", szVolumeName);

        if (!Ret)
        {
            ConFormatMessage(StdOut, GetLastError());
            continue;
        }

        PathOffset = 0;

        if (cchReturnLength > sizeof(UNICODE_NULL))
        {
            /* Print all paths found in multiline string */
            while (PathOffset < cchReturnLength - sizeof(UNICODE_NULL))
            {
                ConPrintf(StdOut, L"        %s\n", lpszVolumePathNames + PathOffset);
                PathOffset += wcslen(lpszVolumePathNames + PathOffset) + 1;
            }
            ConPuts(StdOut, L"\n");
        }
        else
        {
            ConResPuts(StdOut, STRING_MOUNTVOL_NOPOINTS);
        }

    } while (FindNextVolumeW(hVolume, szVolumeName, ARRAYSIZE(szVolumeName)) == TRUE);

    RtlFreeHeap(GetProcessHeap(), 0, lpszVolumePathNames);
    FindVolumeClose(hVolume);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        return FALSE;
    }

    /* If automount is disabled, inform the user */
    if (!QueryAutoMount(&AutoMountState))
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    if (AutoMountState.CurrentState == Disabled)
    {
        ConResPuts(StdOut, STRING_MOUNTVOL_NOAUTOMOUNT);
    }

    if (IsEFI())
        PrintESPMountPoint();

    return TRUE;
}

static
BOOL
RemoveMountPoints()
{
    BOOL Ret;
    DWORD BytesReturned;
    WCHAR szVolumeName[60], szVolumeNameFolder[60];
    LPWSTR lpszVolumeMountPointName, lpszVolumeMountPointPath;
    DWORD dwVolumeMountPointPathLength = MAX_PATH * sizeof(WCHAR);
    HANDLE hVolume, hVolumeMountPoint, VolumeHandle, MountMgrHandle;

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

    /* Invoke Mount Manager registry scrubbing */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_SCRUB_REGISTRY,
                          NULL, 0,
                          NULL, 0, &BytesReturned,
                          NULL);
    
    CloseHandle(MountMgrHandle);
    if (!Ret)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    lpszVolumeMountPointName = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength);
    lpszVolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength + sizeof(szVolumeNameFolder));

    if (!lpszVolumeMountPointName || !lpszVolumeMountPointPath)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Loop through all volumes */
    hVolume = FindFirstVolumeW(szVolumeName, ARRAYSIZE(szVolumeName));

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    do
    {
        /* Loop through all folder mount points on this volume */
        hVolumeMountPoint = FindFirstVolumeMountPointW(szVolumeName, lpszVolumeMountPointName, dwVolumeMountPointPathLength/sizeof(WCHAR));
  
        while (hVolumeMountPoint == INVALID_HANDLE_VALUE && GetLastError() == ERROR_MORE_DATA)
        {
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
            dwVolumeMountPointPathLength += dwVolumeMountPointPathLength;
            lpszVolumeMountPointName = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength);
            lpszVolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength + sizeof(szVolumeName));
            if (!lpszVolumeMountPointName || !lpszVolumeMountPointPath)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ConFormatMessage(StdOut, GetLastError());
                return FALSE;
            }
            FindFirstVolumeMountPointW(szVolumeName, lpszVolumeMountPointName, dwVolumeMountPointPathLength/sizeof(WCHAR));
        }
    
        if (hVolumeMountPoint == INVALID_HANDLE_VALUE)
        {
            ConPrintf(StdOut, L"FindFirstVolumeMountPointW not found %d\n", GetLastError());
            continue;
        }

        wcscpy(lpszVolumeMountPointPath, szVolumeName);
        wcscat(lpszVolumeMountPointPath, lpszVolumeMountPointName);

        do
        {
            ConPrintf(StdOut, L"szVolumeMountPoint %s\n", lpszVolumeMountPointName);
            /* Get the volume name from mounted folder */
            GetVolumeNameForVolumeMountPointW(lpszVolumeMountPointPath,
                                              (LPWSTR)szVolumeNameFolder,
                                              ARRAYSIZE(szVolumeNameFolder));

            /* Trim trailing backslash */
            szVolumeNameFolder[wcslen(szVolumeNameFolder) - 1] = UNICODE_NULL;

            /* Try to open the volume */
            VolumeHandle = CreateFileW(szVolumeNameFolder, 0, 0, NULL, 
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                       INVALID_HANDLE_VALUE);
            ConPrintf(StdOut, L"szVolumeNameFolder %s\n", szVolumeNameFolder);
            if (VolumeHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                ConPrintf(StdOut, L"RemoveDirectory\n");
                /* Mounted fodler appears to be not used, remove it */
                //lpszVolumeMountPointPath[wcslen(lpszVolumeMountPointPath) - 1] = UNICODE_NULL;
                RemoveDirectoryW(lpszVolumeMountPointPath);
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

            Ret = FindNextVolumeMountPointW(hVolumeMountPoint, lpszVolumeMountPointName, dwVolumeMountPointPathLength/sizeof(WCHAR));

            while (Ret == FALSE && GetLastError() == ERROR_MORE_DATA)
            {
                RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
                RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
                dwVolumeMountPointPathLength += dwVolumeMountPointPathLength;
                lpszVolumeMountPointName = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength);
                lpszVolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength + sizeof(szVolumeName));
                if (!lpszVolumeMountPointName || !lpszVolumeMountPointPath)
                {
                    FindVolumeMountPointClose(hVolumeMountPoint);
                    FindVolumeClose(hVolume);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    ConFormatMessage(StdOut, GetLastError());
                    return FALSE;
                }
                Ret = FindNextVolumeMountPointW(hVolumeMountPoint, lpszVolumeMountPointName, dwVolumeMountPointPathLength/sizeof(WCHAR));
            }

            ZeroMemory(lpszVolumeMountPointPath, dwVolumeMountPointPathLength);
            wcscpy(lpszVolumeMountPointPath, szVolumeName);
            wcscat(lpszVolumeMountPointPath, lpszVolumeMountPointName);

        } while (Ret == TRUE);

        FindVolumeMountPointClose(hVolumeMountPoint);

        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
            FindVolumeClose(hVolume);
            ConFormatMessage(StdOut, GetLastError());
            return FALSE;
        }

    } while (FindNextVolumeW(hVolume, szVolumeName, ARRAYSIZE(szVolumeName)) == TRUE);

    RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
    RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
    FindVolumeClose(hVolume);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
BOOL
PrintVolumeNameForMountPoint(LPCWSTR lpMountPoint)
{
    WCHAR szVolumeName[50];

    /* Print the volume name for requested mount point */
    if (GetVolumeNameForVolumeMountPointW(lpMountPoint, szVolumeName, ARRAYSIZE(szVolumeName)))
    {
         ConPrintf(StdOut, L"    %s\n", szVolumeName);
         return TRUE;
    }

    ConFormatMessage(StdOut, GetLastError());
    return FALSE;
}

static
BOOL
DismountVolume(LPCWSTR lpMountPoint)
{
    BOOL Ret;
    DWORD BytesReturned;
    WCHAR szVolumeName[50];
    LPWCH lpszVolumePathNames = NULL;
    HANDLE hVolume;
    DWORD cchReturnLength = MAX_PATH, PathOffset = 0, PathNum = 0;

    /* Get the volume name */
    if (!GetVolumeNameForVolumeMountPointW(lpMountPoint, szVolumeName, ARRAYSIZE(szVolumeName)))
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Get volume mount points for volume */
    lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!lpszVolumePathNames)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }
    Ret = GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength + sizeof(UNICODE_NULL), &cchReturnLength);

    if (GetLastError() == ERROR_MORE_DATA)
    {
        /* We need more heap */
        RtlFreeHeap(GetProcessHeap(), 0, lpszVolumePathNames);
        lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
        if (!lpszVolumePathNames)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ConFormatMessage(StdOut, GetLastError());
            return FALSE;
        }
        Ret = GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength + sizeof(UNICODE_NULL), &cchReturnLength);
    }

    if (!Ret)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Verify there are no more than one path in multiline string */
    if (cchReturnLength > sizeof(UNICODE_NULL))
    {
        while (PathOffset < cchReturnLength - sizeof(UNICODE_NULL) && PathNum < 2)
        {
            PathOffset += wcslen(lpszVolumePathNames + PathOffset) + 1;
            PathNum++;
        }

        /* We cannot dismount the volume if it has multiple mount points */
        if (PathNum > 1)
        {
            ConResPuts(StdOut, STRING_MOUNTVOL_TOOMANYPOINTS);
            return FALSE;
        }
    }

    szVolumeName[wcslen(szVolumeName) - 1] = UNICODE_NULL;

    /* Open a handle to the volume */
    hVolume = CreateFileW(szVolumeName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        ConFormatMessage(StdOut, GetLastError());
        return FALSE;
    }

    /* Verify if volume in question can be offline */
    if (!DeviceIoControl(hVolume,
                        IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        /* It cannot, inform the user */
        ConResPuts(StdOut, STRING_MOUNTVOL_UNSUPPORTEDOPERATION);
        CloseHandle(hVolume);
        return FALSE;
    }

    /* Try locking the volume first */
    if (!DeviceIoControl(hVolume,
                        FSCTL_LOCK_VOLUME,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        /* We couldn't lock the volume before dismounting, inform the user about that */
        ConResPuts(StdOut, STRING_MOUNTVOL_VOLUMESTILLINUSE);
    }

    /* Dismount the vloume */
    if (!DeviceIoControl(hVolume,
                        FSCTL_DISMOUNT_VOLUME,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        ConFormatMessage(StdOut, GetLastError());
        CloseHandle(hVolume);
        return FALSE;
    }

    /* Offline the volume */
    if (!DeviceIoControl(hVolume,
                        IOCTL_VOLUME_OFFLINE,
                        NULL, 0,
                        NULL, 0, &BytesReturned,
                        NULL))
    {
        ConFormatMessage(StdOut, GetLastError());
        CloseHandle(hVolume);
        return FALSE;
    }

    CloseHandle(hVolume);
    return TRUE;
}

static
VOID
PrintHelp()
{
    ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
    IsEFI() ? ConResPuts(StdOut, STRING_MOUNTVOL_ESPMOUNTUSAGE) : ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, STRING_MOUNTVOL_HELP);
    IsEFI() ? ConResPuts(StdOut, STRING_MOUNTVOL_ESPMOUNTHELP) : ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, STRING_MOUNTVOL_LIST);
    PrintVolumeList();
}

int wmain(int argc, WCHAR *argv[])
{
    PWSTR DosPath;

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

        /* If we got more or less than 2 arguments here, print help */
        if (argc != 3)
        {
            PrintHelp();
            return 1;
        }

        /* Check if second argument looks like a switch */
        if ((wcslen(argv[2]) == 2) && argv[2][0] == L'/')
        {
            /* Look for supported switches for managing the mount points */
            switch (towupper(argv[2][1]))
            {
                case L'D':
                    /* Delete the mount point */
                    if (!DeleteVolumeMountPointW(DosPath))
                    {
                        ConFormatMessage(StdOut, GetLastError());
                        RtlFreeHeap(GetProcessHeap(), 0, DosPath);
                        return 1;
                    }
                    
                    RtlFreeHeap(GetProcessHeap(), 0, DosPath);
                    return 0;
                case L'L':
                    /* Print volume name for the mount point */
                    PrintVolumeNameForMountPoint(DosPath);
                    return 0;
                case L'P':
                    /* Delete the mount point and dismount the volume */
                    DismountVolume(DosPath);
                    return 0;
                case L'S':
                    /* ESP mount is requested, we should be running on EFI system
                     * and mount point should be a root path */
                    if (!IsEFI() || wcslen(argv[1]) > 3)
                    {
                        ConFormatMessage(StdOut, ERROR_INVALID_PARAMETER);
                        return 1;
                    }

                    /* Mount ESP partition at requested mount point */
                    DosPath[wcslen(DosPath) - 1] = UNICODE_NULL;
                    MountESPVolume(DosPath);
                    return 0;
               default:
                    /* Unsupported switch, print help */
                    PrintHelp();
                    return 0;
            }    
        }

        /* Not a switch, pass it as a volume name then */
        if (!SetVolumeMountPointW(DosPath, argv[2]))
        {
            ConFormatMessage(StdOut, GetLastError());
            RtlFreeHeap(GetProcessHeap(), 0, DosPath);
            return 1;
        }

        RtlFreeHeap(GetProcessHeap(), 0, DosPath);
        return 0;
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
            /* Remove mount manager entries and mount points for volumes that are no longer in system */
            RemoveMountPoints();
            return 0;
        case L'N':
            /* Disable automatic mounting of new volumes */
            SetAutoMount(Disabled);
            return 0;
        case L'E':
            /* Enable automatic mounting of new volumes */
            SetAutoMount(Enabled);
            return 0;
        default:
            /* Unsupported switch, print help */
            PrintHelp();
            return 0;
    }
}

/* EOF */
