/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/mntpoint.c
 * PURPOSE:         File volume mount point functions
 * PROGRAMMER:      Pierre Schweitzer (pierre@reactos.org)
 */

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* Match a volume name like:
 * \\?\Volume{GUID}
 */
#define IS_VOLUME_NAME(s, l)                       \
  ((l == 96 || (l == 98 && s[48] == '\\')) &&      \
   s[0] == '\\'&& (s[1] == '?' || s[1] == '\\') && \
   s[2] == '?' && s[3] == '\\' && s[4] == 'V' &&   \
   s[5] == 'o' && s[6] == 'l' && s[7] == 'u' &&    \
   s[8] == 'm' && s[9] == 'e' && s[10] == '{' &&   \
   s[19] == '-' && s[24] == '-' && s[29] == '-' && \
   s[34] == '-' && s[47] == '}')

#define MAX_NTFS_PATH 32767

typedef struct _FILE_REPARSE_POINT_INFORMATION {
  LONGLONG FileReference;
  ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

/*
 * @implemented
 */
static BOOL
GetDeleteVolumeRoot(_In_ LPCWSTR lpszRootPath,
                    _Out_opt_ LPWSTR lpszVolumeName,
                    _In_opt_ DWORD cchBufferLength,
                    _In_ BOOL DeleteMountPoint)
{
    BOOL Ret;
    NTSTATUS Status;
    PWSTR FoundVolume;
    DWORD BytesReturned;
    WCHAR Buffer[64];
    UNICODE_STRING NtPathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PMOUNTMGR_MOUNT_POINT MountPoint;
    ULONG CurrentMntPt, FoundVolumeLen;
    PMOUNTMGR_MOUNT_POINTS MountPoints;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle, MountMgrHandle;
    struct
    {
        MOUNTDEV_NAME;
        WCHAR Buffer[MAX_PATH];
    } MountDevName;

    /* It makes no sense on a non-local drive */
    if (GetDriveTypeW(lpszRootPath) == DRIVE_REMOTE)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Get the NT path */
    if (!RtlDosPathNameToNtPathName_U(lpszRootPath, &NtPathName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* If it's a root path - likely - drop backslash to open volume */
    if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        NtPathName.Length -= sizeof(WCHAR);
    }

    /* If that's a DOS volume, upper case the letter */
    if (NtPathName.Length >= 2 * sizeof(WCHAR))
    {
        if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] == L':')
        {
            NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2] = towupper(NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2]);
        }
    }

    if (!DeleteMountPoint)
    {
        /* Attempt to open the volume */
        InitializeObjectAttributes(&ObjectAttributes, &NtPathName,
                                OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                            &ObjectAttributes, &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_ALERT);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Query the device name - that's what we'll translate */
        if (!DeviceIoControl(VolumeHandle, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL,
                            0, &MountDevName, sizeof(MountDevName), &BytesReturned,
                            NULL))
        {
            NtClose(VolumeHandle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
            return FALSE;
        }

        /* No longer need the volume */
        NtClose(VolumeHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);

        /* We'll keep the device name for later usage */
        NtPathName.Length = MountDevName.NameLength;
        NtPathName.MaximumLength = MountDevName.NameLength + sizeof(UNICODE_NULL);
        NtPathName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NtPathName.MaximumLength);
        if (NtPathName.Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        RtlCopyMemory(NtPathName.Buffer, MountDevName.Name, NtPathName.Length);
        NtPathName.Buffer[NtPathName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        /* We need a DOS device name for symbolic link too */
        swprintf(Buffer, L"\\DosDevices\\%c:", towupper(lpszRootPath[0]));
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        NtPathName.Length = wcslen(Buffer) * sizeof(WCHAR);
        NtPathName.MaximumLength = NtPathName.Length + sizeof(UNICODE_NULL);
        NtPathName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NtPathName.MaximumLength * sizeof(WCHAR));
        RtlCopyMemory(NtPathName.Buffer, Buffer, NtPathName.Length);
    }

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                 NtPathName.Length + sizeof(MOUNTMGR_MOUNT_POINT));
    if (MountPoint == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!DeleteMountPoint)
    {
        MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        MountPoint->DeviceNameLength = NtPathName.Length;
        RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_MOUNT_POINT)), NtPathName.Buffer, NtPathName.Length);
    }  
    else
    {     
        MountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        MountPoint->SymbolicLinkNameLength = NtPathName.Length;
        RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_MOUNT_POINT)), NtPathName.Buffer, NtPathName.Length);
    }

    /* Allocate a dummy output buffer to probe for size */
    MountPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(MOUNTMGR_MOUNT_POINTS));
    if (MountPoints == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 
                                 DeleteMountPoint ? (GENERIC_READ | GENERIC_WRITE) : 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        return FALSE;
    }

    /* Query the names associated to our device name */
    Ret = DeviceIoControl(MountMgrHandle,
                          DeleteMountPoint ? IOCTL_MOUNTMGR_DELETE_POINTS :
                          IOCTL_MOUNTMGR_QUERY_POINTS,
                          MountPoint, NtPathName.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                          MountPoints, sizeof(MOUNTMGR_MOUNT_POINTS), &BytesReturned,
                          NULL);
    /* As long as the buffer is too small, keep looping */
    while (!Ret && GetLastError() == ERROR_MORE_DATA)
    {
        ULONG BufferSize;

        /* Get the size we've to allocate */
        BufferSize = MountPoints->Size;
        /* Reallocate the buffer with enough room */
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        MountPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferSize);
        if (MountPoints == NULL)
        {
            CloseHandle(MountMgrHandle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Reissue the request, it should work now! */
        Ret = DeviceIoControl(MountMgrHandle,
                              DeleteMountPoint ? IOCTL_MOUNTMGR_DELETE_POINTS :
                              IOCTL_MOUNTMGR_QUERY_POINTS,
                              MountPoint, NtPathName.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                              MountPoints, BufferSize, &BytesReturned, NULL);
    }

    /* We're done, no longer need the mount manager */
    CloseHandle(MountMgrHandle);
    /* Nor our input buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);

    /* If the mount manager failed, just quit */
    if (!Ret)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* If the caller didn't pass output buffer, let's just end here */
    if (DeleteMountPoint)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        return TRUE;
    }

    CurrentMntPt = 0;
    /* If there were no associated mount points, we'll return the device name */
    if (MountPoints->NumberOfMountPoints == 0)
    {
        FoundVolume = NtPathName.Buffer;
        FoundVolumeLen = NtPathName.Length;
    }
    /* Otherwise, find one which is matching */
    else
    {
        for (; CurrentMntPt < MountPoints->NumberOfMountPoints; ++CurrentMntPt)
        {
            UNICODE_STRING SymbolicLink;

            /* Make a string of it, to easy the checks */
            SymbolicLink.Length = MountPoints->MountPoints[CurrentMntPt].SymbolicLinkNameLength;
            SymbolicLink.MaximumLength = SymbolicLink.Length;
            SymbolicLink.Buffer = (PVOID)((ULONG_PTR)MountPoints + MountPoints->MountPoints[CurrentMntPt].SymbolicLinkNameOffset);
            /* If that's a NT volume name (GUID form), keep it! */
            if (MOUNTMGR_IS_NT_VOLUME_NAME(&SymbolicLink))
            {
                FoundVolume = SymbolicLink.Buffer;
                FoundVolumeLen = SymbolicLink.Length;

                break;
            }
        }
    }

    /* We couldn't find anything matching, return an error */
    if (CurrentMntPt == MountPoints->NumberOfMountPoints)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* We found a matching volume, have we enough memory to return it? */
    if (cchBufferLength * sizeof(WCHAR) < FoundVolumeLen + 2 * sizeof(WCHAR))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    /* Copy it back! */
    RtlCopyMemory(lpszVolumeName, FoundVolume, FoundVolumeLen);
    /* Make it compliant */
    lpszVolumeName[1] = L'\\';
    /* And transform it as root path */
    lpszVolumeName[FoundVolumeLen / sizeof(WCHAR)] = L'\\';
    lpszVolumeName[FoundVolumeLen / sizeof(WCHAR) + 1] = UNICODE_NULL;

    /* We're done! */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
    return TRUE;
}

