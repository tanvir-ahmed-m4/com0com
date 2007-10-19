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
 * Revision 1.9  2007/10/19 16:09:55  vfrolov
 * Implemented --detail-prms option
 *
 * Revision 1.8  2007/09/20 12:43:03  vfrolov
 * Added parameters string length check
 *
 * Revision 1.7  2007/09/17 14:33:38  vfrolov
 * Implemented pseudo pin OPEN
 *
 * Revision 1.6  2007/07/03 14:39:49  vfrolov
 * Implemented pinout customization
 *
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

#define TEXT_PREF
#include "../include/com0com.h"

///////////////////////////////////////////////////////////////
enum {
  m_portName       = 0x0001,
  m_emuBR          = 0x0002,
  m_emuOverrun     = 0x0004,
  m_plugInMode     = 0x0008,
  m_exclusiveMode  = 0x0010,
  m_pinCTS         = 0x0100,
  m_pinDSR         = 0x0200,
  m_pinDCD         = 0x0400,
  m_pinRI          = 0x0800,
};
///////////////////////////////////////////////////////////////
static DWORD flagBits[] = {
  m_emuBR,
  m_emuOverrun,
  m_plugInMode,
  m_exclusiveMode
};
///////////////////////////////////////////////////////////////
static DWORD pinBits[] = {
  m_pinCTS,
  m_pinDSR,
  m_pinDCD,
  m_pinRI
};
///////////////////////////////////////////////////////////////
PortParameters::PortParameters(const char *pService, const char *pPhPortName)
{
  SNPRINTF(service, sizeof(service)/sizeof(service[0]), "%s", pService);
  SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s", pPhPortName);

  Init();
}
///////////////////////////////////////////////////////////////
void PortParameters::Init()
{
  SNPRINTF(portName, sizeof(portName)/sizeof(portName[0]), "%s", phPortName);
  emuBR = 0;
  emuOverrun = 0;
  plugInMode = 0;
  exclusiveMode = 0;

  pinCTS = 0;
  pinDSR = 0;
  pinDCD = 0;
  pinRI = 0;

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
    SNPRINTF(portName, sizeof(portName)/sizeof(portName[0]), "%s", pNewPortName);
    maskChanged |= m_portName;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
DWORD *PortParameters::GetDwPtr(DWORD bit)
{
  switch (bit) {
    case m_emuBR:          return &emuBR;
    case m_emuOverrun:     return &emuOverrun;
    case m_plugInMode:     return &plugInMode;
    case m_exclusiveMode:  return &exclusiveMode;
    case m_pinCTS:         return &pinCTS;
    case m_pinDSR:         return &pinDSR;
    case m_pinDCD:         return &pinDCD;
    case m_pinRI:          return &pinRI;
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
static const DWORD *GetDwPtrDefault(DWORD bit)
{
  static const DWORD emuBR = C0C_DEFAULT_EMUBR;
  static const DWORD emuOverrun = C0C_DEFAULT_EMUOVERRUN;
  static const DWORD plugInMode = C0C_DEFAULT_PLUGINMODE;
  static const DWORD exclusiveMode = C0C_DEFAULT_EXCLUSIVEMODE;
  static const DWORD pinCTS = C0C_DEFAULT_PIN_CTS;
  static const DWORD pinDSR = C0C_DEFAULT_PIN_DSR;
  static const DWORD pinDCD = C0C_DEFAULT_PIN_DCD;
  static const DWORD pinRI = C0C_DEFAULT_PIN_RI;

  switch (bit) {
    case m_emuBR:          return &emuBR;
    case m_emuOverrun:     return &emuOverrun;
    case m_plugInMode:     return &plugInMode;
    case m_exclusiveMode:  return &exclusiveMode;
    case m_pinCTS:         return &pinCTS;
    case m_pinDSR:         return &pinDSR;
    case m_pinDCD:         return &pinDCD;
    case m_pinRI:          return &pinRI;
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
static const char *GetBitName(DWORD bit)
{
  switch (bit) {
    case m_portName:       return "PortName";
    case m_emuBR:          return "EmuBR";
    case m_emuOverrun:     return "EmuOverrun";
    case m_plugInMode:     return "PlugInMode";
    case m_exclusiveMode:  return "ExclusiveMode";
    case m_pinCTS:         return "cts";
    case m_pinDSR:         return "dsr";
    case m_pinDCD:         return "dcd";
    case m_pinRI:          return "ri";
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetFlag(const char *pNewVal, DWORD bit)
{
  if (!lstrcmpi(pNewVal, "*"))
    return TRUE;

  DWORD newFlag;

  if (!lstrcmpi(pNewVal, "yes")) {
    newFlag = 0xFFFFFFFF;
  }
  else
  if (!lstrcmpi(pNewVal, "no")) {
    newFlag = 0;
  }
  else
  if (!lstrcmpi(pNewVal, "-")) {
    newFlag = 0;
  }
  else {
    Trace("Invalid value '%s'\n", pNewVal);
    return FALSE;
  }

  DWORD *pFlag = GetDwPtr(bit);

  if (pFlag == NULL)
    return FALSE;

  if (lstrcmpi("-", pNewVal)) {
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
BOOL PortParameters::SetPin(const char *pNewVal, DWORD bit)
{
  if (!lstrcmpi(pNewVal, "*"))
    return TRUE;

  DWORD newPin;

  if (*pNewVal == '!') {
    newPin = C0C_PIN_NEGATIVE;
    pNewVal++;
  } else {
    newPin = 0;
  }

  if (!lstrcmpi(pNewVal, "rrts")) {
    newPin |= C0C_PIN_RRTS;
  }
  else
  if (!lstrcmpi(pNewVal, "rdtr")) {
    newPin |= C0C_PIN_RDTR;
  }
  else
  if (!lstrcmpi(pNewVal, "rout1")) {
    newPin |= C0C_PIN_ROUT1;
  }
  else
  if (!lstrcmpi(pNewVal, "ropen")) {
    newPin |= C0C_PIN_ROPEN;
  }
  else
  if (!lstrcmpi(pNewVal, "lrts")) {
    newPin |= C0C_PIN_LRTS;
  }
  else
  if (!lstrcmpi(pNewVal, "ldtr")) {
    newPin |= C0C_PIN_LDTR;
  }
  else
  if (!lstrcmpi(pNewVal, "lout1")) {
    newPin |= C0C_PIN_LOUT1;
  }
  else
  if (!lstrcmpi(pNewVal, "lopen")) {
    newPin |= C0C_PIN_LOPEN;
  }
  else
  if (!lstrcmpi(pNewVal, "on")) {
    newPin |= C0C_PIN_ON;
  }
  else
  if (!lstrcmpi(pNewVal, "-")) {
    newPin = 0;
  }
  else {
    Trace("Invalid value '%s'\n", pNewVal);
    return FALSE;
  }

  DWORD *pPin = GetDwPtr(bit);

  if (pPin == NULL)
    return FALSE;

  if (lstrcmpi("-", pNewVal)) {
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

  if (*pPin != newPin) {
    *pPin = newPin;
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
void PortParameters::LoadDw(HKEY hKey, DWORD bit)
{
  DWORD *pDw = GetDwPtr(bit);
  const char *pName = GetBitName(bit);

  if (pDw == NULL || pName == NULL)
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
    *pDw = buf;
    maskExplicit |= bit;

    //Trace("  %s=0x%lX\n", pName, (unsigned long)*pDw);
  }
}
///////////////////////////////////////////////////////////////
LONG PortParameters::SaveDw(HKEY hKey, DWORD bit)
{
  if (maskChanged & bit) {
    DWORD *pDw = GetDwPtr(bit);
    const char *pName = GetBitName(bit);

    if (pDw == NULL || pName == NULL)
      return ERROR_BAD_COMMAND;

    LONG err;

    if (maskExplicit & bit) {
      err = RegSetValueEx(hKey,
                          pName,
                          NULL,
                          REG_DWORD,
                          (PBYTE)pDw,
                          sizeof(*pDw));
    } else {
      err = RegDeleteValue(hKey, pName);

      if (err == ERROR_FILE_NOT_FOUND)
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
      return err;

    maskChanged &= ~bit;
    //Trace("  New %s=0x%lX\n", pName, (unsigned long)*pDw);
  }

  return ERROR_SUCCESS;
}
///////////////////////////////////////////////////////////////
LONG PortParameters::Load()
{
  Init();

  char reqKey[100];

  FillParametersKey(reqKey, sizeof(reqKey)/sizeof(reqKey[0]));

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

  BYTE buf[sizeof(portName)];
  DWORD len;

  len = sizeof(buf);

  err = RegQueryValueEx(hKey,
                        "PortName",
                        NULL,
                        NULL,
                        buf,
                        &len);

  if (err == ERROR_SUCCESS) {
    SNPRINTF(portName, sizeof(portName)/sizeof(portName[0]), "%s", (char *)buf);
    maskExplicit |= m_portName;

    //Trace("  PortName=%s\n", portName);
  }

  int i;

  for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++)
    LoadDw(hKey, flagBits[i]);

  for (i = 0 ; i < sizeof(pinBits)/sizeof(pinBits[0]) ; i++)
    LoadDw(hKey, pinBits[i]);

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

  FillParametersKey(reqKey, sizeof(reqKey)/sizeof(reqKey[0]));

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

  int i;

  for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
    if (SaveDw(hKey, flagBits[i]) != ERROR_SUCCESS)
      goto err;
  }

  for (i = 0 ; i < sizeof(pinBits)/sizeof(pinBits[0]) ; i++) {
    if (SaveDw(hKey, pinBits[i]) != ERROR_SUCCESS)
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

    int i;

    for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
      if (!tmp.SetFlag(pParameters, flagBits[i]))
        return FALSE;
    }

    for (i = 0 ; i < sizeof(pinBits)/sizeof(pinBits[0]) ; i++) {
      if (!tmp.SetPin(pParameters, pinBits[i]))
        return FALSE;
    }
  } else {
    char pars[200];

    if (SNPRINTF(pars, sizeof(pars)/sizeof(pars[0]), "%s", pParameters) < 0) {
      Trace("The parameters string '%s' is too long\n", pParameters);
      return FALSE;
    }

    char *pSave1;

    for (char *pPar = STRTOK_R(pars, ",", &pSave1) ; pPar ; pPar = STRTOK_R(NULL, ",", &pSave1)) {
      char *pSave2;
      const char *pKey = STRTOK_R(pPar, "=", &pSave2);
      const char *pVal = STRTOK_R(NULL, "=", &pSave2);

      //Trace("'%s'='%s'\n", pKey, pVal);

      if (!pVal) {
        Trace("Missing value for '%s'\n", pKey);
        return FALSE;
      }

      if (!lstrcmpi(pKey, "PortName")) {
        if (!tmp.SetPortName(pVal))
          return FALSE;
      } else {
        int i;

        for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
          DWORD bit = flagBits[i];
          DWORD *pFlag = GetDwPtr(bit);
          const char *pName = GetBitName(bit);

          if (pFlag == NULL || pName == NULL)
            continue;

          if (!lstrcmpi(pKey, pName)) {
            if (!tmp.SetFlag(pVal, bit))
              return FALSE;
            break;
          }
        }
        if (i >= sizeof(flagBits)/sizeof(flagBits[0])) {
          for (i = 0 ; i < sizeof(pinBits)/sizeof(pinBits[0]) ; i++) {
            DWORD bit = pinBits[i];
            DWORD *pPin = GetDwPtr(bit);
            const char *pName = GetBitName(bit);

            if (pPin == NULL || pName == NULL)
              continue;

            if (!lstrcmpi(pKey, pName)) {
              if (!tmp.SetPin(pVal, bit))
                return FALSE;
              break;
            }
          }

          if (i >= sizeof(pinBits)/sizeof(pinBits[0])) {
            Trace("Invalid parameter '%s'\n", pKey);
            return FALSE;
          }
        }
      }
    }
  }

  *this = tmp;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::FillParametersStr(char *pParameters, int size, BOOL detail)
{
  int len;

  len = SNPRINTF(pParameters, size, "PortName=%s",
                 (maskExplicit & m_portName) ? portName : (detail ? phPortName : "-"));

  if (len < 0)
    return FALSE;

  pParameters += len;
  size -= len;

  int i;

  for (i = 0 ; i < sizeof(flagBits)/sizeof(flagBits[0]) ; i++) {
    DWORD bit = flagBits[i];
    const char *pName = GetBitName(bit);

    if (pName) {
      const DWORD *pFlag;

      if ((maskExplicit & bit) != 0)
        pFlag = GetDwPtr(bit);
      else
      if (detail)
        pFlag = GetDwPtrDefault(bit);
      else
        continue;

      if (pFlag == NULL)
        continue;

      len = SNPRINTF(pParameters, size, ",%s=%s", pName, *pFlag ? "yes" : "no");

      if (len < 0)
        return FALSE;

      pParameters += len;
      size -= len;
    }
  }

  for (i = 0 ; i < sizeof(pinBits)/sizeof(pinBits[0]) ; i++) {
    DWORD bit = pinBits[i];
    const char *pName = GetBitName(bit);

    if (pName) {
      const DWORD *pPin;

      if ((maskExplicit & bit) != 0)
        pPin = GetDwPtr(bit);
      else
      if (detail)
        pPin = GetDwPtrDefault(bit);
      else
        continue;

      if (pPin == NULL)
        continue;

      const char *pVal = NULL;

      switch (*pPin & ~C0C_PIN_NEGATIVE) {
        case C0C_PIN_RRTS:  pVal = "rrts";  break;
        case C0C_PIN_RDTR:  pVal = "rdtr";  break;
        case C0C_PIN_ROUT1: pVal = "rout1"; break;
        case C0C_PIN_ROPEN: pVal = "ropen"; break;
        case C0C_PIN_LRTS:  pVal = "lrts";  break;
        case C0C_PIN_LDTR:  pVal = "ldtr";  break;
        case C0C_PIN_LOUT1: pVal = "lout1"; break;
        case C0C_PIN_LOPEN: pVal = "lopen"; break;
        case C0C_PIN_ON:    pVal = "on";    break;
      }

      if (pVal == NULL)
        continue;

      len = SNPRINTF(pParameters, size, ",%s=%s%s", pName, (*pPin & C0C_PIN_NEGATIVE) == 0 ? "" : "!", pVal);

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
    "  PlugInMode={yes|no}     - enable/disable plug-in mode, the plug-in mode port\n"
    "                            is hidden and can't be open if the paired port is\n"
    "                            not open (disabled by default)\n"
    "  ExclusiveMode={yes|no}  - enable/disable exclusive mode, the exclusive mode\n"
    "                            port is hidden if it is open (disabled by default)\n"
    "  cts=[!]<p>              - wire CTS pin to <p> (rrts by default)\n"
    "  dsr=[!]<p>              - wire DSR pin to <p> (rdtr by default)\n"
    "  dcd=[!]<p>              - wire DCD pin to <p> (rdtr by default)\n"
    "  ri=[!]<p>               - wire RI pin to <p> (!on by default)\n"
    "\n"
    "The possible values of <p> above can be rrts, lrts, rdtr, ldtr, rout1, lout1\n"
    "(remote/local RTS/DTR/OUT1), ropen, lopen (logical ON if remote/local port is\n"
    "open) or on (logical ON). The exclamation sign (!) can be used to invert the\n"
    "value.\n"
    "\n"
    "Special values:\n"
    "  -                       - use driver's default value\n"
    "  *                       - use current setting\n"
    ;
}
///////////////////////////////////////////////////////////////
