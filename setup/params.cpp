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
 * Revision 1.5  2007/06/01 16:32:04  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.4  2007/01/11 15:05:03  vfrolov
 * Replaced strtok() by STRTOK_R()
 *
 * Revision 1.3  2006/11/02 16:11:58  vfrolov
 * Added default values to help text
 *
 * Revision 1.2  2006/10/27 13:11:58  vfrolov
 * Added PortParameters::FillPortName()
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "params.h"
#include "utils.h"
#include "msg.h"

///////////////////////////////////////////////////////////////
enum {
  m_portName       = 0x0001,
  m_emuBR          = 0x0002,
  m_emuOverrun     = 0x0004,
  m_plugInMode     = 0x0008,
  m_exclusiveMode  = 0x0010,
};
///////////////////////////////////////////////////////////////
static DWORD flagBits[] = {
  m_emuBR,
  m_emuOverrun,
  m_plugInMode,
  m_exclusiveMode
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
  plugInMode = 0;
  exclusiveMode = 0;

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
DWORD *PortParameters::GetFlagPtr(DWORD bit)
{
  switch (bit) {
    case m_emuBR:          return &emuBR;
    case m_emuOverrun:     return &emuOverrun;
    case m_plugInMode:     return &plugInMode;
    case m_exclusiveMode:  return &exclusiveMode;
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
const char *PortParameters::GetBitName(DWORD bit)
{
  switch (bit) {
    case m_portName:       return "PortName";
    case m_emuBR:          return "EmuBR";
    case m_emuOverrun:     return "EmuOverrun";
    case m_plugInMode:     return "PlugInMode";
    case m_exclusiveMode:  return "ExclusiveMode";
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetFlag(const char *pNewFlag, DWORD bit)
{
  if (!lstrcmpi(pNewFlag, "*"))
    return TRUE;

  DWORD newFlag;

  if (!lstrcmpi(pNewFlag, "yes")) {
    newFlag = 0xFFFFFFFF;
  }
  else
  if (!lstrcmpi(pNewFlag, "no")) {
    newFlag = 0;
  }
  else
  if (!lstrcmpi(pNewFlag, "-")) {
    newFlag = 0;
  }
  else {
    return FALSE;
  }

  DWORD *pFlag = GetFlagPtr(bit);

  if (pFlag == NULL)
    return FALSE;

  if (lstrcmpi("-", pNewFlag)) {
    if ((maskExplicit & bit) == 0) {
      maskExplicit |= bit;
      maskChanged |= bit;
    }
  } else {
    if (maskExplicit & bit) {
      maskExplicit &= ~bit;
      maskChanged |= bit;
    }
  }

  if (*pFlag != newFlag) {
    *pFlag = newFlag;
    maskChanged |= bit;
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
void PortParameters::LoadFlag(HKEY hKey, DWORD bit)
{
  DWORD *pFlag = GetFlagPtr(bit);
  const char *pName = GetBitName(bit);

  if (pFlag == NULL || pName == NULL)
    return;

  DWORD buf;
  DWORD len = sizeof(buf);

  LONG err = RegQueryValueEx(hKey,
                             pName,
                             NULL,
                             NULL,
                             (PBYTE)&buf,
                             &len);

  if (err == ERROR_SUCCESS) {
    *pFlag = buf;
    maskExplicit |= bit;

    //Trace("  %s=0x%lX\n", pName, (unsigned long)*pFlag);
  }
}
///////////////////////////////////////////////////////////////
LONG PortParameters::SaveFlag(HKEY hKey, DWORD bit)
{
  if (maskChanged & bit) {
    DWORD *pFlag = GetFlagPtr(bit);
    const char *pName = GetBitName(bit);

    if (pFlag == NULL || pName == NULL)
      return ERROR_BAD_COMMAND;

    LONG err;

    if (maskExplicit & bit) {
      err = RegSetValueEx(hKey,
                          pName,
                          NULL,
                          REG_DWORD,
                          (PBYTE)pFlag,
                          sizeof(*pFlag));
    } else {
      err = RegDeleteValue(hKey, pName);

      if (err == ERROR_FILE_NOT_FOUND)
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
      return err;

    maskChanged &= ~bit;
    //Trace("  New %s=0x%lX\n", pName, (unsigned long)*pFlag);
  }

  return ERROR_SUCCESS;
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

  for (int i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++)
    LoadFlag(hKey, flagBits[i]);

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

  for (int i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
    if (SaveFlag(hKey, flagBits[i]) != ERROR_SUCCESS)
      goto err;
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

    for (int i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
      if (!tmp.SetFlag(pParameters, flagBits[i]))
        return FALSE;
    }
  } else {
    char pars[200];

    lstrcpyn(pars, pParameters, sizeof(pars));
    pars[sizeof(pars) - 1] = 0;

    char *pSave1;

    for (char *pPar = STRTOK_R(pars, ",", &pSave1) ; pPar ; pPar = STRTOK_R(NULL, ",", &pSave1)) {
      char *pSave2;
      const char *pKey = STRTOK_R(pPar, "=", &pSave2);
      const char *pVal = STRTOK_R(NULL, "=", &pSave2);

      //Trace("'%s'='%s'\n", pKey, pVal);

      if (!pVal)
        return FALSE;

      if (!lstrcmpi(pKey, "PortName")) {
        if (!tmp.SetPortName(pVal))
          return FALSE;
      } else {
        int i;

        for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
          DWORD bit = flagBits[i];
          DWORD *pFlag = GetFlagPtr(bit);
          const char *pName = GetBitName(bit);

          if (pFlag == NULL || pName == NULL)
            continue;

          if (!lstrcmpi(pKey, pName)) {
            if (!tmp.SetFlag(pVal, bit))
              return FALSE;
            break;
          }
        }
        if (i >= sizeof(flagBits)/sizeof(flagBits[0]))
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

  len = SNPRINTF(pParameters, size, "PortName=%s", (maskExplicit & m_portName) ? (portName) : "-");

  if (len < 0)
    return FALSE;

  pParameters += len;
  size -= len;

  for (int i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
    DWORD bit = flagBits[i];

    if ((maskExplicit & bit) != 0) {
      DWORD *pFlag = GetFlagPtr(bit);
      const char *pName = GetBitName(bit);

      if (pFlag == NULL || pName == NULL)
        continue;

      len = SNPRINTF(pParameters, size, ",%s=%s", pName, *pFlag ? "yes" : "no");

      if (len < 0)
        return FALSE;

      pParameters += len;
      size -= len;
    }
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::FillPortName(char *pPortName, int size)
{
  int len;

  len = SNPRINTF(pPortName, size, "%s",
                 (maskExplicit & m_portName) ? portName : phPortName);

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
    "                            (port identifier by default)\n"
    "  EmuBR={yes|no}          - enable/disable baud rate emulation in the direction\n"
    "                            to the paired port (disabled by default)\n"
    "  EmuOverrun={yes|no}     - enable/disable buffer overrun (disabled by default)\n"
    "  PlugInMode={yes|no}     - enable/disable plug-in mode (disabled by default),\n"
    "                            the plug-in mode port is hidden and can't be open if\n"
    "                            the paired port is not open\n"
    "  ExclusiveMode={yes|no}  - enable/disable exclusive mode (disabled by default),\n"
    "                            the exclusive mode port is hidden if it is open\n"
    "\n"
    "Special values:\n"
    "  -                       - use driver's default value\n"
    "  *                       - use current setting\n"
    ;
}
///////////////////////////////////////////////////////////////
