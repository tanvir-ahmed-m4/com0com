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
 * Revision 1.4  2006/11/10 14:07:40  vfrolov
 * Implemented remove command
 *
 * Revision 1.3  2006/11/02 16:20:44  vfrolov
 * Added usage the fixed port numbers
 *
 * Revision 1.2  2006/10/13 10:26:35  vfrolov
 * Some defines moved to ../include/com0com.h
 * Changed name of device object (for WMI)
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "inffile.h"
#include "msg.h"
#include "devutils.h"
#include "utils.h"

///////////////////////////////////////////////////////////////
struct EnumParams {
  EnumParams() {
    pParam1 = NULL;
    pParam2 = NULL;
    count = 0;
    pRebootRequired = NULL;
  }

  DevProperties devProperties;
  void *pParam1;
  void *pParam2;
  int count;
  BOOL *pRebootRequired;
};

typedef EnumParams *PEnumParams;

struct DevParams {
  DevParams(PEnumParams _pEnumParams) {
    pEnumParams = _pEnumParams;
  }

  PEnumParams pEnumParams;
  DevProperties devProperties;
};

typedef DevParams *PDevParams;
///////////////////////////////////////////////////////////////
static int EnumDevices(
    InfFile &infFile,
    DWORD flags,
    BOOL (* pFunk)(HDEVINFO, PSP_DEVINFO_DATA, PDevParams),
    PEnumParams pEnumParams)
{
  HDEVINFO hDevInfo;

  hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES|flags);

  if (hDevInfo == INVALID_HANDLE_VALUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetClassDevs()");
    return IDCANCEL;
  }

  SP_DEVINFO_DATA devInfoData;

  devInfoData.cbSize = sizeof(devInfoData);

  int res = IDCONTINUE;

  for (int i = 0 ; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData) ; i++) {
    char classGUID[100];

    if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_CLASSGUID, NULL, (PBYTE)classGUID, sizeof(classGUID), NULL))
      continue;

    if (lstrcmpi(classGUID, infFile.ClassGUID()))
      continue;

    char provider[100];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_MFG, NULL, (PBYTE)provider, sizeof(provider), NULL)) {
      // check provider if exists
      if (lstrcmpi(provider, infFile.Provider()))
        continue;
    }

    DevParams devParams(pEnumParams);

    char hwid[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hwid, sizeof(hwid), NULL))
      devParams.devProperties.pDevId = hwid;

    if (pEnumParams->devProperties.pDevId && (!devParams.devProperties.pDevId ||
        lstrcmpi(devParams.devProperties.pDevId, pEnumParams->devProperties.pDevId)))
    {
      continue;
    }

    char location[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, NULL, (PBYTE)location, sizeof(location), NULL))
      devParams.devProperties.pLocation = location;

    if (pEnumParams->devProperties.pLocation && (!devParams.devProperties.pLocation ||
        lstrcmpi(devParams.devProperties.pLocation, pEnumParams->devProperties.pLocation)))
    {
      continue;
    }

    char name[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)name, sizeof(name), NULL))
      devParams.devProperties.pPhObjName = name;

    if (pEnumParams->devProperties.pPhObjName && (!devParams.devProperties.pPhObjName ||
        lstrcmpi(devParams.devProperties.pPhObjName, pEnumParams->devProperties.pPhObjName)))
    {
      continue;
    }

    res = pFunk(hDevInfo, &devInfoData, &devParams);

    if (res != IDCONTINUE)
      break;
  }

  DWORD err = GetLastError();

  SetupDiDestroyDeviceInfoList(hDevInfo);

  SetLastError(err);

  return res;
}
///////////////////////////////////////////////////////////////
static BOOL UpdateRebootRequired(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, BOOL *pRebootRequired)
{
  if (!pRebootRequired)
    return TRUE;

  if (*pRebootRequired)
    return TRUE;

  SP_DEVINSTALL_PARAMS installParams;

  memset(&installParams, 0, sizeof(installParams));
  installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

  if (!SetupDiGetDeviceInstallParams(hDevInfo, pDevInfoData, &installParams)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetDeviceInstallParams()");
    return FALSE;
  }

  *pRebootRequired = (installParams.Flags & DI_NEEDREBOOT) ? TRUE : FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL ChangeState(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD stateChange)
{
  SP_PROPCHANGE_PARAMS propChangeParams;

  memset(&propChangeParams, 0, sizeof(propChangeParams));
  propChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
  propChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
  propChangeParams.StateChange = stateChange;
  propChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
  propChangeParams.HwProfile = 0;

  if (!SetupDiSetClassInstallParams(hDevInfo, pDevInfoData, (SP_CLASSINSTALL_HEADER *)&propChangeParams, sizeof(propChangeParams))) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiSetClassInstallParams()");
    return FALSE;
  }

  if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, pDevInfoData)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller(DIF_PROPERTYCHANGE)");
    return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
