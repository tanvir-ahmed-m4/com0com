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
 * Revision 1.28  2009/01/12 12:48:05  vfrolov
 * Fixed typo
 *
 * Revision 1.27  2008/12/25 16:58:45  vfrolov
 * Implemented busynames command
 *
 * Revision 1.26  2008/12/24 15:32:22  vfrolov
 * Added logging COM port numbers in the COM port database
 *
 * Revision 1.25  2008/09/12 12:21:49  vfrolov
 * Added --silent option
 *
 * Revision 1.24  2008/09/12 09:55:59  vfrolov
 * Fixed help cutting
 *
 * Revision 1.23  2008/04/02 10:28:24  vfrolov
 * Added reload command
 *
 * Revision 1.22  2007/11/27 16:32:54  vfrolov
 * Added disable and enable options
 *
 * Revision 1.21  2007/10/19 16:09:55  vfrolov
 * Implemented --detail-prms option
 *
 * Revision 1.20  2007/10/15 13:49:04  vfrolov
 * Added entry point MainA
 *
 * Revision 1.19  2007/10/05 07:28:26  vfrolov
 * Added listing pairs w/o PortNum
 *
 * Revision 1.18  2007/10/01 15:44:19  vfrolov
 * Added check for install two ports with the same name
 *
 * Revision 1.17  2007/10/01 15:01:35  vfrolov
 * Added pDevInstID parameter to InstallDevice()
 *
 * Revision 1.16  2007/09/25 12:42:49  vfrolov
 * Fixed update command (bug if multiple pairs active)
 * Fixed uninstall command (restore active ports on cancell)
 *
 * Revision 1.15  2007/07/03 14:42:10  vfrolov
 * Added friendly name setting for bus device
 *
 * Revision 1.14  2007/06/14 16:14:19  vfrolov
 * Added test for "in use" in the COM port database
 *
 * Revision 1.13  2007/05/29 15:30:41  vfrolov
 * Fixed big hepl text interrupt
 *
 * Revision 1.12  2007/01/11 15:05:03  vfrolov
 * Replaced strtok() by STRTOK_R()
 *
 * Revision 1.11  2006/11/21 11:36:06  vfrolov
 * Added --output option
 *
 * Revision 1.10  2006/11/10 14:07:40  vfrolov
 * Implemented remove command
 *
 * Revision 1.9  2006/11/03 16:13:29  vfrolov
 * Added port name length checkings
 *
 * Revision 1.8  2006/11/03 13:22:07  vfrolov
 * Added checking of BusyMask::AddNum() return value
 *
 * Revision 1.7  2006/11/02 16:20:44  vfrolov
 * Added usage the fixed port numbers
 *
 * Revision 1.6  2006/10/27 13:23:49  vfrolov
 * Added check if port name is already used for other device
 * Fixed incorrect port restart
 * Fixed prompts
 *
 * Revision 1.5  2006/10/23 12:08:31  vfrolov
 * Added interactive mode
 * Added more help
 * Added SetTitle() calls
 *
 * Revision 1.4  2006/10/19 13:28:50  vfrolov
 * Added InfFile::UninstallAllInfFiles()
 *
 * Revision 1.3  2006/10/13 10:26:35  vfrolov
 * Some defines moved to ../include/com0com.h
 * Changed name of device object (for WMI)
 *
 * Revision 1.2  2006/08/25 10:36:48  vfrolov
 * Added C0C_PREF_PORT_NAME_A and C0C_PREF_PORT_NAME_B defines
 * Added deleting Class subkeys
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "inffile.h"
#include "params.h"
#include "devutils.h"
#include "msg.h"
#include "utils.h"
#include "portnum.h"
#include "comdb.h"

#define TEXT_PREF
#include "../include/com0com.h"

#define C0C_INF_NAME             "com0com.inf"
#define C0C_CLASS_GUID           "{df799e12-3c56-421b-b298-b6d3642bc878}"
#define C0C_CLASS                "CNCPorts"
#define C0C_PROVIDER             "Vyacheslav Frolov"
#define C0C_REGKEY_EVENTLOG      REGSTR_PATH_SERVICES "\\Eventlog\\System\\" C0C_SERVICE
#define C0C_COPY_DRIVERS_SECTION "com0com_CopyDrivers"

#define C0C_SETUP_TITLE          "Setup for com0com"

