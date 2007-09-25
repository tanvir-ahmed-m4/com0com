/*
 * $Id$
 *
 * Copyright (c) 2006-2007 Vyacheslav Frolov
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
 * Revision 1.7  2007/09/25 12:42:49  vfrolov
 * Fixed update command (bug if multiple pairs active)
 * Fixed uninstall command (restore active ports on cancell)
 *
 * Revision 1.6  2007/09/14 12:58:44  vfrolov
 * Removed INSTALLFLAG_FORCE
 * Added UpdateDriverForPlugAndPlayDevices() retrying
 *
 * Revision 1.5  2007/02/15 08:48:45  vfrolov
 * Fixed 1658441 - Installation Failed
 * Thanks to Michael A. Smith
 *
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
    pDevProperties = NULL;
    pParam1 = NULL;
    pParam2 = NULL;
    count = 0;
    pRebootRequired = NULL;
  }

  PCDevProperties pDevProperties;
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
static const char *SetStr(char **ppDst, const char *pSrc)
{
  if (*ppDst) {
    LocalFree(*ppDst);
    *ppDst = NULL;
  }

  if (pSrc) {
    int len = lstrlen(pSrc) + 1;

    *ppDst = (char *)LocalAlloc(LPTR, len * sizeof(pSrc[0]));

    if (*ppDst) {
      SNPRINTF(*ppDst, len, "%s", pSrc);
    } else {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(len * sizeof(pSrc[0])));
    }
  }

  return *ppDst;
}


const char *DevProperties::DevId(const char *_pDevId)
{
  return SetStr(&pDevId, _pDevId);
}

const char *DevProperties::PhObjName(const char *_pPhObjName)
{
  return SetStr(&pPhObjName, _pPhObjName);
}

const char *DevProperties::Location(const char *_pLocation)
{
  return SetStr(&pLocation, _pLocation);
}
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

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hwid, sizeof(hwid), NULL)) {
      if (!devParams.devProperties.DevId(hwid)) {
        res = IDCANCEL;
        break;
      }
    }

    if (pEnumParams->pDevProperties &&
        pEnumParams->pDevProperties->DevId() && (!devParams.devProperties.DevId() ||
        lstrcmpi(devParams.devProperties.DevId(), pEnumParams->pDevProperties->DevId())))
    {
      continue;
    }

    char location[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, NULL, (PBYTE)location, sizeof(location), NULL)) {
      if (!devParams.devProperties.Location(location)) {
        res = IDCANCEL;
        break;
      }
    }

    if (pEnumParams->pDevProperties &&
        pEnumParams->pDevProperties->Location() && (!devParams.devProperties.Location() ||
        lstrcmpi(devParams.devProperties.Location(), pEnumParams->pDevProperties->Location())))
    {
      continue;
    }

    char name[40];

    if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)name, sizeof(name), NULL)) {
      if (!devParams.devProperties.PhObjName(name)) {
        res = IDCANCEL;
        break;
      }
    }

    if (pEnumParams->pDevProperties &&
        pEnumParams->pDevProperties->PhObjName() && (!devParams.devProperties.PhObjName() ||
        lstrcmpi(devParams.devProperties.PhObjName(), pEnumParams->pDevProperties->PhObjName())))
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
static BOOL IsDisabled(PSP_DEVINFO_DATA pDevInfoData)
{
  ULONG status = 0;
  ULONG problem = 0;

  if (CM_Get_DevNode_Status(&status, &problem, pDevInfoData->DevInst, 0) != CR_SUCCESS)
    return FALSE;

  return (status & DN_HAS_PROBLEM) != 0 && problem == CM_PROB_DISABLED;
}
///////////////////////////////////////////////////////////////
static int EnumDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  /*
  Trace("Enumerated %s %s %s\n",
        pDevParams->devProperties.Location(),
        pDevParams->devProperties.DevId(),
        pDevParams->devProperties.PhObjName());
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

  enumParams.pDevProperties = pDevProperties;
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
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack)
{
  if (IsDisabled(pDevInfoData))
    return IDCONTINUE;

  if (!ChangeState(hDevInfo, pDevInfoData, DICS_DISABLE))
    return IDCANCEL;

  Trace("Disabled %s %s %s\n",
        pDevProperties->Location(),
        pDevProperties->DevId(),
        pDevProperties->PhObjName());

  if (pRebootRequired && !*pRebootRequired) {
    BOOL rebootRequired = FALSE;

    if (!UpdateRebootRequired(hDevInfo, pDevInfoData, &rebootRequired))
      return IDCANCEL;

    if (rebootRequired) {
      int res = ShowMsg(MB_CANCELTRYCONTINUE,
                        "Can't disable device %s %s %s.\n"
                        "Close application that use this device and Try Again.\n"
                        "Or Continue and then reboot system.\n",
                        pDevProperties->Location(),
                        pDevProperties->DevId(),
                        pDevProperties->PhObjName());

      if (res != IDCONTINUE) {
        if (!ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE))
          return IDCANCEL;

        Trace("Enabled %s %s %s\n",
              pDevProperties->Location(),
              pDevProperties->DevId(),
              pDevProperties->PhObjName());

        return res;
      }

      *pRebootRequired = TRUE;
    }
  }

  if (pDevPropertiesStack) {
    DevProperties *pDevProp = new DevProperties(*pDevProperties);

    if (pDevProp) {
      StackEl *pElem = new StackEl(pDevProp);

      if (pElem)
        pDevPropertiesStack->Push(pElem);
      else
        delete pDevProp;
    }
  }

  return IDCONTINUE;
}
///////////////////////////////////////////////////////////////
static int DisableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  int res = DisableDevice(hDevInfo,
                          pDevInfoData,
                          &pDevParams->devProperties,
                          pDevParams->pEnumParams->pRebootRequired,
                          (Stack *)pDevParams->pEnumParams->pParam1);

  if (res == IDCONTINUE)
    pDevParams->pEnumParams->count++;

  return res;
}

BOOL DisableDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    Stack *pDevPropertiesStack)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;
  enumParams.pParam1 = pDevPropertiesStack;

  do {
    res = EnumDevices(infFile, DIGCF_PRESENT, DisableDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  Sleep(1000);

  return TRUE;
}
///////////////////////////////////////////////////////////////
static int EnableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, PDevParams pDevParams)
{
  if (ChangeState(hDevInfo, pDevInfoData, DICS_ENABLE)) {
    Trace("Enabled %s %s %s\n",
          pDevParams->devProperties.Location(),
          pDevParams->devProperties.DevId(),
          pDevParams->devProperties.PhObjName());
  }

  return IDCONTINUE;
}

BOOL EnableDevices(
    InfFile &infFile,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired)
{
  EnumParams enumParams;

  int res;

  enumParams.pRebootRequired = pRebootRequired;
  enumParams.pDevProperties = pDevProperties;

  do {
    res = EnumDevices(infFile, DIGCF_PRESENT, EnableDevice, &enumParams);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

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
                        pDevParams->devProperties.Location(),
                        pDevParams->devProperties.DevId(),
                        pDevParams->devProperties.PhObjName());

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
        pDevParams->devProperties.Location(),
        pDevParams->devProperties.DevId(),
        pDevParams->devProperties.PhObjName());

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
  enumParams.pDevProperties = pDevProperties;

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
                  pDevProperties->DevId(), pDevProperties->PhObjName());
    return FALSE;
  }

  Trace("Removed %s %s %s\n", pDevProperties->Location(), pDevProperties->DevId(), pDevProperties->PhObjName());

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
  enumParams.pDevProperties = pDevProperties;

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
static int TryInstallDevice(
    InfFile &infFile,
    const char *pDevId,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam)
{
  GUID classGUID;
  char className[32];
  DWORD updateErr;

  updateErr = ERROR_SUCCESS;

  if (!SetupDiGetINFClass(infFile.Path(), &classGUID, className, sizeof(className)/sizeof(className[0]), 0)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiGetINFClass(%s)", infFile.Path());
    return IDCANCEL;
  }

  //Trace("className=%s\n", className);

  HDEVINFO hDevInfo;

  hDevInfo = SetupDiCreateDeviceInfoList(&classGUID, 0);

  if (hDevInfo == INVALID_HANDLE_VALUE) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupDiCreateDeviceInfoList()");
    return IDCANCEL;
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

  SNPRINTF(hardwareId, sizeof(hardwareId)/sizeof(hardwareId[0]) - 1, "%s", pDevId);

  int hardwareIdLen;

  hardwareIdLen = lstrlen(hardwareId) + 1 + 1;
  hardwareId[hardwareIdLen - 1] = 0;

  res = SetupDiSetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                         (LPBYTE)hardwareId, hardwareIdLen * sizeof(hardwareId[0]));
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

    if (!devProperties.DevId(pDevId)) {
      res = FALSE;
      goto err1;
    }

    res = pDevCallBack(hDevInfo, &devInfoData, &devProperties, NULL, pCallBackParam);

    if (!res)
      goto err1;
  }

  int i;

  for (i = 0 ; i < 10 ; i++) {
    BOOL rebootRequired;

    res = UpdateDriverForPlugAndPlayDevices(0, pDevId, infFile.Path(), 0, &rebootRequired);

    if (res) {
      updateErr = ERROR_SUCCESS;
    } else {
      updateErr = GetLastError();

      if (updateErr == ERROR_SHARING_VIOLATION) {
        Trace(".");
        Sleep(1000);
        continue;
      }
    }

    if (i)
      Trace("\n");

    break;
  }

  if (updateErr != ERROR_SUCCESS) {
err1:

    if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &devInfoData))
      ShowLastError(MB_OK|MB_ICONWARNING, "SetupDiCallClassInstaller()");
  }

err:

  SetupDiDestroyDeviceInfoList(hDevInfo);

  if (updateErr != ERROR_SUCCESS) {
    if (updateErr == ERROR_FILE_NOT_FOUND) {
      LONG err;
      HKEY hKey;

      err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, KEY_READ, &hKey);

      if (err == ERROR_SUCCESS)
        RegCloseKey(hKey);

      if (err == ERROR_FILE_NOT_FOUND) {
        int res2 = ShowMsg(MB_CANCELTRYCONTINUE,
                           "Can't update driver. Possible it's because your Windows registry is corrupted and\n"
                           "there is not the following key:\n"
                           "\n"
                           "HKEY_LOCAL_MACHINE\\" REGSTR_PATH_RUNONCE "\n"
                           "\n"
                           "Continue to add the key to the registry.\n");

        if (res2 == IDCONTINUE) {
          err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

          if (err == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return IDTRYAGAIN;
          } else {
            ShowLastError(MB_OK|MB_ICONSTOP, "RegCreateKeyEx()");
            return IDCANCEL;
          }
        }

        return res2;
      }
    }

    return ShowError(MB_CANCELTRYCONTINUE, updateErr, "UpdateDriverForPlugAndPlayDevices()");
  }

  return res ? IDCONTINUE : IDCANCEL;
}

BOOL InstallDevice(
    InfFile &infFile,
    const char *pDevId,
    PDEVCALLBACK pDevCallBack,
    void *pCallBackParam)
{
  int res;

  do {
    res = TryInstallDevice(infFile, pDevId, pDevCallBack, pCallBackParam);
  } while (res == IDTRYAGAIN);

  return res == IDCONTINUE;
}
///////////////////////////////////////////////////////////////
