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
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_INFFILE_H_
#define _C0C_INFFILE_H_

class InfFile {
  public:
    InfFile(const char *pInfName, const char *pNearPath);
    ~InfFile();

    const char *Path() const { return pPath; }
    const char *ClassGUID() const;
    const char *Class() const;
    const char *Provider() const;

    BOOL Compare(
        const char *_pClassGUID,
        const char *_pClass,
        const char *_pProvider) const;

    BOOL UninstallFiles(const char *pFilesSection) const;

    BOOL InstallOEMInf() const;
    BOOL UninstallOEMInf() const;
  protected:
    char *pPath;
    char *pClassGUID;
    char *pClass;
    char *pProvider;
};

#endif /* _C0C_PARAMS_H_ */
