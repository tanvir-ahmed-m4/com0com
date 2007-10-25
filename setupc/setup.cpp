/*
 * $Id$
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * Revision 1.1  2007/10/25 14:25:34  vfrolov
 * Initial revision
 *
 *
 */

#include <windows.h>

int CALLBACK MainA(const char *pProgName, const char *pCmdLine);

int _cdecl main(int argc, char *argv[])
{
  char cmd[200] = "";

  for (int i = 1 ; i < argc ; i++) {
    int lenCmd = lstrlen(cmd);

    if (sizeof(cmd)/sizeof(cmd[0]) < lenCmd + 1 + lstrlen(argv[i]) + 1)
      break;

    lstrcpy(cmd + lenCmd, " ");
    lenCmd++;
    lstrcpy(cmd + lenCmd, argv[i]);
  }

  return MainA(argv[0], cmd);
}
