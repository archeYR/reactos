/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/format.c
 * PURPOSE:         Volume format
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define IS_VOLUME_NAME(s, l)                       \
  ((l == 96 || (l == 98 && s[48] == '\\')) &&      \
   s[0] == '\\'&& (s[1] == '?' || s[1] == '\\') && \
   s[2] == '?' && s[3] == '\\' && s[4] == 'V' &&   \
   s[5] == 'o' && s[6] == 'l' && s[7] == 'u' &&    \
   s[8] == 'm' && s[9] == 'e' && s[10] == '{' &&   \
   s[19] == '-' && s[24] == '-' && s[29] == '-' && \
   s[34] == '-' && s[47] == '}')

/* FMIFS.6 */
VOID NTAPI
Format(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN PFMIFSCALLBACK Callback)
{
    FormatEx(DriveRoot,
             MediaFlag,
             Format,
             Label,
             QuickFormat,
             0,
             Callback);
}

/* FMIFS.7 */
VOID
NTAPI
FormatEx(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    PIFS_PROVIDER Provider;
    UNICODE_STRING usDriveRoot;
    UNICODE_STRING usLabel;
    BOOLEAN Success = FALSE;
    BOOLEAN BackwardCompatible = FALSE; // Default to latest FS versions.
    MEDIA_TYPE MediaType;
    WCHAR VolumeName[MAX_PATH];

//
// TODO: Convert filesystem Format into ULIB format string.
//
    __debugbreak();
    Provider = GetProvider(Format);
    if (!Provider)
    {
        /* Unknown file system */
        Callback(DONE, 0, &Success);
        return;
    }

    if (IS_VOLUME_NAME(DriveRoot, wcslen(DriveRoot) * sizeof(WCHAR)))
    {
        /* We already got a volume GUID path, no need for conversion */
        wcscpy(VolumeName, DriveRoot);
    }
    else if (!GetVolumeNameForVolumeMountPointW(DriveRoot, VolumeName, RTL_NUMBER_OF(VolumeName)))
    {
        /* Report an error */
        Callback(DONE, 0, &Success);
        return;
    }

    if (!RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, NULL))
    {
        /* Report an error */
        Callback(DONE, 0, &Success);
        return;
    }

    if (usDriveRoot.Buffer[usDriveRoot.Length / sizeof(WCHAR) - 1] == L'\\')
    {
        /* Trim the trailing backslash since we will work with a device object */
        usDriveRoot.Length -= sizeof(WCHAR);
    }

    RtlInitUnicodeString(&usLabel, Label);

    /* Set the BackwardCompatible flag in case we format with older FAT12/16 */
    if (wcsicmp(Format, L"FAT") == 0)
        BackwardCompatible = TRUE;
    // else if (wcsicmp(Format, L"FAT32") == 0)
        // BackwardCompatible = FALSE;

    /* Convert the FMIFS MediaFlag to a NT MediaType */
    // FIXME: Actually covert all the possible flags.
    switch (MediaFlag)
    {
    case FMIFS_FLOPPY:
        MediaType = F5_320_1024; // FIXME: This is hardfixed!
        break;
    case FMIFS_REMOVABLE:
        MediaType = RemovableMedia;
        break;
    case FMIFS_HARDDISK:
        MediaType = FixedMedia;
        break;
    default:
        DPRINT1("Unknown FMIFS MediaFlag %d, converting 1-to-1 to NT MediaType\n",
                MediaFlag);
        MediaType = (MEDIA_TYPE)MediaFlag;
        break;
    }

    DPRINT("Format() - %S\n", Format);
    Success = Provider->Format(&usDriveRoot,
                               Callback,
                               QuickFormat,
                               BackwardCompatible,
                               MediaType,
                               &usLabel,
                               ClusterSize);
    if (!Success)
        DPRINT1("Format() failed\n");

    /* Report success */
    Callback(DONE, 0, &Success);

    RtlFreeUnicodeString(&usDriveRoot);
}

/* EOF */
