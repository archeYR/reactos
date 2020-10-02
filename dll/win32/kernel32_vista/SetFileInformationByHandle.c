#include "k32_vista.h"
#include <ndk/iofuncs.h>
#include <ndk/iotypes.h>

/* Taken from kernel32 */
DWORD
BaseSetLastNTError(IN NTSTATUS Status)
{
    DWORD dwErrCode;
    dwErrCode = RtlNtStatusToDosError(Status);
    SetLastError(dwErrCode);
    return dwErrCode;
}

/* Quick and dirty table for conversion */
FILE_INFORMATION_CLASS ConvertToFileInfo[MaximumFileInfoByHandlesClass] =
{
    FileBasicInformation, FileStandardInformation, FileNameInformation, FileRenameInformation,
    FileDispositionInformation, FileAllocationInformation, FileEndOfFileInformation, FileStreamInformation,
    FileCompressionInformation, FileAttributeTagInformation, FileIdBothDirectoryInformation, (FILE_INFORMATION_CLASS)-1,
    FileIoPriorityHintInformation, FileRemoteProtocolInformation
};

/* Quick implementation, still going farther than Wine implementation */
BOOL
WINAPI
SetFileInformationByHandle(HANDLE hFile,
                           FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                           LPVOID lpFileInformation,
                           DWORD dwBufferSize)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_INFORMATION_CLASS FileInfoClass;

    FileInfoClass = (FILE_INFORMATION_CLASS)-1;

    /* Attempt to convert the class */
    if (FileInformationClass < MaximumFileInfoByHandlesClass)
    {
        FileInfoClass = ConvertToFileInfo[FileInformationClass];
    }

    /* If wrong, bail out */
    if (FileInfoClass == -1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* And set the information */
    Status = NtSetInformationFile(hFile, &IoStatusBlock, lpFileInformation,
                                  dwBufferSize, FileInfoClass);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}