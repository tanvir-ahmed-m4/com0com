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
 * Revision 1.5  2007/05/29 15:22:00  vfrolov
 * Fixed buffer overflow
 *
 * Revision 1.4  2007/01/11 15:03:43  vfrolov
 * Added STRTOK_R()
 *
 * Revision 1.3  2006/11/03 13:17:28  vfrolov
 * Fixed LocalReAlloc() usage
 * Added return value to BusyMask::AddNum()
 *
 * Revision 1.2  2006/11/02 16:09:13  vfrolov
 * Added StrToInt() and class BusyMask
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "utils.h"

///////////////////////////////////////////////////////////////
int VSNPRINTF(char *pBuf, int size, const char *pFmt, va_list va)
{
  char buf[1025];

  int res1 = wvsprintf(buf, pFmt, va);
  buf[sizeof(buf)/sizeof(buf[0]) - 1] = 0;

  lstrcpyn(pBuf, buf, size);

  int res2 = lstrlen(pBuf);

  return res2 == res1 ? res1 : -1;
}
///////////////////////////////////////////////////////////////
int SNPRINTF(char *pBuf, int size, const char *pFmt, ...)
{
  va_list va;

  va_start(va, pFmt);

  int res1 = VSNPRINTF(pBuf, size, pFmt, va);

  va_end(va);

  return res1;
}
///////////////////////////////////////////////////////////////
static BOOL IsDelim(char c, const char *pDelims)
{
  while (*pDelims) {
    if (c == *pDelims++)
      return TRUE;
  }

  return FALSE;
}

char *STRTOK_R(char *pStr, const char *pDelims, char **ppSave)
{
  if (!pStr)
    pStr = *ppSave;

  while (IsDelim(*pStr, pDelims))
    pStr++;

  if (!*pStr) {
    *ppSave = pStr;
    return NULL;
  }

  char *pToken = pStr;

  while (*pStr && !IsDelim(*pStr, pDelims))
    pStr++;

  if (*pStr)
    *pStr++ = 0;

  *ppSave = pStr;

  return pToken;
}
///////////////////////////////////////////////////////////////
BOOL StrToInt(const char *pStr, int *pNum)
{
  BOOL res = FALSE;
  int num;
  int sign = 1;

  switch (*pStr) {
    case '-':
      sign = -1;
    case '+':
      pStr++;
      break;
  }

  for (num = 0 ;; pStr++) {
    switch (*pStr) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        num = num*10 + (*pStr - '0');
        res = TRUE;
        continue;
      case 0:
        break;
      default:
        res = FALSE;
    }
    break;
  }

  if (pNum)
    *pNum = num*sign;

  return res;
}
///////////////////////////////////////////////////////////////
BusyMask::~BusyMask()
{
  if (pBusyMask)
    LocalFree(pBusyMask);
}

BOOL BusyMask::AddNum(int num)
{
  ULONG maskNum = num/(sizeof(*pBusyMask)*8);

  if (maskNum >= busyMaskLen) {
    SIZE_T newBusyMaskLen = maskNum + 1;
    PBYTE pNewBusyMask;

    if (!pBusyMask)
      pNewBusyMask = (PBYTE)LocalAlloc(LPTR, newBusyMaskLen);
    else
      pNewBusyMask = (PBYTE)LocalReAlloc(pBusyMask, newBusyMaskLen, LMEM_ZEROINIT|LMEM_MOVEABLE);

    if (pNewBusyMask) {
      pBusyMask = pNewBusyMask;
      busyMaskLen = newBusyMaskLen;
    } else {
      return FALSE;
    }
  }

  ULONG mask = 1 << (num%(sizeof(*pBusyMask)*8));

  pBusyMask[maskNum] |= mask;

  return TRUE;
}

BOOL BusyMask::IsFreeNum(int num) const
{
  ULONG maskNum = num/(sizeof(*pBusyMask)*8);

  if (maskNum >= busyMaskLen)
    return TRUE;

  ULONG mask = 1 << (num%(sizeof(*pBusyMask)*8));

  return (pBusyMask[maskNum] & mask) == 0;
}

int BusyMask::GetFirstFreeNum() const
{
  int num;

  for (num = 0 ; !IsFreeNum(num) ; num++)
    ;

  return num;
}
///////////////////////////////////////////////////////////////
