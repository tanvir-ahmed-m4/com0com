/*
 * $Id$
 *
 * Copyright (c) 2006-2009 Vyacheslav Frolov
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

  char buf[4000];

  if (!SetupGetInfInformation(pInfPath, INFINFO_INF_NAME_IS_ABSOLUTE, (PSP_INF_INFORMATION)buf, sizeof(buf), NULL)) {
    if (showErrors)
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupGetInfInformation(%s)", pInfPath);
    return FALSE;
  }

  DWORD size;

  if (!SetupQueryInfVersionInformation((PSP_INF_INFORMATION)buf, 0, pKey, NULL, 0, &size)) {
    if (showErrors)
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s) for %s", pKey, pInfPath);
    return FALSE;
  }

  *ppValue = (char *)LocalAlloc(LPTR, size*sizeof(*ppValue[0]));

  if (*ppValue) {
    if (!SetupQueryInfVersionInformation((PSP_INF_INFORMATION)buf, 0, pKey, *ppValue, size, NULL)) {
      if (showErrors)
        ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s) for %s", pKey, pInfPath);
      LocalFree(*ppValue);
      *ppValue = NULL;
      return FALSE;
    }
  } else {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(size*sizeof(*ppValue[0])));
    return FALSE;
  }

  return TRUE;
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
  : pPath(NULL),
    pClassGUID(NULL),
    pClass(NULL),
    pProvider(NULL)
{
  char path[MAX_PATH];

  if (GetFilePath(pInfName, pNearPath, path, sizeof(path)/sizeof(path[0]))) {
    int len = lstrlen(path);

    pPath = (char *)LocalAlloc(LPTR, (len + 1)*sizeof(path[0]));

    if (pPath) {
      lstrcpy(pPath, path);
    } else {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)(sizeof(path)/sizeof(path[0])));
    }
  }
}
///////////////////////////////////////////////////////////////
InfFile::~InfFile()
{
  if (pPath)
    LocalFree((HLOCAL)pPath);
  if (pClassGUID)
    LocalFree((HLOCAL)pClassGUID);
  if (pClass)
    LocalFree((HLOCAL)pClass);
  if (pProvider)
    LocalFree((HLOCAL)pProvider);
}
///////////////////////////////////////////////////////////////
const char *InfFile::ClassGUID(BOOL showErrors) const
{
  GetVersionInfo(pPath, "ClassGUID", &(char *)pClassGUID, showErrors);

  return pClassGUID;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Class(BOOL showErrors) const
{
  GetVersionInfo(pPath, "Class", &(char *)pClass, showErrors);

  return pClass;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Provider(BOOL showErrors) const
{
  GetVersionInfo(pPath, "Provider", &(char *)pProvider, showErrors);

  return pProvider;
}
///////////////////////////////////////////////////////////////
BOOL InfFile::Compare(
    const char *_pClassGUID,
    const char *_pClass,
    const char *_pProvider,
    BOOL showErrors) const
{
  return (!_pClassGUID || (ClassGUID(showErrors) && !lstrcmpi(pClassGUID, _pClassGUID))) &&
         (!_pClass || (Class(showErrors) && !lstrcmpi(pClass, _pClass))) &&
         (!_pProvider || (Provider(showErrors) && !lstrcmpi(pProvider, _pProvider)));
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

  int res;
  HINF hInf;

  do {
    res = IDCONTINUE;

    UINT errLine;
    hInf = SetupOpenInfFile(pPath, NULL, INF_STYLE_WIN4, &errLine);

    if (hInf == INVALID_HANDLE_VALUE)
      res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupOpenInfFile(%s) on line %u", pPath, errLine);
  } while (res == IDTRYAGAIN);

  if (res != IDCONTINUE)
    return FALSE;

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

  SetupCloseInfFile(hInf);

  if (res != IDCONTINUE)
    return FALSE;

  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL InfFile::InstallOEMInf() const
{
  if (!pPath)
    return FALSE;

  if (!SetupCopyOEMInf(pPath, NULL, SPOST_PATH, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL)) {
    char infPathDest[MAX_PATH];

    if (!SetupCopyOEMInf(pPath, NULL, SPOST_PATH, 0, infPathDest,
        sizeof(infPathDest)/sizeof(infPathDest[0]), NULL, NULL))
    {
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupCopyOEMInf(%s)", pPath);
      return FALSE;
    }

    Trace("Installed %s to %s\n", pPath, infPathDest);
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
#ifndef HAVE_SetupUninstallOEMInf
static BOOL UninstallFile(const char *pPath)
{
  int res;

  do {
    res = IDCONTINUE;

    if (DeleteFile(pPath)) {
      Trace("Deleted File %s\n", pPath);
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
#endif /* HAVE_SetupUninstallOEMInf */

