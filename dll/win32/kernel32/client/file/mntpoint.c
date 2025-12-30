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

typedef struct _FILE_REPARSE_POINT_INFORMATION {
  LONGLONG FileReference;
  ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

/*
 * @implemented
 */
static BOOL
GetVolumeNameForRoot(IN LPCWSTR lpszRootPath,
                     OUT LPWSTR lpszVolumeName,
                     IN DWORD cchBufferLength)
{
    BOOL Ret;
    NTSTATUS Status;
    PWSTR FoundVolume;
    DWORD BytesReturned;
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

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                 NtPathName.Length + sizeof(MOUNTMGR_MOUNT_POINT));
    if (MountPoint == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* 0 everything, we provide a device name */
    RtlZeroMemory(MountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    MountPoint->DeviceNameLength = NtPathName.Length;
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_MOUNT_POINT)), NtPathName.Buffer, NtPathName.Length);

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
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0,
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
    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
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
        Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
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

/**
 * @brief
 * Deletes the drive letter type of mount point. This routine queries the Mount Manager
 * for deleting the requested drive letter mount point.
 *
 * @param[in] lpszRootPath
 * String which contains a drive letter mount point to be deleted.
 *
 * @return
 * TRUE if the mount point deletion is successful, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 */
static BOOL
DeleteVolumeRoot(_In_ LPCWSTR lpszRootPath)
{
    BOOL Ret;
    DWORD BytesReturned;
    UNICODE_STRING DosDevices = RTL_CONSTANT_STRING(L"\\DosDevices\\");
    UNICODE_STRING RootPath;
    ULONG PathLength;
    UNICODE_STRING DosDevicePath;
    PMOUNTMGR_MOUNT_POINT MountPoint;
    PMOUNTMGR_MOUNT_POINTS MountPoints;
    HANDLE MountMgrHandle;

    RtlInitUnicodeString(&RootPath, lpszRootPath);

    /* Drop the trailing backslash */
    if (RootPath.Buffer[(RootPath.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        RootPath.Length -= sizeof(WCHAR);
    }

    PathLength = RootPath.Length + DosDevices.Length + sizeof(UNICODE_NULL);
    DosDevicePath.Length = 0;
    DosDevicePath.MaximumLength = PathLength;
    DosDevicePath.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength);

    if (DosDevicePath.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlCopyUnicodeString(&DosDevicePath, &DosDevices);
    RtlAppendUnicodeStringToString(&DosDevicePath, &RootPath);
    DosDevicePath.Buffer[(DosDevicePath.Length / sizeof(WCHAR)) - 2] = towupper(DosDevicePath.Buffer[(DosDevicePath.Length / sizeof(WCHAR)) - 2]);
    DosDevicePath.Buffer[DosDevicePath.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                 DosDevicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT));
    if (MountPoint == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    MountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    MountPoint->SymbolicLinkNameLength = DosDevicePath.Length;
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_MOUNT_POINT)), DosDevicePath.Buffer, DosDevicePath.Length);

    /* Allocate a dummy output buffer to probe for size */
    MountPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(MOUNTMGR_MOUNT_POINTS));
    if (MountPoints == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Open a handle to the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
        return FALSE;
    }

    /* Query the names associated to our device name */
    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_DELETE_POINTS,
                          MountPoint, DosDevicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT),
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
            RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Reissue the request, it should work now! */
        Ret = DeviceIoControl(MountMgrHandle,
                              IOCTL_MOUNTMGR_DELETE_POINTS,
                              MountPoint, DosDevicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                              MountPoints, BufferSize, &BytesReturned, NULL);
    }

    /* We're done, no longer need the mount manager */
    CloseHandle(MountMgrHandle);
    /* Nor our input buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);

    /* If the mount manager failed, just quit */
    if (!Ret)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* We're done! */
    RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
    return TRUE;
}