///////////////////////////////////////////////////////////////
static BOOL detailPrms = FALSE;
static BOOL silent = FALSE;
///////////////////////////////////////////////////////////////
static BOOL IsValidPortNum(int num)
{
  if (num < 0)
    return FALSE;

  char buf[C0C_PORT_NAME_LEN + 1];

  if (SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_BUS_NAME "%d", num) < 0 ||
      SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_PORT_NAME_A "%d", num) < 0 ||
      SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_PORT_NAME_B "%d", num) < 0)
  {
    int res = ShowMsg(MB_OKCANCEL|MB_ICONWARNING, "The port number %d is too big.\n", num);

    if (res == IDCANCEL)
      return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL IsValidPortName(
    const char *pPortName)
{
  int res;

  if (lstrlen(pPortName) > C0C_PORT_NAME_LEN) {
    res = ShowMsg(MB_OKCANCEL|MB_ICONWARNING,
                  "The length of port name %s\n"
                  "is too big (greater then %d).\n",
                  pPortName, C0C_PORT_NAME_LEN);

    if (res == IDCANCEL)
      return FALSE;
  }

  do {
    res = IDCONTINUE;

    char phDevName[80];

    if (!QueryDosDevice(pPortName, phDevName, sizeof(phDevName)/sizeof(phDevName[0]))) {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        continue;

      phDevName[0] = 0;
    }

    res = ShowMsg(MB_CANCELTRYCONTINUE,
                  "The port name %s is already used for other device %s.",
                  pPortName, phDevName);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  do {
    res = IDCONTINUE;

    BOOL inUse;

    if (!ComDbGetInUse(pPortName, inUse)) {
      res = IDCANCEL;
      continue;
    }

    if (inUse) {
      res = ShowMsg(MB_CANCELTRYCONTINUE,
                    "The port name %s is already logged as \"in use\"\n"
                    "in the COM port database.",
                    pPortName);
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static VOID SetFriendlyName(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    int num)
{
  char portName[2][20];

  for (int j = 0 ; j < 2 ; j++) {
    char phPortName[20];

    SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
             j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, num);

    PortParameters portParameters(C0C_SERVICE, phPortName);

    if (portParameters.Load() == ERROR_SUCCESS)
      portParameters.FillPortName(portName[j], sizeof(portName[j])/sizeof(portName[j][0]));
    else
      SNPRINTF(portName[j], sizeof(portName[j])/sizeof(portName[j][0]), "%s", phPortName);
  }

  char friendlyName[80];

  SNPRINTF(friendlyName, sizeof(friendlyName)/sizeof(friendlyName[0]),
           "com0com - bus for serial port pair emulator (%s <-> %s)",
           portName[0], portName[1]);

  SetupDiSetDeviceRegistryProperty(hDevInfo, pDevInfoData, SPDRP_FRIENDLYNAME,
                                  (LPBYTE)friendlyName, (lstrlen(friendlyName) + 1) * sizeof(*friendlyName));
}
///////////////////////////////////////////////////////////////
static VOID CleanDevPropertiesStack(InfFile &infFile, Stack &stack, BOOL enable, BOOL *pRebootRequired)
{
  for (;;) {
    StackEl *pElem = stack.Pop();

    if (!pElem)
      break;

    DevProperties *pDevProperties = (DevProperties *)pElem->pData;

    delete pElem;

    if (pDevProperties) {
      if (enable && pDevProperties->PhObjName())
        EnableDevices(infFile, pDevProperties, pRebootRequired);

      delete pDevProperties;
    }
  }
}
///////////////////////////////////////////////////////////////
struct ChangeDeviceParams {
  ChangeDeviceParams(InfFile &_infFile, const char *_pPhPortName, const char *_pParameters)
    : pInfFile(&_infFile), pPhPortName(_pPhPortName), pParameters(_pParameters), changed(FALSE) {}

  InfFile *pInfFile;
  const char *pPhPortName;
  const char *pParameters;
  BOOL changed;
};

static BOOL ChangeDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL *pRebootRequired,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i >= 0) {
    InfFile &infFile = *((ChangeDeviceParams *)pParam)->pInfFile;
    const char *pPhPortName = ((ChangeDeviceParams *)pParam)->pPhPortName;
    const char *pParameters = ((ChangeDeviceParams *)pParam)->pParameters;

    for (int j = 0 ; j < 2 ; j++) {
      char phPortName[20];

      SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
               j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, i);

      PortParameters portParameters(C0C_SERVICE, phPortName);

      LONG err = portParameters.Load();

      char buf[200];

      portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]), detailPrms);
      Trace("       %s %s\n", phPortName, buf);

      if (err == ERROR_SUCCESS) {
        if (pPhPortName && !lstrcmpi(pPhPortName, phPortName)) {
          char portNameOld[20];

          portParameters.FillPortName(portNameOld, sizeof(portNameOld)/sizeof(portNameOld[0]));

          portParameters.ParseParametersStr(pParameters);

          char phDevName[40];

          SNPRINTF(phDevName, sizeof(phDevName)/sizeof(phDevName[0]), "%s%d",
                   j ? C0C_PREF_DEVICE_NAME_B : C0C_PREF_DEVICE_NAME_A, i);

          char portName[20];

          portParameters.FillPortName(portName, sizeof(portName)/sizeof(portName[0]));

          if (portParameters.Changed() &&
              (!lstrcmpi(portName, portNameOld) || IsValidPortName(portName)))
          {
            err = portParameters.Save();

            if (err == ERROR_SUCCESS) {
              ((ChangeDeviceParams *)pParam)->changed = TRUE;

              portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]), detailPrms);
              Trace("change %s %s\n", phPortName, buf);

              SetFriendlyName(hDevInfo, pDevInfoData, i);

              DevProperties devProperties;
              if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
                return FALSE;
              if (!devProperties.PhObjName(phDevName))
                return FALSE;

              RestartDevices(infFile, &devProperties, pRebootRequired);
            } else {
              ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Save(%s)", phPortName);
            }
          }
        }
      } else {
        ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Load(%s)", phPortName);
      }
    }
  } else {
    Trace("       " C0C_PREF_PORT_NAME_A "?\n");
    Trace("       " C0C_PREF_PORT_NAME_B "?\n");
  }

  return TRUE;
}

