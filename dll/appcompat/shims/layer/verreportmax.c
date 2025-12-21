/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ForceDxSetupSuccess shim
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winver.h>
#include <shimlib.h>

typedef BOOL (WINAPI* VERQUERYVALUEAPROC)(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);


#define SHIM_NS         VerReportMax
#include <setup_shim.inl>

BOOL WINAPI SHIM_OBJ_NAME(APIHook_VerQueryValueA)(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)
{
    if (CALL_SHIM(0, VERQUERYVALUEAPROC)(pBlock, lpSubBlock, lplpBuffer, puLen))
    {
        if (!strcmp(lpSubBlock, "\\"))
        {
            VS_FIXEDFILEINFO *verinfo = (VS_FIXEDFILEINFO *)(*lplpBuffer);

            verinfo->dwFileVersionMS = 0xFFFFFFFF;
            verinfo->dwFileVersionLS = 0xFFFFFFFF;
            verinfo->dwProductVersionMS = 0xFFFFFFFF;
            verinfo->dwProductVersionLS = 0xFFFFFFFF;
            return TRUE;
        }
    }

    return FALSE;
}


#define SHIM_NUM_HOOKS  1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "VERSION.DLL", "VerQueryValueA", SHIM_OBJ_NAME(APIHook_VerQueryValueA))

#include <implement_shim.inl>
