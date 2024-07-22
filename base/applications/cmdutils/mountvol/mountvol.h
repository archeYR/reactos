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

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>

#endif /* __MOUNTVOL_H__ */
