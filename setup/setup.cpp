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
 *
 */

#include "precomp.h"
#include "inffile.h"
#include "params.h"
#include "devutils.h"
#include "msg.h"
#include "utils.h"

#define TEXT_PREF
#include "../include/com0com.h"

#define C0C_INF_NAME             "com0com.inf"
#define C0C_CLASS_GUID           "{df799e12-3c56-421b-b298-b6d3642bc878}"
#define C0C_CLASS                "CNCPorts"
#define C0C_PROVIDER             "Vyacheslav Frolov"
#define C0C_REGKEY_EVENTLOG      REGSTR_PATH_SERVICES "\\Eventlog\\System\\" C0C_SERVICE
#define C0C_COPY_DRIVERS_SECTION "com0com_CopyDrivers"

///////////////////////////////////////////////////////////////
int Change(InfFile &infFile, const char *pPhPortName, const char *pParameters)
{
  BOOL rebootRequired = FALSE;
  int numPairs = CountDevices(infFile, C0C_BUS_DEVICE_ID);

  //Trace("Found %d pairs\n", numPairs);

  for (int i = 0 ; i < numPairs ; i++) {
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
        if (pPhPortName && !lstrcmpi(pPhPortName, phPortName))
          portParameters.ParseParametersStr(pParameters);

        if (portParameters.Changed()) {
          err = portParameters.Save();

          if (err == ERROR_SUCCESS) {
            portParameters.FillParametersStr(buf, sizeof(buf)/sizeof(buf[0]));
            Trace("change %s %s\n", phPortName, buf);

            char phDevDevName[40];

            SNPRINTF(phDevDevName, sizeof(phDevDevName)/sizeof(phDevDevName[0]), "%s%d",
                     j ? C0C_PREF_DEVICE_NAME_A : C0C_PREF_DEVICE_NAME_B, i);

            RestartDevices(infFile, C0C_PORT_DEVICE_ID, phDevDevName, &rebootRequired);
          } else {
            ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Save(%s)", phPortName);
          }
        }
      } else {
        ShowError(MB_OK|MB_ICONWARNING, err, "portParameters.Load(%s)", phPortName);
      }
    }
  }

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
int Install(InfFile &infFile, const char *pParametersA, const char *pParametersB)
{
  int i = CountDevices(infFile, C0C_BUS_DEVICE_ID);

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

  if (!InstallDevice(infFile, C0C_BUS_DEVICE_ID))
    return 1;

  return  0;
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
int Help(const char *pProgName)
{
  Trace(
    "Usage:\n"
    "  %s <command>\n"
    , pProgName);
  Trace(
    "\n"
    "Commands:\n"
    "  list                         - for each port show parameters\n"
    "  change <port> <params>       - set parameters for port\n"
    "  install <paramsA> <paramsB>  - install a pair of ports\n"
    "  preinstall                   - preinstall driver\n"
    "  update                       - update driver\n"
    "  uninstall                    - uninstall all pairs and driver\n"
    "\n"
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
    "  %s install - -\n"
    , pProgName);
  Trace(
    "  %s install PortName=COM2 PortName=COM4\n"
    , pProgName);
  Trace(
    "  %s change " C0C_PREF_PORT_NAME_A "0 EmuBR=yes,EmuOverrun=yes\n"
    , pProgName);

  return 1;
}
///////////////////////////////////////////////////////////////
int Main(int argc, const char* argv[])
{
  InfFile infFile(C0C_INF_NAME, C0C_INF_NAME);

  if (!infFile.Compare(C0C_CLASS_GUID, C0C_CLASS, C0C_PROVIDER))
    return 1;

  if (argc == 2 && !lstrcmpi(argv[1], "list")) {
    return Change(infFile, NULL, NULL);
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "change")) {
    return Change(infFile, argv[2], argv[3]);
  }
  else
  if (argc == 4 && !lstrcmpi(argv[1], "install")) {
    return Install(infFile, argv[2], argv[3]);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "preinstall")) {
    return Preinstall(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "update")) {
    return Update(infFile);
  }
  else
  if (argc == 2 && !lstrcmpi(argv[1], "uninstall")) {
    return Uninstall(infFile);
  }

  return Help(argv[0]);
}
///////////////////////////////////////////////////////////////
int CALLBACK RunDllA(HWND /*hWnd*/, HINSTANCE /*hInst*/, LPSTR pCmdLine, int /*nCmdShow*/)
{
  int argc;
  const char* argv[5];

  argc = 0;
  argv[argc++] = "rundll32 setup,RunDll";
  argv[argc] = NULL;

  char cmd[200];

  lstrcpyn(cmd, pCmdLine, sizeof(cmd)/sizeof(cmd[0]));

  for (const char *pArg = strtok(cmd, " \t") ; pArg ; pArg = strtok(NULL, " \t")) {
    if ((argc + 2) > sizeof(argv)/sizeof(argv[0]))
      break;

    argv[argc++] = pArg;
    argv[argc] = NULL;
  }

  int res = Main(argc, argv);

  char buf[2];

  ConsoleWriteRead(buf, sizeof(buf)/sizeof(buf[0]), "\nresult = %d\n\nPress <RETURN> to continue\n", res);

  return res;
}
///////////////////////////////////////////////////////////////
