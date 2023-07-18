/*
 * 3D Text OpenGL Screensaver (3dtext.h)
 *
 * Copyright 2007 Marc Piulachs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GLGEARS_H_
#define _GLGEARS_H_

#include <windef.h>
#include <winuser.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <scrnsave.h>
#include <wine/wgl.h>
#include <wine/wglext.h>
#include "resource.h"

typedef struct _SETTINGS
{
    BOOL sRGBMode;
    DWORD Multisampling;
    BOOL Info;
} SETTINGS, *PSETTINGS;

VOID LoadSettings(PSETTINGS Settings);
VOID SaveSettings(PSETTINGS Settings);

#endif /* _GLGEARS_H_ */
