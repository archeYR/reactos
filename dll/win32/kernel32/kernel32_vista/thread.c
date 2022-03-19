#include <k32_vista.h>
#include <debug.h>
#if 1

NTSYSAPI NTSTATUS  WINAPI TpAllocCleanupGroup(TP_CLEANUP_GROUP **);
NTSYSAPI NTSTATUS  WINAPI TpAllocPool(TP_POOL **,PVOID);
NTSYSAPI NTSTATUS  WINAPI TpAllocTimer(TP_TIMER **,PTP_TIMER_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWait(TP_WAIT **,PTP_WAIT_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWork(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,TP_CALLBACK_ENVIRON *environment );
NTSYSAPI NTSTATUS  WINAPI TpCallbackMayRunLong(TP_CALLBACK_INSTANCE *instance);
NTSYSAPI VOID      WINAPI TpSetTimer( TP_TIMER *timer, LARGE_INTEGER *timeout, LONG period, LONG window_length );
NTSYSAPI VOID      WINAPI TpSetWait( TP_WAIT *wait, HANDLE handle, LARGE_INTEGER *timeout );								 


/***********************************************************************
 *              CallbackMayRunLong (KERNEL32.@)
 */
BOOL WINAPI CallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    NTSTATUS status;

    DPRINT1( "CallbackMayRunLong %p\n", instance );

    status = TpCallbackMayRunLong( instance );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              CreateThreadpool (KERNEL32.@)
 */
PTP_POOL WINAPI CreateThreadpool( PVOID reserved )
{
    TP_POOL *pool;
    NTSTATUS status;

    DPRINT1( "CreateThreadpool %p\n", reserved );

    status = TpAllocPool( &pool, reserved );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return pool;
}

/***********************************************************************
 *              CreateThreadpoolCleanupGroup (KERNEL32.@)
 */
PTP_CLEANUP_GROUP WINAPI CreateThreadpoolCleanupGroup( void )
{
    TP_CLEANUP_GROUP *group;
    NTSTATUS status;

    DPRINT1( "CreateThreadpoolCleanupGroup \n" );

    status = TpAllocCleanupGroup( &group );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return group;
}

/***********************************************************************
 *              CreateThreadpoolIo (KERNEL32.@)
 */
PTP_IO WINAPI CreateThreadpoolIo( HANDLE handle, PTP_WIN32_IO_CALLBACK callback,
                                  PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    DPRINT1("CreateThreadpoolIo: stub (%p, %p, %p, %p): stub\n", handle, callback, userdata, environment);
    return FALSE;
}

VOID WINAPI CloseThreadpoolIo(PTP_IO pio)
{
    DPRINT1("CloseThreadpoolIo: stub (%p): stub\n", pio);
}

/***********************************************************************
 *              CreateThreadpoolTimer (KERNEL32.@)
 */
PTP_TIMER WINAPI CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                        TP_CALLBACK_ENVIRON *environment )
{
    TP_TIMER *timer;
    NTSTATUS status;

    DPRINT1( "CreateThreadpoolTimer: %p, %p, %p\n", callback, userdata, environment );

    status = TpAllocTimer( &timer, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return timer;
}

/***********************************************************************
 *              CreateThreadpoolWait (KERNEL32.@)
 */
PTP_WAIT WINAPI CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WAIT *wait;
    NTSTATUS status;

    DPRINT1( "CreateThreadpoolWait: %p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWait( &wait, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return wait;
}

/***********************************************************************
 *              CreateThreadpoolWork (KERNEL32.@)
 */
PTP_WORK WINAPI CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;
    NTSTATUS status;

    DPRINT1( "CreateThreadpoolWork: %p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWork( &work, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return work;
}


/***********************************************************************
 *              SetThreadpoolTimer (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolTimer( TP_TIMER *timer, FILETIME *due_time,
                                DWORD period, DWORD window_length )
{
    LARGE_INTEGER timeout;

    DPRINT1( "SetThreadpoolTimer: %p, %p, %u, %u\n", timer, due_time, period, window_length );

    if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetTimer( timer, due_time ? &timeout : NULL, period, window_length );
}

/***********************************************************************
 *              SetThreadpoolWait (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolWait( TP_WAIT *wait, HANDLE handle, FILETIME *due_time )
{
    LARGE_INTEGER timeout;

    DPRINT1("SetThreadpoolWait: %p, %p, %p\n", wait, handle, due_time );

    if (!handle)
    {
        due_time = NULL;
    }
    else if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetWait( wait, handle, due_time ? &timeout : NULL );
}

/***********************************************************************
 *              TrySubmitThreadpoolCallback (KERNEL32.@)
 */
BOOL WINAPI TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                         TP_CALLBACK_ENVIRON *environment )
{
    NTSTATUS status;

    DPRINT1( "%p, %p, %p\n", callback, userdata, environment );

    status = TpSimpleTryPost( callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

#endif