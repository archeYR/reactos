#include "k32_vista.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
RtlInitializeConditionVariable(OUT PRTL_CONDITION_VARIABLE ConditionVariable);

VOID
NTAPI
RtlWakeConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

VOID
NTAPI
RtlWakeAllConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                            IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                            IN PLARGE_INTEGER TimeOut OPTIONAL);

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                             IN OUT PRTL_SRWLOCK SRWLock,
                             IN PLARGE_INTEGER TimeOut OPTIONAL,
                             IN ULONG Flags);

VOID
NTAPI
RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive( IN OUT PRTL_SRWLOCK SRWLock );

VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

DWORD
WINAPI
RtlRunOnceBeginInitialize( RTL_RUN_ONCE *once, ULONG flags, void **context );

VOID
WINAPI
RtlRunOnceInitialize( RTL_RUN_ONCE *once );

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK Lock)
{
    RtlAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK Lock)
{
    RtlAcquireSRWLockShared((PRTL_SRWLOCK)Lock);
}

BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(PSRWLOCK Lock)
{
    return(RtlTryAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock));
}

VOID
WINAPI
InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlInitializeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
InitializeSRWLock(PSRWLOCK Lock)
{
    RtlInitializeSRWLock((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK Lock)
{
    RtlReleaseSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK Lock)
{
    RtlReleaseSRWLockShared((PRTL_SRWLOCK)Lock);
}

FORCEINLINE
PLARGE_INTEGER
GetNtTimeout(PLARGE_INTEGER Time, DWORD Timeout)
{
    if (Timeout == INFINITE) return NULL;
    Time->QuadPart = (ULONGLONG)Timeout * -10000;
    return Time;
}

BOOL
WINAPI
SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable, PCRITICAL_SECTION CriticalSection, DWORD Timeout)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableCS(ConditionVariable, (PRTL_CRITICAL_SECTION)CriticalSection, GetNtTimeout(&Time, Timeout));
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK Lock, DWORD Timeout, ULONG Flags)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableSRW(ConditionVariable, Lock, GetNtTimeout(&Time, Timeout), Flags);
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

VOID
WINAPI
WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeAllConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}


/*
* @implemented
*/
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                        IN DWORD dwSpinCount,
                                        IN DWORD flags)
{
    NTSTATUS Status;

    /* FIXME: Flags ignored */

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionAndSpinCount(
        (PRTL_CRITICAL_SECTION)lpCriticalSection,
        dwSpinCount);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/***********************************************************************
 *           InitOnceBeginInitialize    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitOnceBeginInitialize( INIT_ONCE *once, DWORD flags,
                                                       BOOL *pending, void **context )
{
    NTSTATUS status = RtlRunOnceBeginInitialize( once, flags, context );
    if (status >= 0) *pending = (status == STATUS_PENDING);
    else SetLastError( RtlNtStatusToDosError(status) );
    return status >= 0;
}

/***********************************************************************
 *           InitOnceComplete    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH InitOnceComplete( INIT_ONCE *once, DWORD flags, void *context )
{
	NTSTATUS status = RtlRunOnceComplete( once, flags, context );
    return status;
}