int Change(InfFile &infFile, const char *pPhPortName, const char *pParameters)
{
  BOOL rebootRequired = FALSE;
  ChangeDeviceParams params(infFile, pPhPortName, pParameters);

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return 1;

  EnumDevices(infFile, &devProperties, &rebootRequired, ChangeDevice, &params);

  if (params.changed)
    ComDbSync(infFile);

  if (rebootRequired)
    SetupPromptReboot(NULL, NULL, FALSE);

  return 0;
}
///////////////////////////////////////////////////////////////
struct RemoveDeviceParams {
  RemoveDeviceParams(int _num) : num(_num), res(IDCANCEL) {}

  int num;
  int res;
};

static BOOL RemoveDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL *pRebootRequired,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i == ((RemoveDeviceParams *)pParam)->num) {
    ((RemoveDeviceParams *)pParam)->res =
        DisableDevice(hDevInfo, pDevInfoData, pDevProperties, pRebootRequired, NULL);

    if (((RemoveDeviceParams *)pParam)->res != IDCONTINUE)
      return FALSE;

    return RemoveDevice(hDevInfo, pDevInfoData, pDevProperties, pRebootRequired);
  }

  return TRUE;
}

int Remove(InfFile &infFile, int num)
{
  int res;
  BOOL rebootRequired = FALSE;

  do {
    RemoveDeviceParams removeDeviceParams(num);

    DevProperties devProperties;
    if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
      return 1;

    EnumDevices(infFile, &devProperties, &rebootRequired, RemoveDevice, &removeDeviceParams);

    res = removeDeviceParams.res;

  } while (res == IDTRYAGAIN);

  if (res == IDCONTINUE) {
    for (int j = 0 ; j < 2 ; j++) {
      char phPortName[20];

      SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
               j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, num);

      DevProperties devProperties;

      if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
        return 1;
      if (!devProperties.Location(phPortName))
        return 1;

      RemoveDevices(infFile, &devProperties, NULL);
    }
  }

  ComDbSync(infFile);

  if (rebootRequired)
    SetupPromptReboot(NULL, NULL, FALSE);

  if (res != IDCONTINUE)
    return 1;

  return 0;
}
///////////////////////////////////////////////////////////////
int Preinstall(InfFile &infFile)
{
  if (!infFile.InstallOEMInf())
    return 1;

  int res;

  do {
    res = IDCONTINUE;

    if (SetupDiInstallClass(NULL, infFile.Path(), 0, NULL)) {
      Trace("Installed Class %s\n", infFile.ClassGUID());
    } else {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupDiInstallClass()");
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return 1;

  return 0;
}
///////////////////////////////////////////////////////////////
int Reload(InfFile &infFile, BOOL update)
{
  Stack stack;
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return 1;

  if (!DisableDevices(infFile, &devProperties, &rebootRequired, &stack)) {
    CleanDevPropertiesStack(infFile, stack, TRUE, &rebootRequired);
    return 1;
  }

  BOOL rr = FALSE;

  if (update) {
    if (!UpdateDriverForPlugAndPlayDevices(0, C0C_BUS_DEVICE_ID, infFile.Path(), INSTALLFLAG_FORCE, &rr)) {
      CleanDevPropertiesStack(infFile, stack, TRUE, &rebootRequired);
      return 1;
    }
  }

  CleanDevPropertiesStack(infFile, stack, TRUE, &rebootRequired);

  ComDbSync(infFile);

  if (rebootRequired || rr)
    SetupPromptReboot(NULL, NULL, FALSE);

  return 0;
}
///////////////////////////////////////////////////////////////
int Disable(InfFile &infFile)
{
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return 1;

  if (!DisableDevices(infFile, &devProperties, &rebootRequired, NULL))
    return 1;

  if (rebootRequired)
    SetupPromptReboot(NULL, NULL, FALSE);

  return 0;
}
///////////////////////////////////////////////////////////////
int Enable(InfFile &infFile)
{
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return 1;

  if (!EnableDevices(infFile, &devProperties, &rebootRequired)) {
    return 1;
  }

  if (rebootRequired)
    SetupPromptReboot(NULL, NULL, FALSE);

  return  0;
}
///////////////////////////////////////////////////////////////
static BOOL InstallDeviceCallBack(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties pDevProperties,
    BOOL * /*pRebootRequired*/,
    void *pParam)
{
  if (!lstrcmp(pDevProperties->DevId(), C0C_BUS_DEVICE_ID)) {
    int res;
    int num = *(int *)pParam;

    do {
      res = IDCONTINUE;

      LONG err = SetPortNum(hDevInfo, pDevInfoData, num);

      if (err != ERROR_SUCCESS)
        res = ShowError(MB_CANCELTRYCONTINUE, err, "SetPortNum(%d)", num);

    } while (res == IDTRYAGAIN);

    if (res != IDCONTINUE)
      return FALSE;

    SetFriendlyName(hDevInfo, pDevInfoData, num);

    return TRUE;
  }

  return FALSE;
}

static BOOL InstallBusDevice(InfFile &infFile, int num)
{
  return InstallDevice(infFile, C0C_BUS_DEVICE_ID, C0C_CLASS, InstallDeviceCallBack, &num);
}

static BOOL AddDeviceToBusyMask(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL * /*pRebootRequired*/,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i >= 0) {
    if (!((BusyMask *)pParam)->AddNum(i)) {
      if (ShowLastError(MB_OKCANCEL|MB_ICONWARNING, "AddDeviceToBusyMask(%d)", i) == IDCANCEL)
        return FALSE;
    }
  }

  return TRUE;
}

static int AllocPortNum(InfFile &infFile, int num)
{
  BusyMask busyMask;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return -1;

  if (EnumDevices(infFile, &devProperties, NULL, AddDeviceToBusyMask, &busyMask) < 0)
    return -1;

  return busyMask.IsFreeNum(num) ? num : busyMask.GetFirstFreeNum();
}

int Install(InfFile &infFile, const char *pParametersA, const char *pParametersB, int num)
{
  int i;
  int res;

  do {
    res = IDCONTINUE;

    i = AllocPortNum(infFile, num >= 0 ? num : 0);

    if (i < 0)
      goto err;

    if (num >= 0 && num != i) {
      res = ShowMsg(MB_CANCELTRYCONTINUE|MB_ICONWARNING,
                    "The identifiers " C0C_PREF_PORT_NAME_A "%d and "
                    C0C_PREF_PORT_NAME_B "%d are already used for other ports\n"
                    "so ports with identifiers " C0C_PREF_PORT_NAME_A "%d and "
                    C0C_PREF_PORT_NAME_B "%d will be installed instead.\n",
                    num, num, i, i);
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    goto err;

  if (!IsValidPortNum(i))
    goto err;

  char portName[2][20];

  for (int j = 0 ; j < 2 ; j++) {
    char phPortName[20];
    const char *pParameters;

    pParameters = j ? pParametersB : pParametersA;

    SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
             j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, i);

    PortParameters portParameters(C0C_SERVICE, phPortName);

    LONG err = portParameters.Load();

    if (err == ERROR_SUCCESS) {
      portParameters.ParseParametersStr(pParameters);

      portParameters.FillPortName(portName[j], sizeof(portName[j])/sizeof(portName[j][0]));

      if (!IsValidPortName(portName[j]))
        goto err;

      if (portParameters.Changed()) {
        err = portParameters.Save();

        if (err != ERROR_SUCCESS)
          ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Save(%s)", phPortName);
      }
    } else {
      ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Load(%s)", phPortName);
      SNPRINTF(portName[j], sizeof(portName[j])/sizeof(portName[j][0]), "%s", phPortName);
    }

    char buf[200];

    portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]), detailPrms);

    Trace("       %s %s\n", phPortName, buf);
  }

  if (!lstrcmpi(portName[0], portName[1]) &&
      ShowMsg(MB_OKCANCEL|MB_ICONWARNING,
              "The same port name %s is used for both ports.",
              portName[0]) == IDCANCEL)
  {
    goto err;
  }

  if (!InstallBusDevice(infFile, i))
    goto err;

  ComDbSync(infFile);

  return  0;

