/*
 * $Id$
 *
 * Copyright (c) 2006 Vyacheslav Frolov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * $Log$
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_DEVUTILS_H_
#define _C0C_DEVUTILS_H_

class InfFile;

int CountDevices(
    InfFile &infFile,
    const char *pDevId);

BOOL DisableDevices(
    InfFile &infFile,
    const char *pDevId,
    BOOL *pRebootRequired);

BOOL RestartDevices(
    InfFile &infFile,
    const char *pDevId,
    const char *pPhDevName,
    BOOL *pRebootRequired);

BOOL RemoveDevices(
    InfFile &infFile,
    const char *pDevId,
    BOOL *pRebootRequired);

BOOL InstallDevice(
    InfFile &infFile,
    const char *pDevId);

#endif /* _C0C_DEVUTILS_H_ */
