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
 * Revision 1.10  2010/07/30 09:27:18  vfrolov
 * Added STRDUP()
 * Fixed updating the source location information by OemPath() and InstallOEMInf()
 * Optimazed UninstallAllInfFiles() to look omly OEM files
 *
 * Revision 1.9  2010/07/16 07:47:13  vfrolov
 * Added using SetupUninstallOEMInf() if it exists in setupapi.dll
 *
 * Revision 1.8  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.7  2009/02/16 10:36:16  vfrolov
 * Done --silent option more silent
 *
 * Revision 1.6  2008/04/02 10:30:26  vfrolov
 * Added check pointer for null
 *
 * Revision 1.5  2007/09/20 12:39:59  vfrolov
 * Added SetLastError(ERROR_NOT_ENOUGH_MEMORY)
 *
 * Revision 1.4  2007/06/15 09:45:50  vfrolov
 * Increased list size for SetupGetInfFileList()
 *
 * Revision 1.3  2007/06/14 16:11:01  vfrolov
 * Added Scan INF files progress indication
 *
 * Revision 1.2  2006/10/19 13:28:48  vfrolov
 * Added InfFile::UninstallAllInfFiles()
 *
 * Revision 1.1  2006/07/28 12:16:43  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "inffile.h"
#include "msg.h"
#include "utils.h"

///////////////////////////////////////////////////////////////
static BOOL GetVersionInfo(const char *pInfPath, const char *pKey, char **ppValue, BOOL showErrors)
{
  if (!pInfPath)
    return FALSE;

  if (*ppValue)
    return TRUE;

  DWORD size;
  PSP_INF_INFORMATION pInformation;

  if (!SetupGetInfInformation(pInfPath, INFINFO_INF_NAME_IS_ABSOLUTE, NULL, 0, &size)) {
    if (showErrors)
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupGetInfInformation() for %s", pInfPath);
    return FALSE;
  }

  pInformation = (PSP_INF_INFORMATION)LocalAlloc(LPTR, size);

  if (pInformation) {
    if (!SetupGetInfInformation(pInfPath, INFINFO_INF_NAME_IS_ABSOLUTE, pInformation, size, NULL)) {
      if (showErrors)
        ShowLastError(MB_OK|MB_ICONSTOP, "SetupGetInfInformation() for %s", pInfPath);
      LocalFree(pInformation);
      return FALSE;
    }
  } else {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)size);
    return FALSE;
  }

  if (!SetupQueryInfVersionInformation(pInformation, 0, pKey, NULL, 0, &size)) {
    if (showErrors)
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s) for %s", pKey, pInfPath);
    LocalFree(pInformation);
    return FALSE;
  }

  *ppValue = (char *)LocalAlloc(LPTR, size*sizeof(*ppValue[0]));

  if (*ppValue) {
    if (!SetupQueryInfVersionInformation(pInformation, 0, pKey, *ppValue, size, NULL)) {
      if (showErrors)
        ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s) for %s", pKey, pInfPath);
      LocalFree(*ppValue);
      *ppValue = NULL;
      LocalFree(pInformation);
      return FALSE;
    }
  } else {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(size*sizeof(*ppValue[0])));
    LocalFree(pInformation);
    return FALSE;
  }

  LocalFree(pInformation);
  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL Open(const char *pInfPath, HINF *phInf, BOOL showErrors)
{
  if (*phInf != INVALID_HANDLE_VALUE)
    return TRUE;

  int res;

  do {
    res = IDCONTINUE;

    UINT errLine;
    *phInf = SetupOpenInfFile(pInfPath, NULL, INF_STYLE_WIN4, &errLine);

    if (*phInf == INVALID_HANDLE_VALUE) {
      if (!showErrors)
        break;

      res = ShowLastError(MB_CANCELTRYCONTINUE,
                          "SetupOpenInfFile(%s) on line %u",
                          pInfPath, errLine);
    }
  } while (res == IDTRYAGAIN);

  return *phInf != INVALID_HANDLE_VALUE;
}
///////////////////////////////////////////////////////////////
static BOOL IsPathInList(
    const char *pPath,
    const char *const *ppList)
{
  if (pPath == NULL || ppList == NULL)
    return FALSE;

  while (*ppList) {
    if (lstrcmpi(*ppList++, pPath) == 0)
      return TRUE;
  }

  return FALSE;
}
///////////////////////////////////////////////////////////////
static BOOL GetFilePath(
    const char *pFileName,
    const char *pNearPath,
    char *pFilePath,
    DWORD lenFilePath)
{
  char *pBuf;
  DWORD res;

  if (!pNearPath) {
    if (DWORD(lstrlen(pFileName)) >= lenFilePath) {
      ShowError(MB_OK|MB_ICONSTOP, ERROR_BUFFER_OVERFLOW, "lstrlen(%s) >= %lu",
                pFileName, (long)lenFilePath);
      return FALSE;
    }

    lstrcpy(pFilePath, pFileName);

    return TRUE;
  }

  res = GetFullPathName(pNearPath, lenFilePath, pFilePath, &pBuf);

  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "GetFullPathName(%s)", pNearPath);
    return FALSE;
  }

  if (res >= lenFilePath) {
    ShowError(MB_OK|MB_ICONSTOP, ERROR_BUFFER_OVERFLOW, "GetFullPathName(%s)", pNearPath);
    return FALSE;
  }

  if (!pBuf) {
    ShowError(MB_OK|MB_ICONSTOP, ERROR_INVALID_NAME, "GetFullPathName(%s)", pNearPath);
    return FALSE;
  }

  if (DWORD(pBuf - pFilePath + lstrlen(pFileName)) >= lenFilePath) {
    ShowError(MB_OK|MB_ICONSTOP, ERROR_BUFFER_OVERFLOW, "GetFullPathName(%s)", pNearPath);
    return FALSE;
  }

  lstrcpy(pBuf, pFileName);

  //ShowMsg(MB_OK, "pFilePath=%s\n", pFilePath);

  return TRUE;
}