static BOOL
SetVolumeNameForRoot(_In_ LPCWSTR lpszRootPath,
                     _In_ LPCWSTR lpszVolumeName)
{
    BOOL Ret;
    NTSTATUS Status;
    WCHAR Buffer[64];
    DWORD BytesReturned;
    UNICODE_STRING NtPathName;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtPathVolumeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle, MountMgrHandle;
    PMOUNTMGR_CREATE_POINT_INPUT MountPoint;
    struct
    {
        MOUNTDEV_NAME;
        WCHAR Buffer[MAX_PATH];
    } MountDevName;

    /* Get the NT path */
    if (!RtlDosPathNameToNtPathName_U(lpszRootPath, &NtPathName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Get the NT path */
    if (!RtlDosPathNameToNtPathName_U(lpszVolumeName, &NtPathVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Drop backslash to open volume */
    if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        NtPathName.Length -= sizeof(WCHAR);
    }

    /* Drop backslash to open volume */
    if (NtPathVolumeName.Buffer[(NtPathVolumeName.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        NtPathVolumeName.Buffer[(NtPathVolumeName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
        NtPathVolumeName.Length -= sizeof(WCHAR);
    }

    /* Upper case the letter on DOS path */
    if (NtPathName.Length >= 2 * sizeof(WCHAR))
    {
        if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] == L':')
        {
            NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2] = towupper(NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2]);
        }
    }

    /* We need a DOS device name for symbolic link too */
    swprintf(Buffer, L"\\DosDevices\\%c:", NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2]);
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
    NtPathName.Length = wcslen(Buffer) * sizeof(WCHAR);
    NtPathName.MaximumLength = NtPathName.Length + sizeof(UNICODE_NULL);
    NtPathName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NtPathName.MaximumLength * sizeof(WCHAR));
    RtlCopyMemory(NtPathName.Buffer, Buffer, NtPathName.Length);

    /* Attempt to open the volume */
    InitializeObjectAttributes(&ObjectAttributes, &NtPathVolumeName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Query the device name - that's what we'll translate */
    if (!DeviceIoControl(VolumeHandle, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL,
                         0, &MountDevName, sizeof(MountDevName), &BytesReturned,
                         NULL))
    {
        NtClose(VolumeHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);
        return FALSE;
    }

    /* No longer need the volume */
    NtClose(VolumeHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);

    /* We'll keep the device name for later usage */
    NtPathVolumeName.Length = MountDevName.NameLength;
    NtPathVolumeName.MaximumLength = MountDevName.NameLength + sizeof(UNICODE_NULL);
    NtPathVolumeName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.MaximumLength);
    if (NtPathVolumeName.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlCopyMemory(NtPathVolumeName.Buffer, MountDevName.Name, NtPathVolumeName.Length);
    NtPathVolumeName.Buffer[NtPathVolumeName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Drop the trailing backslash from both */
    //NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    //NtPathName.Length -= sizeof(WCHAR);
    //NtPathVolumeName.Buffer[(NtPathVolumeName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    //NtPathVolumeName.Length -= sizeof(WCHAR);

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                 NtPathVolumeName.Length + NtPathName.Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT));
    if (MountPoint == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* 0 everything, we provide a device name */
    RtlZeroMemory(MountPoint, sizeof(MOUNTMGR_CREATE_POINT_INPUT));
    MountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    MountPoint->SymbolicLinkNameLength = NtPathName.Length;
    MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + MountPoint->SymbolicLinkNameLength;
    MountPoint->DeviceNameLength = NtPathVolumeName.Length;
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_CREATE_POINT_INPUT)), NtPathName.Buffer, NtPathName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_CREATE_POINT_INPUT) + MountPoint->SymbolicLinkNameLength), NtPathVolumeName.Buffer, NtPathVolumeName.Length);

    /* Free device name buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        return FALSE;
    }

    /* Query the names associated to our device name */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_CREATE_POINT,
                          MountPoint, NtPathName.Length + NtPathVolumeName.Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT),
                          NULL, 0, &BytesReturned,
                          NULL);

    /* We're done, no longer need the mount manager */
    CloseHandle(MountMgrHandle);
    /* Nor our input buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);

    /* If the mount manager failed, just quit */
    if (!Ret)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    return TRUE;
}

