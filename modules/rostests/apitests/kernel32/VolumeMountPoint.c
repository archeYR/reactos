#include "precomp.h"

START_TEST(VolumeMountPoint)
{
	BOOL Ret;
	HANDLE VolumeMountPointHandle;
	UINT Err;
#if 0
	WCHAR VolumeName[60];
	WCHAR VolumeNameRoot[60];
	WCHAR MountedFolder[60];
#endif
	CHAR VolumeNameA[60];
	CHAR VolumeNameRootA[60];
	CHAR MountedFolderA[60];
	//WCHAR MountedFolderShort[5];

#if 0
	GetVolumeNameForVolumeMountPointW(L"D:\\", VolumeName, ARRAYSIZE(VolumeName));
	GetVolumeNameForVolumeMountPointW(L"C:\\", VolumeNameRoot, ARRAYSIZE(VolumeNameRoot));
#endif
	GetVolumeNameForVolumeMountPointA("D:\\", VolumeNameA, ARRAYSIZE(VolumeNameA));
	GetVolumeNameForVolumeMountPointA("C:\\", VolumeNameRootA, ARRAYSIZE(VolumeNameRootA));

#if 0
    //SetLastError(0xdeadbeef);
    //Ret = DeleteVolumeMountPointW(NULL);
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"\nD:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"\\");
    Err = GetLastError();
    ok(Err == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"D");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"D:");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"D:D0");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"\\??\\D:\\");
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"0:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"D:\\");
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"D:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //Ret = SetVolumeMountPointW(NULL, VolumeName);
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"\nD:\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:D0", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"\\??\\D:\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", NULL);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", L"");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", L"C:\\");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
	VolumeName[wcslen(VolumeName) - 1] = UNICODE_NULL;
    Ret = SetVolumeMountPointW(L"D:\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);
    VolumeName[wcslen(VolumeName)] = L'\\';
    VolumeName[wcslen(VolumeName) + 1] = UNICODE_NULL;

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", VolumeName);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"D:\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

    /****** Reparse points ******/
    CreateDirectoryW(L"C:\\mounted_folder", NULL);
    CreateDirectoryW(L"C:\\mounted_folder2", NULL);
    CreateDirectoryW(L"C:\\mounted_folder3", NULL);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"C:\\nonexistent_folder\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"C:\\mounted_folder", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"C:\\mounted_folder\\", VolumeName);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointW(L"C:\\mounted_folder\\", VolumeName);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

	SetVolumeMountPointW(L"C:\\mounted_folder2\\", VolumeName);
	SetVolumeMountPointW(L"C:\\mounted_folder3\\", VolumeName);
    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(VolumeNameRoot, MountedFolder, 0);
    Err = GetLastError();
    ok(Err == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //FindFirstVolumeMountPointW(VolumeNameRoot, NULL, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //VolumeMountPointHandle = FindFirstVolumeMountPointW(VolumeNameRoot, MountedFolderShort, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(VolumeMountPointHandle != INVALID_HANDLE_VALUE && !(wcscmp(MountedFolderShort, L"mount")), "Expected success and 'mount' as output, got %u and %S\n", Err, MountedFolderShort);
	//FindVolumeMountPointClose(VolumeMountPointHandle);

    //SetLastError(0xdeadbeef);
    //FindFirstVolumeMountPointW(NULL, MountedFolder, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(L"", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(L"\\", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(L"D", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(L"D:\\", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

	VolumeNameRoot[wcslen(VolumeNameRoot) - 1] = UNICODE_NULL;
    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointW(VolumeNameRoot, MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);
    VolumeNameRoot[wcslen(VolumeNameRoot)] = L'\\';
    VolumeNameRoot[wcslen(VolumeNameRoot) + 1] = UNICODE_NULL;

    SetLastError(0xdeadbeef);
    VolumeMountPointHandle = FindFirstVolumeMountPointW(VolumeNameRoot, MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(VolumeMountPointHandle != INVALID_HANDLE_VALUE && !(wcscmp(MountedFolder, L"mounted_folder\\")), "Expected success and 'mounted_folder\\' as output, got %u and %s\n", Err, MountedFolder);

    SetLastError(0xdeadbeef);
    FindNextVolumeMountPointW(VolumeMountPointHandle, MountedFolder, 0);
    Err = GetLastError();
    ok(Err == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //FindNextVolumeMountPointW(VolumeMountPointHandle, NULL, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //Ret = FindNextVolumeMountPointW(VolumeMountPointHandle, MountedFolderShort, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(Ret == TRUE && !(wcscmp(MountedFolderShort, L"mount")), "Expected success and 'mount' as output, got %u and %S\n", Err, MountedFolderShort);

    //SetLastError(0xdeadbeef);
    //FindNextVolumeMountPointW(NULL, MountedFolder, ARRAYSIZE(MountedFolder));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindNextVolumeMountPointW(L"D:\\", MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindNextVolumeMountPointW(VolumeMountPointHandle, MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Ret == TRUE && !(wcscmp(MountedFolder, L"mounted_folder3\\")), "Expected success and 'mounted_folder3\\' as output, got %u and %S\n", Err, MountedFolder);

    SetLastError(0xdeadbeef);
    Ret = FindNextVolumeMountPointW(VolumeMountPointHandle, MountedFolder, ARRAYSIZE(MountedFolder));
    Err = GetLastError();
    ok(Err == ERROR_NO_MORE_FILES && !(wcscmp(MountedFolder, L"mounted_folder3\\")), "Expected ERROR_NO_MORE_DATA and 'mounted_folder3\\' as output, got %u and %S\n", Err, MountedFolder);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose(NULL);
    Err = GetLastError();
	ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose(L"D:\\");
    Err = GetLastError();
	ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose(VolumeMountPointHandle);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    DeleteVolumeMountPointW(L"C:\\mounted_folder3\\");
    DeleteVolumeMountPointW(L"C:\\mounted_folder2\\");

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"C:\\nonexistent_folder\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"C:\\mounted_folder");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"C:\\mounted_folder\\");
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointW(L"C:\\mounted_folder\\");
    Err = GetLastError();
    ok(Err == ERROR_NOT_A_REPARSE_POINT, "Expected ERROR_NOT_A_REPARSE_POINT, got %u\n", Err);

    RemoveDirectoryW(L"C:\\mounted_folder");
    RemoveDirectoryW(L"C:\\mounted_folder2");
    RemoveDirectoryW(L"C:\\mounted_folder3");
#endif
    /******* ANSI *******/

    //SetLastError(0xdeadbeef);
    //Ret = DeleteVolumeMountPointA(NULL);
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("\nD:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("\\");
    Err = GetLastError();
    ok(Err == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("D");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("D:");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("D:D0");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("\\??\\D:\\");
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("0:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("D:\\");
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("D:\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //Ret = SetVolumeMountPointA(NULL, VolumeNameA);
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("\nD:\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:D0", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("\\??\\D:\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", NULL);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", "");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", "C:\\");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
	VolumeNameA[strlen(VolumeNameA) - 1] = ANSI_NULL;
    Ret = SetVolumeMountPointA("D:\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);
    VolumeNameA[strlen(VolumeNameA)] = '\\';
    VolumeNameA[strlen(VolumeNameA) + 1] = ANSI_NULL;

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", VolumeNameA);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("D:\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

    /****** Reparse points ******/
    CreateDirectoryA("C:\\mounted_folder", NULL);
    CreateDirectoryA("C:\\mounted_folder2", NULL);
    CreateDirectoryA("C:\\mounted_folder3", NULL);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("C:\\nonexistent_folder\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("C:\\mounted_folder", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("C:\\mounted_folder\\", VolumeNameA);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = SetVolumeMountPointA("C:\\mounted_folder\\", VolumeNameA);
    Err = GetLastError();
    ok(Err == ERROR_DIR_NOT_EMPTY, "Expected ERROR_DIR_NOT_EMPTY, got %u\n", Err);

	SetVolumeMountPointA("C:\\mounted_folder2\\", VolumeNameA);
	SetVolumeMountPointA("C:\\mounted_folder3\\", VolumeNameA);
    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA(VolumeNameRootA, MountedFolderA, 0);
    Err = GetLastError();
    ok(Err == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //FindFirstVolumeMountPointA(VolumeNameRootA, NULL, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //VolumeMountPointHandle = FindFirstVolumeMountPointA(VolumeNameRootA, MountedFolderShortA, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(VolumeMountPointHandle != INVALID_HANDLE_VALUE && !(strcmp(MountedFolderShortA, "mount")), "Expected success and 'mount' as output, got %u and %S\n", Err, MountedFolderShortA);
	//FindVolumeMountPointClose(VolumeMountPointHandle);

    //SetLastError(0xdeadbeef);
    //FindFirstVolumeMountPointA(NULL, MountedFolderA, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA("", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA("\\", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA("D", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA("D:\\", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %u\n", Err);

	VolumeNameRootA[strlen(VolumeNameRootA) - 1] = ANSI_NULL;
    SetLastError(0xdeadbeef);
    FindFirstVolumeMountPointA(VolumeNameRootA, MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);
    VolumeNameRootA[strlen(VolumeNameRootA)] = '\\';
    VolumeNameRootA[strlen(VolumeNameRootA) + 1] = ANSI_NULL;

    SetLastError(0xdeadbeef);
    VolumeMountPointHandle = FindFirstVolumeMountPointA(VolumeNameRootA, MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(VolumeMountPointHandle != INVALID_HANDLE_VALUE && !(strcmp(MountedFolderA, "mounted_folder\\")), "Expected success and 'mounted_folder\\' as output, got %u and %s\n", Err, MountedFolderA);

    SetLastError(0xdeadbeef);
    FindNextVolumeMountPointA(VolumeMountPointHandle, MountedFolderA, 0);
    Err = GetLastError();
    ok(Err == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //FindNextVolumeMountPointA(VolumeMountPointHandle, NULL, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    //SetLastError(0xdeadbeef);
    //Ret = FindNextVolumeMountPointA(VolumeMountPointHandle, MountedFolderShortA, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(Ret == TRUE && !(strcmp(MountedFolderShortA, "mount")), "Expected success and 'mount' as output, got %u and %s\n", Err, MountedFolderShortA);

    //SetLastError(0xdeadbeef);
    //FindNextVolumeMountPointA(NULL, MountedFolderA, ARRAYSIZE(MountedFolderA));
    //Err = GetLastError();
    //ok(Err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", Err);

    SetLastError(0xdeadbeef);
    FindNextVolumeMountPointA("D:\\", MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindNextVolumeMountPointA(VolumeMountPointHandle, MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Ret == TRUE && !(strcmp(MountedFolderA, "mounted_folder3\\")), "Expected success and 'mounted_folder3\\' as output, got %u and %s\n", Err, MountedFolderA);

    SetLastError(0xdeadbeef);
    Ret = FindNextVolumeMountPointA(VolumeMountPointHandle, MountedFolderA, ARRAYSIZE(MountedFolderA));
    Err = GetLastError();
    ok(Err == ERROR_NO_MORE_FILES && !(strcmp(MountedFolderA, "mounted_folder3\\")), "Expected ERROR_NO_MORE_DATA and 'mounted_folder3\\' as output, got %u and %s\n", Err, MountedFolderA);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose(NULL);
    Err = GetLastError();
	ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose("D:\\");
    Err = GetLastError();
	ok(Err == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = FindVolumeMountPointClose(VolumeMountPointHandle);
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    DeleteVolumeMountPointA("C:\\mounted_folder3\\");
    DeleteVolumeMountPointA("C:\\mounted_folder2\\");

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("C:\\nonexistent_folder\\");
    Err = GetLastError();
    ok(Err == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("C:\\mounted_folder");
    Err = GetLastError();
    ok(Err == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("C:\\mounted_folder\\");
    Err = GetLastError();
    ok(Ret == TRUE, "Expected success, got %u\n", Err);

    SetLastError(0xdeadbeef);
    Ret = DeleteVolumeMountPointA("C:\\mounted_folder\\");
    Err = GetLastError();
    ok(Err == ERROR_NOT_A_REPARSE_POINT, "Expected ERROR_NOT_A_REPARSE_POINT, got %u\n", Err);

    RemoveDirectoryA("C:\\mounted_folder");
    RemoveDirectoryA("C:\\mounted_folder2");
    RemoveDirectoryA("C:\\mounted_folder3");

}
