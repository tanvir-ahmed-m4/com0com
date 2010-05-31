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
 * Revision 1.9  2010/05/31 07:58:14  vfrolov
 * Added ability to invoke the system-supplied advanced settings dialog box
 *
 * Revision 1.8  2008/09/17 07:58:32  vfrolov
 * Added AddRTTO and AddRITO parameters
 *
 * Revision 1.7  2008/06/26 13:39:19  vfrolov
 * Implemented noise emulation
 *
 * Revision 1.6  2008/05/04 09:53:51  vfrolov
 * Implemented HiddenMode option
 *
 * Revision 1.5  2007/10/19 16:09:55  vfrolov
 * Implemented --detail-prms option
 *
 * Revision 1.4  2007/07/03 14:39:49  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.3  2007/06/01 16:32:04  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.2  2006/10/27 13:11:58  vfrolov
 * Added PortParameters::FillPortName()
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 */

#ifndef _C0C_PARAMS_H_
#define _C0C_PARAMS_H_

struct Bit;

class PortParameters {
  public:
    PortParameters(const char *pService, const char *pPhPortName);

    void Init();
    LONG Load();
    LONG Save();
    BOOL ParseParametersStr(const char *pParameters);
    BOOL FillParametersStr(char *pParameters, int size, BOOL detail);
    BOOL FillPortName(char *pPortName, int size);
    BOOL Changed() const { return maskChanged != 0; }
    BOOL ClassChanged() const { return classChanged; }
    BOOL DialogRequested() const { return dialogRequested; }

    static const char *PortParameters::GetHelp();

  protected:
    BOOL FillParametersKey(char *pRegKey, int size);
    DWORD *GetDwPtr(DWORD bit);
    void LoadDw(HKEY hKey, DWORD bit);
    LONG SaveDw(HKEY hKey, DWORD bit);

    BOOL SetPortName(const char *pNewPortName);
    BOOL SetFlag(const char *pNewVal, DWORD bit);
    BOOL SetPin(const char *pNewVal, DWORD bit);
    BOOL SetProbability(const char *pNewVal, DWORD bit);
    BOOL SetUnsigned(const char *pNewVal, DWORD bit);
    BOOL SetBit(const char *pVal, const Bit &bit);

    BOOL classChanged;
    BOOL dialogRequested;
    DWORD maskChanged;
    DWORD maskExplicit;
    char portName[20];
    DWORD emuBR;
    DWORD emuOverrun;
    DWORD plugInMode;
    DWORD exclusiveMode;
    DWORD hiddenMode;
    DWORD pinCTS;
    DWORD pinDSR;
    DWORD pinDCD;
    DWORD pinRI;
    DWORD emuNoise;
    DWORD addRTTO;
    DWORD addRITO;

    char service[20];
    char phPortName[20];
};

#endif /* _C0C_PARAMS_H_ */
