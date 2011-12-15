/*
 * $Id$
 *
 * Copyright (c) 2006-2011 Vyacheslav Frolov
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
 * Revision 1.48  2011/12/15 16:43:20  vfrolov
 * Added parameters parsing result check
 *
 * Revision 1.47  2011/12/15 15:51:48  vfrolov
 * Fixed types
 *
 * Revision 1.46  2011/07/15 16:09:05  vfrolov
 * Disabled MessageBox() for silent mode and added default processing
 *
 * Revision 1.45  2011/07/13 17:39:56  vfrolov
 * Fixed result treatment of UpdateDriverForPlugAndPlayDevices()
 *
 * Revision 1.44  2010/07/30 09:19:29  vfrolov
 * Added STRDUP()
 *
 * Revision 1.43  2010/07/29 12:18:43  vfrolov
 * Fixed waiting stuff
 *
 * Revision 1.42  2010/07/19 11:23:54  vfrolov
 * Added install command w/o prms to update driver
 * Added ability to use --wait option with any command
 *
 * Revision 1.41  2010/07/15 18:11:10  vfrolov
 * Fixed --wait option for Ports class
 *
 * Revision 1.40  2010/07/12 18:14:44  vfrolov
 * Fixed driver update duplication
 *
 * Revision 1.39  2010/06/07 07:03:31  vfrolov
 * Added wrapper UpdateDriver() for UpdateDriverForPlugAndPlayDevices()
 *
 * Revision 1.38  2010/06/01 06:14:10  vfrolov
 * Improved driver updating
 *
 * Revision 1.37  2010/05/31 07:58:14  vfrolov
 * Added ability to invoke the system-supplied advanced settings dialog box
 *
 * Revision 1.36  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.35  2010/03/30 08:05:15  vfrolov
 * Fixed bugs item #2979007 "setupc command line limited to 200 chars"
 * Reported by Henrik Maier (hwmaier)
 *
 * Revision 1.34  2010/03/11 13:40:57  vfrolov
 * Fixed size typo, bug #2968585
 * Thanks Xlnt (xlnt9568)
 *
 * Revision 1.33  2009/11/16 08:43:44  vfrolov
 * Fixed endless loop if no ports logged in ComDB
 *
 * Revision 1.32  2009/09/18 11:21:31  vfrolov
 * Added --wait option
 *
 * Revision 1.31  2009/09/18 07:48:11  vfrolov
 * Added missing argv[0] shift
 *
 * Revision 1.30  2009/02/16 10:32:56  vfrolov
 * Added Silent() and PromptReboot()
 *
 * Revision 1.29  2009/02/11 07:35:22  vfrolov
 * Added --no-update option
 *
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
#define SERIAL_ADVANCED_SETTINGS 1
#include <msports.h>

#define TEXT_PREF
#include "../include/com0com.h"

#define C0C_INF_NAME             "com0com.inf"
#define C0C_INF_NAME_CNCPORT     "cncport.inf"
#define C0C_INF_NAME_COMPORT     "comport.inf"
#define C0C_CNCCLASS_GUID        "{df799e12-3c56-421b-b298-b6d3642bc878}"
#define C0C_COMCLASS_GUID        "{4d36e978-e325-11ce-bfc1-08002be10318}"
#define C0C_PROVIDER             "Vyacheslav Frolov"
#define C0C_REGKEY_EVENTLOG      REGSTR_PATH_SERVICES "\\Eventlog\\System\\" C0C_SERVICE
#define C0C_COPY_DRIVERS_SECTION "com0com_CopyDrivers"

#define C0C_SETUP_TITLE          "Setup for com0com"
///////////////////////////////////////////////////////////////
static const InfFile::InfFileField requiredFieldsInfBusInstall[] = {
  { "Version",         "ClassGUID",          1, C0C_CNCCLASS_GUID },
  { "Version",         "Provider",           1, C0C_PROVIDER },
  { "Version",         "UninstallInfTag",    1, C0C_CNCCLASS_GUID },
  { "Manufacturer",    NULL,                 1, C0C_SERVICE },
  { NULL },
};

static const InfFile::InfFileField requiredFieldsInfCNCPortInstall[] = {
  { "Version",         "ClassGUID",          1, C0C_CNCCLASS_GUID },
  { "Version",         "Provider",           1, C0C_PROVIDER },
  { "Version",         "UninstallInfTag",    1, C0C_CNCCLASS_GUID },
  { "Manufacturer",    NULL,                 1, C0C_SERVICE },
  { NULL },
};

static const InfFile::InfFileField requiredFieldsInfCOMPortInstall[] = {
  { "Version",         "ClassGUID",          1, C0C_COMCLASS_GUID },
  { "Version",         "Provider",           1, C0C_PROVIDER },
  { "Version",         "UninstallInfTag",    1, C0C_CNCCLASS_GUID },
  { "Manufacturer",    NULL,                 1, C0C_SERVICE },
  { NULL },
};

struct InfFileInstall {
  const char *pInfName;
  const char *pCopyDriversSection;
  const char *pHardwareId;
  bool preinstallClass;
  const InfFile::InfFileField *pRequiredFields;
};

static const InfFileInstall infFileInstallList[] = {
  { C0C_INF_NAME,          C0C_COPY_DRIVERS_SECTION,  C0C_BUS_DEVICE_ID,        TRUE,  requiredFieldsInfBusInstall },
  { C0C_INF_NAME_CNCPORT,  NULL,                      C0C_PORT_HW_ID_CNCCLASS,  FALSE, requiredFieldsInfCNCPortInstall },
  { C0C_INF_NAME_COMPORT,  NULL,                      C0C_PORT_HW_ID_COMCLASS,  FALSE, requiredFieldsInfCOMPortInstall },
  { NULL },
};
///////////////////////////////////////////////////////////////
static const InfFile::InfFileField requiredFieldsInfUnnstallInfTag[] = {
  { "Version",         "UninstallInfTag",    1, C0C_CNCCLASS_GUID },
  { "Manufacturer",    NULL,                 1, C0C_SERVICE },
  { NULL },
};

static const InfFile::InfFileField requiredFieldsInfUnnstallCNCOld[] = {
  { "Version",         "ClassGUID",          1, C0C_CNCCLASS_GUID },
  { "Version",         "Provider",           1, C0C_PROVIDER },
  { "Version",         "CatalogFile",        1, "com0com.cat" },
  { NULL },
};

static const InfFile::InfFileField requiredFieldsInfUnnstallCNCClass[] = {
  { "Version",         "ClassGUID",          1, C0C_CNCCLASS_GUID },
  { NULL },
};

static const InfFile::InfFileField requiredFieldsInfUnnstallCOMClass[] = {
  { "Version",         "ClassGUID",          1, C0C_COMCLASS_GUID },
  { "Manufacturer",    NULL,                 1, C0C_SERVICE },
  { NULL },
};

static const InfFile::InfFileUninstall infFileUnnstallList[] = {
  { requiredFieldsInfUnnstallInfTag,    FALSE },
  { requiredFieldsInfUnnstallCNCOld,    FALSE },
  { requiredFieldsInfUnnstallCNCClass,  TRUE },
  { requiredFieldsInfUnnstallCOMClass,  TRUE },
  { NULL },
};
///////////////////////////////////////////////////////////////
static int timeout = 0;
static bool detailPrms = FALSE;
static bool no_update = FALSE;
///////////////////////////////////////////////////////////////
static CNC_ENUM_FILTER EnumFilter;
static bool EnumFilter(const char *pHardwareId)
{
  if (!pHardwareId)
    return FALSE;

  if (lstrcmpi(pHardwareId, C0C_BUS_DEVICE_ID) == 0 ||
      lstrcmpi(pHardwareId, C0C_PORT_DEVICE_ID) == 0)
  {
    //Trace("CNC %s\n", pHardwareId);
    return TRUE;
  }

  return FALSE;
}
///////////////////////////////////////////////////////////////
static bool IsValidPortNum(int num)
{
  if (num < 0)
    return FALSE;

  char buf[C0C_PORT_NAME_LEN + 1];

  if (SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_BUS_NAME "%d", num) < 0 ||
      SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_PORT_NAME_A "%d", num) < 0 ||
      SNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), C0C_PREF_PORT_NAME_B "%d", num) < 0)
  {
    int res = ShowMsg(MB_OKCANCEL|MB_ICONWARNING, "The port number %d is too big.\n", num);

    if (res == IDCANCEL || res == 0)
      return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
static bool IsValidPortName(
    const char *pPortName)
{
  int res;

  if (lstrlen(pPortName) > C0C_PORT_NAME_LEN) {
    res = ShowMsg(MB_OKCANCEL|MB_ICONWARNING,
                  "The length of port name %s\n"
                  "is too big (greater then %d).\n",
                  pPortName, C0C_PORT_NAME_LEN);

    if (res == IDCANCEL || res == 0)
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
                  "The port name %s is already used for other device %s.\n",
                  pPortName, phDevName);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  do {
    res = IDCONTINUE;

    bool inUse;

    if (!ComDbGetInUse(pPortName, inUse)) {
      res = IDCANCEL;
      continue;
    }

    if (inUse) {
      res = ShowMsg(MB_CANCELTRYCONTINUE,
                    "The port name %s is already logged as \"in use\"\n"
                    "in the COM port database.\n",
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
           "com0com - bus for serial port pair emulator %d (%s <-> %s)",
           num, portName[0], portName[1]);

  SetupDiSetDeviceRegistryProperty(hDevInfo, pDevInfoData, SPDRP_FRIENDLYNAME,
                                  (LPBYTE)friendlyName, (lstrlen(friendlyName) + 1) * sizeof(*friendlyName));
}
///////////////////////////////////////////////////////////////
/*
static int SleepTillPortNotFound(
    const char *pPortName,
    int timeLimit)
{
  if (lstrcmpi(C0C_PORT_NAME_COMCLASS, pPortName) == 0)
    return 0;

  if (int(DWORD(timeLimit * 1000)/1000) != timeLimit)
    timeLimit = -1;

  DWORD startTime = GetTickCount();
  char path[40];

  SNPRINTF(path, sizeof(path)/sizeof(path[0]), "\\\\.\\%s", pPortName);

  Trace("Wating for %s ", path);

  for (;;) {
    HANDLE handle = CreateFile(path, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
      if (GetLastError() != ERROR_FILE_NOT_FOUND)
        break;
    } else {
      CloseHandle(handle);
      break;
    }

    Trace(".");

    if (timeLimit != -1 && GetTickCount() - startTime >= DWORD(timeLimit * 1000)) {
      Trace(" timeout\n");
      return -1;
    }

    Sleep(1000);
  }

  Trace(". OK\n");

  return int((GetTickCount() - startTime) / 1000);
}
*/
///////////////////////////////////////////////////////////////
static VOID CleanDevPropertiesStack(
    Stack &stack,
    bool enable,
    BOOL *pRebootRequired)
{
  for (;;) {
    StackEl *pElem = stack.Pop();

    if (!pElem)
      break;

    DevProperties *pDevProperties = (DevProperties *)pElem->pData;

    delete pElem;

    if (pDevProperties) {
      if (enable && pDevProperties->PhObjName())
        EnableDevices(EnumFilter, pDevProperties, pRebootRequired);

      delete pDevProperties;
    }
  }
}
///////////////////////////////////////////////////////////////
struct ChangeDeviceParams {
  ChangeDeviceParams(
    const char *_pPhPortName,
    const char *_pParameters)
  : pPhPortName(_pPhPortName), pParameters(_pParameters), changed(FALSE) {}

