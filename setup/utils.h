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
 * Revision 1.2  2006/11/02 16:09:13  vfrolov
 * Added StrToInt() and class BusyMask
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_UTILS_H_
#define _C0C_UTILS_H_

int VSNPRINTF(char *pBuf, int size, const char *pFmt, va_list va);
int SNPRINTF(char *pBuf, int size, const char *pFmt, ...);
BOOL StrToInt(const char *pStr, int *pNum);

class BusyMask {
public:
  BusyMask() : pBusyMask(NULL), busyMaskLen(0) {}
  ~BusyMask();

  void AddNum(int num);
  BOOL IsFreeNum(int num) const;
  int GetFirstFreeNum() const;
private:
  PBYTE pBusyMask;
  SIZE_T busyMaskLen;
};

#endif /* _C0C_UTILS_H_ */
