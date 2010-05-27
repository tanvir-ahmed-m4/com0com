/*
 * $Id$
 *
 * Copyright (c) 2004-2010 Vyacheslav Frolov
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
 * Revision 1.6  2010/05/27 11:06:23  vfrolov
 * Added StrAppendPortParametersRegistryPath() and StrAppendParameterPortName()
 *
 * Revision 1.5  2009/09/21 08:26:12  vfrolov
 * Fixed checking for overflow
 *
 * Revision 1.4  2007/01/11 14:50:29  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
 * Revision 1.3  2006/11/03 13:13:26  vfrolov
 * CopyStrW() now gets size in characters (not in bytes)
 *
 * Revision 1.2  2006/03/27 09:37:28  vfrolov
 * Added StrAppendDeviceProperty()
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "strutils.h"

NTSTATUS CopyStrW(OUT PWCHAR pDestStr, IN LONG size, IN PWCHAR pStr)
{
  NTSTATUS status;
  LONG len;
  PWCHAR pStrTmp;

  pStrTmp = pStr;

  while (*(pStrTmp++))
    ;

  len = (LONG)(pStrTmp - pStr);

  if (len > size) {
    len = size;
    status = STATUS_BUFFER_TOO_SMALL;
  } else {
    status = STATUS_SUCCESS;
  }

  if (len > 0) {
    RtlCopyMemory(pDestStr, pStr, len * sizeof(WCHAR));
    pDestStr[len - 1] = 0;
  }

  return status;
}

NTSTATUS DupStrW(OUT PWCHAR *ppDestStr, IN PWCHAR pStr, IN BOOLEAN multiStr)
{
  PWCHAR pStrTmp = pStr;
  ULONG len;

  if (multiStr) {
    do {
      while (*(pStrTmp++))
        ;
    } while (*(pStrTmp++));
  }
  else {
    while (*(pStrTmp++))
      ;
  }

  len = (ULONG)(pStrTmp - pStr) * sizeof(WCHAR);
  pStrTmp = (PWCHAR)C0C_ALLOCATE_POOL(PagedPool, len);

  if (!pStrTmp)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlCopyMemory(pStrTmp, pStr, len);
  *ppDestStr = pStrTmp;

  return STATUS_SUCCESS;
}

VOID StrFree(IN OUT PUNICODE_STRING  pDest)
{
  if (pDest->Buffer)
    C0C_FREE_POOL(pDest->Buffer);
  RtlZeroMemory(pDest, sizeof(*pDest));
}

BOOLEAN StrFreeBad(NTSTATUS status, IN OUT PUNICODE_STRING  pDest)
{
  if (!NT_SUCCESS(status)) {
    StrFree(pDest);
    return TRUE;
  }
  return FALSE;
}

VOID StrAppendStr(
    PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING  pDest,
    IN PWCHAR pSrc,
    IN USHORT lenSrc)
{
  UNICODE_STRING old;
  NTSTATUS status;
  SIZE_T newLength;

  status = *pStatus;

  if (!NT_SUCCESS(status) || !pSrc || !lenSrc)
    return;

  old = *pDest;

  RtlZeroMemory(pDest, sizeof(*pDest));

  newLength = (SIZE_T)old.Length + (SIZE_T)lenSrc;

  if ((USHORT)newLength == newLength) {  /* checking for overflow */
    pDest->MaximumLength = (USHORT)newLength;
    pDest->Buffer = C0C_ALLOCATE_POOL(PagedPool, pDest->MaximumLength + sizeof(WCHAR));
  }

  if (pDest->Buffer) {
    RtlZeroMemory(pDest->Buffer, pDest->MaximumLength + sizeof(WCHAR));
    status = RtlAppendUnicodeStringToString(pDest, &old);
    if (NT_SUCCESS(status)) {
      PWCHAR pSrc0;

      pSrc0 = C0C_ALLOCATE_POOL(PagedPool, lenSrc + sizeof(WCHAR));

      if (pSrc0) {
        RtlZeroMemory(pSrc0, lenSrc + sizeof(WCHAR));
        RtlCopyMemory(pSrc0, pSrc, lenSrc);

        status = RtlAppendUnicodeToString(pDest, pSrc0);

        C0C_FREE_POOL(pSrc0);
      } else
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
  } else
    status = STATUS_INSUFFICIENT_RESOURCES;

  StrFreeBad(status, pDest);

  if (old.Buffer)
    C0C_FREE_POOL(old.Buffer);

  *pStatus = status;
}

