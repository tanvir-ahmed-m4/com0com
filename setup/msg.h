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
 * Revision 1.4  2007/09/20 12:29:03  vfrolov
 * Added return value to SetOutputFile()
 *
 * Revision 1.3  2006/11/21 11:34:55  vfrolov
 * Added
 *   ConsoleWrite()
 *   IsConsoleOpen()
 *   SetOutputFile()
 *   GetOutputFile()
 *
 * Revision 1.2  2006/10/23 12:04:23  vfrolov
 * Added SetTitle()
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 */

#ifndef _C0C_MSG_H_
#define _C0C_MSG_H_

int ShowMsg(UINT type, const char *pFmt, ...);
int ShowError(UINT type, DWORD err, const char *pFmt, ...);
int ShowLastError(UINT type, const char *pFmt, ...);
void Trace(const char *pFmt, ...);
void ConsoleWriteRead(char *pReadBuf, int lenReadBuf, const char *pFmt, ...);
void ConsoleWrite(const char *pFmt, ...);
BOOL IsConsoleOpen();
void SetTitle(const char *pTitle);
BOOL SetOutputFile(const char *pFile);
const char *GetOutputFile();

#endif /* _C0C_MSG_H_ */
