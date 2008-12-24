/*
 * $Id$
 *
 * Copyright (c) 2008 Vyacheslav Frolov
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
 * Revision 1.1  2008/12/24 15:20:35  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "params.h"
#include "devutils.h"
#include "msg.h"
#include "utils.h"
#include "portnum.h"
#include "comdb.h"
#include <msports.h>

#define TEXT_PREF
#include "../include/com0com.h"
///////////////////////////////////////////////////////////////
static const char comDbLocalKey[] = REGSTR_PATH_SERVICES "\\" C0C_SERVICE "\\COM Name Arbiter";
static const char comDbLocalName[] = "ComDB";
///////////////////////////////////////////////////////////////
static WORD name2num(const char *pPortName)
{
  int num;

  if ((pPortName[0] != 'C' && pPortName[0] != 'c') ||
      (pPortName[1] != 'O' && pPortName[1] != 'o') ||
      (pPortName[2] != 'M' && pPortName[2] != 'm') ||
      !StrToInt(pPortName + 3, &num) ||
      num <= 0 ||
      num > COMDB_MAX_PORTS_ARBITRATED)
  {
    return 0;
  }

  return (WORD)num;
}
///////////////////////////////////////////////////////////////
static BOOL LoadComDb(BusyMask &comDb)
{
  comDb.Clear();

  int res;

  do {
    res = IDCONTINUE;

    HCOMDB  hComDB;
    LONG err;

    err = ComDBOpen(&hComDB);

    if (err != ERROR_SUCCESS) {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "ComDBOpen()");
      continue;
    }

    DWORD maxPortsReported;

    err = ComDBGetCurrentPortUsage(hComDB, NULL, 0, CDB_REPORT_BITS, &maxPortsReported);

    if (err != ERROR_SUCCESS) {
      ComDBClose(hComDB);
      res = ShowError(MB_CANCELTRYCONTINUE, err, "ComDBGetCurrentPortUsage()");
      continue;
    }

    DWORD bufSize = (maxPortsReported + 7)/8;
    BYTE *pBuf = (BYTE *)LocalAlloc(LPTR, bufSize);

    if (!pBuf) {
      ComDBClose(hComDB);

      res = ShowError(MB_CANCELTRYCONTINUE, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)bufSize);
      continue;
    }

    err = ComDBGetCurrentPortUsage(hComDB, pBuf, bufSize, CDB_REPORT_BITS, &maxPortsReported);
    ComDBClose(hComDB);

    if (err != ERROR_SUCCESS) {
      LocalFree(pBuf);
      res = ShowError(MB_CANCELTRYCONTINUE, err, "ComDBGetCurrentPortUsage()");
      continue;
    }

    for (DWORD num = 0 ; num < maxPortsReported ; num++) {
      if (((pBuf[num/8] >> (num%8)) & 1) != 0)
        comDb.AddNum(num);
    }

    LocalFree(pBuf);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL ClaimReleasePort(DWORD num, BOOL claim)
{
  int res;

  do {
    res = IDCONTINUE;

    HCOMDB  hComDB;
    LONG err;

    err = ComDBOpen(&hComDB);

    if (err != ERROR_SUCCESS) {
      res = ShowLastError(MB_RETRYCANCEL, "ComDBOpen()");
      continue;
    }

    if (claim) {
      err = ComDBClaimPort(hComDB, num, FALSE, NULL);

      if (err != ERROR_SUCCESS) {
        ComDBClose(hComDB);

        if (err == ERROR_SHARING_VIOLATION)
          res = IDCANCEL;
        else
          res = ShowError(MB_RETRYCANCEL, err, "ComDBClaimPort(COM%u)", (unsigned)num);

        continue;
      }
    } else {
      err = ComDBReleasePort(hComDB, num);

      if (err != ERROR_SUCCESS) {
        ComDBClose(hComDB);

        res = ShowError(MB_RETRYCANCEL, err, "ComDBReleasePort(COM%u)", (unsigned)num);
        continue;
      }
    }

    ComDBClose(hComDB);
  } while (res == IDRETRY);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL LoadComDbLocal(BusyMask &comDb)
{
  comDb.Clear();

  int res;

  do {
    res = IDCONTINUE;

    LONG err;
    HKEY hKey;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       comDbLocalKey,
                       0,
                       KEY_READ,
                       &hKey);

    if (err != ERROR_SUCCESS) {
      if (err != ERROR_FILE_NOT_FOUND)
        res = ShowError(MB_CANCELTRYCONTINUE, err, "RegOpenKeyEx(%s)", comDbLocalKey);

      continue;
    }

    DWORD bufSize = 1;

    err = RegQueryValueEx(hKey,
                          comDbLocalName,
                          NULL,
                          NULL,
                          NULL,
                          &bufSize);

    if (err != ERROR_SUCCESS) {
      RegCloseKey(hKey);

      if (err != ERROR_FILE_NOT_FOUND)
        res = ShowError(MB_CANCELTRYCONTINUE, err, "RegQueryValueEx(%s\\%s)", comDbLocalKey, comDbLocalName);

      continue;
    }

    BYTE *pBuf = (BYTE *)LocalAlloc(LPTR, bufSize);

    if (!pBuf) {
      RegCloseKey(hKey);

      res = ShowError(MB_CANCELTRYCONTINUE, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)bufSize);
      continue;
    }

    err = RegQueryValueEx(hKey,
                          comDbLocalName,
                          NULL,
                          NULL,
                          pBuf,
                          &bufSize);

    RegCloseKey(hKey);

    if (err != ERROR_SUCCESS) {
      LocalFree(pBuf);

      if (err != ERROR_FILE_NOT_FOUND)
        res = ShowError(MB_CANCELTRYCONTINUE, err, "RegQueryValueEx(%s\\%s)", comDbLocalKey, comDbLocalName);

      continue;
    }

    DWORD maxPortsReported = bufSize*8;

    for (DWORD num = 0 ; num < maxPortsReported ; num++) {
      if (((pBuf[num/8] >> (num%8)) & 1) != 0)
        comDb.AddNum(num);
    }

    LocalFree(pBuf);

  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL AddComNames(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    PCDevProperties /*pDevProperties*/,
    BOOL * /*pRebootRequired*/,
    void *pParam)
{
  int i = GetPortNum(hDevInfo, pDevInfoData);

  if (i < 0)
    return TRUE;

  for (int j = 0 ; j < 2 ; j++) {
    char phPortName[20];

    SNPRINTF(phPortName, sizeof(phPortName)/sizeof(phPortName[0]), "%s%d",
             j ? C0C_PREF_PORT_NAME_B : C0C_PREF_PORT_NAME_A, i);

    PortParameters portParameters(C0C_SERVICE, phPortName);

    if (portParameters.Load() != ERROR_SUCCESS)
      return FALSE;

    char portName[20];

    portParameters.FillPortName(portName, sizeof(portName)/sizeof(portName[0]));

    WORD num = name2num(portName);

    if (num > 0)
      ((BusyMask *)pParam)->AddNum(num - 1);
  }

  return TRUE;
}

