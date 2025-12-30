#include "precomp.h"

typedef struct _volume_mount_point_test_data
{
    BOOL Ret;
    UINT Err;
    WCHAR TestPathU[60];
    CHAR TestPathA[60];
} _volume_mount_point_test_data;

#define SET_VOLUME_MOUNTPOINT_TESTS_SECOND 3
#define SET_VOLUME_MOUNTPOINT_TESTS_FIRST (SET_VOLUME_MOUNTPOINT_TESTS_SECOND + 9)
#define DELETE_VOLUME_MOUNTPOINT_TESTS (SET_VOLUME_MOUNTPOINT_TESTS_FIRST + 10)
#define FIND_FIRST_VOLUME_MOUNTPOINT_TESTS (DELETE_VOLUME_MOUNTPOINT_TESTS + 6)
#define FIND_NEXT_VOLUME_MOUNTPOINT_TESTS (FIND_NEXT_VOLUME_MOUNTPOINT_TESTS + 6)

_volume_mount_point_test_data tests_standard[] = {
    /* SetVolumeMountPoint (second parameter) */
    { FALSE, ERROR_INVALID_PARAMETER, L"",                  ""},
    { FALSE, ERROR_INVALID_PARAMETER, L"T:\\",              "U:\\"},
    { FALSE, ERROR_INVALID_PARAMETER, L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"},
    /* SetVolumeMountPoint */
    { FALSE, ERROR_INVALID_PARAMETER, L"",                  ""},
    { FALSE, ERROR_INVALID_PARAMETER, L"\nT:\\",            "\nU:\\"},
    { FALSE, ERROR_DIR_NOT_EMPTY,     L"\\",               "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_INVALID_NAME,      L"T:",                "U:"},
    { FALSE, ERROR_INVALID_NAME,      L"T:T0",              "U:U0"},
    { FALSE, ERROR_INVALID_PARAMETER, L"\\??\\T:\\",        "\\??\\U:\\"},
    { TRUE,  ERROR_SUCCESS,           L"T:\\",              "U:\\"},
    { FALSE, ERROR_DIR_NOT_EMPTY,     L"T:\\",              "U:\\"},
    /* DeleteVolumeMountPoint */
    { FALSE, ERROR_INVALID_NAME,      L"",                  ""},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"\nT:\\",            "\nU:\\"},
    { FALSE, ERROR_ACCESS_DENIED,     L"\\",                "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_INVALID_NAME,      L"T:",                "U:"},
    { FALSE, ERROR_INVALID_NAME,      L"T:T0",              "U:U0"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\??\\T:\\",        "\\??\\U:\\"},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"0:\\",              "0:\\"},
    { TRUE,  ERROR_SUCCESS,           L"T:\\",              "U:\\"},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"T:\\",              "U:\\"},
    /* FindFirstVolumeMountPoint */
    { FALSE, ERROR_INVALID_NAME,      L"",                  ""},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\",                "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"T:\\",              "U:\\"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"},
    { TRUE,  ERROR_SUCCESS,           L"mounted_folder\\",  "mounted_folder\\"},
    /* FindNextVolumeMountPoint */
    { FALSE, ERROR_INVALID_HANDLE,    L"T:\\",              "U:\\"},
    { TRUE,  ERROR_INVALID_HANDLE,    L"mounted_folder3\\", "mounted_folder3\\"},
    { FALSE, ERROR_NO_MORE_FILES,     L"mounted_folder3\\", "mounted_folder3\\"},
    { FALSE, ERROR_INVALID_HANDLE,    L"",                 ""},
    { FALSE, ERROR_INVALID_HANDLE,    L"T:\\",              "U:\\"},
    { TRUE,  ERROR_SUCCESS,           L"",                   ""},
};

_volume_mount_point_test_data tests_no_drive[] = {
    /* SetVolumeMountPoint */
    { FALSE, ERROR_INVALID_PARAMETER, L"",                  ""},
    { FALSE, ERROR_INVALID_NAME,      L"\nT:\\",            "\nU:\\"},
    { FALSE, ERROR_DIR_NOT_EMPTY,     L"\\",               "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_INVALID_NAME,      L"T:",                "U:"},
    { FALSE, ERROR_INVALID_NAME,      L"T:T0",              "U:U0"},
    { FALSE, ERROR_INVALID_NAME,      L"\\??\\T:\\",        "\\??\\U:\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T:\\",              "U:\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T:\\",              "U:\\"},
    /* SetVolumeMountPoint (second parameter) */
    { FALSE, ERROR_INVALID_PARAMETER, L"",                  ""},
    { FALSE, ERROR_INVALID_NAME,      L"T:\\",              "U:\\"},
    { FALSE, ERROR_INVALID_NAME,      L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"},
    /* DeleteVolumeMountPoint */
    { FALSE, ERROR_INVALID_NAME,      L"",                  ""},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"\nT:\\",            "\nU:\\"},
    { FALSE, ERROR_ACCESS_DENIED,     L"\\",                "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_INVALID_NAME,      L"T:",                "U:"},
    { FALSE, ERROR_INVALID_NAME,      L"T:T0",              "U:U0"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\??\\T:\\",        "\\??\\U:\\"},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"0:\\",              "0:\\"},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"T:\\",              "U:\\"},
    { FALSE, ERROR_FILE_NOT_FOUND,    L"T:\\",              "U:\\"},
    /* FindFirstVolumeMountPoint */
    { FALSE, ERROR_INVALID_NAME,      L"",                  ""},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\",                "\\"},
    { FALSE, ERROR_INVALID_NAME,      L"T",                 "U"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"T:\\",              "U:\\"},
    { FALSE, ERROR_PATH_NOT_FOUND,    L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"},
    { TRUE,  ERROR_SUCCESS,           L"mounted_folder\\",  "mounted_folder\\"},
    /* FindNextVolumeMountPoint */
    { FALSE, ERROR_INVALID_HANDLE,    L"T:\\",              "U:\\"},
    { TRUE,  ERROR_INVALID_HANDLE,    L"mounted_folder3\\", "mounted_folder3\\"},
    { FALSE, ERROR_NO_MORE_FILES,     L"mounted_folder3\\", "mounted_folder3\\"},
    { FALSE, ERROR_INVALID_HANDLE,    L"",                 ""},
    { FALSE, ERROR_INVALID_HANDLE,    L"T:\\",              "U:\\"},
    { TRUE,  ERROR_SUCCESS,           L"",                   ""},
};

START_TEST(VolumeMountPoint)
{
    DWORD DriveLettersLength, PathOffset = 0, n = 0;
    _volume_mount_point_test_data *tests;
    BOOL Ret;
    //HANDLE VolumeMountPointHandle;
    UINT Err;
    WCHAR SystemDirectory[MAX_PATH], DriveLetters[MAX_PATH], SystemDrive[4], TestDrive[4],  SystemVolume[60], TestVolume[60], MountedFolder[20];
    CHAR SystemVolumeA[60], TestVolumeA[60];

    /* Get the system volume's drive letter */
    GetSystemDirectoryW(SystemDirectory, ARRAYSIZE(SystemDirectory));
    _wsplitpath(SystemDirectory, SystemDrive, NULL, NULL, NULL);

    /* Create folders for reparse points */
    CreateDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder"), NULL);
    ZeroMemory(MountedFolder, sizeof(MountedFolder));
    CreateDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder2"), NULL);
    ZeroMemory(MountedFolder, sizeof(MountedFolder));
    CreateDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder3"), NULL);
    ZeroMemory(MountedFolder, sizeof(MountedFolder));

    /* Get the system volume name */
    wcscat(SystemDrive, L"\\");
    GetVolumeNameForVolumeMountPointW(SystemDrive, SystemVolume, ARRAYSIZE(SystemVolume));
    printf("VolumeName: %S\n", SystemVolume);
    WideCharToMultiByte(CP_ACP, 0, SystemVolume, -1, SystemVolumeA, sizeof(SystemVolumeA), NULL, NULL);

    /* Now look for some additional disk devices to work with */
    DriveLettersLength = GetLogicalDriveStringsW(ARRAYSIZE(DriveLetters), DriveLetters);
    while (PathOffset < (DriveLettersLength - 1))
    {
        wcscpy(TestDrive, (DriveLetters + PathOffset));
        if (wcscmp(SystemDrive, TestDrive) && GetDriveTypeW(TestDrive) >= DRIVE_REMOVABLE && GetDriveTypeW(TestDrive) != DRIVE_REMOTE)
        {
            break;
        }
        PathOffset += wcslen(DriveLetters + PathOffset) + 1;
    }

    printf("TestDrive: %S, PathOffset %d\n", TestDrive, PathOffset);
    if (PathOffset >= (DriveLettersLength - 1))
    {
        /* We've no spare drive that is suitable for tests, we've to use the test data for "no spare drive" scenario */
        printf("Will not use any disks in system for testing!\n");
        tests = tests_no_drive;
    }
    else
    {
        GetVolumeNameForVolumeMountPointW(TestDrive, TestVolume, ARRAYSIZE(TestVolume));
        WideCharToMultiByte(CP_ACP, 0, TestVolume, -1, TestVolumeA, sizeof(TestVolumeA), NULL, NULL);
        DeleteVolumeMountPointW(TestDrive);
        tests = tests_standard;
    }

    /* SetVolumeMountPointA (second parameter) */
    while (n < SET_VOLUME_MOUNTPOINT_TESTS_SECOND)
    {
        Ret = SetVolumeMountPointA("T:\\", tests[n].TestPathA);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "SetVolumeMountPointA test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    /* SetVolumeMountPointA */
    while (n < SET_VOLUME_MOUNTPOINT_TESTS_FIRST)
    {
        Ret = SetVolumeMountPointA(tests[n].TestPathA, TestVolumeA);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "SetVolumeMountPointA test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    /* DeleteVolumeMountPointA */
    while (n < DELETE_VOLUME_MOUNTPOINT_TESTS)
    {
        Ret = DeleteVolumeMountPointA(tests[n].TestPathA);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "DeleteVolumeMountPointA test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    n = 0;

    /* SetVolumeMountPointW (second parameter) */
    while (n < SET_VOLUME_MOUNTPOINT_TESTS_SECOND)
    {
        Ret = SetVolumeMountPointW(L"T:\\", tests[n].TestPathU);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "SetVolumeMountPointW test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    /* SetVolumeMountPointW */
    while (n < SET_VOLUME_MOUNTPOINT_TESTS_FIRST)
    {
        Ret = SetVolumeMountPointW(tests[n].TestPathU, TestVolume);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "SetVolumeMountPointW test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    /* DeleteVolumeMountPointW */
    while (n < DELETE_VOLUME_MOUNTPOINT_TESTS)
    {
        Ret = DeleteVolumeMountPointW(tests[n].TestPathU);
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "DeleteVolumeMountPointW test %u expected %s and %u, got %s and %u\n", n+1, (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
        n++;
    }

    /* Clean up after reparse point tests */
    SetVolumeMountPointW(TestDrive, TestVolume);
    SystemDrive[2] = UNICODE_NULL;
    RemoveDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder"));
    ZeroMemory(MountedFolder, sizeof(MountedFolder));
    RemoveDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder2"));
    ZeroMemory(MountedFolder, sizeof(MountedFolder));
    RemoveDirectoryW(wcscat(wcscat(MountedFolder, SystemDrive), L"\\mounted_folder3"));
#if 0
    do
    {
        Ret = FindFirstVolumeMountPointW(tests[n].MountPointU, tests[n].MountPointA)
        Err = GetLastError();
        ok(((Err == tests[n].Err) && (Ret == tests[n].Ret)), "Expected %s and %u, got %s and %u\n", (tests[n].Ret ? "success" : "failure"), tests[n].Err, (Ret ? "success" : "failure"), Err);
    }
    while (!tests[n].End)
#endif
}