VOID StrAppendStr0(PNTSTATUS pStatus, IN OUT PUNICODE_STRING  pDest, IN PWCHAR pSrc)
{
  StrAppendStr(pStatus, pDest, pSrc, (USHORT)(wcslen(pSrc) * sizeof(WCHAR)));
}

VOID StrAppendNum(
    PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN ULONG num,
    IN ULONG base)
{
  UNICODE_STRING numStr;
  WCHAR numStrBuf[20];

  if (!NT_SUCCESS(*pStatus))
    return;

  RtlInitUnicodeString(&numStr, NULL);
  numStr.MaximumLength = sizeof(numStrBuf);
  numStr.Buffer = numStrBuf;
  *pStatus = RtlIntegerToUnicodeString(num, base, &numStr);

  if (StrFreeBad(*pStatus, pDest))
    return;

  StrAppendStr(pStatus, pDest, numStr.Buffer, numStr.Length);
}

VOID StrAppendDeviceProperty(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PDEVICE_OBJECT pDevObj,
    IN DEVICE_REGISTRY_PROPERTY deviceProperty)
{
  NTSTATUS status;
  ULONG len;

  status = *pStatus;

  if (!NT_SUCCESS(status))
    return;

  status = IoGetDeviceProperty(pDevObj,
                               deviceProperty,
                               0,
                               NULL,
                               &len);

  if (status == STATUS_BUFFER_TOO_SMALL && len) {
    PWCHAR pStrTmp;

    pStrTmp = (PWCHAR)C0C_ALLOCATE_POOL(PagedPool, len);

    if (pStrTmp) {
      status = IoGetDeviceProperty(pDevObj,
                                   deviceProperty,
                                   len,
                                   pStrTmp,
                                   &len);

      if (NT_SUCCESS(status))
        StrAppendStr0(&status, pDest, pStrTmp);

      C0C_FREE_POOL(pStrTmp);
    } else {
      status = STATUS_INSUFFICIENT_RESOURCES;
    }
  }

  StrFreeBad(status, pDest);

  *pStatus = status;
}

VOID StrAppendPortParametersRegistryPath(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PWCHAR pPhPortName)
{
  StrAppendStr(pStatus, pDest, c0cGlobal.registryPath.Buffer, c0cGlobal.registryPath.Length);
  StrAppendStr0(pStatus, pDest, L"\\Parameters\\");
  StrAppendStr0(pStatus, pDest, pPhPortName);
}

VOID StrAppendParameterPortName(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PWCHAR pPortParametersRegistryPath)
{
  NTSTATUS status;
  WCHAR portNameBuf[C0C_PORT_NAME_LEN + 1];
  UNICODE_STRING portNameTmp;
  RTL_QUERY_REGISTRY_TABLE queryTable[2];

  status = *pStatus;

  if (!NT_SUCCESS(status))
    return;

  RtlZeroMemory(queryTable, sizeof(queryTable));

  portNameTmp.Length = 0;
  portNameTmp.MaximumLength = sizeof(portNameBuf);
  portNameTmp.Buffer = portNameBuf;

  queryTable[0].Flags        = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  queryTable[0].Name         = L"PortName";
  queryTable[0].EntryContext = &portNameTmp;

  status = RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      pPortParametersRegistryPath,
      queryTable,
      NULL,
      NULL);

  StrAppendStr(&status, pDest, portNameTmp.Buffer, portNameTmp.Length);

  StrFreeBad(status, pDest);

  *pStatus = status;
}
