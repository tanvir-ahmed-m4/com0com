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
    const char *pPortName,
    const char *pPhDevName)
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
        break;

      phDevName[0] = 0;
    }

    if (pPhDevName && !lstrcmpi(pPhDevName, phDevName))
      break;

    res = ShowMsg(MB_CANCELTRYCONTINUE,
                  "The port name %s is already used for other device %s.",
                  pPortName, phDevName);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
struct ChangeDeviceParams {
  ChangeDeviceParams(InfFile &_infFile, const char *_pPhPortName, const char *_pParameters)
    : pInfFile(&_infFile), pPhPortName(_pPhPortName), pParameters(_pParameters) {}

  InfFile *pInfFile;
  const char *pPhPortName;
  const char *pParameters;
};

static BOOL ChangeDevice(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
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

      char buf[100];

      portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]));
      Trace("       %s %s\n", phPortName, buf);

      if (err == ERROR_SUCCESS) {
        if (pPhPortName && !lstrcmpi(pPhPortName, phPortName)) {
          portParameters.ParseParametersStr(pParameters);

          char phDevName[40];

          SNPRINTF(phDevName, sizeof(phDevName)/sizeof(phDevName[0]), "%s%d",
                   j ? C0C_PREF_DEVICE_NAME_B : C0C_PREF_DEVICE_NAME_A, i);

          char portName[20];

          portParameters.FillPortName(portName, sizeof(portName)/sizeof(portName[0]));

          if (IsValidPortName(portName, phDevName) && portParameters.Changed()) {
            err = portParameters.Save();

            if (err == ERROR_SUCCESS) {
              portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]));
              Trace("change %s %s\n", phPortName, buf);

              RestartDevices(infFile, C0C_PORT_DEVICE_ID, phDevName, pRebootRequired);
            } else {
              ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Save(%s)", phPortName);
            }
          }
        }
      } else {
        ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Load(%s)", phPortName);
      }
    }
  }

  return TRUE;
}

int Change(InfFile &infFile, const char *pPhPortName, const char *pParameters)
{
  BOOL rebootRequired = FALSE;
  ChangeDeviceParams params(infFile, pPhPortName, pParameters);

  EnumDevices(infFile, C0C_BUS_DEVICE_ID, &rebootRequired, ChangeDevice, &params);

  if (rebootRequired)
    SetupPromptReboot(NULL, NULL, FALSE);

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
int Update(InfFile &infFile)
{
  if (!UpdateDriverForPlugAndPlayDevices(0, C0C_BUS_DEVICE_ID, infFile.Path(), INSTALLFLAG_FORCE, NULL))
    return 1;

  return  0;
}
///////////////////////////////////////////////////////////////
static BOOL SetPortNum(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    BOOL * /* pRebootRequired */,
    void *pParam)
{
  int res;

  do {
    res = IDCONTINUE;

    LONG err = SetPortNum(hDevInfo, pDevInfoData, *(int *)pParam);

    if (err != ERROR_SUCCESS)
      res = ShowError(MB_CANCELTRYCONTINUE, err, "SetPortNum(%d)", *(int *)pParam);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}

static BOOL InstallBusDevice(InfFile &infFile, int num)
{
  return InstallDevice(infFile, C0C_BUS_DEVICE_ID, SetPortNum, &num);
}

static BOOL AddDeviceToBusyMask(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    BOOL * /* pRebootRequired */,
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

  if (EnumDevices(infFile, C0C_BUS_DEVICE_ID, NULL, AddDeviceToBusyMask, &busyMask) < 0)
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

      char portName[20];

      portParameters.FillPortName(portName, sizeof(portName)/sizeof(portName[0]));

      if (!IsValidPortName(portName, NULL))
        goto err;

      if (portParameters.Changed()) {
        err = portParameters.Save();

        if (err != ERROR_SUCCESS)
          ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Save(%s)", phPortName);
      }
    } else {
      ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Load(%s)", phPortName);
    }

    char buf[100];

    portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]));

    Trace("       %s %s\n", phPortName, buf);
  }

  if (!InstallBusDevice(infFile, i))
    goto err;

  return  0;