/**
 * @brief
 * Creates the drive letter type of mount point. This routine queries the Mount Manager
 * for creating the requested drive letter mount point.
 *
 * @param[in] lpszRootPath
 * String which contains a drive letter mount point to be created.
 *
 * @param[in] lpszVolumeName
 * String which contains a volume GUID path for the volume to be mounted.
 *
 * @return
 * TRUE if the mount point creation is successful, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * @remarks
 * If the lpszRootPath parameter is incorrect, the routine will return ERROR_INVALID_PARAMETER
 */
static BOOL
SetVolumeRoot(_In_ LPCWSTR lpszRootPath,
              _In_ LPCWSTR lpszVolumeName)
{
    BOOL Ret;
    NTSTATUS Status;
    DWORD BytesReturned;
    UNICODE_STRING RootPath;
    ULONG PathLength;
    UNICODE_STRING DosDevicePath;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtPathVolumeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle, MountMgrHandle;
    PMOUNTMGR_CREATE_POINT_INPUT MountPoint;
    UNICODE_STRING DosDevices = RTL_CONSTANT_STRING(L"\\DosDevices\\");
    struct
    {
        MOUNTDEV_NAME;
        WCHAR Buffer[MAX_PATH];
    } MountDevName;

    RtlInitUnicodeString(&RootPath, lpszRootPath);

    /* Drop the backslash from root path */
    if (RootPath.Buffer[(RootPath.Length / sizeof(WCHAR)) - 1] == L'\\')
    {
        RootPath.Length -= sizeof(WCHAR);
    }

    /* Get the NT path for volume */
    if (!RtlDosPathNameToNtPathName_U(lpszVolumeName, &NtPathVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Now we can safely drop the trailing backslashes */
    NtPathVolumeName.Buffer[(NtPathVolumeName.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    NtPathVolumeName.Length -= sizeof(WCHAR);

    PathLength = RootPath.Length + DosDevices.Length + sizeof(UNICODE_NULL);
    DosDevicePath.Length = 0;
    DosDevicePath.MaximumLength = PathLength;
    DosDevicePath.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength);

    if (DosDevicePath.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlCopyUnicodeString(&DosDevicePath, &DosDevices);
    RtlAppendUnicodeStringToString(&DosDevicePath, &RootPath);
    DosDevicePath.Buffer[(DosDevicePath.Length / sizeof(WCHAR)) - 2] = towupper(DosDevicePath.Buffer[(DosDevicePath.Length / sizeof(WCHAR)) - 2]);
    DosDevicePath.Buffer[DosDevicePath.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Attempt to open the volume */
    InitializeObjectAttributes(&ObjectAttributes, &NtPathVolumeName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Query the device name - that's what we'll translate */
    if (!DeviceIoControl(VolumeHandle, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL,
                         0, &MountDevName, sizeof(MountDevName), &BytesReturned,
                         NULL))
    {
        NtClose(VolumeHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, DosDevicePath.Buffer);
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

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                 NtPathVolumeName.Length + DosDevicePath.Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT));
    if (MountPoint == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathVolumeName.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Initialize the mount point data buffer */
    MountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    MountPoint->SymbolicLinkNameLength = DosDevicePath.Length;
    MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + MountPoint->SymbolicLinkNameLength;
    MountPoint->DeviceNameLength = NtPathVolumeName.Length;
    RtlCopyMemory((PVOID)((ULONG_PTR)MountPoint + sizeof(MOUNTMGR_CREATE_POINT_INPUT)), DosDevicePath.Buffer, DosDevicePath.Length);
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
                          MountPoint, DosDevicePath.Length + NtPathVolumeName.Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT),
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

/**
 * @brief
 * Sends a notification about mounted folder creation or deletion to the Mount Manager.
 *
 * @param[in] lpszMountPoint
 * String which contains an absolute path to the mounted folder.
 *
 * @param[in] lpszVolumeName
 * String which contains a volume GUID name for the volume mounted via the mounted folder.
 *
 * @param[in] MountPointDeleted
 * Specifies the Mount Manager notification to be sent,
 * IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED if TRUE,
 * IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED if FALSE.
 *
 * @return
 * TRUE if the mount point creation is successful, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * @remarks
 * This routine will fail when lpszMountPoint is a folder on removable media, since Mount Manager
 * does not write to the database on removable or remote media.
 */
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

    /* Allocate the structure for querying the mount mgr */
    MountPoint = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                 NtPathVolumeName.Length + NtPathName.Length + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT));
    if (MountPoint == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Initialize the mount point data buffer */
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

    /* Send the notification to mount manager */
    Ret = DeviceIoControl(MountMgrHandle,
                          MountPointDeleted ? IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED :
                          IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED,
                          MountPoint, NtPathName.Length + NtPathVolumeName.Length + sizeof(MOUNTMGR_VOLUME_MOUNT_POINT),
                          NULL, 0, &BytesReturned,
                          NULL);

    /* Clean up */
    CloseHandle(MountMgrHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);

    /* If the mount manager failed, just quit */
    if (!Ret)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
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
        Ret = GetVolumeNameForRoot(lpszMountPoint, lpszVolumeName, cchBufferLength);
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
        Ret = GetVolumeNameForRoot(lpszMountPoint, lpszVolumeName, cchBufferLength);
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

/**
 * @brief
 * Associates a volume with a drive letter or a directory on another volume.
 *
 * @param[in] lpszVolumeMountPoint
 * String which contains the user-mode path to be associated with the volume.
 * This may be a drive letter (for example, "X:\")
 * or a directory on another volume (for example, "Y:\mounted_folder_for_X\").
 * The path to the mounted folder may be absolute or relative.
 * The string must end with a trailing backslash ('\').
 *
 * @param[in] lpszVolumeName
 * String which contains a volume GUID name for the volume
 * to be mounted via the requested mount point.
 * The string must end with a trailing backslash ('\').
 *
 * @return
 * TRUE if the function is succeeds, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * If the lpszVolumeMountPoint parameter contains a path already used for mount point,
 * GetLastError returns ERROR_DIR_NOT_EMPTY, even if the directory is empty.
 *
 * If either of parameters do not include the trailing backslash,
 * GetLastError returns ERROR_INVALID_NAME.
 *
 * If either of parameters contains an incorrect path,
 * GetLastError returns ERROR_INVALID_PARAMETER or ERROR_INVALID_NAME.
 *
 * If the lpszVolumeMountPoint parameter contains a path to the directory on filesystem which
 * does not support reparse points, GetLastError returns ERROR_NOT_SUPPORTED.
 *
 * @remarks
 * This routine depending on the mount point requested, will either associate the drive letter
 * with volume by querying the Mount Manager, or if the requested mount point is a path
 * to the directory, will setup the mount point type of reparse point on supported filesystems.
 */
BOOL
WINAPI
SetVolumeMountPointW(IN LPCWSTR lpszVolumeMountPoint,
                     IN LPCWSTR lpszVolumeName)
{
    BOOL Ret;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    DWORD BytesReturned;
    HANDLE VolumeHandle;
    WCHAR VolumeName[64];
    UNICODE_STRING NtPathVolumeName;
    DWORD DataSize;

    /* Sanity checks */
    if (!lpszVolumeMountPoint || !lpszVolumeName || wcslen(lpszVolumeMountPoint) < 1 ||
        !IS_VOLUME_NAME(lpszVolumeName, wcslen(lpszVolumeName) * sizeof(WCHAR)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Both paths need to have a trailing backslash */
    if (lpszVolumeMountPoint[wcslen(lpszVolumeMountPoint) - 1] != L'\\' ||
        lpszVolumeName[wcslen(lpszVolumeName) - 1] != L'\\')
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* If this mount point is already used, we will get a volume */
    if (BasepGetVolumeNameForVolumeMountPoint(lpszVolumeMountPoint, VolumeName, ARRAYSIZE(VolumeName), NULL))
    {
        SetLastError(ERROR_DIR_NOT_EMPTY);
        return FALSE;
    }

    /* Check if this is a request for mounted folder */
    if (lpszVolumeMountPoint[0] != L'\\' && wcslen(lpszVolumeMountPoint) < 4)
    {
        return SetVolumeRoot(lpszVolumeMountPoint, lpszVolumeName);
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
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Get the NT path */
    if (!RtlDosPathNameToNtPathName_U(lpszVolumeName, &NtPathVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DataSize = (DWORD)(FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer)
               + NtPathVolumeName.MaximumLength
               + (wcslen(lpszVolumeName) * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

    ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, DataSize);

    if (!ReparseDataBuffer)
    {
        CloseHandle(VolumeHandle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Initialize reparse data buffer */
    ReparseDataBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    ReparseDataBuffer->ReparseDataLength = DataSize - FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer);
    ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset = 0;
    ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength = NtPathVolumeName.Length;
    ReparseDataBuffer->MountPointReparseBuffer.PrintNameOffset = NtPathVolumeName.MaximumLength;
    ReparseDataBuffer->MountPointReparseBuffer.PrintNameLength = (USHORT)(wcslen(lpszVolumeName) * sizeof(WCHAR));
    RtlCopyMemory(ReparseDataBuffer->MountPointReparseBuffer.PathBuffer, NtPathVolumeName.Buffer, NtPathVolumeName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer + NtPathVolumeName.MaximumLength), lpszVolumeName, wcslen(lpszVolumeName) * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    /* Create the folder mount */
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

    /* Sanity checks */
    if (!lpszVolumeMountPoint || !lpszVolumeName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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

    /* Sanity checks */
    if (!lpszVolumeMountPoint)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlInitAnsiString(&VolumeMountPointA, lpszVolumeMountPoint);

    /* Convert mount point to unicode */
    Status = RtlAnsiStringToUnicodeString(&VolumeMountPointU, &VolumeMountPointA, TRUE);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Invoke unicode variant */
    return DeleteVolumeMountPointW(VolumeMountPointU.Buffer);

    RtlFreeUnicodeString(&VolumeMountPointU);
}

/**
 * @brief
 * Deletes a drive letter or a reparse point.
 *
 * @param[in] lpszVolumeMountPoint
 * String which contains the user-mode path to the volume mount point.
 * This may be a drive letter (for example, "X:\")
 * or a directory on another volume (for example, "Y:\mounted_folder_for_X\").
 * The path to the mounted folder may be absolute or relative.
 * The string must end with a trailing backslash ('\').
 *
 * @return
 * TRUE if the function is succeeds, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * If the lpszVolumeMountPoint parameter contains a path to the directory
 * which is not a mounted folder, GetLastError returns ERROR_NOT_A_REPARSE_POINT
 *
 * If the lpszVolumeMountPoint parameter do not include the trailing backslash,
 * GetLastError returns ERROR_INVALID_NAME.
 *
 * If either of parameters contains an incorrect path,
 * GetLastError returns ERROR_INVALID_NAME, ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND.
 *
 * @remarks
 * This routine depending on the mount point requested, will either adelete the drive letter
 * and unassociate it with volume by querying the Mount Manager, or if the mount point
 * requested for removal is a path to the directory, will remove the reparse point.
 * Note that this routine will not remove the actual directory under any circumstances.
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

    /* Sanity checks */
    if (!lpszVolumeMountPoint)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (wcslen(lpszVolumeMountPoint) < 1)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* If this any mount point, we should be able to get a volume name for it */
    if (!BasepGetVolumeNameForVolumeMountPoint(lpszVolumeMountPoint, VolumeName, ARRAYSIZE(VolumeName), NULL))
    {
        //return FALSE; Perhaps this is wrong
    }

    /* Check if this is a reparse point or a relative path */
    FileAttributes = GetFileAttributesW(lpszVolumeMountPoint);
    if (lpszVolumeMountPoint[0] != L'\\' &&
        (!(FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        (FileAttributes == INVALID_FILE_ATTRIBUTES)))
    {
        /* None of either, let's just delete it now then */
        return DeleteVolumeRoot(lpszVolumeMountPoint);
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
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
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
        return TRUE;
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

/**
 * @brief
 * Retrieves the name of a mounted folder and returns a mounted folder search handle
 * on the specified volume
 *
 * @param[in] lpszRootPathName
 * String which contains volume GUID path or relative path
 * for the volume to scan for mounted folders.
 * The string must end with a trailing backslash ('\').
 *
 * @param[out] lpszVolumeMountPoint
 * A pointer to a caller allocated buffer that receives the name of
 * the first mounted folder that is found.
 *
 * @param[in] cchBufferLength
 * The length of the buffer that receives the path to the mounted folder, in WCHARs.
 *
 * @return
 * A search handle which can be later used a subsequent call to the
 * FindNextVolumeMountPoint and FindVolumeMountPointClose functions.
 *
 * If the function fails to find a mounted folder on the volume, the return value is the
 * INVALID_HANDLE_VALUE error code.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * If the lpszVolumeMountPoint parameter do not include the trailing backslash,
 * GetLastError returns ERROR_INVALID_NAME.
 *
 * If either of parameters contains an incorrect path,
 * GetLastError returns ERROR_INVALID_NAME, ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND.
 *
 * @remarks
 * This routine works in a similar manner as FindFirstVolume or FindFirstFile routines.
 * The FindFirstVolumeMountPoint function opens a mounted folder search handle and returns
 * information about the first mounted folder that is found on the specified volume.
 * After the search handle is established, you can use the FindNextVolumeMountPoint function
 * to search for other mounted folders. When the search handle is no longer needed,
 * close it with the FindVolumeMountPointClose function.
 * The FindFirstVolumeMountPoint and FindNextVolumeMountPoint functions return
 * names (not full paths) of the mounted folders for a specified volume,
 * appended with a trailing backslash.
 * They do not return drive letters or volume GUID paths.
 * Note that the usage of FindFirstVolumeMountPoint and FindNextVolumeMountPoint routines is
 * supported only with NTFS volumes.
 */
HANDLE
WINAPI
FindFirstVolumeMountPointW(IN LPCWSTR lpszRootPathName,
                           IN LPWSTR lpszVolumeMountPoint,
                           IN DWORD cchBufferLength)
{
    UNICODE_STRING RootPathName;
    /* FIXME: Figure out if we can somehow fetch a list of reparse points on other filesystems such
     * as Btrfs or ext. Looks Windows drivers for those don't index the reparse points anywhere? */
    UNICODE_STRING ReparseIndex = RTL_CONSTANT_STRING(L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
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
    BOOL Ret;

    /* Sanity check */
    if (!lpszRootPathName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    /* It makes no sense on a non-local drive */
    if (GetDriveTypeW(lpszRootPathName) == DRIVE_REMOTE)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    /* We don't accept lack of trailing backslash until now */
    if (lpszRootPathName[wcslen(lpszRootPathName) - 1] != L'\\')
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }

    if (lpszRootPathName[0] != L'\\' && !IS_VOLUME_NAME(lpszRootPathName, wcslen(lpszRootPathName) * sizeof(WCHAR)))
    {
        /* Not a volume */
        SetLastError(ERROR_PATH_NOT_FOUND); /* CHECKME */
        return FALSE;
    }

    RtlInitUnicodeString(&RootPathName, lpszRootPathName);
    PathLength = RootPathName.Length + ReparseIndex.Length + sizeof(UNICODE_NULL);
    ReparseFile.Length = 0;
    ReparseFile.MaximumLength = PathLength;
    ReparseFile.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength);

    if (ReparseFile.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    RtlCopyUnicodeString(&ReparseFile, &RootPathName);
    RtlAppendUnicodeStringToString(&ReparseFile, &ReparseIndex);
    ReparseFile.Buffer[ReparseFile.Length / sizeof(WCHAR)] = UNICODE_NULL;
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
        NtOpenFile(&VolumeHandle, FILE_GENERIC_READ,
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

    } while (NT_SUCCESS(Status));

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
        if ((cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL)) < FileInformation->FileNameLength)
        {
            SetLastError(ERROR_BAD_LENGTH);
        }
        else
        {
            BaseSetLastNTError(Status);
        }
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
	NTSTATUS Status;
    HANDLE VolmeMountPointHandle;
    ANSI_STRING VolumeMountPoint;
    ANSI_STRING RootPathNameA;
    UNICODE_STRING VolumeMountPointU;
    UNICODE_STRING RootPathNameU;

    RtlInitAnsiString(&RootPathNameA, lpszRootPathName);

    /* Convert mount point to unicode */
    Status = RtlAnsiStringToUnicodeString(&RootPathNameU, &RootPathNameA, TRUE);

    if (!NT_SUCCESS(Status))
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
    VolmeMountPointHandle = FindFirstVolumeMountPointW(RootPathNameU.Buffer, VolumeMountPointU.Buffer, cchBufferLength);
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
            VolumeMountPoint.Buffer[VolumeMountPoint.Length / sizeof(CHAR)] = ANSI_NULL;
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
            VolumeMountPoint.Buffer[VolumeMountPoint.Length / sizeof(CHAR)] = ANSI_NULL;
        }
    }

    /* Internal buffer no longer needed */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeMountPointU.Buffer);

    return Ret;
}

/**
 * @brief
 * Continues a mounted folder search started by a call to the FindFirstVolumeMountPoint function.
 * FindNextVolumeMountPoint finds one mounted folder per call.
 *
 * @param[in] hFindVolumeMountPoint
 * A mounted folder search handle returned by the FindFirstVolumeMountPoint function.
 *
 * @param[out] lpszVolumeMountPoint
 * A pointer to a caller allocated buffer that receives the name of
 * the first mounted folder that is found.
 *
 * @param[in] cchBufferLength
 * The length of the buffer that receives the path to the mounted folder, in WCHARs.
 *
 * @return
 * TRUE if another mounted folder was found, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * If the lpszVolumeMountPoint parameter do not include the trailing backslash,
 * GetLastError returns ERROR_INVALID_NAME.
 *
 * If either of parameters contains an incorrect path,
 * GetLastError returns ERROR_INVALID_NAME, ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND.
 *
 * @remarks
 * This routine works in a similar manner as FindNextVolume or FindNextFile routines.
 * The FindNextVolumeMountPoint function continues the mounted folder search via the provided
 * mounted folder search handle returned by FindFirstVolumeMountPoint function and returns
 * information about another mounted folder that is found on the specified volume.
 * When the search handle is no longer needed, close it with the FindVolumeMountPointClose function.
 * The FindFirstVolumeMountPoint and FindNextVolumeMountPoint functions return
 * names (not full paths) of the mounted folders for a specified volume,
 * appended with a trailing backslash.
 * They do not return drive letters or volume GUID paths.
 * Note that the usage of FindFirstVolumeMountPoint and FindNextVolumeMountPoint routines is
 * supported only with NTFS volumes.
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
    BOOL Ret;

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
        NtOpenFile(&VolumeHandle, FILE_GENERIC_READ,
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

    } while (NT_SUCCESS(Status));

    if (VolumeHandle == 0)
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    FileInformation = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));

    Status = NtQueryInformationFile(VolumeHandle, &IoStatusBlock, FileInformation, sizeof(*FileInformation) + cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL), FileNameInformation);

    if(!NT_SUCCESS(Status))
    {
        if ((cchBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL)) < FileInformation->FileNameLength)
        {
            SetLastError(ERROR_BAD_LENGTH);
        }
        else
        {
            BaseSetLastNTError(Status);
        }
        CloseHandle(VolumeHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);
        return FALSE;
    }
    swprintf(lpszVolumeMountPoint, L"%s\\", &FileInformation->FileName[1]);
    CloseHandle(VolumeHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FileInformation);

    return TRUE;
}

/**
 * @brief
 * Closes the mounted folder search handle.
 *
 * @param[in] hFindVolumeMountPoint
 * A mounted folder search handle returned by the FindFirstVolumeMountPoint function.
 *
 * @return
 * TRUE if the function was successful, FALSE otherwise.
 *
 * To get extended error information in case of failure, call GetLastError.
 *
 * @remarks
 * After the FindVolumeMountPointClose function is called, the handle hFindVolumeMountPoint cannot
 * be used in subsequent calls to either FindNextVolumeMountPoint or FindVolumeMountPointClose.
 */
BOOL
WINAPI
FindVolumeMountPointClose(IN HANDLE hFindVolumeMountPoint)
{
    return CloseHandle(hFindVolumeMountPoint);
}

/* EOF */
