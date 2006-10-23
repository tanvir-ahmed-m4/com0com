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
 * Revision 1.3  2006/10/23 12:04:23  vfrolov
 * Added SetTitle()
 *
 * Revision 1.2  2006/10/17 10:03:59  vfrolov
 * Added MB_SETFOREGROUND flag to MessageBox()
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "msg.h"
#include "utils.h"

char title[80] = "";

///////////////////////////////////////////////////////////////
static int ShowMsgDefault(LPCSTR pText, UINT type)
{
  return MessageBox(NULL, pText, title, type|MB_SETFOREGROUND);
}

static int (* pShowMsg)(LPCSTR pText, UINT type) = ShowMsgDefault;
///////////////////////////////////////////////////////////////
static void ConsoleWriteReadDefault(LPSTR pReadBuf, DWORD lenReadBuf, LPCSTR pText)
{
  static HANDLE handle = INVALID_HANDLE_VALUE;

  if (handle == INVALID_HANDLE_VALUE) {
    AllocConsole();
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitle(title);
  }

  if (pText)
    WriteConsole(handle, pText, lstrlen(pText), NULL, NULL);

  if (pReadBuf && lenReadBuf > 0) {
    if (lenReadBuf > 1 &&
        ReadConsole(GetStdHandle(STD_INPUT_HANDLE), pReadBuf, lenReadBuf - 1, &lenReadBuf, 0))
    {
      pReadBuf[lenReadBuf] = 0;
    } else {
      pReadBuf[0] = 0;
    }
  }
}

static void (* pConsole)(LPSTR pReadBuf, DWORD lenReadBuf, LPCSTR pText) = ConsoleWriteReadDefault;
///////////////////////////////////////////////////////////////
static void TraceDefault(LPCSTR pText)
{
  pConsole(NULL, 0, pText);
}

static void (* pTrace)(LPCSTR pText) = TraceDefault;
///////////////////////////////////////////////////////////////
static int ShowMsgVA(UINT type, const char *pFmt, va_list va)
{
  char buf[1024];

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  return pShowMsg(buf, type);
}
///////////////////////////////////////////////////////////////
static int ShowErrorVA(UINT type, DWORD err, const char *pFmt, va_list va)
{
  char buf[1024];

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  LPVOID pMsgBuf;

  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      err,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
      (LPTSTR) &pMsgBuf,
      0,
      NULL);

  int len = lstrlen(buf);

  SNPRINTF(buf + len, sizeof(buf)/sizeof(buf[0]) - len, "\nERROR: 0x%08lX - %s\n", (unsigned long)err, pMsgBuf);

  LocalFree(pMsgBuf);

  return pShowMsg(buf, type);
}
///////////////////////////////////////////////////////////////
int ShowMsg(UINT type, const char *pFmt, ...)
{
  int res;
  va_list va;

  va_start(va, pFmt);
  res = ShowMsgVA(type, pFmt, va);
  va_end(va);

  return res;
}
///////////////////////////////////////////////////////////////
int ShowError(UINT type, DWORD err, const char *pFmt, ...)
{
  int res;
  va_list va;

  va_start(va, pFmt);
  res = ShowErrorVA(type, err, pFmt, va);
  va_end(va);

  return res;
}
///////////////////////////////////////////////////////////////
int ShowLastError(UINT type, const char *pFmt, ...)
{
  int res;

  DWORD err = GetLastError();

  va_list va;

  va_start(va, pFmt);
  res = ShowErrorVA(type, err, pFmt, va);
  va_end(va);

  SetLastError(err);

  return res;
}
///////////////////////////////////////////////////////////////
void Trace(const char *pFmt, ...)
{
  char buf[1024];
  va_list va;

  va_start(va, pFmt);

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  va_end(va);

  pTrace(buf);
}
///////////////////////////////////////////////////////////////
void ConsoleWriteRead(char *pReadBuf, int lenReadBuf, const char *pFmt, ...)
{
  char buf[1024];
  va_list va;

  va_start(va, pFmt);

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  va_end(va);

  pConsole(pReadBuf, lenReadBuf, buf);
}
///////////////////////////////////////////////////////////////
void SetTitle(const char *pTitle)
{
  lstrcpyn(title, pTitle, sizeof(title)/sizeof(title[0]));
}
///////////////////////////////////////////////////////////////