static int EnumDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  /*
  Trace("Enumerated %s %s %s\n",
        pDevParams->devProperties.pLocation,
        pDevParams->devProperties.pDevId,
        pDevParams->devProperties.pPhObjName);
  */

  int res = IDCONTINUE;

  if (pDevParams->pEnumParams->pParam1) {
    if (!PDEVCALLBACK(pDevParams->pEnumParams->pParam1)(hDevInfo,
                                                        pDevInfoData,
                                                        &pDevParams->devProperties,
                                                        pDevParams->pEnumParams->pRebootRequired,
                                                        pDevParams->pEnumParams->pParam2))
    {
      res = IDCANCEL;
    }
  }

  pDevParams->pEnumParams->count++;

  return res;
}

int EnumDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam)
{
  EnumParams enumParams;

  if (pDevProperties)
    enumParams.devProperties = *pDevProperties;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pParam1 = pDevCallBack;
  enumParams.pParam2 = pCallBackParam;

  if (EnumDevices(infFile, DIGCF_PRESENT, EnumDevice, &enumParams) != IDCONTINUE)
    return -1;

  return enumParams.count;
}
///////////////////////////////////////////////////////////////
int DisableDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  if (!ChangeState(hDevInfo, pDevInfoData, DICS_DISABLE))
    return IDCANCEL;

  Trace("Disabled %s %s %s\n",
        pDevProperties->pLocation,
        pDevProperties->pDevId,
        pDevProperties->pPhObjName);

  if (pRebootRequired && !*pRebootRequired) {
    BOOL rebootRequired = FALSE;

    if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
      return IDCANCEL;

    if (rebootRequired) {
      int res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't disable device %s %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevProperties->pLocation,
                        pDevProperties->pDevId,
                        pDevProperties->pPhObjName);

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        Trace("Enabled %s %s %s\n",
              pDevProperties->pLocation,
              pDevProperties->pDevId,
              pDevProperties->pPhObjName);

        return res;
      }

      *pRebootRequired = TRUE;
    }
  }

  return IDCONTINUE;
}
///////////////////////////////////////////////////////////////
static int DisableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  int res = DisableDevice(hDevInfo, pDevInfoData, &pDevParams->devProperties, pDevParams->pEnumParams->pRebootRequired);

  if (res != IDCONTINUE)
    pDevParams->pEnumParams->count++;

  return res;
}

BOOL DisableDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  if (pDevProperties)
    enumParams.devProperties = *pDevProperties;

  do {
    res = EnumDevices(infFile, DIGCF_PRESENT, DisableDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  Sleep(1000);

  return TRUE;
}
///////////////////////////////////////////////////////////////
static int RestartDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!ChangeState(hDevInfo, pDevInfoData, DICS_PROPCHANGE))
    return IDCANCEL;

  if (pDevParams->pEnumParams->pRebootRequired && !*pDevParams->pEnumParams->pRebootRequired) {
    BOOL rebootRequired = FALSE;

    if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
      return IDCANCEL;

    if (rebootRequired) {
      int res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't reastart device %s %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevParams->devProperties.pLocation,
                        pDevParams->devProperties.pDevId,
                        pDevParams->devProperties.pPhObjName);

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        return res;
      }

      *pDevParams->pEnumParams->pRebootRequired = TRUE;
    }
  }

  if (!pDevParams->pEnumParams->pRebootRequired || !*pDevParams->pEnumParams->pRebootRequired) {
    Trace("Restarted %s %s %s\n",
        pDevParams->devProperties.pLocation,
        pDevParams->devProperties.pDevId,
        pDevParams->devProperties.pPhObjName);

    pDevParams->pEnumParams->count++;
  }

  return IDCONTINUE;
}

