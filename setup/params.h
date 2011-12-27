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
 * Revision 1.12  2011/12/27 11:38:13  vfrolov
 * Superseded incorrect PortParameters::ClassChanged()
 *
 * Revision 1.11  2011/12/15 15:51:48  vfrolov
 * Fixed types
 *
 * Revision 1.10  2011/12/06 16:03:22  vfrolov
 * Added cleaning high data bits for less then 8 bit data
 * Added AllDataBits option to force 8 bit data
 *
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
    bool ParseParametersStr(const char *pParameters);
    bool FillParametersStr(char *pParameters, int size, bool detail);
    bool FillPortName(char *pPortName, int size);
    bool Changed() const { return maskChanged != 0; }
    bool DialogRequested() const { return dialogRequested; }

    static const char *PortParameters::GetHelp();

  protected:
    bool FillParametersKey(char *pRegKey, int size);
    DWORD *GetDwPtr(DWORD bit);
    void LoadDw(HKEY hKey, DWORD bit);
    LONG SaveDw(HKEY hKey, DWORD bit);

    bool SetPortName(const char *pNewPortName);
    bool SetFlag(const char *pNewVal, DWORD bit);
    bool SetPin(const char *pNewVal, DWORD bit);
    bool SetProbability(const char *pNewVal, DWORD bit);
    bool SetUnsigned(const char *pNewVal, DWORD bit);
    bool SetBit(const char *pVal, const Bit &bit);

    bool dialogRequested;
    DWORD maskChanged;
    DWORD maskExplicit;
    char portName[20];
    DWORD emuBR;
    DWORD emuOverrun;
    DWORD plugInMode;
    DWORD exclusiveMode;
    DWORD hiddenMode;
    DWORD allDataBits;
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
