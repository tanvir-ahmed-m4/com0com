/*
 * $Id$
 *
 * Copyright (c) 2006-2010 Vyacheslav Frolov
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
 * Revision 1.16  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.15  2008/12/25 16:55:23  vfrolov
 * Added converting portnames to uppercase
 *
 * Revision 1.14  2008/12/02 11:54:28  vfrolov
 * Fixed typo
 *
 * Revision 1.13  2008/09/17 07:58:32  vfrolov
 * Added AddRTTO and AddRITO parameters
 *
 * Revision 1.12  2008/06/26 13:39:19  vfrolov
 * Implemented noise emulation
 *
 * Revision 1.11  2008/05/04 09:53:51  vfrolov
 * Implemented HiddenMode option
 *
 * Revision 1.10  2008/04/08 06:49:44  vfrolov
 * Added pin OUT2
 *
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
  m_portName       = 0x00000001,
  m_emuBR          = 0x00000002,
  m_emuOverrun     = 0x00000004,
  m_plugInMode     = 0x00000008,
  m_exclusiveMode  = 0x00000010,
  m_hiddenMode     = 0x00000020,
  m_pinCTS         = 0x00000100,
  m_pinDSR         = 0x00000200,
  m_pinDCD         = 0x00000400,
  m_pinRI          = 0x00000800,
  m_emuNoise       = 0x00010000,
  m_addRTTO        = 0x00020000,
  m_addRITO        = 0x00040000,
};
///////////////////////////////////////////////////////////////
static struct Bit
{
  DWORD bit;

  enum {
    OTHER,
    FLAG,
    PIN,
    PROBABILITY,
    UNSIGNED,
  } type;
} bits[] = {
  {m_portName,       Bit::OTHER},
  {m_emuBR,          Bit::FLAG},
  {m_emuOverrun,     Bit::FLAG},
  {m_plugInMode,     Bit::FLAG},
  {m_exclusiveMode,  Bit::FLAG},
  {m_hiddenMode,     Bit::FLAG},
  {m_pinCTS,         Bit::PIN},
  {m_pinDSR,         Bit::PIN},
  {m_pinDCD,         Bit::PIN},
  {m_pinRI,          Bit::PIN},
  {m_emuNoise,       Bit::PROBABILITY},
  {m_addRTTO,        Bit::UNSIGNED},
  {m_addRITO,        Bit::UNSIGNED},
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
  portName[0] = 0;
  emuBR = 0;
  emuOverrun = 0;
  plugInMode = 0;
  exclusiveMode = 0;
  hiddenMode = 0;

  pinCTS = 0;
  pinDSR = 0;
  pinDCD = 0;
  pinRI = 0;

  emuNoise = 0;
  addRTTO = 0;
  addRITO = 0;

  maskChanged = 0;
  maskExplicit = 0;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetPortName(const char *pNewPortName)
{
  if (lstrcmpi(portName, pNewPortName)) {
    SNPRINTF(portName, sizeof(portName)/sizeof(portName[0]), "%s", pNewPortName);
    CharUpper(portName);
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
    case m_hiddenMode:     return &hiddenMode;
    case m_pinCTS:         return &pinCTS;
    case m_pinDSR:         return &pinDSR;
    case m_pinDCD:         return &pinDCD;
    case m_pinRI:          return &pinRI;
    case m_emuNoise:       return &emuNoise;
    case m_addRTTO:        return &addRTTO;
    case m_addRITO:        return &addRITO;
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
  static const DWORD hiddenMode = C0C_DEFAULT_HIDDENMODE;
  static const DWORD pinCTS = C0C_DEFAULT_PIN_CTS;
  static const DWORD pinDSR = C0C_DEFAULT_PIN_DSR;
  static const DWORD pinDCD = C0C_DEFAULT_PIN_DCD;
  static const DWORD pinRI = C0C_DEFAULT_PIN_RI;
  static const DWORD emuNoise = C0C_DEFAULT_EMUNOISE;
  static const DWORD addRTTO = C0C_DEFAULT_ADDRTTO;
  static const DWORD addRITO = C0C_DEFAULT_ADDRITO;

  switch (bit) {
    case m_emuBR:          return &emuBR;
    case m_emuOverrun:     return &emuOverrun;
    case m_plugInMode:     return &plugInMode;
    case m_exclusiveMode:  return &exclusiveMode;
    case m_hiddenMode:     return &hiddenMode;
    case m_pinCTS:         return &pinCTS;
    case m_pinDSR:         return &pinDSR;
    case m_pinDCD:         return &pinDCD;
    case m_pinRI:          return &pinRI;
    case m_emuNoise:       return &emuNoise;
    case m_addRTTO:        return &addRTTO;
    case m_addRITO:        return &addRITO;
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
    case m_hiddenMode:     return "HiddenMode";
    case m_pinCTS:         return "cts";
    case m_pinDSR:         return "dsr";
    case m_pinDCD:         return "dcd";
    case m_pinRI:          return "ri";
    case m_emuNoise:       return "EmuNoise";
    case m_addRTTO:        return "AddRTTO";
    case m_addRITO:        return "AddRITO";
  }

  return NULL;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetFlag(const char *pNewVal, DWORD bit)
{
  DWORD newFlag;

  if (!lstrcmpi(pNewVal, "yes")) {
    newFlag = 0xFFFFFFFF;
  }
  else
  if (!lstrcmpi(pNewVal, "no")) {
    newFlag = 0;
  }
  else {
    Trace("Invalid value '%s'\n", pNewVal);
    return FALSE;
  }

  DWORD *pFlag = GetDwPtr(bit);

  if (pFlag == NULL)
    return FALSE;

  if (*pFlag != newFlag) {
    *pFlag = newFlag;
    maskChanged |= bit;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetPin(const char *pNewVal, DWORD bit)
{
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
  if (!lstrcmpi(pNewVal, "rout2")) {
    newPin |= C0C_PIN_ROUT2;
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
  if (!lstrcmpi(pNewVal, "lout2")) {
    newPin |= C0C_PIN_LOUT2;
  }
  else
  if (!lstrcmpi(pNewVal, "lopen")) {
    newPin |= C0C_PIN_LOPEN;
  }
  else
  if (!lstrcmpi(pNewVal, "on")) {
    newPin |= C0C_PIN_ON;
  }
  else {
    Trace("Invalid value '%s'\n", pNewVal);
    return FALSE;
  }

  DWORD *pPin = GetDwPtr(bit);

  if (pPin == NULL)
    return FALSE;

  if (*pPin != newPin) {
    *pPin = newPin;
    maskChanged |= bit;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetProbability(const char *pNewVal, DWORD bit)
{
  DWORD newVal = 0;

  if (pNewVal[0] == '0' && pNewVal[1] == '.') {
    const char *p = pNewVal + 2;

    for (DWORD one = C0C_PROBABILITY_ONE ; one > 1 ; one /= 10) {
      newVal *= 10;

      if (*p == 0)
        continue;

      if (*p < '0' || *p > '9') {
        Trace("Invalid value '%s'\n", pNewVal);
        return FALSE;
      }

      newVal += *p++ - '0';
    }

    while (*p == '0')
      p++;

    if (*p != 0) {
      Trace("Too long value '%s'\n", pNewVal);
      return FALSE;
    }
  }
  else
  if (pNewVal[0] == '0') {
  }
  else {
    Trace("Invalid value '%s'\n", pNewVal);
    return FALSE;
  }

  DWORD *pVal = GetDwPtr(bit);

  if (pVal == NULL)
    return FALSE;

  if (*pVal != newVal) {
    *pVal = newVal;
    maskChanged |= bit;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetUnsigned(const char *pNewVal, DWORD bit)
{
  DWORD newVal = 0;

  for (const char *p = pNewVal ; *p ; p++) {
    if (*p < '0' || *p > '9') {
      Trace("Invalid value '%s'\n", pNewVal);
      return FALSE;
    }
    newVal = newVal*10 + (*p - '0');
  }

  DWORD *pVal = GetDwPtr(bit);

  if (pVal == NULL)
    return FALSE;

  if (*pVal != newVal) {
    *pVal = newVal;
    maskChanged |= bit;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL PortParameters::SetBit(const char *pVal, const Bit &bit)
{
  if (!lstrcmpi(pVal, "*"))
    return TRUE;

  if (!lstrcmpi("-", pVal)) {
    if (maskExplicit & bit.bit) {
      maskExplicit &= ~bit.bit;
      maskChanged |= bit.bit;
    }
    return TRUE;
  }

  if (bit.type == Bit::FLAG) {
    if (!SetFlag(pVal, bit.bit))
      return FALSE;
  }
  else
  if (bit.type == Bit::PIN) {
    if (!SetPin(pVal, bit.bit))
      return FALSE;
  }
  else
  if (bit.type == Bit::PROBABILITY) {
    if (!SetProbability(pVal, bit.bit))
      return FALSE;
  }
  else
  if (bit.type == Bit::UNSIGNED) {
    if (!SetUnsigned(pVal, bit.bit))
      return FALSE;
  }
  else
  if (bit.type == Bit::OTHER) {
    if (bit.bit == m_portName) {
      if (!SetPortName(pVal))
        return FALSE;
    }
    else {
      return FALSE;
    }
  }
  else {
    return FALSE;
  }

  if ((maskExplicit & bit.bit) == 0) {
    maskExplicit |= bit.bit;
    maskChanged |= bit.bit;
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

  for (i = 0 ; i < sizeof(bits)/sizeof(bits[0]) ; i++) {
    if (!GetDwPtr(bits[i].bit))
      continue;

    LoadDw(hKey, bits[i].bit);
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

  for (i = 0 ; i < sizeof(bits)/sizeof(bits[0]) ; i++) {
    if (!GetDwPtr(bits[i].bit))
      continue;

    if (SaveDw(hKey, bits[i].bit) != ERROR_SUCCESS)
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
    int i;

    for (i = 0 ; i < sizeof(bits)/sizeof(bits[0]) ; i++) {
      if (!tmp.SetBit(pParameters, bits[i]))
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

      int i;

      for (i = 0 ; i < sizeof(bits)/sizeof(bits[0]) ; i++) {
        DWORD bit = bits[i].bit;
        const char *pName = GetBitName(bit);

        if (pName == NULL)
          continue;

        if (!lstrcmpi(pKey, pName)) {
          if (!tmp.SetBit(pVal, bits[i]))
            return FALSE;
          break;
        }
      }

      if (i >= sizeof(bits)/sizeof(bits[0])) {
        Trace("Invalid parameter '%s'\n", pKey);
        return FALSE;
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

  for (i = 0 ; i < sizeof(bits)/sizeof(bits[0]) ; i++) {
    DWORD bit = bits[i].bit;

    if (!GetDwPtr(bit))
      continue;

    const char *pName = GetBitName(bit);

    if (pName) {
      const DWORD *pBit;

      if ((maskExplicit & bit) != 0)
        pBit = GetDwPtr(bit);
      else
      if (detail)
        pBit = GetDwPtrDefault(bit);
      else
        continue;

      if (pBit == NULL)
        continue;

      if (bits[i].type == Bit::FLAG) {
        len = SNPRINTF(pParameters, size, ",%s=%s", pName, *pBit ? "yes" : "no");
      }
      else
      if (bits[i].type == Bit::PIN) {
        const char *pVal = NULL;

        switch (*pBit & ~C0C_PIN_NEGATIVE) {
          case C0C_PIN_RRTS:  pVal = "rrts";  break;
          case C0C_PIN_RDTR:  pVal = "rdtr";  break;
          case C0C_PIN_ROUT1: pVal = "rout1"; break;
          case C0C_PIN_ROUT2: pVal = "rout2"; break;
          case C0C_PIN_ROPEN: pVal = "ropen"; break;
          case C0C_PIN_LRTS:  pVal = "lrts";  break;
          case C0C_PIN_LDTR:  pVal = "ldtr";  break;
          case C0C_PIN_LOUT1: pVal = "lout1"; break;
          case C0C_PIN_LOUT2: pVal = "lout2"; break;
          case C0C_PIN_LOPEN: pVal = "lopen"; break;
          case C0C_PIN_ON:    pVal = "on";    break;
        }

        if (pVal == NULL)
          continue;

        len = SNPRINTF(pParameters, size, ",%s=%s%s", pName, (*pBit & C0C_PIN_NEGATIVE) == 0 ? "" : "!", pVal);
      }
      else
      if (bits[i].type == Bit::PROBABILITY) {
          if (*pBit == 0) {
            len = SNPRINTF(pParameters, size, ",%s=0", pName);
          } else {
            char strVal[11] = "";
            char digits[11];

            SNPRINTF(digits, sizeof(digits)/sizeof(digits[0]), "%ld", (unsigned long)*pBit);

            for (int i = lstrlen(digits) ; i ; i--) {
              if (digits[i - 1] > '0')
                break;

              digits[i - 1] = 0;
            }

            const char *p = digits;

            for (DWORD one = C0C_PROBABILITY_ONE/10 ; one > 0 ; one /= 10) {
              if (one > *pBit) {
                lstrcat(strVal, "0");
              }
              else
              if (*p) {
                char sc[2];

                sc[0] = *p++;
                sc[1] = 0;
                lstrcat(strVal, sc);
              }
              else {
                break;
              }
            }

            len = SNPRINTF(pParameters, size, ",%s=0.%s", pName, strVal);
          }
      }
      else
      if (bits[i].type == Bit::UNSIGNED) {
        len = SNPRINTF(pParameters, size, ",%s=%lu", pName, (unsigned long)*pBit);
      }

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
    "  PortName=<portname>     - set port name to <portname>\n"
    "                            (port identifier by default).\n"
    "  EmuBR={yes|no}          - enable/disable baud rate emulation in the direction\n"
    "                            to the paired port (disabled by default)\n"
    "  EmuOverrun={yes|no}     - enable/disable buffer overrun (disabled by default)\n"
    "  EmuNoise=<n>            - probability in range 0-0.99999999 of error per\n"
    "                            character frame in the direction to the paired port\n"
    "                            (0 by default)\n"
    "  AddRTTO=<n>             - add <n> milliseconds to the total time-out period\n"
    "                            for read operations (0 by default)\n"
    "  AddRITO=<n>             - add <n> milliseconds to the maximum time allowed to\n"
    "                            elapse between the arrival of two characters for\n"
    "                            read operations (0 by default)\n"
    "  PlugInMode={yes|no}     - enable/disable plug-in mode, the plug-in mode port\n"
    "                            is hidden and can't be open if the paired port is\n"
    "                            not open (disabled by default)\n"
    "  ExclusiveMode={yes|no}  - enable/disable exclusive mode, the exclusive mode\n"
    "                            port is hidden if it is open (disabled by default)\n"
    "  HiddenMode={yes|no}     - enable/disable hidden mode, the hidden mode port is\n"
    "                            hidden as it is possible for port enumerators\n"
    "                            (disabled by default)\n"
    "  cts=[!]<p>              - wire CTS pin to <p> (rrts by default)\n"
    "  dsr=[!]<p>              - wire DSR pin to <p> (rdtr by default)\n"
    "  dcd=[!]<p>              - wire DCD pin to <p> (rdtr by default)\n"
    "  ri=[!]<p>               - wire RI pin to <p> (!on by default)\n"
    "\n"
    "The possible values of <p> above can be rrts, lrts, rdtr, ldtr, rout1, lout1,\n"
    "rout2, lout2 (remote/local RTS/DTR/OUT1/OUT2), ropen, lopen (logical ON if\n"
    "remote/local port is open) or on (logical ON). The exclamation sign (!) can be\n"
    "used to invert the value.\n"
    "\n"
    "If <portname> above is '" C0C_PORT_NAME_COMCLASS "' then the Ports class installer will be used to\n"
    "manage port name. The Ports class installer selects the COM port number and\n"
    "sets the port name to COM<n>, where <n> is the selected port number.\n"
    "\n"
    "Special values:\n"
    "  -                       - use driver's default value\n"
    "  *                       - use current setting\n"
    ;
}
///////////////////////////////////////////////////////////////
