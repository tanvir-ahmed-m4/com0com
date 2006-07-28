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

#include "precomp.h"
#include "params.h"
#include "utils.h"

///////////////////////////////////////////////////////////////
enum {
  m_portName    = 0x0001,
  m_emuBR       = 0x0002,
  m_emuOverrun  = 0x0004,
};
///////////////////////////////////////////////////////////////
PortParameters::PortParameters(const char *pService, const char *pPhPortName)
{
  SNPRINTF(service, sizeof(service), "%s", pService);
  SNPRINTF(phPortName, sizeof(phPortName), "%s", pPhPortName);

  Init();
}
///////////////////////////////////////////////////////////////
void PortParameters::Init()
{
  SNPRINTF(portName, sizeof(portName), "%s", phPortName);
  emuBR = 0;
  emuOverrun = 0;

  maskChanged = 0;
  maskExplicit = 0;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetPortName(const char *pNewPortName)
{
  if (!lstrcmpi(pNewPortName, "*"))
    return TRUE;

  if (lstrcmpi(pNewPortName, "-")) {
    if ((maskExplicit & m_portName) == 0) {
      maskExplicit |= m_portName;
      maskChanged |= m_portName;
    }
  } else {
    pNewPortName = phPortName;
    if (maskExplicit & m_portName) {
      maskExplicit &= ~m_portName;
      maskChanged |= m_portName;
    }
  }

  if (lstrcmpi(portName, pNewPortName)) {
    lstrcpyn(portName, pNewPortName, sizeof(portName));
    portName[sizeof(portName) - 1] = 0;
    maskChanged |= m_portName;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetEmuBR(const char *pNewEmuBR)
{
  if (!lstrcmpi(pNewEmuBR, "*"))
    return TRUE;

  DWORD newEmuBR;

  if (!lstrcmpi(pNewEmuBR, "yes")) {
    newEmuBR = 0xFFFFFFFF;
  }
  else
  if (!lstrcmpi(pNewEmuBR, "no")) {
    newEmuBR = 0;
  }
  else
  if (!lstrcmpi(pNewEmuBR, "-")) {
    newEmuBR = 0;
  }
  else {
    return FALSE;
  }

  if (lstrcmpi("-", pNewEmuBR)) {
    if ((maskExplicit & m_emuBR) == 0) {
      maskExplicit |= m_emuBR;
      maskChanged |= m_emuBR;
    }
  } else {
    if (maskExplicit & m_emuBR) {
      maskExplicit &= ~m_emuBR;
      maskChanged |= m_emuBR;
    }
  }

  if (emuBR != newEmuBR) {
    emuBR = newEmuBR;
    maskChanged |= m_emuBR;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetEmuOverrun(const char *pNewEmuOverrun)
{
  if (!lstrcmpi(pNewEmuOverrun, "*"))
    return TRUE;

  DWORD newEmuOverrun;

  if (!lstrcmpi(pNewEmuOverrun, "yes")) {
    newEmuOverrun = 0xFFFFFFFF;
  }
  else
  if (!lstrcmpi(pNewEmuOverrun, "no")) {
    newEmuOverrun = 0;
  }
  else
  if (!lstrcmpi(pNewEmuOverrun, "-")) {
    newEmuOverrun = 0;
  }
  else {
    return FALSE;
  }

  if (lstrcmpi("-", pNewEmuOverrun)) {
    if ((maskExplicit & m_emuOverrun) == 0) {
      maskExplicit |= m_emuOverrun;
      maskChanged |= m_emuOverrun;
    }
  } else {
    if (maskExplicit & m_emuOverrun) {
      maskExplicit &= ~m_emuOverrun;
      maskChanged |= m_emuOverrun;
    }
  }

  if (emuOverrun != newEmuOverrun) {
    emuOverrun = newEmuOverrun;
    maskChanged |= m_emuOverrun;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::FillParametersKey(char *pRegKey, int size)
{
  int len;

  len = SNPRINTF(pRegKey, size, REGSTR_PATH_SERVICES "\\%s\\Parameters\\%s", service, phPortName);

  //Trace("%s\n", pRegKey);

  return len >= 0;
}
///////////////////////////////////////////////////////////////
LONG PortParameters::Load()
{
  Init();

  char reqKey[100];

  FillParametersKey(reqKey, sizeof(reqKey));

  LONG err;
  HKEY hKey;

  err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     reqKey,
                     0,
                     KEY_READ,
                     &hKey);

  if (err == ERROR_FILE_NOT_FOUND)
    return ERROR_SUCCESS;

  if (err != ERROR_SUCCESS)
    return err;

  char buf[20];
  DWORD len;

  len = sizeof(buf);

  err = RegQueryValueEx(hKey,
                        "PortName",
                        NULL,
                        NULL,
                        (PBYTE)buf,
                        &len);

  if (err == ERROR_SUCCESS) {
    SNPRINTF(portName, sizeof(portName), "%s", buf);
    maskExplicit |= m_portName;

    //Trace("  PortName=%s\n", portName);
  }

  len = sizeof(buf);

  err = RegQueryValueEx(hKey,
                        "EmuBR",
                        NULL,
                        NULL,
                        (PBYTE)buf,
                        &len);

  if (err == ERROR_SUCCESS) {
    emuBR = *(PDWORD)buf;
    maskExplicit |= m_emuBR;

    //Trace("  EmuBR=0x%lX\n", (unsigned long)emuBR);
  }

  len = sizeof(buf);

  err = RegQueryValueEx(hKey,
                        "EmuOverrun",
                        NULL,
                        NULL,
                        (PBYTE)buf,
                        &len);

  if (err == ERROR_SUCCESS) {
    emuOverrun = *(PDWORD)buf;
    maskExplicit |= m_emuOverrun;

    //Trace("  EmuOverrun=0x%lX\n", (unsigned long)emuOverrun);
  }

  RegCloseKey(hKey);

  return ERROR_SUCCESS;
}
///////////////////////////////////////////////////////////////
LONG PortParameters::Save()
{
  if (!Changed()) {
    return ERROR_SUCCESS;
  }

  char reqKey[100];

  FillParametersKey(reqKey, sizeof(reqKey));

  LONG err;
  HKEY hKey;

  err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       reqKey,
                       0,
                       NULL,
                       0,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       NULL);

  if (err != ERROR_SUCCESS)
    return err;

  if (maskChanged & m_portName) {
    if (maskExplicit & m_portName) {
      err = RegSetValueEx(hKey,
                          "PortName",
                          NULL,
                          REG_SZ,
                          (PBYTE)portName,
                          (lstrlen(portName) + 1) * sizeof(portName[0]));
    } else {
      err = RegDeleteValue(hKey, "PortName");

      if (err == ERROR_FILE_NOT_FOUND)
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
      goto err;

    maskChanged &= ~m_portName;

    //Trace("  New PortName=%s\n", portName);
  }

  if (maskChanged & m_emuBR) {
    if (maskExplicit & m_emuBR) {
      err = RegSetValueEx(hKey,
                          "EmuBR",
                          NULL,
                          REG_DWORD,
                          (PBYTE)&emuBR,
                          sizeof(emuBR));
    } else {
      err = RegDeleteValue(hKey, "EmuBR");

      if (err == ERROR_FILE_NOT_FOUND)
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
      goto err;

    maskChanged &= ~m_emuBR;
    //Trace("  New EmuBR=0x%lX\n", (unsigned long)emuBR);
  }

  if (maskChanged & m_emuOverrun) {
    if (maskExplicit & m_emuOverrun) {
      err = RegSetValueEx(hKey,
                          "EmuOverrun",
                          NULL,
                          REG_DWORD,
                          (PBYTE)&emuOverrun,
                          sizeof(emuOverrun));
    } else {
      err = RegDeleteValue(hKey, "EmuOverrun");

      if (err == ERROR_FILE_NOT_FOUND)
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
      goto err;

    maskChanged &= ~m_emuOverrun;
    //Trace("  New EmuOverrun=0x%lX\n", (unsigned long)emuOverrun);
  }

err:

  RegCloseKey(hKey);

  return err;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::ParseParametersStr(const char *pParameters)
{
  PortParameters tmp = *this;

  if (!lstrcmpi(pParameters, "-") || !lstrcmpi(pParameters, "*")) {
    if (!tmp.SetPortName(pParameters))
      return FALSE;
    if (!tmp.SetEmuBR(pParameters))
      return FALSE;
    if (!tmp.SetEmuOverrun(pParameters))
      return FALSE;
  } else {
    char pars[100];

    lstrcpyn(pars, pParameters, sizeof(pars));
    pars[sizeof(pars) - 1] = 0;

    for (const char *pPar = strtok(pars, "=") ; pPar ; pPar = strtok(NULL, "=")) {
      const char *pVal = strtok(NULL, ",");

      //Trace("'%s'='%s'\n", pPar, pVal);

      if (!pVal)
        return FALSE;

      if (!lstrcmpi(pPar, "PortName")) {
        if (!tmp.SetPortName(pVal))
          return FALSE;
      }
      else
      if (!lstrcmpi(pPar, "EmuBR")) {
        if (!tmp.SetEmuBR(pVal))
          return FALSE;
      }
      else
      if (!lstrcmpi(pPar, "EmuOverrun")) {
        if (!tmp.SetEmuOverrun(pVal))
          return FALSE;
      }
      else {
        return FALSE;
      }
    }
  }

  *this = tmp;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::FillParametersStr(char *pParameters, int size)
{
  int len;

  len = SNPRINTF(pParameters, size, "PortName=%s,EmuBR=%s,EmuOverrun=%s",
                 (maskExplicit & m_portName) ? (portName) : "-",
                 (maskExplicit & m_emuBR) ? (emuBR ? "yes" : "no") : "-",
                 (maskExplicit & m_emuOverrun) ? (emuOverrun ? "yes" : "no") : "-");

  return len >= 0;
}
///////////////////////////////////////////////////////////////
const char *PortParameters::GetHelp()
{
  return
    "Syntax of port parameters string:\n"
    "  -                       - use driver's defaults for all parameters\n"
    "  *                       - use current settings for all parameters\n"
    "  <par>=<val>[,...]       - set value <val> for each parameter <par>\n"
    "\n"
    "Parameters:\n"
    "  PortName=<name>         - set port name to <name>\n"
    "  EmuBR={yes|no}          - enable/disable baud rate emulation\n"
    "  EmuOverrun={yes|no}     - enable/disable buffer overrun\n"
    "\n"
    "Special values:\n"
    "  -                       - use driver's default value\n"
    "  *                       - use current setting\n"
    ;
}
///////////////////////////////////////////////////////////////
