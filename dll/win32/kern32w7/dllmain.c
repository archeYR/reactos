#include <ndk/rtlfuncs.h>
#include "winbase.h"

/* Windows 7 kernel32 exports */
#pragma comment(linker, "/export:Module32Next=kernel32.Module32Next")
#pragma comment(linker, "/export:Module32First=kernel32.Module32First")
#pragma comment(linker, "/export:GetModuleHandleA=kernel32.GetModuleHandleA")
#pragma comment(linker, "/export:CreateToolhelp32Snapshot=kernel32.CreateToolhelp32Snapshot")
#pragma comment(linker, "/export:GetLastError=kernel32.GetLastError")
#pragma comment(linker, "/export:CloseHandle=kernel32.CloseHandle")
#pragma comment(linker, "/export:LoadLibraryW=kernel32.LoadLibraryW")
#pragma comment(linker, "/export:GetProcAddress=kernel32.GetProcAddress")
#pragma comment(linker, "/export:FreeLibrary=kernel32.FreeLibrary")
#pragma comment(linker, "/export:IsDebuggerPresent=kernel32.IsDebuggerPresent")
#pragma comment(linker, "/export:InitializeSListHead=kernel32.InitializeSListHead")
#pragma comment(linker, "/export:DisableThreadLibraryCalls=kernel32.DisableThreadLibraryCalls")
#pragma comment(linker, "/export:GetSystemTimeAsFileTime=kernel32.GetSystemTimeAsFileTime")
#pragma comment(linker, "/export:GetCurrentThreadId=kernel32.GetCurrentThreadId")
#pragma comment(linker, "/export:GetCurrentProcessId=kernel32.GetCurrentProcessId")
#pragma comment(linker, "/export:QueryPerformanceCounter=kernel32.QueryPerformanceCounter")
#pragma comment(linker, "/export:IsProcessorFeaturePresent=kernel32.IsProcessorFeaturePresent")
#pragma comment(linker, "/export:TerminateProcess=kernel32.TerminateProcess")
#pragma comment(linker, "/export:GetCurrentProcess=kernel32.GetCurrentProcess")
#pragma comment(linker, "/export:SetUnhandledExceptionFilter=kernel32.SetUnhandledExceptionFilter")
#pragma comment(linker, "/export:UnhandledExceptionFilter=kernel32.UnhandledExceptionFilter")
#pragma comment(linker, "/export:RtlUnwind=kernel32.RtlUnwind")
#pragma comment(linker, "/export:CreateTimerQueueTimer=kernel32.CreateTimerQueueTimer")
#pragma comment(linker, "/export:ChangeTimerQueueTimer=kernel32.ChangeTimerQueueTimer")
#pragma comment(linker, "/export:SetFileAttributesW=kernel32.SetFileAttributesW")
#pragma comment(linker, "/export:RtlCaptureStackBackTrace=kernel32.RtlCaptureStackBackTrace")
#pragma comment(linker, "/export:WaitForMultipleObjectsEx=kernel32.WaitForMultipleObjectsEx")
#pragma comment(linker, "/export:UnregisterWaitEx=kernel32.UnregisterWaitEx")
#pragma comment(linker, "/export:QueryDepthSList=kernel32.QueryDepthSList")
#pragma comment(linker, "/export:InterlockedPopEntrySList=kernel32.InterlockedPopEntrySList")
#pragma comment(linker, "/export:SetProcessAffinityMask=kernel32.SetProcessAffinityMask")
#pragma comment(linker, "/export:GetVersionExW=kernel32.GetVersionExW")
#pragma comment(linker, "/export:GetThreadTimes=kernel32.GetThreadTimes")
#pragma comment(linker, "/export:UnregisterWait=kernel32.UnregisterWait")
#pragma comment(linker, "/export:RegisterWaitForSingleObject=kernel32.RegisterWaitForSingleObject")
#pragma comment(linker, "/export:SetThreadAffinityMask=kernel32.SetThreadAffinityMask")
#pragma comment(linker, "/export:GetProcessAffinityMask=kernel32.GetProcessAffinityMask")
#pragma comment(linker, "/export:GetNumaHighestNodeNumber=kernel32.GetNumaHighestNodeNumber")
#pragma comment(linker, "/export:EnterCriticalSection=kernel32.EnterCriticalSection")
#pragma comment(linker, "/export:LeaveCriticalSection=kernel32.LeaveCriticalSection")
#pragma comment(linker, "/export:InitializeCriticalSection=kernel32.InitializeCriticalSection")
#pragma comment(linker, "/export:DeleteCriticalSection=kernel32.DeleteCriticalSection")
#pragma comment(linker, "/export:Sleep=kernel32.Sleep")
#pragma comment(linker, "/export:QueryPerformanceFrequency=kernel32.QueryPerformanceFrequency")
#pragma comment(linker, "/export:QueryPerformanceCounter=kernel32.QueryPerformanceCounter")
#pragma comment(linker, "/export:LoadLibraryA=kernel32.LoadLibraryA")
#pragma comment(linker, "/export:GetProcAddress=kernel32.GetProcAddress")
#pragma comment(linker, "/export:FreeLibrary=kernel32.FreeLibrary")
#pragma comment(linker, "/export:GetSystemDirectoryA=kernel32.GetSystemDirectoryA")
#pragma comment(linker, "/export:GetCurrentProcess=kernel32.GetCurrentProcess")
#pragma comment(linker, "/export:GetModuleHandleA=kernel32.GetModuleHandleA")
#pragma comment(linker, "/export:TlsSetValue=kernel32.TlsSetValue")
#pragma comment(linker, "/export:Thread32Next=kernel32.Thread32Next")
#pragma comment(linker, "/export:Thread32First=kernel32.Thread32First")
#pragma comment(linker, "/export:GetCurrentThreadId=kernel32.GetCurrentThreadId")
#pragma comment(linker, "/export:CreateToolhelp32Snapshot=kernel32.CreateToolhelp32Snapshot")
#pragma comment(linker, "/export:TlsAlloc=kernel32.TlsAlloc")
#pragma comment(linker, "/export:CloseHandle=kernel32.CloseHandle")
#pragma comment(linker, "/export:GetCurrentProcessId=kernel32.GetCurrentProcessId")
#pragma comment(linker, "/export:TlsGetValue=kernel32.TlsGetValue")
#pragma comment(linker, "/export:TlsFree=kernel32.TlsFree")
#pragma comment(linker, "/export:SetLastError=kernel32.SetLastError")
#pragma comment(linker, "/export:OutputDebugStringA=kernel32.OutputDebugStringA")
#pragma comment(linker, "/export:SwitchToThread=kernel32.SwitchToThread")
#pragma comment(linker, "/export:GetModuleFileNameA=kernel32.GetModuleFileNameA")
#pragma comment(linker, "/export:SetErrorMode=kernel32.SetErrorMode")
#pragma comment(linker, "/export:GlobalMemoryStatusEx=kernel32.GlobalMemoryStatusEx")
#pragma comment(linker, "/export:GetConsoleWindow=kernel32.GetConsoleWindow")
#pragma comment(linker, "/export:IsDebuggerPresent=kernel32.IsDebuggerPresent")
#pragma comment(linker, "/export:ReleaseSemaphore=kernel32.ReleaseSemaphore")
#pragma comment(linker, "/export:WaitForSingleObject=kernel32.WaitForSingleObject")
#pragma comment(linker, "/export:GetExitCodeThread=kernel32.GetExitCodeThread")
#pragma comment(linker, "/export:CreateSemaphoreA=kernel32.CreateSemaphoreA")
#pragma comment(linker, "/export:GetSystemInfo=kernel32.GetSystemInfo")
#pragma comment(linker, "/export:GetCommandLineA=kernel32.GetCommandLineA")
#pragma comment(linker, "/export:GetSystemTimeAsFileTime=kernel32.GetSystemTimeAsFileTime")
#pragma comment(linker, "/export:GetProcessTimes=kernel32.GetProcessTimes")
#pragma comment(linker, "/export:VirtualFree=kernel32.VirtualFree")
#pragma comment(linker, "/export:VirtualAlloc=kernel32.VirtualAlloc")
#pragma comment(linker, "/export:VerSetConditionMask=kernel32.VerSetConditionMask")
#pragma comment(linker, "/export:VerifyVersionInfoW=kernel32.VerifyVersionInfoW")
#pragma comment(linker, "/export:RaiseException=kernel32.RaiseException")
#pragma comment(linker, "/export:TryEnterCriticalSection=kernel32.TryEnterCriticalSection")
#pragma comment(linker, "/export:GetLastError=kernel32.GetLastError")
#pragma comment(linker, "/export:LoadLibraryW=kernel32.LoadLibraryW")
#pragma comment(linker, "/export:LocalFree=kernel32.LocalFree")
#pragma comment(linker, "/export:FormatMessageA=kernel32.FormatMessageA")
#pragma comment(linker, "/export:FlushInstructionCache=kernel32.FlushInstructionCache")
#pragma comment(linker, "/export:VirtualProtect=kernel32.VirtualProtect")
#pragma comment(linker, "/export:VirtualQuery=kernel32.VirtualQuery")
#pragma comment(linker, "/export:GetStdHandle=kernel32.GetStdHandle")
#pragma comment(linker, "/export:GetCommandLineW=kernel32.GetCommandLineW")
#pragma comment(linker, "/export:GetEnvironmentVariableW=kernel32.GetEnvironmentVariableW")
#pragma comment(linker, "/export:FindClose=kernel32.FindClose")
#pragma comment(linker, "/export:FindFirstFileW=kernel32.FindFirstFileW")
#pragma comment(linker, "/export:FindNextFileW=kernel32.FindNextFileW")
#pragma comment(linker, "/export:GetLongPathNameW=kernel32.GetLongPathNameW")
#pragma comment(linker, "/export:GetNativeSystemInfo=kernel32.GetNativeSystemInfo")
#pragma comment(linker, "/export:GetModuleFileNameW=kernel32.GetModuleFileNameW")
#pragma comment(linker, "/export:GetConsoleMode=kernel32.GetConsoleMode")
#pragma comment(linker, "/export:GetConsoleScreenBufferInfo=kernel32.GetConsoleScreenBufferInfo")
#pragma comment(linker, "/export:SetConsoleTextAttribute=kernel32.SetConsoleTextAttribute")
#pragma comment(linker, "/export:SetCurrentDirectoryW=kernel32.SetCurrentDirectoryW")
#pragma comment(linker, "/export:GetCurrentDirectoryW=kernel32.GetCurrentDirectoryW")
#pragma comment(linker, "/export:CreateDirectoryW=kernel32.CreateDirectoryW")
#pragma comment(linker, "/export:CreateFileW=kernel32.CreateFileW")
#pragma comment(linker, "/export:DeleteFileW=kernel32.DeleteFileW")
#pragma comment(linker, "/export:GetDiskFreeSpaceExA=kernel32.GetDiskFreeSpaceExA")
#pragma comment(linker, "/export:GetDriveTypeW=kernel32.GetDriveTypeW")
#pragma comment(linker, "/export:GetFileAttributesW=kernel32.GetFileAttributesW")
#pragma comment(linker, "/export:GetFileInformationByHandle=kernel32.GetFileInformationByHandle")
#pragma comment(linker, "/export:GetFileType=kernel32.GetFileType")
#pragma comment(linker, "/export:GetFinalPathNameByHandleW=kernel32.GetFinalPathNameByHandleW")
#pragma comment(linker, "/export:GetVolumePathNameW=kernel32.GetVolumePathNameW")
#pragma comment(linker, "/export:RemoveDirectoryW=kernel32.RemoveDirectoryW")
#pragma comment(linker, "/export:GetLogicalProcessorInformation=kernel32.GetLogicalProcessorInformation")
#pragma comment(linker, "/export:SetFileTime=kernel32.SetFileTime")
#pragma comment(linker, "/export:CreateFileMappingW=kernel32.CreateFileMappingW")
#pragma comment(linker, "/export:MapViewOfFile=kernel32.MapViewOfFile")
#pragma comment(linker, "/export:UnmapViewOfFile=kernel32.UnmapViewOfFile")
#pragma comment(linker, "/export:MoveFileExW=kernel32.MoveFileExW")
#pragma comment(linker, "/export:ReplaceFileW=kernel32.ReplaceFileW")
#pragma comment(linker, "/export:CreateHardLinkW=kernel32.CreateHardLinkW")
#pragma comment(linker, "/export:MultiByteToWideChar=kernel32.MultiByteToWideChar")
#pragma comment(linker, "/export:WideCharToMultiByte=kernel32.WideCharToMultiByte")
#pragma comment(linker, "/export:RtlCaptureContext=kernel32.RtlCaptureContext")
#pragma comment(linker, "/export:ExpandEnvironmentStringsW=kernel32.ExpandEnvironmentStringsW")
#pragma comment(linker, "/export:SetUnhandledExceptionFilter=kernel32.SetUnhandledExceptionFilter")
#pragma comment(linker, "/export:GetCurrentThread=kernel32.GetCurrentThread")
#pragma comment(linker, "/export:SetConsoleCtrlHandler=kernel32.SetConsoleCtrlHandler")
#pragma comment(linker, "/export:GetModuleHandleW=kernel32.GetModuleHandleW")
#pragma comment(linker, "/export:SearchPathW=kernel32.SearchPathW")
#pragma comment(linker, "/export:DuplicateHandle=kernel32.DuplicateHandle")
#pragma comment(linker, "/export:TerminateProcess=kernel32.TerminateProcess")
#pragma comment(linker, "/export:GetExitCodeProcess=kernel32.GetExitCodeProcess")
#pragma comment(linker, "/export:CreateProcessW=kernel32.CreateProcessW")
#pragma comment(linker, "/export:CreateJobObjectW=kernel32.CreateJobObjectW")
#pragma comment(linker, "/export:AssignProcessToJobObject=kernel32.AssignProcessToJobObject")
#pragma comment(linker, "/export:SetInformationJobObject=kernel32.SetInformationJobObject")
#pragma comment(linker, "/export:GetThreadPriority=kernel32.GetThreadPriority")
#pragma comment(linker, "/export:RtlLookupFunctionEntry=kernel32.RtlLookupFunctionEntry")
#pragma comment(linker, "/export:RtlVirtualUnwind=kernel32.RtlVirtualUnwind")
#pragma comment(linker, "/export:UnhandledExceptionFilter=kernel32.UnhandledExceptionFilter")
#pragma comment(linker, "/export:IsProcessorFeaturePresent=kernel32.IsProcessorFeaturePresent")
#pragma comment(linker, "/export:InitializeCriticalSectionAndSpinCount=kernel32.InitializeCriticalSectionAndSpinCount")
#pragma comment(linker, "/export:SetEvent=kernel32.SetEvent")
#pragma comment(linker, "/export:ResetEvent=kernel32.ResetEvent")
#pragma comment(linker, "/export:WaitForSingleObjectEx=kernel32.WaitForSingleObjectEx")
#pragma comment(linker, "/export:CreateEventW=kernel32.CreateEventW")
#pragma comment(linker, "/export:InitializeSListHead=kernel32.InitializeSListHead")
#pragma comment(linker, "/export:GetStartupInfoW=kernel32.GetStartupInfoW")
#pragma comment(linker, "/export:FormatMessageW=kernel32.FormatMessageW")
#pragma comment(linker, "/export:RtlPcToFileHeader=kernel32.RtlPcToFileHeader")
#pragma comment(linker, "/export:EncodePointer=kernel32.EncodePointer")
#pragma comment(linker, "/export:DecodePointer=kernel32.DecodePointer")
#pragma comment(linker, "/export:GetTickCount=kernel32.GetTickCount")
#pragma comment(linker, "/export:CompareStringW=kernel32.CompareStringW")
#pragma comment(linker, "/export:LCMapStringW=kernel32.LCMapStringW")
#pragma comment(linker, "/export:GetLocaleInfoW=kernel32.GetLocaleInfoW")
#pragma comment(linker, "/export:GetStringTypeW=kernel32.GetStringTypeW")
#pragma comment(linker, "/export:GetCPInfo=kernel32.GetCPInfo")
#pragma comment(linker, "/export:InterlockedPushEntrySList=kernel32.InterlockedPushEntrySList")
#pragma comment(linker, "/export:InterlockedFlushSList=kernel32.InterlockedFlushSList")
#pragma comment(linker, "/export:RtlUnwindEx=kernel32.RtlUnwindEx")
#pragma comment(linker, "/export:LoadLibraryExW=kernel32.LoadLibraryExW")
#pragma comment(linker, "/export:ExitProcess=kernel32.ExitProcess")
#pragma comment(linker, "/export:GetModuleHandleExW=kernel32.GetModuleHandleExW")
#pragma comment(linker, "/export:CreateThread=kernel32.CreateThread")
#pragma comment(linker, "/export:ExitThread=kernel32.ExitThread")
#pragma comment(linker, "/export:ResumeThread=kernel32.ResumeThread")
#pragma comment(linker, "/export:FreeLibraryAndExitThread=kernel32.FreeLibraryAndExitThread")
#pragma comment(linker, "/export:SetFilePointerEx=kernel32.SetFilePointerEx")
#pragma comment(linker, "/export:HeapValidate=kernel32.HeapValidate")
#pragma comment(linker, "/export:HeapWalk=kernel32.HeapWalk")
#pragma comment(linker, "/export:SetStdHandle=kernel32.SetStdHandle")
#pragma comment(linker, "/export:SetEndOfFile=kernel32.SetEndOfFile")
#pragma comment(linker, "/export:WriteFile=kernel32.WriteFile")
#pragma comment(linker, "/export:GetConsoleCP=kernel32.GetConsoleCP")
#pragma comment(linker, "/export:ReadFile=kernel32.ReadFile")
#pragma comment(linker, "/export:ReadConsoleW=kernel32.ReadConsoleW")
#pragma comment(linker, "/export:HeapFree=kernel32.HeapFree")
#pragma comment(linker, "/export:HeapAlloc=kernel32.HeapAlloc")
#pragma comment(linker, "/export:HeapReAlloc=kernel32.HeapReAlloc")
#pragma comment(linker, "/export:HeapSize=kernel32.HeapSize")
#pragma comment(linker, "/export:HeapQueryInformation=kernel32.HeapQueryInformation")
#pragma comment(linker, "/export:GetDateFormatW=kernel32.GetDateFormatW")
#pragma comment(linker, "/export:GetTimeFormatW=kernel32.GetTimeFormatW")
#pragma comment(linker, "/export:IsValidLocale=kernel32.IsValidLocale")
#pragma comment(linker, "/export:GetUserDefaultLCID=kernel32.GetUserDefaultLCID")
#pragma comment(linker, "/export:EnumSystemLocalesW=kernel32.EnumSystemLocalesW")
#pragma comment(linker, "/export:FlushFileBuffers=kernel32.FlushFileBuffers")
#pragma comment(linker, "/export:GetProcessHeap=kernel32.GetProcessHeap")
#pragma comment(linker, "/export:GetTimeZoneInformation=kernel32.GetTimeZoneInformation")
#pragma comment(linker, "/export:FindFirstFileExW=kernel32.FindFirstFileExW")
#pragma comment(linker, "/export:IsValidCodePage=kernel32.IsValidCodePage")
#pragma comment(linker, "/export:GetACP=kernel32.GetACP")
#pragma comment(linker, "/export:GetOEMCP=kernel32.GetOEMCP")
#pragma comment(linker, "/export:GetEnvironmentStringsW=kernel32.GetEnvironmentStringsW")
#pragma comment(linker, "/export:FreeEnvironmentStringsW=kernel32.FreeEnvironmentStringsW")
#pragma comment(linker, "/export:SetEnvironmentVariableW=kernel32.SetEnvironmentVariableW")
#pragma comment(linker, "/export:OutputDebugStringW=kernel32.OutputDebugStringW")
#pragma comment(linker, "/export:WriteConsoleW=kernel32.WriteConsoleW")
#pragma comment(linker, "/export:CreateTimerQueue=kernel32.CreateTimerQueue")
#pragma comment(linker, "/export:SignalObjectAndWait=kernel32.SignalObjectAndWait")
#pragma comment(linker, "/export:SetThreadPriority=kernel32.SetThreadPriority")
#pragma comment(linker, "/export:DeleteTimerQueueTimer=kernel32.DeleteTimerQueueTimer")
#pragma comment(linker, "/export:GetFileSizeEx=kernel32.GetFileSizeEx")
#pragma comment(linker, "/export:GetSystemTime=kernel32.GetSystemTime")
#pragma comment(linker, "/export:SetConsoleMode=kernel32.SetConsoleMode")
#pragma comment(linker, "/export:SetFileInformationByHandle=kernel32.SetFileInformationByHandle")
#pragma comment(linker, "/export:SystemTimeToFileTime=kernel32.SystemTimeToFileTime")

