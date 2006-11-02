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
 * Revision 1.1  2006/11/02 16:07:02  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "portnum.h"

#define TEXT_PREF
#include "../include/com0com.h"

///////////////////////////////////////////////////////////////
int GetPortNum(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData)
{
  HKEY hKey;

  hKey = SetupDiOpenDevRegKey(hDevInfo, pDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

  if (hKey == INVALID_HANDLE_VALUE)
    return -1;

  int num;
  DWORD len;
  DWORD portNum;

  len = sizeof(portNum);

  if (RegQueryValueEx(hKey, C0C_REGSTR_VAL_PORT_NUM, NULL, NULL, (PBYTE)&portNum, &len) == ERROR_SUCCESS)
    num = portNum;
  else
    num = -1;

  RegCloseKey(hKey);

  return num;
}
///////////////////////////////////////////////////////////////
LONG SetPortNum(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    int num)
{
  HKEY hKey;

  hKey = SetupDiCreateDevRegKey(hDevInfo, pDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);

  if (hKey == INVALID_HANDLE_VALUE)
    return GetLastError();

  DWORD portNum = num;

  LONG err = RegSetValueEx(hKey, C0C_REGSTR_VAL_PORT_NUM, NULL, REG_DWORD, (PBYTE)&portNum, sizeof(portNum));

  RegCloseKey(hKey);

  return err;
}
///////////////////////////////////////////////////////////////