err:

  Trace("\nInstall not completed!\n");

  return  1;
}
///////////////////////////////////////////////////////////////
int Uninstall(InfFile &infFile)
{
  BOOL rebootRequired = FALSE;
  DevProperties devProperties;

  devProperties = DevProperties();
  if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
    return 1;

  Stack stack;

  if (!DisableDevices(infFile, &devProperties, &rebootRequired, &stack)) {
    CleanDevPropertiesStack(infFile, stack, TRUE, &rebootRequired);
    return 1;
  }

  CleanDevPropertiesStack(infFile, stack, FALSE, &rebootRequired);

  devProperties = DevProperties();
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return 1;

  if (!RemoveDevices(infFile, &devProperties, &rebootRequired))
    return 1;

  if (!RemoveDevices(infFile, NULL, NULL))
    return 1;

  ComDbSync(infFile);

  if (rebootRequired) {
    SetupPromptReboot(NULL, NULL, FALSE);
    return 0;
  }

  int res;
  BOOL notDeleted;
  LONG err;

  do {
    notDeleted = TRUE;
    res = IDCONTINUE;

    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM) {
      do {
        res = IDCONTINUE;

        SC_HANDLE hSrv = OpenService(hSCM, C0C_SERVICE, DELETE|SERVICE_QUERY_STATUS);

        if (hSrv) {
          SERVICE_STATUS srvStatus;

          if (QueryServiceStatus(hSrv, &srvStatus)) {
            if (srvStatus.dwCurrentState == SERVICE_STOPPED) {
              if (silent ||
                  ShowMsg(MB_YESNO,
                  "The deleting %s service will remove your manual settings.\n"
                  "Would you like to delete service?\n",
                  C0C_SERVICE) == IDYES)
              {
                if (DeleteService(hSrv)) {
                  Trace("Deleted Service %s\n", C0C_SERVICE);
                  notDeleted = FALSE;
                } else {
                  res = ShowLastError(MB_CANCELTRYCONTINUE, "DeleteService(%s)", C0C_SERVICE);
                }
              }
            } else {
              res = ShowMsg(MB_CANCELTRYCONTINUE,
                            "Service %s is not stopped (state %ld).",
                            C0C_SERVICE, (long)srvStatus.dwCurrentState);
            }
          } else {
            res = ShowLastError(MB_CANCELTRYCONTINUE, "QueryServiceStatus(%s)", C0C_SERVICE);
          }

          if (!CloseServiceHandle(hSrv))
            ShowLastError(MB_OK|MB_ICONWARNING, "CloseServiceHandle(hSrv)");
        } else {
          if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            Trace("Service %s not installed\n", C0C_SERVICE);
            notDeleted = FALSE;
          } else {
            res = ShowLastError(MB_CANCELTRYCONTINUE, "OpenService(%s)", C0C_SERVICE);
          }
        }
      } while (res == IDTRYAGAIN);

      if (!CloseServiceHandle(hSCM))
        ShowLastError(MB_OK|MB_ICONWARNING, "CloseServiceHandle(hSCM)");
    } else {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "OpenSCManager()");
    }
  } while (res == IDTRYAGAIN);

  if (notDeleted)
    Trace("WARNING: Service %s not deleted\n", C0C_SERVICE);

  if (res != IDCONTINUE)
    return 1;

  do {
    notDeleted = TRUE;
    res = IDCONTINUE;

    err = RegDeleteKey(HKEY_LOCAL_MACHINE, C0C_REGKEY_EVENTLOG);

    if (err == ERROR_SUCCESS) {
      Trace("Deleted EventLog %s\n", C0C_SERVICE);
      notDeleted = FALSE;
    }
    else
    if (err == ERROR_FILE_NOT_FOUND) {
      Trace("EventLog %s not installed\n", C0C_SERVICE);
      notDeleted = FALSE;
    }
    else {
      res = ShowError(MB_CANCELTRYCONTINUE, err, "RegDeleteKey(%s)", C0C_REGKEY_EVENTLOG);
    }
  } while (res == IDTRYAGAIN);

  if (notDeleted)
    Trace("WARNING: Key %s not deleted\n", C0C_REGKEY_EVENTLOG);

  if (res != IDCONTINUE)
    return 1;

  do {
    notDeleted = TRUE;
    res = IDCONTINUE;

    HKEY hKey = SetupDiOpenClassRegKey(NULL, DELETE);

    if (hKey != INVALID_HANDLE_VALUE) {
      do {
        res = IDCONTINUE;

        err = RegDeleteKey(hKey, infFile.ClassGUID());

        if (err != ERROR_SUCCESS) {
          HKEY hClassGuidKey;
          err = RegOpenKeyEx(hKey, infFile.ClassGUID(), 0, KEY_READ, &hClassGuidKey);

          if (err == ERROR_SUCCESS) {
            for (;;) {
              char subKey[MAX_PATH+1];
              DWORD subKeySize = sizeof(subKey)/sizeof(subKey[0]);

              err = RegEnumKeyEx(hClassGuidKey, 0, subKey, &subKeySize, NULL, NULL, NULL, NULL);

              if (err != ERROR_SUCCESS)
                break;

              err = RegDeleteKey(hClassGuidKey, subKey);

              if (err == ERROR_SUCCESS)
                Trace("Deleted Class subkey %s\\%s\n", infFile.ClassGUID(), subKey);
              else
              if (err != ERROR_FILE_NOT_FOUND) {
                ShowError(MB_OK|MB_ICONWARNING, err, "RegDeleteKey(%s\\%s)", infFile.ClassGUID(), subKey);
                break;
              }
            }

            err = RegCloseKey(hClassGuidKey);

            if (err != ERROR_SUCCESS)
              ShowError(MB_OK|MB_ICONWARNING, err, "RegCloseKey()");
          }

          err = RegDeleteKey(hKey, infFile.ClassGUID());
        }

        if (err == ERROR_SUCCESS) {
          Trace("Deleted Class %s\n", infFile.ClassGUID());
          notDeleted = FALSE;
        }
        else
        if (err == ERROR_FILE_NOT_FOUND) {
          Trace("Class %s not installed\n", infFile.ClassGUID());
          notDeleted = FALSE;
        }
        else {
          res = ShowError(MB_CANCELTRYCONTINUE, err, "RegDeleteKey(%s)", infFile.ClassGUID());
        }
      } while (res == IDTRYAGAIN);

      err = RegCloseKey(hKey);

      if (err != ERROR_SUCCESS)
        ShowError(MB_OK|MB_ICONWARNING, err, "RegCloseKey()");
    } else {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupDiOpenClassRegKey(NULL)");
    }
  } while (res == IDTRYAGAIN);

  if (notDeleted)
    Trace("WARNING: Class %s not deleted\n", infFile.ClassGUID());

  if (res != IDCONTINUE)
    return 1;

  if (!infFile.UninstallOEMInf())
    return 1;

  if (!infFile.UninstallFiles(C0C_COPY_DRIVERS_SECTION))
    return 1;

  if (!InfFile::UninstallAllInfFiles(C0C_CLASS_GUID, NULL, NULL))
    return 1;

  return 0;
}
///////////////////////////////////////////////////////////////
int ShowBusyNames(const char *pPattern)
{
  char *pPatternUp;
  SIZE_T sizePattern;

  sizePattern = (lstrlen(pPattern) + 1) * sizeof(pPatternUp[0]);

  pPatternUp = (char *)LocalAlloc(LPTR, sizePattern);

  if (!pPatternUp) {
    ShowError(MB_OK|MB_ICONSTOP, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)sizePattern);
    return 1;
  }

  lstrcpy(pPatternUp, pPattern);
  CharUpper(pPatternUp);

  char *pNames = NULL;
  DWORD iNamesEnd = 0;

  for (int i = 0 ; i < 2 ; i++) {
    char *pBuf = NULL;

    for (DWORD size = 1024 ; size >= 1024 ; size += 1024) {
      char *pBufNew;

      if (!pBuf)
        pBufNew = (char *)LocalAlloc(LPTR, size);
      else
        pBufNew = (char *)LocalReAlloc(pBuf, size, LMEM_ZEROINIT|LMEM_MOVEABLE);

      if (!pBufNew) {
        if (pBuf) {
          LocalFree(pBuf);
          pBuf = NULL;
        }

        if (ShowError(MB_OKCANCEL, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)size) == IDCANCEL) {
          if (pNames)
            LocalFree(pNames);

          LocalFree(pPatternUp);
          return 1;
        }

        break;
      }

      pBuf = pBufNew;

      DWORD res = (i != 0
                   ? QueryDosDevice(NULL, pBuf, size/sizeof(pBuf[0]))
                   : ComDbQueryNames(pBuf, size/sizeof(pBuf[0])));

      if (!res) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
          continue;

        LocalFree(pBuf);
        pBuf = NULL;

        if (GetLastError() == ERROR_FILE_NOT_FOUND)
          continue;

        if (ShowLastError(MB_OKCANCEL,
            i != 0 ? "QueryDosDevice()" : "ComDbNames()") == IDCANCEL)
        {
          if (pNames)
            LocalFree(pNames);

          LocalFree(pPatternUp);
          return 1;
        }

        break;
      }

      // Workaround for succeeds even if buffer cannot hold all returned information

      if ((size/sizeof(pBuf[0]) - res) > 100)
        break;
    }

    if (pBuf) {
      for (char *pName = pBuf ; *pName ; pName += lstrlen(pName) + 1) {
        static const char strangeChars[] = ":#&$?!{}()[]/\\ \t\r\n";

        if (strpbrk(pName, strangeChars))
          continue;  // skip strange names

        CharUpper(pName);

        if (!MatchPattern(pPatternUp, pName))
          continue;

        if (pNames) {
          char *pNamesName;

          for (pNamesName = pNames ; *pNamesName ; pNamesName += lstrlen(pNamesName) + 1) {
            if (!lstrcmp(pNamesName, pName))
              break;
          }

          if (*pNamesName)
            continue;
        }

        DWORD iNamesEndNew = iNamesEnd + lstrlen(pName) + 1;
        DWORD sizeNamesNew = (iNamesEndNew + 1) * sizeof(pNames[0]);

        char *pNamesNew;

        if (!pNames)
          pNamesNew = (char *)LocalAlloc(LPTR, sizeNamesNew);
        else
          pNamesNew = (char *)LocalReAlloc(pNames, sizeNamesNew, LMEM_ZEROINIT|LMEM_MOVEABLE);

        if (!pNamesNew) {
          if (ShowError(MB_OKCANCEL, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)size) == IDCANCEL) {
            if (pNames)
              LocalFree(pNames);

            LocalFree(pPatternUp);
            return 1;
          }

          break;
        }

        pNames = pNamesNew;

        lstrcpy(pNames + iNamesEnd, pName);

        iNamesEnd = iNamesEndNew;
      }

      LocalFree(pBuf);
    }
  }

  if (pNames) {
    for (char *pName = pNames ; *pName ; pName += lstrlen(pName) + 1)
      Trace("%s\n", pName);

    LocalFree(pNames);
  }

  LocalFree(pPatternUp);

  return 0;
}
///////////////////////////////////////////////////////////////
int Help(const char *pProgName)
{
  SetTitle(C0C_SETUP_TITLE " (HELP)");

  ConsoleWrite(
    C0C_SETUP_TITLE "\n"
    "\n");
  ConsoleWrite(
    "Usage:\n"
    "  %s%s[options] <command>\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "\n"
    "Options:\n"
    "  --output <file>              - file for output, default is console\n"
    "  --detail-prms                - show detailed parameters\n"
    "  --silent                     - suppress dialogs if possible\n"
    );
  ConsoleWrite(
    "\n"
    "Commands:\n"
    "  install <n> <prmsA> <prmsB>  - install a pair of linked ports with\n"
    "   or                            identifiers " C0C_PREF_PORT_NAME_A "<n> and "
                                      C0C_PREF_PORT_NAME_B "<n>\n"
    "  install <prmsA> <prmsB>        (by default <n> is the first not used number),\n"
    "                                 set their parameters to <prmsA> and <prmsB>\n"
    "  remove <n>                   - remove a pair of linked ports with\n"
    "                                 identifiers " C0C_PREF_PORT_NAME_A "<n> and "
                                      C0C_PREF_PORT_NAME_B "<n>\n"
    "  disable all                  - disable all ports in current hardware profile\n"
    "  enable all                   - enable all ports in current hardware profile\n"
    );
  ConsoleWrite(
    "  change <portid> <prms>       - set parameters <prms> for port with\n"
    "                                 identifier <portid>\n"
    "  list                         - for each port show its identifier and\n"
    "                                 parameters\n"
    "  preinstall                   - preinstall driver\n"
    "  update                       - update driver\n"
    "  reload                       - reload driver\n"
    "  uninstall                    - uninstall all ports and the driver\n"
    "  busynames <pattern>          - show names that already in use and match the\n"
    "                                 <pattern> (wildcards: '*' and '?')\n"
    "  quit                         - quit\n"
    "  help                         - print this help\n"
    );
  ConsoleWrite(
    "\n");

  const char *pStr = PortParameters::GetHelp();

  while (*pStr) {
    char buf[100];

    SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), "%s", pStr);
    pStr += lstrlen(buf);

    ConsoleWrite("%s", buf);
  }

  ConsoleWrite(
    "\n"
    "Examples:\n"
    );
  ConsoleWrite(
    "  %s%sinstall - -\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%sinstall 5 * *\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%sremove 0\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%sinstall PortName=COM2 PortName=COM4\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%sinstall PortName=COM5,EmuBR=yes,EmuOverrun=yes -\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%schange " C0C_PREF_PORT_NAME_A "0 EmuBR=yes,EmuOverrun=yes\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%slist\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%suninstall\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "  %s%sbusynames COM?*\n"
    , pProgName, (pProgName && *pProgName) ? " " : "");
  ConsoleWrite(
    "\n");

  return 1;
}
///////////////////////////////////////////////////////////////
int Main(int argc, const char* argv[])
{
  if (!SetOutputFile(NULL))
    return 1;

  detailPrms = FALSE;

  while (argc > 1) {
    if (*argv[1] != '-')
      break;

    if (!strcmp(argv[1], "--output") && argc > 2) {
      if (!SetOutputFile(argv[2]))
        return 1;
      argv[2] = argv[0];
      argv += 2;
      argc -= 2;
    }
    else
    if (!strcmp(argv[1], "--detail-prms")) {
      detailPrms = TRUE;
      argv++;
      argc--;
    }
    else
    if (!strcmp(argv[1], "--silent")) {
      silent = TRUE;
      argv++;
      argc--;
    }
    else {
      ConsoleWrite("Invalid option %s\n", argv[1]);
      return 1;
    }
  }

  if (argc == 1) {
    return 0;
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "help")) {
    Help(argv[0]);
    return 0;
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "quit")) {
    return 0;
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "busynames")) {
    SetTitle(C0C_SETUP_TITLE " (BUSY NAMES)");
    return ShowBusyNames(argv[2]);
  }

  InfFile infFile(C0C_INF_NAME, C0C_INF_NAME);

  if (!infFile.Compare(C0C_CLASS_GUID, C0C_CLASS, C0C_PROVIDER))
    return 1;

  if (argc == 2 && !lstrcmpi(argv[1], "list")) {
    SetTitle(C0C_SETUP_TITLE " (LIST)");
    return Change(infFile, NULL, NULL);
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "change")) {
    SetTitle(C0C_SETUP_TITLE " (CHANGE)");
    return Change(infFile, argv[2], argv[3]);
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "install")) {
    SetTitle(C0C_SETUP_TITLE " (INSTALL)");
    return Install(infFile, argv[2], argv[3], -1);
  }
  else
  if (argc == 5 && !lstrcmpi(argv[1], "install")) {
    SetTitle(C0C_SETUP_TITLE " (INSTALL)");

    int num;

    if (StrToInt(argv[2], &num) && num >= 0)
      return Install(infFile, argv[3], argv[4], num);
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "remove")) {
    SetTitle(C0C_SETUP_TITLE " (REMOVE)");

    int num;

    if (StrToInt(argv[2], &num) && num >= 0)
      return Remove(infFile, num);
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "disable")) {
    SetTitle(C0C_SETUP_TITLE " (DISABLE)");

    if (!lstrcmpi(argv[2], "all"))
      return Disable(infFile);
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "enable")) {
    SetTitle(C0C_SETUP_TITLE " (ENABLE)");

    if (!lstrcmpi(argv[2], "all"))
      return Enable(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "preinstall")) {
    SetTitle(C0C_SETUP_TITLE " (PREINSTALL)");
    return Preinstall(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "update")) {
    SetTitle(C0C_SETUP_TITLE " (UPDATE)");
    return Reload(infFile, TRUE);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "reload")) {
    SetTitle(C0C_SETUP_TITLE " (RELOAD)");
    return Reload(infFile, FALSE);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "uninstall")) {
    SetTitle(C0C_SETUP_TITLE " (UNINSTALL)");
    return Uninstall(infFile);
  }

  ConsoleWrite("Invalid command\n");

  return 1;
}
///////////////////////////////////////////////////////////////
static int ParseCmd(char *pCmd, const char* argv[], int sizeArgv)
{
  int argc;

  argc = 0;

  char *pSave;

  for (char *pArg = STRTOK_R(pCmd, " \t\r\n", &pSave) ; pArg ; pArg = STRTOK_R(NULL, " \t\r\n", &pSave)) {
    if ((argc + 2) > sizeArgv)
      break;

    if (*pArg == '"')
      pArg++;

    char *pEnd = pArg + lstrlen(pArg);

    if (pEnd-- != pArg && *pEnd == '"')
      *pEnd = 0;

    argv[argc++] = pArg;
  }

  argv[argc] = NULL;

  return argc;
}
///////////////////////////////////////////////////////////////
int CALLBACK MainA(const char *pProgName, const char *pCmdLine)
{
  SetTitle(C0C_SETUP_TITLE);

  char cmd[200];

  SNPRINTF(cmd, sizeof(cmd)/sizeof(cmd[0]), "%s", pCmdLine);

  int argc;
  const char* argv[10];

  argc = ParseCmd(cmd, argv + 1, sizeof(argv)/sizeof(argv[0]) - 1) + 1;

  if (argc == 1) {
    ConsoleWrite("Enter 'help' to get info about usage of " C0C_SETUP_TITLE ".\n\n");

    argv[0] = "";

    for (;;) {
      argv[1] = NULL;

      ConsoleWriteRead(cmd, sizeof(cmd)/sizeof(cmd[0]), "command> ");

      argc = ParseCmd(cmd, argv + 1, sizeof(argv)/sizeof(argv[0]) - 1) + 1;

      if (argc == 2 && !lstrcmpi(argv[1], "quit"))
        return 0;

      Main(argc, argv);
    }
  }

  argv[0] = pProgName;

  return Main(argc, argv);
}
///////////////////////////////////////////////////////////////
int CALLBACK RunDllA(HWND /*hWnd*/, HINSTANCE /*hInst*/, LPSTR pCmdLine, int /*nCmdShow*/)
{
  int res = MainA("rundll32 setup,RunDll", pCmdLine);

  if (!GetOutputFile()) {
    char buf[10];

    ConsoleWriteRead(buf, sizeof(buf)/sizeof(buf[0]), "\nPress <RETURN> to continue\n");
  }

  return res;
}
///////////////////////////////////////////////////////////////
