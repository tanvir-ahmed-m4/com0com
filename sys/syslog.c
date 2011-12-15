/*
 * $Id$
 *
 * Copyright (c) 2004-2011 Vyacheslav Frolov
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
 * Revision 1.2  2011/12/15 06:17:12  vfrolov
 * Removed usage undocumented PDRIVER_OBJECT->Type
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "syslog.h"
#include "c0clog.h"

VOID SysLog(
    IN PVOID pIoObject,
    IN NTSTATUS status,
    IN PWCHAR pStr,
    IN NTSTATUS msgId)
{
  PIO_ERROR_LOG_PACKET pErrorLogEntry;
  PUCHAR pInsert;
  SIZE_T lenStr = wcslen(pStr) * sizeof(WCHAR);
  SIZE_T lenInsert = lenStr + sizeof(WCHAR);
  SIZE_T entrySize = sizeof(IO_ERROR_LOG_PACKET) + lenInsert;

  if (entrySize > ERROR_LOG_MAXIMUM_SIZE) {
    lenStr = ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET);
    lenStr = (lenStr / sizeof(WCHAR)) * sizeof(WCHAR) - sizeof(WCHAR);
    lenInsert = lenStr + sizeof(WCHAR);
    entrySize = sizeof(IO_ERROR_LOG_PACKET) + lenInsert;
  }

  if ((pErrorLogEntry = IoAllocateErrorLogEntry(pIoObject, (UCHAR)entrySize)) == NULL)
    return;

  pInsert = (PUCHAR)&pErrorLogEntry->DumpData[0];
  RtlZeroMemory(pInsert, lenInsert);

  pErrorLogEntry->ErrorCode = msgId;

  pErrorLogEntry->SequenceNumber = 0;
  pErrorLogEntry->MajorFunctionCode = 0;
  pErrorLogEntry->RetryCount = 0;
  pErrorLogEntry->UniqueErrorValue = 0;
  pErrorLogEntry->FinalStatus = status;
  pErrorLogEntry->DumpDataSize = 0;

  pErrorLogEntry->NumberOfStrings = 1;
  pErrorLogEntry->StringOffset = (USHORT)(pInsert - (PUCHAR)pErrorLogEntry);
  RtlCopyMemory(pInsert, pStr, lenStr);

  IoWriteErrorLogEntry(pErrorLogEntry);
}

VOID SysLogDrv(
    IN PDRIVER_OBJECT pDrvObj,
    IN NTSTATUS status,
    IN PWCHAR pStr)
{
  SysLog(pDrvObj, status, pStr, COM0COM_LOG_DRV);
}

VOID SysLogDev(
    IN PDEVICE_OBJECT pDevObj,
    IN NTSTATUS status,
    IN PWCHAR pStr)
{
  SysLog(pDevObj, status, pStr, COM0COM_LOG);
}
