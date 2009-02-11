/*
 * $Id$
 *
 * Copyright (c) 2006-2009 Vyacheslav Frolov
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
 * Revision 1.7  2009/02/11 07:35:22  vfrolov
 * Added --no-update option
 *
 * Revision 1.6  2007/10/01 15:01:35  vfrolov
 * Added pDevInstID parameter to InstallDevice()
 *
 * Revision 1.5  2007/09/25 12:42:49  vfrolov
 * Fixed update command (bug if multiple pairs active)
 * Fixed uninstall command (restore active ports on cancell)
 *
 * Revision 1.4  2007/09/17 14:35:06  vfrolov
 * Fixed typo
 *
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

class DevProperties {
  public:
    DevProperties() : pDevId(NULL), pPhObjName(NULL), pLocation(NULL) {}
    DevProperties(const DevProperties &o)
      : pDevId(NULL), pPhObjName(NULL), pLocation(NULL) { *this = o; }
    ~DevProperties() { DevId(NULL); PhObjName(NULL); Location(NULL); }

    DevProperties &operator=(const DevProperties &o)
    {
      DevId(o.DevId()); PhObjName(o.PhObjName()); Location(o.Location());
      return *this;
    }

    const char *DevId(const char *_pDevId);
    const char *PhObjName(const char *_pPhObjName);
    const char *Location(const char *_pLocation);

    const char *DevId() const { return pDevId; }
    const char *PhObjName() const { return pPhObjName; }
    const char *Location() const { return pLocation; }

  private:
    char *pDevId;
    char *pPhObjName;
    char *pLocation;
};

typedef const DevProperties *PCDevProperties;

class InfFile;
class Stack;

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
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack);

BOOL DisableDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack);

BOOL EnableDevices(
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
    const char *pDevInstID,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam,
    BOOL update);

///////////////////////////////////////////////////////////////

#endif /* _C0C_DEVUTILS_H_ */
