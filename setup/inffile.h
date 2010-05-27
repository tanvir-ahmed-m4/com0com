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
 * Revision 1.3  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.2  2006/10/19 13:28:50  vfrolov
 * Added InfFile::UninstallAllInfFiles()
 *
 * Revision 1.1  2006/07/28 12:16:42  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_INFFILE_H_
#define _C0C_INFFILE_H_

class InfFile {
  public:
    struct InfFileField {
      const char *pSection;
      const char *pKey;
      int         nField;
      const char *pFieldValue;
    };

    struct InfFileUninstall {
      const InfFile::InfFileField *pRequiredFields;
      BOOL queryConfirmation;
    };

  public:
    InfFile(const char *pInfName, const char *pNearPath);
    ~InfFile();

    BOOL Test(const InfFileField *pFields, BOOL showErrors = TRUE) const;

    const char *Path() const { return pPath; }
    const char *OemPath(BOOL showErrors = TRUE) const;
    const char *ClassGUID(BOOL showErrors = TRUE) const;
    const char *Class(BOOL showErrors = TRUE) const;
    const char *Provider(BOOL showErrors = TRUE) const;
    const char *DriverVer(BOOL showErrors = TRUE) const;
    const char *UninstallInfTag(BOOL showErrors = TRUE) const;

    BOOL UninstallFiles(const char *pFilesSection) const;

    BOOL InstallOEMInf() const;
    BOOL UninstallOEMInf() const;

    static BOOL UninstallAllInfFiles(
        const InfFileUninstall *pInfFileUninstallList,
        const char *const *ppOemPathExcludeList);

  protected:
    char *pPath;
    mutable char *pOemPath;
    mutable char *pClassGUID;
    mutable char *pClass;
    mutable char *pProvider;
    mutable char *pDriverVer;
    mutable char *pUninstallInfTag;
    mutable HINF hInf;
};

#endif /* _C0C_PARAMS_H_ */
