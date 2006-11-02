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
    pDevId = NULL;
    pPhObjName = NULL;
    pParam1 = NULL;
    pParam2 = NULL;

    count = 0;
    pRebootRequired = NULL;
  }

  const char *pDevId;
  const char *pPhObjName;
  void *pParam1;
  void *pParam2;

  int count;
  BOOL *pRebootRequired;
};

typedef EnumParams *PEnumParams;

struct DevParams {
  DevParams(PEnumParams _pEnumParams) {
    pEnumParams = _pEnumParams;

    pDevId = NULL;
    pPhObjName = NULL;
  }

  PEnumParams pEnumParams;

  const char *pDevId;
  const char *pPhObjName;
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

    char hwid[150];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hwid, sizeof(hwid), NULL))
      devParams.pDevId = hwid;

    if (pEnumParams->pDevId && (!devParams.pDevId || lstrcmpi(devParams.pDevId, pEnumParams->pDevId)))
      continue;

    char name[150];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)name, sizeof(name), NULL))
      devParams.pPhObjName = name;

    if (pEnumParams->pPhObjName && (!devParams.pPhObjName || lstrcmpi(devParams.pPhObjName, pEnumParams->pPhObjName)))
      continue;

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
  //Trace("Enumerated %s %s\n", pDevParams->pDevId, pDevParams->pPhObjName);

  int res = IDCONTINUE;

  if (pDevParams->pEnumParams->pParam1) {
    if (!PDEVCALLBACK(pDevParams->pEnumParams->pParam1)(hDevInfo,
                                                        pDevInfoData,
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
    const char *pDevId,
    BOOL *pRebootRequired,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam)
{
  EnumParams enumParams;

  enumParams.pDevId = pDevId;
  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pParam1 = pDevCallBack;
  enumParams.pParam2 = pCallBackParam;

  if (EnumDevices(infFile, DIGCF_PRESENT, EnumDevice, &enumParams) != IDCONTINUE)
    return -1;

  return enumParams.count;
}
///////////////////////////////////////////////////////////////
static int DisableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!ChangeState(hDevInfo, pDevInfoData, DICS_DISABLE))
    return IDCANCEL;

  Trace("Disabled %s %s\n", pDevParams->pDevId, pDevParams->pPhObjName);
  pDevParams->pEnumParams->count++;

  if (pDevParams->pEnumParams->pRebootRequired && !*pDevParams->pEnumParams->pRebootRequired) {
    BOOL rebootRequired = FALSE;

    if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
      return IDCANCEL;

    if (rebootRequired) {
      int res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't disable device %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevParams->pDevId, pDevParams->pPhObjName);

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        Trace("Enabled %s %s\n", pDevParams->pDevId, pDevParams->pPhObjName);
        pDevParams->pEnumParams->count--;

        return res;
      }

      *pDevParams->pEnumParams->pRebootRequired = TRUE;
    }
  }

  return IDCONTINUE;
}

BOOL DisableDevices(
    InfFile &infFile,
    const char *pDevId,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevId = pDevId;

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
                        "Can't reastart device %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevParams->pDevId, pDevParams->pPhObjName);

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        return res;
      }

      *pDevParams->pEnumParams->pRebootRequired = TRUE;
    }
  }

  if (!pDevParams->pEnumParams->pRebootRequired || !*pDevParams->pEnumParams->pRebootRequired) {
    Trace("Restarted %s %s\n", pDevParams->pDevId, pDevParams->pPhObjName);
    pDevParams->pEnumParams->count++;
  }

  return IDCONTINUE;
}

BOOL RestartDevices(
    InfFile &infFile,
    const char *pDevId,
    const char *pPhDevName,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevId = pDevId;
  enumParams.pPhObjName = pPhDevName;

  do {
    res = EnumDevices(infFile, DIGCF_PRESENT, RestartDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static int RemoveDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, pDevInfoData)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCallClassInstaller(DIF_REMOVE, %s, %s)", pDevParams->pDevId, pDevParams->pPhObjName);
    return IDCANCEL;
  }

  Trace("Removed %s %s\n", pDevParams->pDevId, pDevParams->pPhObjName);
  pDevParams->pEnumParams->count++;

  if (!UpdateRebootRequired(hDevInfo, pDevInfoData, pDevParams->pEnumParams->pRebootRequired))
    return IDCANCEL;

  return IDCONTINUE;
}

BOOL RemoveDevices(
    InfFile &infFile,
    const char *pDevId,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevId = pDevId;

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
    res = pDevCallBack(hDevInfo, &devInfoData, NULL, pCallBackParam);

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
