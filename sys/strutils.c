/*
 * $Id$
 *
 * Copyright (c) 2004-2005 Vyacheslav Frolov
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
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "strutils.h"

NTSTATUS CopyStrW(OUT PWCHAR pDestStr, IN SIZE_T size, IN PWCHAR pStr)
{
  NTSTATUS status;
  SIZE_T len;
  PWCHAR pStrTmp;

  pStrTmp = pStr;

  while (*(pStrTmp++))
    ;

  len = (pStrTmp - pStr) * sizeof(WCHAR);

  if (len > size) {
    len = (size/sizeof(WCHAR)) * sizeof(WCHAR);
    status = STATUS_BUFFER_TOO_SMALL;
  } else {
    status = STATUS_SUCCESS;
  }

  if (len) {
    RtlCopyMemory(pDestStr, pStr, len);
    pDestStr[(len / sizeof(WCHAR)) - 1] = 0;
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
  pStrTmp = (PWCHAR)ExAllocatePool(PagedPool, len);
  
  if (!pStrTmp)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlCopyMemory(pStrTmp, pStr, len);
  *ppDestStr = pStrTmp;

  return STATUS_SUCCESS;
}

VOID StrFree(IN OUT PUNICODE_STRING  pDest)
{
  if (pDest->Buffer)
    ExFreePool(pDest->Buffer);
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

  status = *pStatus;

  if (!NT_SUCCESS(status) || !pSrc || !lenSrc)
	  return;

  old = *pDest;

  RtlZeroMemory(pDest, sizeof(*pDest));

  pDest->MaximumLength = (USHORT)(old.Length + lenSrc);

  if (pDest->MaximumLength == (old.Length + lenSrc))
    pDest->Buffer = ExAllocatePool(PagedPool, pDest->MaximumLength + sizeof(WCHAR));

  if (pDest->Buffer) {
    RtlZeroMemory(pDest->Buffer, pDest->MaximumLength + sizeof(WCHAR));
    status = RtlAppendUnicodeStringToString(pDest, &old);
	  if (NT_SUCCESS(status)) {
      PWCHAR pSrc0;

      pSrc0 = ExAllocatePool(PagedPool, lenSrc + sizeof(WCHAR));

      if (pSrc0) {
        RtlZeroMemory(pSrc0, lenSrc + sizeof(WCHAR));
        RtlCopyMemory(pSrc0, pSrc, lenSrc);

        status = RtlAppendUnicodeToString(pDest, pSrc0);

        ExFreePool(pSrc0);
      } else
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
  } else
    status = STATUS_INSUFFICIENT_RESOURCES;

  StrFreeBad(status, pDest);

  if (old.Buffer)
    ExFreePool(old.Buffer);

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