InfFile::InfFile(const char *pInfName, const char *pNearPath)
  : pPath(NULL)
  , pOemPath(NULL)
  , pClassGUID(NULL)
  , pClass(NULL)
  , pProvider(NULL)
  , pDriverVer(NULL)
  , pUninstallInfTag(NULL)
  , hInf(INVALID_HANDLE_VALUE)
{
  char path[MAX_PATH + 1];

  if (GetFilePath(pInfName, pNearPath, path, sizeof(path)/sizeof(path[0])))
    pPath = STRDUP(path);
}
///////////////////////////////////////////////////////////////
InfFile::~InfFile()
{
  if (pPath)
    LocalFree((HLOCAL)pPath);
  if (pOemPath)
    LocalFree((HLOCAL)pOemPath);
  if (pClassGUID)
    LocalFree((HLOCAL)pClassGUID);
  if (pClass)
    LocalFree((HLOCAL)pClass);
  if (pProvider)
    LocalFree((HLOCAL)pProvider);
  if (pUninstallInfTag)
    LocalFree((HLOCAL)pUninstallInfTag);
  if (pDriverVer)
    LocalFree((HLOCAL)pDriverVer);
  if (hInf != INVALID_HANDLE_VALUE)
    SetupCloseInfFile(hInf);
}
///////////////////////////////////////////////////////////////
BOOL InfFile::Test(const InfFileField *pFields, BOOL showErrors) const
{
  if (!Open(pPath, &hInf, showErrors))
    return FALSE;

  for (const InfFileField *pField = pFields ; pField->pSection != NULL ; pField++) {
    INFCONTEXT context;

    if (!SetupFindFirstLine(hInf, pField->pSection, pField->pKey, &context))
      return FALSE;

    for (;;) {
      DWORD size;

      if (!SetupGetStringField(&context, pField->nField, NULL, 0, &size))
        return FALSE;

      char *pValue = (char *)LocalAlloc(LPTR, size*sizeof(pValue[0]));

      if (pValue == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(size*sizeof(pValue[0])));
        return FALSE;
      }

      if (!SetupGetStringField(&context, pField->nField, pValue, size, NULL)) {
        LocalFree(pValue);
        return FALSE;
      }

      if (lstrcmpi(pValue, pField->pFieldValue) == 0) {
        LocalFree(pValue);
        break;
      }

      LocalFree(pValue);

      if (!SetupFindNextMatchLine(&context, pField->pKey, &context))
        return FALSE;
    }
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
const char *InfFile::OemPath(BOOL showErrors) const
{
  if (!pOemPath) {
    char path[MAX_PATH + 1];

    if (SetupCopyOEMInf(pPath, NULL, SPOST_NONE, SP_COPY_REPLACEONLY|SP_COPY_NOOVERWRITE,
          path, sizeof(path)/sizeof(path[0]), NULL, NULL) ||
        GetLastError() == ERROR_FILE_EXISTS)
    {
      pOemPath = STRDUP(path, showErrors);
    }
  }

  return pOemPath;
}
///////////////////////////////////////////////////////////////
const char *InfFile::ClassGUID(BOOL showErrors) const
{
  GetVersionInfo(pPath, "ClassGUID", &pClassGUID, showErrors);

  return pClassGUID;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Class(BOOL showErrors) const
{
  GetVersionInfo(pPath, "Class", &pClass, showErrors);

  return pClass;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Provider(BOOL showErrors) const
{
  GetVersionInfo(pPath, "Provider", &pProvider, showErrors);

  return pProvider;
}
///////////////////////////////////////////////////////////////
const char *InfFile::DriverVer(BOOL showErrors) const
{
  GetVersionInfo(pPath, "DriverVer", &pDriverVer, showErrors);

  return pDriverVer;
}
///////////////////////////////////////////////////////////////
const char *InfFile::UninstallInfTag(BOOL showErrors) const
{
  GetVersionInfo(pPath, "UninstallInfTag", &pUninstallInfTag, showErrors);

  return pUninstallInfTag;
}
///////////////////////////////////////////////////////////////
static UINT FileCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2)
{
  if (Notification == SPFILENOTIFY_ENDDELETE) {
    PFILEPATHS pFilePaths = (PFILEPATHS)Param1;

    if (pFilePaths->Win32Error == ERROR_SUCCESS) {
      Trace("Deleted File %s\n", pFilePaths->Target);
    }
    else
    if (pFilePaths->Win32Error == ERROR_FILE_NOT_FOUND) {
      Trace("File %s not installed\n", pFilePaths->Target);
    }
    else {
      ShowError(MB_OK|MB_ICONWARNING, pFilePaths->Win32Error, "Delete(%s)", pFilePaths->Target);
    }
  }

  return SetupDefaultQueueCallback(Context, Notification, Param1, Param2);
}

BOOL InfFile::UninstallFiles(const char *pFilesSection) const
{
  if (!pPath)
    return FALSE;

  if (!Open(pPath, &hInf, TRUE))
    return FALSE;

  int res;

  do {
    res = IDCONTINUE;

    HSPFILEQ hFileQueue = SetupOpenFileQueue();

    if (hFileQueue != INVALID_HANDLE_VALUE) {
      if (SetupQueueDeleteSection(hFileQueue, hInf, NULL, pFilesSection)) {
        PVOID pContext = SetupInitDefaultQueueCallback(NULL);

        if (pContext) {
          if(!SetupCommitFileQueue(NULL, hFileQueue, FileCallback, pContext))
            res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupCommitFileQueue()");

          SetupTermDefaultQueueCallback(pContext);
        } else {
          res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupInitDefaultQueueCallback()");
        }
      } else {
        res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupQueueDeleteSection(%s)", pFilesSection);
      }

      SetupCloseFileQueue(hFileQueue);
    } else {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupOpenFileQueue()");
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL InfFile::InstallOEMInf() const
{
  if (!pPath)
    return FALSE;

  BOOL wasInstalled = (OemPath() != NULL);

  if (!SetupCopyOEMInf(pPath, NULL, SPOST_PATH, 0, NULL, 0, NULL, NULL)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupCopyOEMInf(%s)", pPath);
    return FALSE;
  }

  if (!wasInstalled)
    Trace("Installed %s to %s\n", pPath, OemPath());
  else
    Trace("Re-installed %s to %s\n", pPath, OemPath());

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL UninstallFile(const char *pPath)
{
  int res;

  do {
    res = IDCONTINUE;

    if (DeleteFile(pPath)) {
      Trace("Deleted %s\n", pPath);
    }
    else
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      Trace("File %s not installed\n", pPath);
    }
    else {
      res = ShowLastError(MB_CANCELTRYCONTINUE, "DeleteFile(%s)", pPath);
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}

static BOOL UninstallInf(const char *pPath)
{
  if (pPath == NULL)
    return FALSE;

  int res;

  do {
    res = IDCONTINUE;

    char infPathDest[MAX_PATH + 1];

    if (SNPRINTF(infPathDest, sizeof(infPathDest)/sizeof(infPathDest[0]), "%s", pPath) > 0) {
      typedef BOOL (WINAPI *PSETUPUNINSTALLOEMINFA)(IN PCSTR, IN DWORD, IN PVOID);
      static PSETUPUNINSTALLOEMINFA pSetupUninstallOEMInf = NULL;

      if(!pSetupUninstallOEMInf) {
        HMODULE hModule = GetModuleHandle("setupapi.dll");

        if (hModule) {
          pSetupUninstallOEMInf =
              (PSETUPUNINSTALLOEMINFA)GetProcAddress(hModule, "SetupUninstallOEMInfA");
        }
      }

      if (pSetupUninstallOEMInf) {
        char *pInfNameDest, *p;

        for (pInfNameDest = p = infPathDest ; *p ; p++)
          if (*p == '\\')
            pInfNameDest = p + 1;

        if (pSetupUninstallOEMInf(pInfNameDest, 0, NULL)) {
          Trace("Uninstalled %s\n", pInfNameDest);
        } else {
          res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupUninstallOEMInf(%s)", pInfNameDest);
        }
      }
      else
      if (UninstallFile(infPathDest)) {
        int infPathDestLen = lstrlen(infPathDest);

        if (infPathDestLen > 4) {
          char *pInfPathDestExt = infPathDest + infPathDestLen - 4;

          if (!lstrcmpi(pInfPathDestExt, ".inf")) {
            pInfPathDestExt[1] = 'p';     // pInfPathDestExt = ".pnf"

            if (!UninstallFile(infPathDest))
              res = IDCANCEL;
          }
        }
      } else {
        res = IDCANCEL;
      }
    } else {
      Trace("Can't uninstall %s\n", pPath);
      res = IDCANCEL;
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return res;
}
///////////////////////////////////////////////////////////////
BOOL InfFile::UninstallOEMInf() const
{
  if (!pPath)
    return FALSE;

  int res;

  do {
    res = IDCONTINUE;

    if (OemPath()) {
      UninstallInf(OemPath());
    } else {
      if (GetLastError() == ERROR_FILE_NOT_FOUND) {
        Trace("File %s not installed\n", pPath);
      } else {
        res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupCopyOEMInf(%s)", pPath);
      }
    }
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
static BOOL TestUninstall(
    const InfFile &infFile,
    const InfFile::InfFileUninstall *pInfFileUninstallList,
    BOOL queryConfirmation)
{
  for (
      const InfFile::InfFileUninstall *pInfFileUninstall = pInfFileUninstallList ;
      pInfFileUninstall->pRequiredFields != NULL ;
      pInfFileUninstall++)
  {
    if (pInfFileUninstall->queryConfirmation != queryConfirmation)
      continue;

    if (infFile.Test(pInfFileUninstall->pRequiredFields, FALSE))
      return TRUE;
  }

  return FALSE;
}

BOOL InfFile::UninstallAllInfFiles(
    const InfFileUninstall *pInfFileUninstallList,
    const char *const *ppOemPathExcludeList)
{
  Trace("Scan INF files .");

  DWORD size;

  if (!SetupGetInfFileList(NULL, INF_STYLE_WIN4, NULL, 0, &size)) {
    DWORD err = GetLastError();

    Trace(" FAIL\n");
    ShowError(MB_OK|MB_ICONSTOP, err, "SetupGetInfFileList()");
    return FALSE;
  }

  Trace("...");

  size += 256; // possible new INF files were added since
  char *pList = (char *)LocalAlloc(LPTR, size*sizeof(pList[0]));

  if (pList) {
    if (!SetupGetInfFileList(NULL, INF_STYLE_WIN4, pList, size, NULL)) {
      DWORD err = GetLastError();

      Trace(" FAIL\n");
      ShowError(MB_OK|MB_ICONSTOP, err, "SetupGetInfFileList(%lu)", (unsigned long)size);
      LocalFree(pList);
      return FALSE;
    }
  } else {
    Trace(" FAIL\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)size);
    return FALSE;
  }

  Trace(".");

  char windir[MAX_PATH + 1];

  size = GetEnvironmentVariable("windir", windir, sizeof(windir)/sizeof(windir[0]));

  if (!size || size >= sizeof(windir)/sizeof(windir[0])) {
    DWORD err = !size ? GetLastError() : ERROR_BUFFER_OVERFLOW;

    Trace(" FAIL\n");
    ShowError(MB_OK|MB_ICONSTOP, err, "GetEnvironmentVariable(windir)");
    LocalFree(pList);
    return FALSE;
  }

  char *p = pList;
  int i;
  int m;

  p = pList;
  i = 0;

  do {
    i++;
    p += lstrlen(p) + 1;
  } while (*p);

  m = i/3;

  if (m == 0)
    m = 1;

  p = pList;
  i = 0;

  do {
    if (++i%m == 0)
      Trace(".");

    char infPath[MAX_PATH + 1];

    if ((p[0] == 'O' || p[0] == 'o') &&
        (p[1] == 'E' || p[1] == 'e') &&
        (p[2] == 'M' || p[2] == 'm') &&
        SNPRINTF(infPath, sizeof(infPath)/sizeof(infPath[0]), "%s\\inf\\%s", windir, p) > 0)
    {
      InfFile infFile(infPath, NULL);
      BOOL doUninstall;

      if (IsPathInList(infFile.Path(), ppOemPathExcludeList)) {
        //Trace("\nSkipped %s\n", infFile.Path());
        doUninstall = FALSE;
      }
      else
      if (TestUninstall(infFile, pInfFileUninstallList, FALSE)) {
        doUninstall = TRUE;
      }
      else
      if (TestUninstall(infFile, pInfFileUninstallList, TRUE)) {
        int res;

        if (!Silent()) {
          res = ShowMsg(MB_YESNO,
            "The file %s possible should be deleted too.\n"
            "\n"
            "%s:\n"
            "  ClassGUID = %s\n"
            "  Class = %s\n"
            "  Provider = %s\n"
            "  DriverVer = %s\n"
            "  UninstallInfTag = %s\n"
            "\n"
            "Would you like to delete it?\n",
            infFile.Path(),
            infFile.Path(),
            infFile.ClassGUID(FALSE),
            infFile.Class(FALSE),
            infFile.Provider(FALSE),
            infFile.DriverVer(FALSE),
            infFile.UninstallInfTag(FALSE));
        } else {
          Trace("\nThe file %s possible should be deleted too:\n"
                "  ClassGUID = %s\n"
                "  Class = %s\n"
                "  Provider = %s\n"
                "  DriverVer = %s\n"
                "  UninstallInfTag = %s\n",
                infFile.Path(),
                infFile.ClassGUID(FALSE),
                infFile.Class(FALSE),
                infFile.Provider(FALSE),
                infFile.DriverVer(FALSE),
                infFile.UninstallInfTag(FALSE));

          res = IDNO;
        }

        doUninstall = (res == IDYES);
      } else {
        doUninstall = FALSE;
      }

      if (doUninstall) {
        Trace("\n");

        if (infFile.hInf != INVALID_HANDLE_VALUE) {
          SetupCloseInfFile(infFile.hInf);
          infFile.hInf = INVALID_HANDLE_VALUE;
        }

        UninstallInf(infFile.Path());
      }
    }

    p += lstrlen(p) + 1;
  } while (*p);

  Trace(" done.\n");

  LocalFree(pList);

  return TRUE;
}
///////////////////////////////////////////////////////////////
