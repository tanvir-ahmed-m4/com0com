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
 * Revision 1.3  2006/11/10 14:07:40  vfrolov
 * Implemented remove command
 *
 * Revision 1.2  2006/11/02 16:20:44  vfrolov
 * Added usage the fixed port numbers
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 */

#ifndef _C0C_DEVUTILS_H_
#define _C0C_DEVUTILS_H_

///////////////////////////////////////////////////////////////

struct DevProperties {
  DevProperties() : pDevId(), pPhObjName(), pLocation() {}

  const char *pDevId;
  const char *pPhObjName;
  const char *pLocation;
};

typedef const DevProperties *PCDevProperties;

class InfFile;

typedef BOOL (* PDEVCALLBACK)(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    void *pParam);

///////////////////////////////////////////////////////////////

int EnumDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam);

int DisableDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired);

BOOL DisableDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired);

BOOL RestartDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired);

BOOL RemoveDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired);

BOOL RemoveDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired);

BOOL InstallDevice(
    InfFile &infFile,
    const char *pDevId,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam);

///////////////////////////////////////////////////////////////

#endif /* _C0C_DEVUTILS_H_ */