static BOOL UninstallInf(const char *pPath)
{
  int res;

  do {
    res = IDCONTINUE;

    char infPathDest[MAX_PATH];

    if (SNPRINTF(infPathDest, sizeof(infPathDest)/sizeof(infPathDest[0]), "%s", pPath) > 0) {
#ifdef HAVE_SetupUninstallOEMInf
      char *pInfNameDest, *p;

      for (pInfNameDest = p = infPathDest ; *p ; p++)
        if (*p == '\\')
          pInfNameDest = p + 1;

      if (SetupUninstallOEMInf(pInfNameDest, 0, NULL)) {
        Trace("Deleted %s\n", pInfNameDest);
      } else {
        res = ShowLastError(MB_CANCELTRYCONTINUE, "SetupUninstallOEMInf(%s)", pInfNameDest);
      }
#else /* HAVE_SetupUninstallOEMInf */
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
#endif /* HAVE_SetupUninstallOEMInf */
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

    char infPathDest[MAX_PATH];

    if (SetupCopyOEMInf(pPath, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, infPathDest,
        sizeof(infPathDest)/sizeof(infPathDest[0]), NULL, NULL))
    {
      UninstallInf(infPathDest);
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
BOOL InfFile::UninstallAllInfFiles(
    const char *_pClassGUID,
    const char *_pClass,
    const char *_pProvider)
{
  Trace("Scan INF files .");

  DWORD size;

  if (!SetupGetInfFileList(NULL, INF_STYLE_WIN4, NULL, 0, &size)) {
    DWORD err = GetLastError();

    Trace("\n");

    ShowError(MB_OK|MB_ICONSTOP, err, "SetupGetInfFileList()");
    return FALSE;
  }

  Trace("...");

  size += 256; // possible new INF files were added since
  char *pList = (char *)LocalAlloc(LPTR, size*sizeof(pList[0]));

  if (pList) {
    if (!SetupGetInfFileList(NULL, INF_STYLE_WIN4, pList, size, NULL)) {
      DWORD err = GetLastError();

      Trace("\n");

      ShowError(MB_OK|MB_ICONSTOP, err, "SetupGetInfFileList(%lu)", (unsigned long)size);
      LocalFree(pList);
      return FALSE;
    }
  } else {
    Trace("\n");

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc(%lu)", (unsigned long)size);
    return FALSE;
  }

  Trace(".");

  char windir[MAX_PATH];

  size = GetEnvironmentVariable("windir", windir, sizeof(windir)/sizeof(windir[0]));

  if (!size || size >= sizeof(windir)/sizeof(windir[0])) {
    DWORD err = !size ? GetLastError() : ERROR_BUFFER_OVERFLOW;

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

    char infPath[MAX_PATH];

    if (SNPRINTF(infPath, sizeof(infPath)/sizeof(infPath[0]), "%s\\inf\\%s", windir, p) > 0) {
      InfFile infFile(infPath, NULL);

      if (infFile.Compare(_pClassGUID, _pClass, _pProvider, FALSE)) {
        int res;

        if (!Silent()) {
          res = ShowMsg(MB_YESNO,
            "The file %s possible should be deleted too.\n"
            "\n"
            "%s:\n"
            "  ClassGUID = %s\n"
            "  Class = %s\n"
            "  Provider = %s\n"
            "\n"
            "Would you like to delete it?\n",
            infFile.Path(),
            infFile.Path(),
            infFile.ClassGUID(FALSE),
            infFile.Class(FALSE),
            infFile.Provider(FALSE));
        } else {
          Trace("\nThe file %s possible should be deleted too\n"
                "  ClassGUID = %s\n"
                "  Class = %s\n"
                "  Provider = %s\n",
                infFile.Path(),
                infFile.ClassGUID(FALSE),
                infFile.Class(FALSE),
                infFile.Provider(FALSE));

          res = IDNO;
        }

        if (res == IDYES) {
          Trace("\n");
          UninstallInf(infFile.Path());
        }
      }
    }

    p += lstrlen(p) + 1;
  } while (*p);

  Trace(" done.\n");

  LocalFree(pList);

  return TRUE;
}
///////////////////////////////////////////////////////////////