static BOOL LoadComNames(InfFile &infFile, BusyMask &comDb)
{
  comDb.Clear();

  DevProperties devProperties;

  if (!devProperties.DevId(C0C_BUS_DEVICE_ID))
    return FALSE;

  if (EnumDevices(infFile, &devProperties, NULL, AddComNames, &comDb) < 0)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL SaveComDbLocal(const BusyMask &comDb)
{
  int res;

  do {
    res = IDCONTINUE;

    LONG err;
    HKEY hKey;

    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         comDbLocalKey,
                         0,
                         NULL,
                         0,
                         KEY_WRITE,
                         NULL,
                         &hKey,
                         NULL);

    if (err != ERROR_SUCCESS) {
      res = ShowError(MB_CANCELTRYCONTINUE, err, "RegCreateKeyEx(%s)", comDbLocalKey);
      continue;
    }

    DWORD maxPortsReported = 0;

    for (DWORD num = 0 ; num < COMDB_MAX_PORTS_ARBITRATED ; num++) {
      if (!comDb.IsFreeNum(num))
        maxPortsReported = num + 1;
    }

    if (!maxPortsReported) {
      err = RegDeleteValue(hKey, comDbLocalName);

      RegCloseKey(hKey);

      if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
        res = ShowError(MB_CANCELTRYCONTINUE, err, "RegDeleteValue(%s\\%s)", comDbLocalKey, comDbLocalName);

      continue;
    }

    DWORD bufSize = (maxPortsReported + 7)/8;
    BYTE *pBuf = (BYTE *)LocalAlloc(LPTR, bufSize);

    if (!pBuf) {
      RegCloseKey(hKey);

      res = ShowError(MB_CANCELTRYCONTINUE, ERROR_NOT_ENOUGH_MEMORY, "LocalAlloc(%lu)", (unsigned long)bufSize);
      continue;
    }

    for (DWORD num = 0 ; num < maxPortsReported ; num++) {
      if (!comDb.IsFreeNum(num))
        pBuf[num/8] |= (1 << (num%8));
    }

    err = RegSetValueEx(hKey,
                        comDbLocalName,
                        NULL,
                        REG_BINARY,
                        pBuf,
                        bufSize);

    RegCloseKey(hKey);
    LocalFree(pBuf);

    if (err != ERROR_SUCCESS) {
      res = ShowError(MB_CANCELTRYCONTINUE, err, "RegSetValueEx(%s\\%s)", comDbLocalKey, comDbLocalName);
      continue;
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL ComDbGetInUse(const char *pPortName, BOOL &inUse)
{
  WORD num = name2num(pPortName);

  if (num == 0) {
    inUse = FALSE;  // not arbitered by ComDB
  } else {
    BusyMask comDb;

    if (!LoadComDb(comDb))
      return FALSE;

    inUse = !comDb.IsFreeNum(num - 1);
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
void ComDbSync(InfFile &infFile)
{
  BusyMask comNames;

  if (!LoadComNames(infFile, comNames))
    return;

  BusyMask comDbLocal;

  if (!LoadComDbLocal(comDbLocal))
    return;

  for (DWORD num = 0 ; num < COMDB_MAX_PORTS_ARBITRATED ; num++) {
    if (comNames.IsFreeNum(num)) {
      if (!comDbLocal.IsFreeNum(num)) {
        if (ClaimReleasePort(num + 1, FALSE)) {
          Trace("ComDB: COM%u - released\n", unsigned(num + 1));
          comDbLocal.DelNum(num);
          SaveComDbLocal(comDbLocal);
        }
      }
    } else {
      if (comDbLocal.IsFreeNum(num)) {
        if (ClaimReleasePort(num + 1, TRUE)) {
          Trace("ComDB: COM%u - logged as \"in use\"\n", unsigned(num + 1));
          comDbLocal.AddNum(num);
          SaveComDbLocal(comDbLocal);
        }
      }
    }
  }
}
///////////////////////////////////////////////////////////////
