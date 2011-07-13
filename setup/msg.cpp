/*
 * $Id$
 *
 * Copyright (c) 2006-2011 Vyacheslav Frolov
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
 * Revision 1.10  2011/07/13 17:42:46  vfrolov
 * Added tracing of dialogs
 *
 * Revision 1.9  2010/07/30 09:19:29  vfrolov
 * Added STRDUP()
 *
 * Revision 1.8  2009/11/09 11:16:43  vfrolov
 * Added restoring last error
 *
 * Revision 1.7  2009/02/16 10:32:56  vfrolov
 * Added Silent() and PromptReboot()
 *
 * Revision 1.6  2007/10/19 16:11:56  vfrolov
 * Added ability to redirect console output
 *
 * Revision 1.5  2007/09/20 12:29:03  vfrolov
 * Added return value to SetOutputFile()
 *
 * Revision 1.4  2006/11/21 11:34:55  vfrolov
 * Added
 *   ConsoleWrite()
 *   IsConsoleOpen()
 *   SetOutputFile()
 *   GetOutputFile()
 *
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

static char *pOutputFile = NULL;
static char title[80] = "";
static BOOL silent = FALSE;

///////////////////////////////////////////////////////////////
static int ShowMsgDefault(LPCSTR pText, UINT type)
{
  return MessageBox(NULL, pText, title, type|MB_SETFOREGROUND);
}

static int (* pShowMsg)(LPCSTR pText, UINT type) = ShowMsgDefault;
///////////////////////////////////////////////////////////////
static BOOL isConsoleOpen = FALSE;

static void ConsoleWriteReadDefault(LPSTR pReadBuf, DWORD lenReadBuf, LPCSTR pText)
{
  static HANDLE handle = INVALID_HANDLE_VALUE;

  if (handle == INVALID_HANDLE_VALUE) {
    AllocConsole();
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitle(title);
    isConsoleOpen = TRUE;
  }

  if (pText) {
    DWORD cnt;

    if (GetFileType(handle) == FILE_TYPE_CHAR)
      WriteConsole(handle, pText, lstrlen(pText), &cnt, NULL);
    else
      WriteFile(handle, pText, lstrlen(pText), &cnt, NULL);
  }

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

  if (!pOutputFile || !pText)
    return;

  HANDLE hFile = CreateFile(
                   pOutputFile,
                   GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);

  if (hFile != INVALID_HANDLE_VALUE) {
    SetFilePointer(hFile, 0, NULL, FILE_END);

    LPCSTR p;

    for (p = pText ; *p ; p++) {
      DWORD not_used;

      if (*p == '\n')
        WriteFile(hFile, "\r", sizeof(*p), &not_used, NULL);
      WriteFile(hFile, p, sizeof(*p), &not_used, NULL);
    }

    CloseHandle(hFile);
  }
}

static void (* pTrace)(LPCSTR pText) = TraceDefault;
///////////////////////////////////////////////////////////////
static int ShowMsg(LPCSTR pText, UINT type)
{
  Trace("\nDIALOG: {\n%s} ... ", pText);

#define TRACECASE(p, s) case p##s: Trace(#s); break;

  int res = pShowMsg(pText, type);

  switch(res) {
    TRACECASE(ID, OK)
    TRACECASE(ID, CANCEL)
    TRACECASE(ID, ABORT)
    TRACECASE(ID, RETRY)
    TRACECASE(ID, IGNORE)
    TRACECASE(ID, YES)
    TRACECASE(ID, NO)
    TRACECASE(ID, CLOSE)
    TRACECASE(ID, HELP)
    TRACECASE(ID, TRYAGAIN)
    TRACECASE(ID, CONTINUE)
    case 0:
      Trace("error");
      break;
    default:
      Trace("%d", res);
  }

  Trace("\n");

  return res;
}
///////////////////////////////////////////////////////////////
static int ShowMsgVA(UINT type, const char *pFmt, va_list va)
{
  char buf[1024];

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  return ShowMsg(buf, type);
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

  if ((err & 0xFFFF0000) == 0)
    SNPRINTF(buf + len, sizeof(buf)/sizeof(buf[0]) - len, "\nERROR: %lu - %s\n", (unsigned long)err, pMsgBuf);
  else
    SNPRINTF(buf + len, sizeof(buf)/sizeof(buf[0]) - len, "\nERROR: 0x%08lX - %s\n", (unsigned long)err, pMsgBuf);

  LocalFree(pMsgBuf);

  return ShowMsg(buf, type);
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

  DWORD err = GetLastError();

  pTrace(buf);

  SetLastError(err);
}
///////////////////////////////////////////////////////////////
void ConsoleWriteRead(char *pReadBuf, int lenReadBuf, const char *pFmt, ...)
{
  char buf[1024];
  va_list va;

  va_start(va, pFmt);

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  va_end(va);

  DWORD err = GetLastError();

  pConsole(pReadBuf, lenReadBuf, buf);

  SetLastError(err);
}
///////////////////////////////////////////////////////////////
void ConsoleWrite(const char *pFmt, ...)
{
  char buf[1024];
  va_list va;

  va_start(va, pFmt);

  VSNPRINTF(buf, sizeof(buf)/sizeof(buf[0]), pFmt, va);

  va_end(va);

  DWORD err = GetLastError();

  pConsole(NULL, 0, buf);

  SetLastError(err);
}
///////////////////////////////////////////////////////////////
BOOL IsConsoleOpen()
{
  return isConsoleOpen;
}
///////////////////////////////////////////////////////////////
void SetTitle(const char *pTitle)
{
  SNPRINTF(title, sizeof(title)/sizeof(title[0]), "%s", pTitle);
}
///////////////////////////////////////////////////////////////
BOOL SetOutputFile(const char *pFile)
{
  if (pOutputFile) {
    LocalFree(pOutputFile);
    pOutputFile = NULL;
  }

  if (pFile) {
    pOutputFile = STRDUP(pFile);

    if (!pOutputFile)
      return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
const char *GetOutputFile()
{
  return pOutputFile;
}
///////////////////////////////////////////////////////////////
BOOL Silent()
{
  return silent;
}
///////////////////////////////////////////////////////////////
void Silent(BOOL val)
{
  silent = val;
}
///////////////////////////////////////////////////////////////
void PromptReboot()
{
  Trace("\nReboot required.\n");

  if (!silent)
    SetupPromptReboot(NULL, NULL, FALSE);
}
///////////////////////////////////////////////////////////////
