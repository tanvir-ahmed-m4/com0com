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

#include "precomp.h"
#include "inffile.h"
#include "msg.h"

///////////////////////////////////////////////////////////////
static BOOL GetVersionInfo(const char *pInfPath, const char *pKey, char **ppValue)
{
  if (!pInfPath)
    return FALSE;

  if (*ppValue)
    return TRUE;

  char buf[4000];

  if (!SetupGetInfInformation(pInfPath, INFINFO_INF_NAME_IS_ABSOLUTE, (PSP_INF_INFORMATION)buf, sizeof(buf), NULL)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupGetInfInformation()");
    return FALSE;
  }

  DWORD size;

  if (!SetupQueryInfVersionInformation((PSP_INF_INFORMATION)buf, 0, pKey, NULL, 0, &size)) {
    ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s)", pKey);
    return FALSE;
  }

  *ppValue = (char *)LocalAlloc(LPTR, size*sizeof(*ppValue[0]));

  if (*ppValue) {
    if (!SetupQueryInfVersionInformation((PSP_INF_INFORMATION)buf, 0, pKey, *ppValue, size, NULL)) {
      ShowLastError(MB_OK|MB_ICONSTOP, "SetupQueryInfVersionInformation(%s)", pKey);
      LocalFree(*ppValue);
      *ppValue = NULL;
      return FALSE;
    }
  } else {
    ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc()");
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

  res = GetFullPathName(pNearPath, lenFilePath, pFilePath, &pBuf);

  if (!res) {
    ShowLastError(MB_OK|MB_ICONSTOP, "GetFullPathName(%s)", pNearPath);
    return FALSE;
  }

  if (res >= lenFilePath) {
    ShowError(MB_OK|MB_ICONSTOP, ERROR_BUFFER_OVERFLOW, "GetFullPathName(%s)", pNearPath);
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

  if (GetFilePath(pInfName, pNearPath, path, sizeof(path))) {
    int len = lstrlen(path);

    pPath = (char *)LocalAlloc(LPTR, (len + 1)*sizeof(path[0]));

    if (pPath)
      lstrcpy(pPath, path);
    else
      ShowLastError(MB_OK|MB_ICONSTOP, "LocalAlloc()");
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
const char *InfFile::ClassGUID() const
{
  GetVersionInfo(pPath, "ClassGUID", &(char *)pClassGUID);

  return pClassGUID;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Class() const
{
  GetVersionInfo(pPath, "Class", &(char *)pClass);

  return pClass;
}
///////////////////////////////////////////////////////////////
const char *InfFile::Provider() const
{
  GetVersionInfo(pPath, "Provider", &(char *)pProvider);

  return pProvider;
}
///////////////////////////////////////////////////////////////
BOOL InfFile::Compare(
    const char *_pClassGUID,
    const char *_pClass,
    const char *_pProvider) const
{
  return ClassGUID() && !lstrcmpi(pClassGUID, _pClassGUID) &&
         Class() && !lstrcmpi(pClass, _pClass) &&
         Provider() && !lstrcmpi(pProvider, _pProvider);
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

    if (!SetupCopyOEMInf(pPath, NULL, SPOST_PATH, 0, infPathDest, sizeof(infPathDest), NULL, NULL)) {
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

BOOL InfFile::UninstallOEMInf() const
{
  if (!pPath)
    return FALSE;

  int res;

  do {
    res = IDCONTINUE;

    char infPathDest[MAX_PATH];
    char *pInfNameDest;

    if (SetupCopyOEMInf(pPath, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, infPathDest, sizeof(infPathDest), NULL, &pInfNameDest)) {
#ifdef HAVE_SetupUninstallOEMInf
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
