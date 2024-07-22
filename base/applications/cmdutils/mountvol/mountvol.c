
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
PrintVolumeList()
{
    WCHAR szVolumeName[60];
    LPWCH lpszVolumePathNames = NULL;
    HANDLE hVolume;
    DWORD cchReturnLength = 0;

    /* Loop through all volumes */
    hVolume = FindFirstVolumeW((LPWSTR)szVolumeName, ARRAYSIZE(szVolumeName));

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    ConResPuts(StdOut, STRING_MOUNTVOL_LIST);
    
    lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR));
    do
    {
        /* Get volume mount points */
        GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength, &cchReturnLength);
        
        if (GetLastError() == ERROR_MORE_DATA)
        {
            /* We need more heap */
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumePathNames);
            lpszVolumePathNames = RtlAllocateHeap(GetProcessHeap(), 0, cchReturnLength * sizeof(WCHAR));
            GetVolumePathNamesForVolumeNameW(szVolumeName, lpszVolumePathNames, cchReturnLength, &cchReturnLength);
        }

        ConPrintf(StdOut, L"    %s\n        ", szVolumeName);

        if (cchReturnLength > sizeof(UNICODE_NULL))
        {
            ConPrintf(StdOut, L"%s\n\n", lpszVolumePathNames);
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
    
    return TRUE;
}

static
BOOL
RemoveMountPoints()
{
    BOOL Ret;
    DWORD BytesReturned;
    WCHAR szVolumeName[60];
    WCHAR szVolumeNameFolder[60];
    LPWSTR lpszVolumeMountPointName;
    LPWSTR lpszVolumeMountPointPath;
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
        ConPrintf(StdOut, L"MountMgrHandle INVALID_HANDLE_VALUE %d\n", GetLastError());
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
        ConPrintf(StdOut, L"DeviceIoControl fail %d\n", GetLastError());
        return FALSE;
    }

    lpszVolumeMountPointName = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength);
    lpszVolumeMountPointPath = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVolumeMountPointPathLength + sizeof(szVolumeNameFolder));

    if (!lpszVolumeMountPointName || !lpszVolumeMountPointPath)
    {
        return FALSE;
    }

    /* Loop through all volumes */
    hVolume = FindFirstVolumeW(szVolumeName, ARRAYSIZE(szVolumeName));

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        ConPrintf(StdOut, L"hVolume INVALID_HANDLE_VALUE %d\n", GetLastError());
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
            ConPrintf(StdOut, L"FindFirstVolumeMountPointW fail %d\n", GetLastError());
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
            RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
            FindVolumeClose(hVolume);
            return FALSE;
        }

    } while (FindNextVolumeW(hVolume, szVolumeName, ARRAYSIZE(szVolumeName)) == TRUE);

    RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointName);
    RtlFreeHeap(GetProcessHeap(), 0, lpszVolumeMountPointPath);
    FindVolumeClose(hVolume);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        ConPrintf(StdOut, L"Failed! %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
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
        ConPrintf(StdOut, L"MountMgrHandle INVALID_HANDLE_VALUE %d\n", GetLastError());
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

int wmain(int argc, WCHAR *argv[])
{
    PWSTR DosPath;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Print help */
    if (argc == 1 || (argc > 1 && wcscmp(argv[1], L"/?") == 0))
    {
        ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
        PrintVolumeList();
        return 0;
    }

    /* Argument should at least have two characters */
    if (wcslen(argv[1]) < 2)
    {
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
            ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
            PrintVolumeList();
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
                    /* List mount points */
                    ConFormatMessage(StdOut, ERROR_CALL_NOT_IMPLEMENTED);
                    return 1;
                case L'P':
                    /* Delete the mount point and dismount the volume */
                    ConFormatMessage(StdOut, ERROR_CALL_NOT_IMPLEMENTED);
                    return 1;
                case L'S':
                    /* ESP mount is requested, mount point should be a root path */
                    if (wcslen(argv[1]) > 3)
                    {
                        ConFormatMessage(StdOut, ERROR_INVALID_PARAMETER);
                        return 1;
                    }

                    /* Mount ESP partition at requested mount point */
                    ConFormatMessage(StdOut, ERROR_CALL_NOT_IMPLEMENTED);
                    return 1;
               default:
                    /* Unsupported switch, print help */
                    ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
                    PrintVolumeList();
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
        /* Windows prints invalid parameter error, we will print help */
        ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
        PrintVolumeList();
        return 0;
    }

    if ((wcslen(argv[1]) == 2) && (argv[1][0] != L'/'))
    {
        /* Not a switch either, print help */
        ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
        PrintVolumeList();
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
            ConResPuts(StdOut, STRING_MOUNTVOL_USAGE);
            PrintVolumeList();
            return 0;
    }
}

/* EOF */
