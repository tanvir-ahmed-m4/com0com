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
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_PARAMS_H_
#define _C0C_PARAMS_H_

class PortParameters {
  public:
    PortParameters(const char *pService, const char *pPhPortName);

    void Init();
    LONG Load();
    LONG Save();
    BOOL ParseParametersStr(const char *pParameters);
    BOOL FillParametersStr(char *pParameters, int size);
    BOOL Changed() const { return maskChanged != 0; }

    static const char *PortParameters::GetHelp();

  protected:
    BOOL FillParametersKey(char *pRegKey, int size);
    BOOL SetPortName(const char *pNewPortName);
    BOOL SetEmuBR(const char *pNewEmuBR);
    BOOL SetEmuOverrun(const char *pNewEmuOverrun);

    DWORD maskChanged;
    DWORD maskExplicit;
    char portName[20];
    DWORD emuBR;
    DWORD emuOverrun;

    char service[20];
    char phPortName[20];
};

#endif /* _C0C_PARAMS_H_ */