BOOL RestartDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  if (pDevProperties)
    enumParams.devProperties = *pDevProperties;

  do {
    res = EnumDevices(infFile, DIGCF_PRESENT, RestartDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL RemoveDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, pDevInfoData)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller(DIF_REMOVE, %s, %s)",
                  pDevProperties->pDevId, pDevProperties->pPhObjName);
    return FALSE;
  }

  Trace("Removed %s %s %s\n", pDevProperties->pLocation, pDevProperties->pDevId, pDevProperties->pPhObjName);

  return UpdateRebootRequired(hDevInfo, pDevInfoData, pRebootRequired);
}
///////////////////////////////////////////////////////////////
static int RemoveDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!RemoveDevice(hDevInfo, pDevInfoData, &pDevParams->devProperties, pDevParams->pEnumParams->pRebootRequired))
    return IDCANCEL;

  pDevParams->pEnumParams->count++;

  return IDCONTINUE;
}

BOOL RemoveDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  if (pDevProperties)
    enumParams.devProperties = *pDevProperties;

  do {
    res = EnumDevices(infFile, 0, RemoveDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  if (!enumParams.count)
    Trace("No devices found\n");

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL InstallDevice(
    InfFile &infFile,
    const char *pDevId,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam)
{
  GUID classGUID;
  char className[32];

  if (!SetupDiGetINFClass(infFile.Path(), &classGUID, className, sizeof(className), 0)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetINFClass(%s)", infFile.Path());
    return FALSE;
  }

  //Trace("className=%s\n", className);

  HDEVINFO hDevInfo;

  hDevInfo = SetupDiCreateDeviceInfoList(&classGUID, 0);

  if (hDevInfo == INVALID_HANDLE_VALUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCreateDeviceInfoList()");
    return FALSE;
  }

  BOOL res;
  SP_DEVINFO_DATA devInfoData;

  devInfoData.cbSize = sizeof(devInfoData);

  res = SetupDiCreateDeviceInfo(hDevInfo, className, &classGUID, NULL, 0,
                                DICD_GENERATE_ID, &devInfoData);
  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCreateDeviceInfo()");
    goto err;
  }

  char hardwareId[MAX_DEVICE_ID_LEN + 1 + 1];

  memset(hardwareId, 0, sizeof(hardwareId));
  lstrcpyn(hardwareId, pDevId, sizeof(hardwareId) - 1 - 1);

  int hardwareIdSize;

  hardwareIdSize = (lstrlen(hardwareId) + 1 + 1) * sizeof(hardwareId[0]);

  res = SetupDiSetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                         (LPBYTE)hardwareId, hardwareIdSize);
  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiSetDeviceRegistryProperty()");
    goto err;
  }

  res = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &devInfoData);

  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller()");
    goto err;
  }

  if (pDevCallBack) {
    DevProperties devProperties;

    devProperties.pDevId = pDevId;

    res = pDevCallBack(hDevInfo, &devInfoData, &devProperties, NULL, pCallBackParam);

    if (!res)
      goto err1;
  }

  BOOL rebootRequired;
  res = UpdateDriverForPlugAndPlayDevices(0, pDevId, infFile.Path(), INSTALLFLAG_FORCE, &rebootRequired);

  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "UpdateDriverForPlugAndPlayDevices()");

err1:

    if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &devInfoData))
      ShowLastError(MB_OK|MB_ICONWARNING, "SetupDiCallClassInstaller()");
  }

err:

  SetupDiDestroyDeviceInfoList(hDevInfo);
  return res;
}
///////////////////////////////////////////////////////////////