static BOOL
NotifyMountMgr(_In_ LPCWSTR lpszMountPoint,
               _In_ LPCWSTR lpszVolumeName,
               _In_ BOOL MountPointDeleted)
{
    PMOUNTMGR_VOLUME_MOUNT_POINT MountPoint;
    UNICODE_STRING NtPathName;
    UNICODE_STRING NtPathVolumeName;
    DWORD BytesReturned;
    HANDLE MountMgrHandle;
    BOOL Ret;

    /* It makes no sense on a non-local drive */
    if (GetDriveTypeW(lpszMountPoint) == DRIVE_REMOTE)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Get the NT path for mount point */
    if (!RtlDosPathNameToNtPathName_U(lpszMountPoint, &NtPathName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Get the NT path for volume */
    if (!RtlDosPathNameToNtPathName_U(lpszVolumeName, &NtPathVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Drop the trailing backslash from both */
    NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    NtPathName.Length -= sizeof(WCHAR);
    NtPathVolumeName.Buffer[(NtPathVolumeName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    NtPathVolumeName.Length -= sizeof(WCHAR);

    /* If that's a DOS volume, upper case the letter */
    if (NtPathName.Length >= 2 * sizeof(WCHAR))
    {
        if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] == L':')
        {
            NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2] = towupper(NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 2]);
        }
    }

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                 NtPathVolumeName.Length + NtPathName.Length + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT));
    if (MountPoint == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* 0 everything, we provide a device name */
    MountPoint->SourceVolumeNameOffset = sizeof(MOUNTMGR_VOLUME_MOUNT_POINT);
    MountPoint->SourceVolumeNameLength = NtPathName.Length;
    MountPoint->TargetVolumeNameOffset = sizeof(MOUNTMGR_VOLUME_MOUNT_POINT) + NtPathName.Length;
    MountPoint->TargetVolumeNameLength = NtPathVolumeName.Length;
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT)), NtPathName.Buffer, NtPathName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT) + NtPathName.Length), NtPathVolumeName.Buffer, NtPathVolumeName.Length);

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        return FALSE;
    }

    /* Query the names associated to our device name */
    Ret = DeviceIoControl(MountMgrHandle,
                          MountPointDeleted ? IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED :
                          IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED,
                          MountPoint, NtPathName.Length + NtPathVolumeName.Length + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT),
                          NULL, 0, &BytesReturned,
                          NULL);

    /* We're done, no longer need the mount manager */
    CloseHandle(MountMgrHandle);
    /* Nor our input buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);

    /* If the mount manager failed, just quit */
    if (!Ret)
    {
        //SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
BasepGetVolumeNameFromReparsePoint(IN LPCWSTR lpszMountPoint,
                                   OUT LPWSTR lpszVolumeName,
                                   IN DWORD cchBufferLength,
                                   OUT LPBOOL IsAMountPoint)
{
    WCHAR Old;
    DWORD BytesReturned;
    HANDLE ReparseHandle;
    UNICODE_STRING SubstituteName;
    PREPARSE_DATA_BUFFER ReparseBuffer;

    /* Try to open the reparse point */
    ReparseHandle = CreateFileW(lpszMountPoint, 0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL,
                                INVALID_HANDLE_VALUE);
    /* It failed! */
    if (ReparseHandle == INVALID_HANDLE_VALUE)
    {
        /* Report it's not a mount point (it's not a reparse point) */
        if (IsAMountPoint != NULL)
        {
            *IsAMountPoint = FALSE;
        }

        /* And zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* This is a mount point! */
    if (IsAMountPoint != NULL)
    {
        *IsAMountPoint = TRUE;
    }

    /* Prepare a buffer big enough to read its data */
    ReparseBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (ReparseBuffer == NULL)
    {
        CloseHandle(ReparseHandle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        /* Zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* Dump the reparse point data */
    if (!DeviceIoControl(ReparseHandle, FSCTL_GET_REPARSE_POINT, NULL, 0,
                         ReparseBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &BytesReturned,
                         NULL))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseBuffer);
        CloseHandle(ReparseHandle);

        /* Zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* We no longer need the reparse point */
    CloseHandle(ReparseHandle);

    /* We only handle mount points */
    if (ReparseBuffer->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseBuffer);

        /* Zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* Do we have enough room for copying substitue name? */
    if ((ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof(UNICODE_NULL)) > cchBufferLength * sizeof(WCHAR))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseBuffer);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);

        /* Zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* Copy the link target */
    RtlCopyMemory(lpszVolumeName,
                  &ReparseBuffer->MountPointReparseBuffer.PathBuffer[ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)],
                  ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength);
    /* Make it DOS valid */
    Old = lpszVolumeName[1];
    /* We want a root path */
    lpszVolumeName[1] = L'\\';
    /* And null terminate obviously */
    lpszVolumeName[ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    /* Make it a string to easily check it */
    SubstituteName.Length = ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength;
    SubstituteName.MaximumLength = SubstituteName.Length;
    SubstituteName.Buffer = lpszVolumeName;

    /* No longer need the data? */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseBuffer);

    /* Is that a dos volume name with backslash? */
    if (MOUNTMGR_IS_DOS_VOLUME_NAME_WB(&SubstituteName))
    {
        return TRUE;
    }

    /* No, so restore previous name and return to the caller */
    lpszVolumeName[1] = Old;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
BasepGetVolumeNameForVolumeMountPoint(IN LPCWSTR lpszMountPoint,
                                      OUT LPWSTR lpszVolumeName,
                                      IN DWORD cchBufferLength,
                                      OUT LPBOOL IsAMountPoint)
{
    BOOL Ret;
    UNICODE_STRING MountPoint;

    /* Assume it's a mount point (likely for non reparse points) */
    if (IsAMountPoint != NULL)
    {
        *IsAMountPoint = 1;
    }

    /* Make a string with the mount point name */
    RtlInitUnicodeString(&MountPoint, lpszMountPoint);
    /* Not a root path? */
    if (MountPoint.Buffer[(MountPoint.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        /* Zero output */
        if (lpszVolumeName != NULL && cchBufferLength >= 1)
        {
            lpszVolumeName[0] = UNICODE_NULL;
        }

        return FALSE;
    }

    /* Does it look like <letter>:\? */
    if (MountPoint.Length == 3 * sizeof(WCHAR))
    {
        /* Try to get volume name for root path */
        Ret = GetDeleteVolumeRoot(lpszMountPoint, lpszVolumeName, cchBufferLength, FALSE);
        /* It failed? */
        if (!Ret)
        {
            /* If wasn't a drive letter, so maybe a reparse point? */
            if (MountPoint.Buffer[1] != ':')
            {
                Ret = BasepGetVolumeNameFromReparsePoint(lpszMountPoint, lpszVolumeName, cchBufferLength, IsAMountPoint);
            }
            /* It was, so zero output */
            else if (lpszVolumeName != NULL && cchBufferLength >= 1)
            {
                lpszVolumeName[0] = UNICODE_NULL;
            }
        }
    }
    else
    {
        /* Try to get volume name for root path */
        Ret = GetDeleteVolumeRoot(lpszMountPoint, lpszVolumeName, cchBufferLength, FALSE);
        /* It failed? */
        if (!Ret)
        {
            /* It was a DOS volume as UNC name, so fail and zero output */
            if (MountPoint.Length == 14 && MountPoint.Buffer[0] == '\\' && MountPoint.Buffer[1] == '\\' &&
                (MountPoint.Buffer[2] == '.' || MountPoint.Buffer[2] == '?') && MountPoint.Buffer[3] == L'\\' &&
                MountPoint.Buffer[5] == ':')
            {
                if (lpszVolumeName != NULL && cchBufferLength >= 1)
                {
                    lpszVolumeName[0] = UNICODE_NULL;
                }
            }
            /* Maybe it's a reparse point? */
            else
            {
                Ret = BasepGetVolumeNameFromReparsePoint(lpszMountPoint, lpszVolumeName, cchBufferLength, IsAMountPoint);
            }
        }
    }

    return Ret;
}

/**
 * @name GetVolumeNameForVolumeMountPointW
 * @implemented
 *
 * Return an unique volume name for a drive root or mount point.
 *
 * @param VolumeMountPoint
 *        Pointer to string that contains either root drive name or
 *        mount point name.
 * @param VolumeName
 *        Pointer to buffer that is filled with resulting unique
 *        volume name on success.
 * @param VolumeNameLength
 *        Size of VolumeName buffer in TCHARs.
 *
 * @return
 *     TRUE when the function succeeds and the VolumeName buffer is filled,
 *     FALSE otherwise.
 */
BOOL
WINAPI
GetVolumeNameForVolumeMountPointW(IN LPCWSTR VolumeMountPoint,
                                  OUT LPWSTR VolumeName,
                                  IN DWORD VolumeNameLength)
{
    BOOL Ret;

    /* Just query our internal function */
    Ret = BasepGetVolumeNameForVolumeMountPoint(VolumeMountPoint, VolumeName,
                                                VolumeNameLength, NULL);
    if (!Ret && VolumeName != NULL && VolumeNameLength >= 1)
    {
        VolumeName[0] = UNICODE_NULL;
    }

    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint,
                                  IN LPSTR lpszVolumeName,
                                  IN DWORD cchBufferLength)
{
    BOOL Ret;
    ANSI_STRING VolumeName;
    UNICODE_STRING VolumeNameU;
    PUNICODE_STRING VolumeMountPointU;

    /* Convert mount point to unicode */
    VolumeMountPointU = Basep8BitStringToStaticUnicodeString(lpszVolumeMountPoint);
    if (VolumeMountPointU == NULL)
    {
        return FALSE;
    }

    /* Initialize the strings we'll use for convention */
    VolumeName.Buffer = lpszVolumeName;
    VolumeName.Length = 0;
    VolumeName.MaximumLength = cchBufferLength - 1;

    VolumeNameU.Length = 0;
    VolumeNameU.MaximumLength = (cchBufferLength - 1) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    /* Allocate a buffer big enough to contain the returned name */
    VolumeNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumeNameU.MaximumLength);
    if (VolumeNameU.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Query -W */
    Ret = GetVolumeNameForVolumeMountPointW(VolumeMountPointU->Buffer, VolumeNameU.Buffer, cchBufferLength);
    /* If it succeed, perform -A conversion */
    if (Ret)
    {
        NTSTATUS Status;

        /* Reinit our string for length */
        RtlInitUnicodeString(&VolumeNameU, VolumeNameU.Buffer);
        /* Convert to ANSI */
        Status = RtlUnicodeStringToAnsiString(&VolumeName, &VolumeNameU, FALSE);
        /* If conversion failed, force failure, otherwise, just null terminate */
        if (!NT_SUCCESS(Status))
        {
            Ret = FALSE;
            BaseSetLastNTError(Status);
        }
        else
        {
            VolumeName.Buffer[VolumeName.Length] = ANSI_NULL;
        }
    }

    /* Internal buffer no longer needed */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeNameU.Buffer);

    return Ret;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointW(IN LPCWSTR lpszVolumeMountPoint,
                     IN LPCWSTR lpszVolumeName)
{
    BOOL Ret;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    DWORD BytesReturned;
    //PWSTR SubstituteName;
    HANDLE VolumeHandle;
    WCHAR VolumeName[64];
    UNICODE_STRING NtPathVolumeName;
    DWORD DataSize;

    /* Sanity checks */
    if (!lpszVolumeMountPoint || !lpszVolumeMountPoint)
    {
        return FALSE;
    }

    /* If this is mount point is already used, we will get a volume */
    if (BasepGetVolumeNameForVolumeMountPoint(lpszVolumeMountPoint, VolumeName, ARRAYSIZE(VolumeName), NULL))
    {
        SetLastError(ERROR_DIR_NOT_EMPTY);
        return FALSE;
    }

    /* Make sure we got a proper volume name */
    if (!IS_VOLUME_NAME(lpszVolumeName, wcslen(lpszVolumeName) * sizeof(WCHAR)))
    {
        return FALSE;
    }

    /* Check if this is a request for mounted folder */
    if (wcslen(lpszVolumeMountPoint) == 3)
    {
        return SetVolumeNameForRoot(lpszVolumeMountPoint, lpszVolumeName);
    }

    /* Open a handle to the volume */
    VolumeHandle = CreateFileW(lpszVolumeMountPoint,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                               INVALID_HANDLE_VALUE);
    if (VolumeHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    /* Get the NT path */
    if (!RtlDosPathNameToNtPathName_U(lpszVolumeName, &NtPathVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    DataSize = (DWORD)(FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer)
               + NtPathVolumeName.MaximumLength
               + (wcslen(lpszVolumeName) * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

    ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, DataSize);

    if (!ReparseDataBuffer)
    {
        CloseHandle(VolumeHandle);
        return FALSE;
    }

    ReparseDataBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    ReparseDataBuffer->ReparseDataLength = DataSize - FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer);
    ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset = 0;
    ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength = NtPathVolumeName.Length;
    ReparseDataBuffer->MountPointReparseBuffer.PrintNameOffset = NtPathVolumeName.MaximumLength;
    ReparseDataBuffer->MountPointReparseBuffer.PrintNameLength = wcslen(lpszVolumeName) * sizeof(WCHAR);
    RtlCopyMemory(ReparseDataBuffer->MountPointReparseBuffer.PathBuffer, NtPathVolumeName.Buffer, NtPathVolumeName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer + NtPathVolumeName.MaximumLength), lpszVolumeName, wcslen(lpszVolumeName) * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    /* This reparse point is a mounted folder, remove it then */
    Ret = DeviceIoControl(VolumeHandle, FSCTL_SET_REPARSE_POINT,
                          ReparseDataBuffer, DataSize,
                          NULL, 0, &BytesReturned,
                          NULL);
    if (Ret)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        CloseHandle(VolumeHandle);
        return NotifyMountMgr(lpszVolumeMountPoint, lpszVolumeName, FALSE);
        //return TRUE;
    }

    /* We no longer need the volume handle */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
    CloseHandle(VolumeHandle);

    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint,
                     IN LPCSTR lpszVolumeName)
{
    NTSTATUS Status;
    UNICODE_STRING VolumeNameU;
    UNICODE_STRING VolumeMountPointU;
    ANSI_STRING VolumeNameA;
    ANSI_STRING VolumeMountPointA;

    RtlInitAnsiString(&VolumeNameA, lpszVolumeName);
    RtlInitAnsiString(&VolumeMountPointA, lpszVolumeMountPoint);

    Status = RtlAnsiStringToUnicodeString(&VolumeNameU, &VolumeNameA, TRUE);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    Status = RtlAnsiStringToUnicodeString(&VolumeMountPointU, &VolumeMountPointA, TRUE);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Invoke unicode variant */
    return SetVolumeMountPointW(VolumeMountPointU.Buffer, VolumeNameU.Buffer);
    
    RtlFreeUnicodeString(&VolumeNameU);
    RtlFreeUnicodeString(&VolumeMountPointU);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint)
{
    NTSTATUS Status;
    ANSI_STRING VolumeMountPointA;
    UNICODE_STRING VolumeMountPointU;

    /* Convert mount point to unicode */
    Status = RtlAnsiStringToUnicodeString(&VolumeMountPointU, &VolumeMountPointA, TRUE);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Invoke unicode variant */
    return DeleteVolumeMountPointW(VolumeMountPointU.Buffer);
    
    RtlFreeUnicodeString(&VolumeMountPointU);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointW(IN LPCWSTR lpszVolumeMountPoint)
{
    BOOL Ret;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    DWORD FileAttributes;
    DWORD BytesReturned;
    PWSTR SubstituteName;
    HANDLE VolumeHandle;
    WCHAR VolumeName[64];

    /* If this any mount point, we should be able to get a volume name for it */
    if (!BasepGetVolumeNameForVolumeMountPoint(lpszVolumeMountPoint, VolumeName, ARRAYSIZE(VolumeName), NULL))
    {
        return FALSE;
    }

    /* Check if this is a reparse point */
    FileAttributes = GetFileAttributesW(lpszVolumeMountPoint);
    if (!(FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
         (FileAttributes == INVALID_FILE_ATTRIBUTES))
    {
        /* Not a reparse point, let's just delete it now then */
        return GetDeleteVolumeRoot(lpszVolumeMountPoint, NULL, 0, TRUE);
    }

    /* Open a handle to the volume */
    VolumeHandle = CreateFileW(lpszVolumeMountPoint,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                               INVALID_HANDLE_VALUE);
    if (VolumeHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    if (!ReparseDataBuffer)
    {
        CloseHandle(VolumeHandle);
        return FALSE;
    }

    /* Get the reparse point data */
    Ret = DeviceIoControl(VolumeHandle, FSCTL_GET_REPARSE_POINT,
                          NULL, 0,
                          ReparseDataBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &BytesReturned,
                          NULL);
    if (!Ret)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        CloseHandle(VolumeHandle);
        return FALSE;
    }

    /* Get volume name */
    SubstituteName = (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                             ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset);
    if (!IS_VOLUME_NAME(SubstituteName, ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength))
    {
        /* Not a volume, do nothing then */
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        CloseHandle(VolumeHandle);
        return FALSE;
    }

    ReparseDataBuffer->ReparseDataLength = 0;

    /* This reparse point is a mounted folder, remove it then */
    Ret = DeviceIoControl(VolumeHandle, FSCTL_DELETE_REPARSE_POINT,
                          ReparseDataBuffer, FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer),
                          NULL, 0, &BytesReturned,
                          NULL);
    if (Ret)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        CloseHandle(VolumeHandle);
        return NotifyMountMgr(lpszVolumeMountPoint, VolumeName, TRUE);
        //return TRUE;
    }

    /* We no longer need the volume handle */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
    CloseHandle(VolumeHandle);

    return FALSE;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointW(IN LPCWSTR lpszRootPathName,
                           IN LPWSTR lpszVolumeMountPoint,
                           IN DWORD cchBufferLength)
{
    UNICODE_STRING RootPathName;
    UNICODE_STRING ReparseIndex = RTL_CONSTANT_STRING(L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
    UNICODE_STRING ReparseFile;
    UNICODE_STRING Reference;
    DWORD BytesReturned;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    ULONG PathLength;
    HANDLE VolmeMountPointHandle, VolumeHandle = 0;
    PFILE_REPARSE_POINT_INFORMATION FileReparseInformation;
    PFILE_NAME_INFORMATION FileInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR SubstituteName;
    NTSTATUS Status;
    NTSTATUS Status2;
    BOOL Ret;
    __debugbreak();
    /* Sanity check */
    if (!lpszRootPathName)
    {
        return INVALID_HANDLE_VALUE;
    }
    WCHAR slashhh = lpszRootPathName[wcslen(lpszRootPathName) - 1];
    /* We don't accept lack of trailing backslash until now */
    if (slashhh != L'\\')
    {
        return INVALID_HANDLE_VALUE;
    }

    /* It makes no sense on a non-local drive */
    if (GetDriveTypeW(lpszRootPathName) == DRIVE_REMOTE)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    RtlInitUnicodeString(&RootPathName, lpszRootPathName);
    PathLength = RootPathName.Length + ReparseIndex.Length + sizeof(UNICODE_NULL);
    if (PathLength > MAX_NTFS_PATH)
    {
        return INVALID_HANDLE_VALUE;
    }

    ReparseFile.Length = 0;
    ReparseFile.MaximumLength = PathLength;
    ReparseFile.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength);
    
    if (ReparseFile.Buffer == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    RtlCopyUnicodeString(&ReparseFile, &RootPathName);
    RtlAppendUnicodeStringToString(&ReparseFile, &ReparseIndex);
    ReparseFile.Buffer[ReparseFile.Length] = UNICODE_NULL;
    VolmeMountPointHandle = CreateFileW(ReparseFile.Buffer, GENERIC_READ, FILE_SHARE_READ,
                                        0, OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_HIDDEN,
                                        INVALID_HANDLE_VALUE);
    RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseFile.Buffer);

    if (VolmeMountPointHandle == INVALID_HANDLE_VALUE)
    {
        return VolmeMountPointHandle;
    }

    do
    {
        FileReparseInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*FileReparseInformation));
        Status = NtQueryDirectoryFile(VolmeMountPointHandle, NULL, NULL, NULL,
                                      &IoStatusBlock,
                                      FileReparseInformation,
                                      sizeof(*FileReparseInformation),
                                      FileReparsePointInformation,
                                      TRUE,
                                      NULL,
                                      FALSE);

        if (FileReparseInformation->Tag != IO_REPARSE_TAG_MOUNT_POINT)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FileReparseInformation);
            continue;
        }
        Reference.Length = Reference.MaximumLength = sizeof(FileReparseInformation->FileReference);
        Reference.Buffer = (PWSTR)&(FileReparseInformation->FileReference);
        InitializeObjectAttributes(&ObjectAttributes, &Reference,
                                0, VolmeMountPointHandle, NULL);
        Status2 = NtOpenFile(&VolumeHandle, FILE_GENERIC_READ,
                &ObjectAttributes, &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_BY_FILE_ID);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileReparseInformation);
        ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

        if (!ReparseDataBuffer)
        {
            CloseHandle(VolmeMountPointHandle);
            CloseHandle(VolumeHandle);
            return INVALID_HANDLE_VALUE;
        }

        /* Get the reparse point data */
        Ret = DeviceIoControl(VolumeHandle, FSCTL_GET_REPARSE_POINT,
                            NULL, 0,
                            ReparseDataBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &BytesReturned,
                            NULL);
    
        if (!Ret)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            CloseHandle(VolumeHandle);
            continue;
        }

        /* Get volume name */
        SubstituteName = (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                                ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset);
        if (IS_VOLUME_NAME(SubstituteName, ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength))
        {
            /* Not a volume, do nothing then */
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            break;
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);

    } while (Status != STATUS_NO_MORE_FILES);

    if (VolumeHandle == 0)
    {
        CloseHandle(VolmeMountPointHandle);
        return INVALID_HANDLE_VALUE;
    }

    FileInformation = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    if (!FileNameInformation)
    {
        CloseHandle(VolmeMountPointHandle);
        CloseHandle(VolumeHandle);
        return INVALID_HANDLE_VALUE;
    }

    Status = NtQueryInformationFile(VolumeHandle, &IoStatusBlock, FileInformation, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL), FileNameInformation);

    if(!NT_SUCCESS(Status))
    {
        CloseHandle(VolumeHandle);
        CloseHandle(VolmeMountPointHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);
        return INVALID_HANDLE_VALUE;
    }
    swprintf(lpszVolumeMountPoint, L"%s\\", &FileInformation->FileName[1]);
    CloseHandle(VolumeHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);

    return VolmeMountPointHandle;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointA(IN LPCSTR lpszRootPathName,
                           IN LPSTR lpszVolumeMountPoint,
                           IN DWORD cchBufferLength)
{
    HANDLE VolmeMountPointHandle;
    ANSI_STRING VolumeMountPoint;
    UNICODE_STRING VolumeMountPointU;
    PUNICODE_STRING RootPathNameU;

    /* Convert mount point to unicode */
    RootPathNameU = Basep8BitStringToStaticUnicodeString(lpszRootPathName);
    if (RootPathNameU == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    /* Initialize the strings we'll use for convention */
    VolumeMountPoint.Buffer = lpszVolumeMountPoint;
    VolumeMountPoint.Length = 0;
    VolumeMountPoint.MaximumLength = cchBufferLength - 1;

    VolumeMountPointU.Length = 0;
    VolumeMountPointU.MaximumLength = (cchBufferLength - 1) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    /* Allocate a buffer big enough to contain the returned name */
    VolumeMountPointU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumeMountPointU.MaximumLength);
    if (VolumeMountPointU.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    /* Query -W */
    VolmeMountPointHandle = FindFirstVolumeMountPointW(RootPathNameU->Buffer, VolumeMountPointU.Buffer, cchBufferLength);
    /* If it succeed, perform -A conversion */
    if (VolmeMountPointHandle != INVALID_HANDLE_VALUE)
    {
        NTSTATUS Status;

        /* Reinit our string for length */
        RtlInitUnicodeString(&VolumeMountPointU, VolumeMountPointU.Buffer);
        /* Convert to ANSI */
        Status = RtlUnicodeStringToAnsiString(&VolumeMountPoint, &VolumeMountPointU, FALSE);
        /* If conversion failed, force failure, otherwise, just null terminate */
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            CloseHandle(VolmeMountPointHandle);
            return INVALID_HANDLE_VALUE;
        }
        else
        {
            VolumeMountPoint.Buffer[VolumeMountPoint.Length] = ANSI_NULL;
        }
    }

    /* Internal buffer no longer needed */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeMountPointU.Buffer);

    return VolmeMountPointHandle;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointA(IN HANDLE hFindVolumeMountPoint,
                          IN LPSTR lpszVolumeMountPoint,
                          DWORD cchBufferLength)
{
    BOOL Ret;
    ANSI_STRING VolumeMountPoint;
    UNICODE_STRING VolumeMountPointU;

    /* Initialize the strings we'll use for convention */
    VolumeMountPoint.Buffer = lpszVolumeMountPoint;
    VolumeMountPoint.Length = 0;
    VolumeMountPoint.MaximumLength = cchBufferLength - 1;

    VolumeMountPointU.Length = 0;
    VolumeMountPointU.MaximumLength = (cchBufferLength - 1) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    /* Allocate a buffer big enough to contain the returned name */
    VolumeMountPointU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumeMountPointU.MaximumLength);
    if (VolumeMountPointU.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Query -W */
    Ret = FindNextVolumeMountPointW(hFindVolumeMountPoint, VolumeMountPointU.Buffer, cchBufferLength);
    /* If it succeed, perform -A conversion */
    if (Ret)
    {
        NTSTATUS Status;

        /* Reinit our string for length */
        RtlInitUnicodeString(&VolumeMountPointU, VolumeMountPointU.Buffer);
        /* Convert to ANSI */
        Status = RtlUnicodeStringToAnsiString(&VolumeMountPoint, &VolumeMountPointU, FALSE);
        /* If conversion failed, force failure, otherwise, just null terminate */
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            Ret = FALSE;
        }
        else
        {
            VolumeMountPoint.Buffer[VolumeMountPoint.Length] = ANSI_NULL;
        }
    }

    /* Internal buffer no longer needed */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeMountPointU.Buffer);

    return Ret;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointW(IN HANDLE hFindVolumeMountPoint,
                          IN LPWSTR lpszVolumeMountPoint,
                          DWORD cchBufferLength)
{
    HANDLE VolumeHandle = 0;
    PFILE_REPARSE_POINT_INFORMATION FileReparseInformation;
    PFILE_NAME_INFORMATION FileInformation;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING Reference;
    PWSTR SubstituteName;
    DWORD BytesReturned;
    NTSTATUS Status;
    NTSTATUS Status2;
    BOOL Ret;
    __debugbreak();
    do
    {
        FileReparseInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*FileReparseInformation));
        Status = NtQueryDirectoryFile(hFindVolumeMountPoint, NULL, NULL, NULL,
                                      &IoStatusBlock,
                                      FileReparseInformation,
                                      sizeof(*FileReparseInformation),
                                      FileReparsePointInformation,
                                      TRUE,
                                      NULL,
                                      FALSE);

        if (FileReparseInformation->Tag != IO_REPARSE_TAG_MOUNT_POINT)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FileReparseInformation);
            continue;
        }
        Reference.Length = Reference.MaximumLength = sizeof(FileReparseInformation->FileReference);
        Reference.Buffer = (PWSTR)&(FileReparseInformation->FileReference);
        InitializeObjectAttributes(&ObjectAttributes, &Reference,
                                0, hFindVolumeMountPoint, NULL);
        Status2 = NtOpenFile(&VolumeHandle, FILE_GENERIC_READ,
                &ObjectAttributes, &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_BY_FILE_ID);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileReparseInformation);
        ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

        if (!ReparseDataBuffer)
        {
            CloseHandle(VolumeHandle);
            return FALSE;
        }

        /* Get the reparse point data */
        Ret = DeviceIoControl(VolumeHandle, FSCTL_GET_REPARSE_POINT,
                            NULL, 0,
                            ReparseDataBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &BytesReturned,
                            NULL);
    
        if (!Ret)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            CloseHandle(VolumeHandle);
            continue;
        }

        /* Get volume name */
        SubstituteName = (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                                ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset);
        if (IS_VOLUME_NAME(SubstituteName, ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength))
        {
            /* Not a volume, do nothing then */
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            break;
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);

    } while (Status != STATUS_NO_MORE_FILES);

    if (VolumeHandle == 0)
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    FileInformation = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    Status = NtQueryInformationFile(VolumeHandle, &IoStatusBlock, FileInformation, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL), FileNameInformation);

    if(!NT_SUCCESS(Status))
    {
        CloseHandle(VolumeHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);
        BaseSetLastNTError(Status);
    }
    swprintf(lpszVolumeMountPoint, L"%s\\", &FileInformation->FileName[1]);
    CloseHandle(VolumeHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindVolumeMountPointClose(IN HANDLE hFindVolumeMountPoint)
{
    return CloseHandle(hFindVolumeMountPoint);
}

/* EOF */