/* kernel32 > psapi redirection */
#pragma comment(linker, "/export:K32EnumProcessModulesEx=psapi.EnumProcessModulesEx")
#pragma comment(linker, "/export:K32EnumProcessModules=psapi.EnumProcessModules")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/*
BOOL
WINAPI
VerifyVersionInfoW(IN LPOSVERSIONINFOEXW lpVersionInformation,
    IN DWORD dwTypeMask,
    IN DWORDLONG dwlConditionMask)
{
        return TRUE;

}
*/

static DWORD rtlmode_to_win32mode(DWORD rtlmode)
{
    DWORD win32mode = 0;

    if (rtlmode & 0x10) win32mode |= SEM_FAILCRITICALERRORS;
    if (rtlmode & 0x20) win32mode |= SEM_NOGPFAULTERRORBOX;
    if (rtlmode & 0x40) win32mode |= SEM_NOOPENFILEERRORBOX;
    return win32mode;
}

BOOL WINAPI SetThreadErrorMode(DWORD mode, DWORD* old)
{
    NTSTATUS status;
    DWORD new = 0;

    if (mode & ~(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (mode & SEM_FAILCRITICALERRORS) new |= 0x10;
    if (mode & SEM_NOGPFAULTERRORBOX) new |= 0x20;
    if (mode & SEM_NOOPENFILEERRORBOX) new |= 0x40;

    status = RtlSetThreadErrorMode(new, old);
    if (!status && old) *old = rtlmode_to_win32mode(*old);
    return status;
}