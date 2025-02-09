/*
 * Copyright 2017 Hugh McMaster
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __MOUNTVOL_H__
#define __MOUNTVOL_H__

#include <stdio.h>
#include <tchar.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include "resource.h"
#include <winbase.h>

#include <conutils.h>

#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>

#define NTOS_MODE_USER
#include <ndk/extypes.h>
#include <ndk/exfuncs.h>
#include <ndk/rtlfuncs.h>

//#ifndef __REACTOS__
#define SystemSystemPartitionInformation 0x62
#define SystemBootEnvironmentInformation 0x5A

typedef struct _SYSTEM_SYSTEM_PARTITION_INFORMATION
{
    UNICODE_STRING SystemPartition;
} SYSTEM_SYSTEM_PARTITION_INFORMATION, *PSYSTEM_SYSTEM_PARTITION_INFORMATION;

typedef enum _FIRMWARE_TYPE
{
    FirmwareTypeUnknown,
    FirmwareTypeBios,
    FirmwareTypeUefi,
    FirmwareTypeMax
} FIRMWARE_TYPE, *PFIRMWARE_TYPE;

//#if (NTDDI_VERSION >= NTDDI_LONGHORN)
typedef struct _SYSTEM_BOOT_ENVIRONMENT_INFORMATION
{
    GUID BootIdentifier;
    FIRMWARE_TYPE FirmwareType;
//#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONGLONG BootFlags;
//#endif
} SYSTEM_BOOT_ENVIRONMENT_INFORMATION, *PSYSTEM_BOOT_ENVIRONMENT_INFORMATION;
//#endif
//#endif

#endif /* __MOUNTVOL_H__ */
