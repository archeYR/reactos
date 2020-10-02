
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall GetFileInformationByHandleEx(long long ptr long)
@ stdcall GetFinalPathNameByHandleW(long ptr long long)
@ stdcall -ret64 GetTickCount64()

@ stdcall InitializeSRWLock(ptr)
@ stdcall AcquireSRWLockExclusive(ptr)
@ stdcall AcquireSRWLockShared(ptr)
@ stdcall ReleaseSRWLockExclusive(ptr)
@ stdcall ReleaseSRWLockShared(ptr)

@ stdcall InitializeConditionVariable(ptr)
@ stdcall SleepConditionVariableCS(ptr ptr long)
@ stdcall SleepConditionVariableSRW(ptr ptr long long)
@ stdcall WakeAllConditionVariable(ptr)
@ stdcall WakeConditionVariable(ptr)

@ stdcall InitializeCriticalSectionEx(ptr long long)
@ stdcall TryAcquireSRWLockExclusive(ptr)
@ stdcall TryAcquireSRWLockShared(ptr) ntdll_vista.RtlTryAcquireSRWLockShared

@ stdcall InitOnceBeginInitialize(ptr long ptr ptr)
@ stdcall InitOnceComplete(ptr long ptr)

@ stdcall SetFileInformationByHandle(long long ptr long)