err:

  Trace("\nInstall not completed!\n");

  return  1;
}
///////////////////////////////////////////////////////////////
int Uninstall(InfFile &infFile)
{
  BOOL rebootRequired = FALSE;

  if (!DisableDevices(infFile, C0C_PORT_DEVICE_ID, &rebootRequired))
    return 1;

  if (!RemoveDevices(infFile, C0C_BUS_DEVICE_ID, &rebootRequired))
    return 1;

  if (!RemoveDevices(infFile, NULL, NULL))
    return 1;

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
              if (ShowMsg(MB_YESNO,
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
int Help(const char *pCmdPref)
{
  SetTitle(C0C_SETUP_TITLE " (HELP)");

  Trace(
    C0C_SETUP_TITLE "\n"
    "\n");
  Trace(
    "Usage:\n"
    "  %s<command>\n"
    , pCmdPref);
  Trace(
    "\n"
    "Commands:\n"
    "  install <n> <prmsA> <prmsB>  - install a pair of linked ports with\n"
    "   or                            identifiers " C0C_PREF_PORT_NAME_A "<n> and "
                                      C0C_PREF_PORT_NAME_B "<n>\n"
    "  install <prmsA> <prmsB>        (by default <n> is the first not used number),\n"
    "                                 set their parameters to <prmsA> and <prmsB>\n"
    "  change <portid> <prms>       - set parameters <prms> for port with\n"
    "                                 identifier <portid>\n"
    "  list                         - for each port show its identifier and\n"
    "                                 parameters\n"
    "  preinstall                   - preinstall driver\n"
    "  update                       - update driver\n"
    "  uninstall                    - uninstall all ports and the driver\n"
    "  quit                         - quit\n"
    "  help                         - print this help\n"
    );
  Trace(
    "\n"
    "%s",
    PortParameters::GetHelp());
  Trace(
    "\n"
    "Examples:\n"
    );
  Trace(
    "  %sinstall - -\n"
    , pCmdPref);
  Trace(
    "  %sinstall 5 * *\n"
    , pCmdPref);
  Trace(
    "  %sinstall PortName=COM2 PortName=COM4\n"
    , pCmdPref);
  Trace(
    "  %sinstall PortName=COM5,EmuBR=yes,EmuOverrun=yes -\n"
    , pCmdPref);
  Trace(
    "  %schange " C0C_PREF_PORT_NAME_A "0 EmuBR=yes,EmuOverrun=yes\n"
    , pCmdPref);
  Trace(
    "  %slist\n"
    , pCmdPref);
  Trace(
    "  %suninstall\n"
    , pCmdPref);
  Trace(
    "\n");

  return 1;
}
///////////////////////////////////////////////////////////////
int Main(int argc, const char* argv[])
{
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
  if (argc == 2 && !lstrcmpi(argv[1], "preinstall")) {
    SetTitle(C0C_SETUP_TITLE " (PREINSTALL)");
    return Preinstall(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "update")) {
    SetTitle(C0C_SETUP_TITLE " (UPDATE)");
    return Update(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "uninstall")) {
    SetTitle(C0C_SETUP_TITLE " (UNINSTALL)");
    return Uninstall(infFile);
  }

  Trace("Invalid command\n");

  return 1;
}
///////////////////////////////////////////////////////////////
static int ParseCmd(char *pCmd, const char* argv[], int sizeArgv)
{
  int argc;

  argc = 0;

  for (char *pArg = strtok(pCmd, " \t\r\n") ; pArg ; pArg = strtok(NULL, " \t\r\n")) {
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
int CALLBACK RunDllA(HWND /*hWnd*/, HINSTANCE /*hInst*/, LPSTR pCmdLine, int /*nCmdShow*/)
{
  SetTitle(C0C_SETUP_TITLE);

  char cmd[200];

  lstrcpyn(cmd, pCmdLine, sizeof(cmd)/sizeof(cmd[0]));

  int argc;
  const char* argv[10];

  argc = ParseCmd(cmd, argv + 1, sizeof(argv)/sizeof(argv[0]) - 1) + 1;

  if (argc == 1) {
    Trace("Enter 'help' to get info about usage of " C0C_SETUP_TITLE ".\n\n");

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

  argv[0] = "rundll32 setup,RunDll ";

  int res = Main(argc, argv);

  ConsoleWriteRead(cmd, sizeof(cmd)/sizeof(cmd[0]), "\nPress <RETURN> to continue\n");

  return res;
}
///////////////////////////////////////////////////////////////