  const char *pPhPortName;
  const char *pParameters;
  bool changed;
};

static CNC_DEV_CALLBACK ShowDialog;
static bool ShowDialog(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL * /*pRebootRequired*/,
    void * /*pParam*/)
{
  if (SerialDisplayAdvancedSettings(NULL, hDevInfo, pDevInfoData) != ERROR_SUCCESS)
    return FALSE;

  return TRUE;
}

static CNC_DEV_CALLBACK ChangeDevice;
static bool ChangeDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL *pRebootRequired,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i >= 0) {
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
        if (pPhPortName && lstrcmpi(pPhPortName, phPortName) == 0 && pParameters) {
          char portNameOld[20];

          portParameters.FillPortName(portNameOld, sizeof(portNameOld)/sizeof(portNameOld[0]));

          if (!portParameters.ParseParametersStr(pParameters))
            return FALSE;

          char phDevName[40];

          SNPRINTF(phDevName, sizeof(phDevName)/sizeof(phDevName[0]), "%s%d",
                   j ? C0C_PREF_DEVICE_NAME_B : C0C_PREF_DEVICE_NAME_A, i);

          char portName[20];

          portParameters.FillPortName(portName, sizeof(portName)/sizeof(portName[0]));

          if (!Silent() && portParameters.DialogRequested()) {
            if (lstrcmpi(C0C_PORT_NAME_COMCLASS, portName) == 0) {
              if (!portParameters.ClassChanged()) {
                DevProperties devProperties;
                if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
                  return FALSE;
                if (!devProperties.PhObjName(phDevName))
                  return FALSE;

                EnumDevices(EnumFilter, &devProperties, pRebootRequired, ShowDialog, NULL);
              } else {
                ShowMsg(MB_OK|MB_ICONWARNING, "Can't display the dialog while changing the class of port.\n");
              }
            } else {
              ShowMsg(MB_OK|MB_ICONWARNING, "Can't display the dialog for non Ports class port.\n");
            }
          }

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

              if (portParameters.ClassChanged()) {
                Trace("Changed port class for %s (%s -> %s)\n", phPortName, portNameOld, portName);
                DisableDevices(EnumFilter, &devProperties, pRebootRequired, NULL);  // show msg if in use
                RemoveDevices(EnumFilter, &devProperties, pRebootRequired);
                ReenumerateDeviceNode(pDevInfoData);
              } else {
                RestartDevices(EnumFilter, &devProperties, pRebootRequired);
              }
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

bool Change(const char *pPhPortName, const char *pParameters)
{
  BOOL rebootRequired = FALSE;
  ChangeDeviceParams params(pPhPortName, pParameters);

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return FALSE;

  EnumDevices(EnumFilter, &devProperties, &rebootRequired, ChangeDevice, &params);

  if (params.changed)
    ComDbSync(EnumFilter);

  if (rebootRequired)
    PromptReboot();

  return TRUE;
}
///////////////////////////////////////////////////////////////
struct RemoveDeviceParams {
  RemoveDeviceParams(int _num) : num(_num), res(IDCANCEL) {}

  int num;
  int res;
};

static CNC_DEV_CALLBACK RemoveDevice;
static bool RemoveDevice(
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

bool Remove(int num)
{
  int res;
  BOOL rebootRequired = FALSE;

  do {
    RemoveDeviceParams removeDeviceParams(num);

    DevProperties devProperties;
    if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
      return FALSE;

    EnumDevices(EnumFilter, &devProperties, &rebootRequired, RemoveDevice, &removeDeviceParams);

    res = removeDeviceParams.res;

  } while (res == IDTRYAGAIN);

  if (res == IDCONTINUE) {
    for (int j = 0 ; j < 2 ; j++) {
      char phPortName[20];

      SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
               j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, num);

      DevProperties devProperties;

      if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
        return FALSE;

      if (!devProperties.Location(phPortName))
        return FALSE;

      RemoveDevices(EnumFilter, &devProperties, NULL);
    }
  }

  ComDbSync(EnumFilter);

  if (rebootRequired)
    PromptReboot();

  return (res == IDCONTINUE);
}
///////////////////////////////////////////////////////////////
bool Preinstall(const InfFileInstall *pInfFileInstallList)
{
  for (
      const InfFileInstall *pInfFileInstall = pInfFileInstallList ;
      pInfFileInstall->pInfName != NULL ;
      pInfFileInstall++)
  {
    InfFile infFile(pInfFileInstall->pInfName, pInfFileInstall->pInfName);

    if (pInfFileInstall->preinstallClass) {
      int res;

      do {
        res = IDCONTINUE;

        if (SetupDiInstallClass(NULL, infFile.Path(), Silent() ? 0 : DI_QUIETINSTALL, NULL)) {
          Trace("Installed Class %s\n", infFile.ClassGUID());
        } else {
          res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupDiInstallClass()");
        }
      } while (res == IDTRYAGAIN);

      if (res != IDCONTINUE)
        goto err;
    }

    if (!infFile.InstallOEMInf())
      goto err;
  }

  return TRUE;

err:

  Trace("\nPreinstall not completed!\n");

  return FALSE;
}
///////////////////////////////////////////////////////////////
bool Reload(
    const char *pHardwareId,
    const char *pInfFilePath,
    BOOL *pRebootRequired)
{
  Stack stack;
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(pHardwareId))
    return FALSE;

  if (!DisableDevices(EnumFilter, &devProperties, &rebootRequired, &stack)) {
    CleanDevPropertiesStack(stack, TRUE, &rebootRequired);
    return FALSE;
  }

  if (pHardwareId && pInfFilePath && !no_update) {
    int res;

    do {
      res = UpdateDriver(pInfFilePath, pHardwareId, INSTALLFLAG_FORCE, FALSE, &rebootRequired);
    } while (res == IDTRYAGAIN);

    if (res != IDCONTINUE) {
      CleanDevPropertiesStack(stack, TRUE, &rebootRequired);
      return FALSE;
    }
  }

  CleanDevPropertiesStack(stack, TRUE, &rebootRequired);

  ComDbSync(EnumFilter);

  if (rebootRequired) {
    if (pRebootRequired != NULL)
      *pRebootRequired = TRUE;
    else
      PromptReboot();
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
bool Update(const InfFileInstall *pInfFileInstallList)
{
  bool ok = TRUE;
  BOOL rebootRequired = FALSE;

  for (
      const InfFileInstall *pInfFileInstall = pInfFileInstallList ;
      pInfFileInstall->pInfName != NULL ;
      pInfFileInstall++)
  {
    InfFile infFile(pInfFileInstall->pInfName, pInfFileInstall->pInfName);

    if (!Reload(pInfFileInstall->pHardwareId, infFile.Path(), &rebootRequired))
      ok = FALSE;
  }

  if (!ok) {
    Trace("\nUpdate not completed!\n");
  }
  else
  if (rebootRequired) {
    PromptReboot();
  }

  return ok;
}
///////////////////////////////////////////////////////////////
bool Disable()
{
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return FALSE;

  if (!DisableDevices(EnumFilter, &devProperties, &rebootRequired, NULL))
    return FALSE;

  if (rebootRequired)
    PromptReboot();

  return TRUE;
}
///////////////////////////////////////////////////////////////
bool Enable()
{
  BOOL rebootRequired = FALSE;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return FALSE;

  if (!EnableDevices(EnumFilter, &devProperties, &rebootRequired)) {
    return FALSE;
  }

  if (rebootRequired)
    PromptReboot();

  return TRUE;
}
///////////////////////////////////////////////////////////////
bool Install(const char *pInfFilePath)
{
  if (no_update)
    return TRUE;

  BOOL rebootRequired = FALSE;

  int res;

  do {
    res = UpdateDriver(pInfFilePath, C0C_BUS_DEVICE_ID, 0, FALSE, &rebootRequired);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  ComDbSync(EnumFilter);

  if (rebootRequired)
    PromptReboot();

  return TRUE;
}
///////////////////////////////////////////////////////////////
static CNC_DEV_CALLBACK InstallDeviceCallBack;
static bool InstallDeviceCallBack(
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

static bool InstallBusDevice(const char *pInfFilePath, int num)
{
  BOOL rebootRequired = FALSE;

  if (!InstallDevice(pInfFilePath, C0C_BUS_DEVICE_ID, NULL, InstallDeviceCallBack, &num, !no_update, &rebootRequired))
    return FALSE;

  if (rebootRequired)
    PromptReboot();

  return TRUE;
}

static CNC_DEV_CALLBACK AddDeviceToBusyMask;
static bool AddDeviceToBusyMask(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL * /*pRebootRequired*/,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i >= 0) {
    if (!((BusyMask *)pParam)->AddNum(i)) {
      if (ShowLastError(MB_OKCANCEL|MB_ICONWARNING, "AddDeviceToBusyMask(%d)", i) != IDOK)
        return FALSE;
    }
  }

  return TRUE;
}

static int AllocPortNum(int num)
{
  BusyMask busyMask;

  DevProperties devProperties;
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return -1;

  if (EnumDevices(EnumFilter, &devProperties, NULL, AddDeviceToBusyMask, &busyMask) < 0)
    return -1;

  return busyMask.IsFreeNum(num) ? num : busyMask.GetFirstFreeNum();
}

bool Install(const char *pInfFilePath, const char *pParametersA, const char *pParametersB, int num)
{
  int i;
  int res;

  do {
    res = IDCONTINUE;

    i = AllocPortNum(num >= 0 ? num : 0);

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
      if (!portParameters.ParseParametersStr(pParameters))
        goto err;

      portParameters.FillPortName(portName[j], sizeof(portName[j])/sizeof(portName[j][0]));

      if (!IsValidPortName(portName[j]))
        goto err;

      if (!Silent() && portParameters.DialogRequested())
        ShowMsg(MB_OK|MB_ICONWARNING, "Can't display the dialog while installing a pair of linked ports.\n");

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

  if (lstrcmpi(portName[0], portName[1]) == 0 &&
      lstrcmpi(portName[0], C0C_PORT_NAME_COMCLASS) != 0 &&
      ShowMsg(MB_OKCANCEL|MB_ICONWARNING,
              "The same port name %s is used for both ports.\n",
              portName[0]) != IDOK)
  {
    goto err;
  }

  if (!InstallBusDevice(pInfFilePath, i))
    goto err;

  ComDbSync(EnumFilter);

  /*
  if (timeout > 0 && !no_update) {
    if (WaitNoPendingInstallEvents(timeout)) {
      timeout = 0;
    } else {
      for (int j = 0 ; j < 2 ; j++) {
        int timeElapsed = SleepTillPortNotFound(portName[j], timeout);

        if (timeElapsed < 0 || timeout < timeElapsed) {
          timeout = 0;
          break;
        }

        timeout -= timeElapsed;
      }
    }
  }
  */

  return TRUE;

err:

  Trace("\nInstall not completed!\n");

  return FALSE;
}
///////////////////////////////////////////////////////////////
bool Uninstall(
    const InfFileInstall *pInfFileInstallList,
    const InfFile::InfFileUninstall *pInfFileUninstallList)
{
  BOOL rebootRequired = FALSE;
  DevProperties devProperties;

  devProperties = DevProperties();
  if (!devProperties.DevId(C0C_PORT_DEVICE_ID))
    goto err;

  {
    Stack stack;

    if (!DisableDevices(EnumFilter, &devProperties, &rebootRequired, &stack)) {
      CleanDevPropertiesStack(stack, TRUE, &rebootRequired);
      goto err;
    }

    CleanDevPropertiesStack(stack, FALSE, &rebootRequired);
  }

  devProperties = DevProperties();
  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    goto err;

  if (!RemoveDevices(EnumFilter, &devProperties, &rebootRequired))
    goto err;

  if (!RemoveDevices(EnumFilter, NULL, NULL))
    goto err;

  ComDbSync(EnumFilter);

  if (rebootRequired) {
    PromptReboot();
    goto err;
  }

  int res;
  bool notDeleted;
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
              if (Silent() ||
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
                            "Service %s is not stopped (state %ld).\n",
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
    goto err;

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
    goto err;

  do {
    notDeleted = TRUE;
    res = IDCONTINUE;

    HKEY hKey = SetupDiOpenClassRegKey(NULL, DELETE);

    if (hKey != INVALID_HANDLE_VALUE) {
      do {
        res = IDCONTINUE;

        err = RegDeleteKey(hKey, C0C_CNCCLASS_GUID);

        if (err != ERROR_SUCCESS) {
          HKEY hClassGuidKey;
          err = RegOpenKeyEx(hKey, C0C_CNCCLASS_GUID, 0, KEY_READ, &hClassGuidKey);

          if (err == ERROR_SUCCESS) {
            for (;;) {
              char subKey[MAX_PATH + 1];
              DWORD subKeySize = sizeof(subKey)/sizeof(subKey[0]);

              err = RegEnumKeyEx(hClassGuidKey, 0, subKey, &subKeySize, NULL, NULL, NULL, NULL);

              if (err != ERROR_SUCCESS)
                break;

              err = RegDeleteKey(hClassGuidKey, subKey);

              if (err == ERROR_SUCCESS)
                Trace("Deleted Class subkey %s\\%s\n", C0C_CNCCLASS_GUID, subKey);
              else
              if (err != ERROR_FILE_NOT_FOUND) {
                ShowError(MB_OK|MB_ICONWARNING, err, "RegDeleteKey(%s\\%s)", C0C_CNCCLASS_GUID, subKey);
                break;
              }
            }

            err = RegCloseKey(hClassGuidKey);

            if (err != ERROR_SUCCESS)
              ShowError(MB_OK|MB_ICONWARNING, err, "RegCloseKey()");
          }

          err = RegDeleteKey(hKey, C0C_CNCCLASS_GUID);
        }

        if (err == ERROR_SUCCESS) {
          Trace("Deleted Class %s\n", C0C_CNCCLASS_GUID);
          notDeleted = FALSE;
        }
        else
        if (err == ERROR_FILE_NOT_FOUND) {
          Trace("Class %s not installed\n", C0C_CNCCLASS_GUID);
          notDeleted = FALSE;
        }
        else {
          res = ShowError(MB_CANCELTRYCONTINUE, err, "RegDeleteKey(%s)", C0C_CNCCLASS_GUID);
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
    Trace("WARNING: Class %s not deleted\n", C0C_CNCCLASS_GUID);

  if (res != IDCONTINUE)
    goto err;

  for (
      const InfFileInstall *pInfFileInstall = pInfFileInstallList ;
      pInfFileInstall->pInfName != NULL ;
      pInfFileInstall++)
  {
    InfFile infFile(pInfFileInstall->pInfName, pInfFileInstall->pInfName);

    if (!infFile.UninstallOEMInf())
      goto err;

    if (pInfFileInstall->pCopyDriversSection != NULL) {
      if (!infFile.UninstallFiles(pInfFileInstall->pCopyDriversSection))
        goto err;
    }
  }

  if (!InfFile::UninstallAllInfFiles(pInfFileUninstallList, NULL))
    goto err;

  return TRUE;

err:

  Trace("\nUninstall not completed!\n");

  return FALSE;
}
///////////////////////////////////////////////////////////////
bool InfClean(
    const InfFileInstall *pInfFileInstallList,
    const InfFile::InfFileUninstall *pInfFileUninstallList)
{
  bool ok = TRUE;
  InfFile **pInfFiles = NULL;
  const char **ppOemPathExcludeList = NULL;
  int numInfFiles = 0;

  for (
      const InfFileInstall *pInfFileInstall = pInfFileInstallList ;
      pInfFileInstall->pInfName != NULL ;
      pInfFileInstall++)
  {
    InfFile *pInfFile = new InfFile(pInfFileInstall->pInfName, pInfFileInstall->pInfName);

    if (pInfFile->OemPath() == NULL) {
      delete pInfFile;
      continue;
    }

    InfFile **pNewInfFiles;

    if (!pInfFiles)
      pNewInfFiles = (InfFile **)LocalAlloc(LPTR, (numInfFiles + 1) * sizeof(InfFile *));
    else
      pNewInfFiles = (InfFile **)LocalReAlloc(pInfFiles, (numInfFiles + 1) * sizeof(InfFile *), LMEM_ZEROINIT|LMEM_MOVEABLE);

    if (!pNewInfFiles) {
      ShowError(MB_OK|MB_ICONSTOP, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (numInfFiles + 1) * sizeof(InfFile *));
      ok = FALSE;
      goto end;
    }

    pInfFiles = pNewInfFiles;
    pInfFiles[numInfFiles++] = pInfFile;
  }

  if (pInfFiles != NULL) {
    ppOemPathExcludeList = (const char **)LocalAlloc(LPTR, (numInfFiles + 1) * sizeof(const char *));

    if (!ppOemPathExcludeList) {
      ShowError(MB_OK|MB_ICONSTOP, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (numInfFiles + 1) * sizeof(const char *));
      ok = FALSE;
      goto end;
    }

    for (int i = 0 ; i < numInfFiles ; i++)
      ppOemPathExcludeList[i] = pInfFiles[i]->OemPath();
  }


  if (!InfFile::UninstallAllInfFiles(pInfFileUninstallList, ppOemPathExcludeList)) {
    ok = FALSE;
    goto end;
  }

end:

  if (ppOemPathExcludeList != NULL)
    LocalFree((HLOCAL)ppOemPathExcludeList);

  if (pInfFiles != NULL) {
    for (int i = 0 ; i < numInfFiles ; i++) {
      delete pInfFiles[i];
    }

    LocalFree((HLOCAL)pInfFiles);
  }

  if (!ok)
    Trace("\nCleaning not completed!\n");

  return ok;
}
///////////////////////////////////////////////////////////////
bool ShowBusyNames(const char *pPattern)
{
  char *pPatternUp;

  pPatternUp = STRDUP(pPattern);

  if (!pPatternUp)
    return FALSE;

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

        if (ShowError(MB_OKCANCEL, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)size) != IDOK) {
          if (pNames)
            LocalFree(pNames);

          LocalFree(pPatternUp);
          return FALSE;
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
          break;

        if (ShowLastError(MB_OKCANCEL,
            i != 0 ? "QueryDosDevice()" : "ComDbNames()") != IDOK)
        {
          if (pNames)
            LocalFree(pNames);

          LocalFree(pPatternUp);
          return FALSE;
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
          if (ShowError(MB_OKCANCEL, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)sizeNamesNew) != IDOK) {
            if (pNames)
              LocalFree(pNames);

            LocalFree(pPatternUp);
            return FALSE;
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

  return TRUE;
}
///////////////////////////////////////////////////////////////
void Help(const char *pProgName)
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
    "  --wait <to>                  - wait <to> seconds for install completion\n"
    "                                 (by default <to> is 0 - no wait)\n"
    "  --detail-prms                - show detailed parameters\n"
    "  --silent                     - suppress dialogs if possible\n"
    "  --no-update                  - do not update driver while install command\n"
    "                                 execution (the other install command w/o this\n"
    "                                 option expected later)\n"
    );
  ConsoleWrite(
    "\n"
    "Commands:\n"
    "  install <n> <prmsA> <prmsB>  - install a pair of linked ports with\n"
    "   or                            identifiers " C0C_PREF_PORT_NAME_A "<n> and "
                                      C0C_PREF_PORT_NAME_B "<n>\n"
    "  install <prmsA> <prmsB>        (by default <n> is the first not used number),\n"
    "                                 set their parameters to <prmsA> and <prmsB>\n"
    "  install                      - can be used to update driver after execution\n"
    "                                 of install commands with --no-update option\n"
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
    "  infclean                     - clean old INF files\n"
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
}
///////////////////////////////////////////////////////////////
static int Complete(bool ok)
{
  if (ok) {
    if (timeout > 0)
      WaitNoPendingInstallEvents(timeout);

    return 0;
  }

  return 1;
}

int Main(int argc, const char* argv[])
{
  if (!SetOutputFile(NULL))
    return 1;

  Silent(FALSE);
  timeout = 0;
  detailPrms = FALSE;
  no_update = FALSE;

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
    if (!strcmp(argv[1], "--wait") && argc > 2) {
      int num;

      if (StrToInt(argv[2], &num) && num >= 0)
        timeout = num;

      argv[2] = argv[0];
      argv += 2;
      argc -= 2;
    }
    else
    if (!strcmp(argv[1], "--detail-prms")) {
      detailPrms = TRUE;
      argv[1] = argv[0];
      argv++;
      argc--;
    }
    else
    if (!strcmp(argv[1], "--silent")) {
      Silent(TRUE);
      argv[1] = argv[0];
      argv++;
      argc--;
    }
    else
    if (!strcmp(argv[1], "--no-update")) {
      no_update = TRUE;
      argv[1] = argv[0];
      argv++;
      argc--;
    }
    else {
      ConsoleWrite("Invalid option %s\n", argv[1]);
      return 1;
    }
  }

  if (argc == 1) {
    return Complete(TRUE);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "help")) {
    Help(argv[0]);
    return Complete(TRUE);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "quit")) {
    return Complete(TRUE);
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "busynames")) {
    SetTitle(C0C_SETUP_TITLE " (BUSY NAMES)");
    return Complete(ShowBusyNames(argv[2]));
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "list")) {
    SetTitle(C0C_SETUP_TITLE " (LIST)");
    return Complete(Change(NULL, NULL));
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "change")) {
    SetTitle(C0C_SETUP_TITLE " (CHANGE)");
    return Complete(Change(argv[2], argv[3]));
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "remove")) {
    SetTitle(C0C_SETUP_TITLE " (REMOVE)");

    int num;

    if (StrToInt(argv[2], &num) && num >= 0)
      return Complete(Remove(num));
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "disable")) {
    SetTitle(C0C_SETUP_TITLE " (DISABLE)");

    if (!lstrcmpi(argv[2], "all"))
      return Complete(Disable());
  }
  else
  if (argc == 3 && !lstrcmpi(argv[1], "enable")) {
    SetTitle(C0C_SETUP_TITLE " (ENABLE)");

    if (!lstrcmpi(argv[2], "all"))
      return Complete(Enable());
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "reload")) {
    SetTitle(C0C_SETUP_TITLE " (RELOAD)");
    return Complete(Reload(C0C_BUS_DEVICE_ID, NULL, NULL));
  }

  for (
      const InfFileInstall *pInfFileInstall = infFileInstallList ;
      pInfFileInstall->pInfName != NULL ;
      pInfFileInstall++)
  {
    InfFile infFile(pInfFileInstall->pInfName, pInfFileInstall->pInfName);
    if (!infFile.Test(pInfFileInstall->pRequiredFields))
      return 1;
  }

  if (argc == 2 && !lstrcmpi(argv[1], "preinstall")) {
    SetTitle(C0C_SETUP_TITLE " (PREINSTALL)");
    return Complete(Preinstall(infFileInstallList));
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "uninstall")) {
    SetTitle(C0C_SETUP_TITLE " (UNINSTALL)");
    return Complete(Uninstall(infFileInstallList, infFileUnnstallList));
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "infclean")) {
    SetTitle(C0C_SETUP_TITLE " (INF CLEAN)");
    return Complete(InfClean(infFileInstallList, infFileUnnstallList));
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "update")) {
    SetTitle(C0C_SETUP_TITLE " (UPDATE)");
    return Complete(Update(infFileInstallList));
  }

  InfFile infFile(C0C_INF_NAME, C0C_INF_NAME);

  const char *pPath = infFile.Path();

  if (!pPath)
    return 1;

  if (argc == 2 && !lstrcmpi(argv[1], "install")) {
    SetTitle(C0C_SETUP_TITLE " (INSTALL)");
    return Complete(Install(pPath));
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "install")) {
    SetTitle(C0C_SETUP_TITLE " (INSTALL)");
    return Complete(Install(pPath, argv[2], argv[3], -1));
  }
  else
  if (argc == 5 && !lstrcmpi(argv[1], "install")) {
    SetTitle(C0C_SETUP_TITLE " (INSTALL)");

    int num;

    if (StrToInt(argv[2], &num) && num >= 0)
      return Complete(Install(pPath, argv[3], argv[4], num));
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

  char cmd[1024];

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
