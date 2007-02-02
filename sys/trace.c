/*
 * $Id$
 *
 * Copyright (c) 2004-2007 Vyacheslav Frolov
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
 * Revision 1.23  2007/02/02 09:52:21  vfrolov
 * Fixed huge system error logging if bad trace file used
 *
 * Revision 1.22  2006/10/27 12:36:58  vfrolov
 * Removed unnecessary InterlockedExchange*()
 *
 * Revision 1.21  2006/08/23 13:05:43  vfrolov
 * Added ability to trace w/o table
 * Added tracing IRP_MN_QUERY_ID result
 * Added tracing GUID for IRP_MN_QUERY_INTERFACE
 * Added tracing WMI
 *
 * Revision 1.20  2006/06/08 11:30:52  vfrolov
 * Added params check to Trace0() and Trace00()
 *
 * Revision 1.19  2006/05/19 15:02:03  vfrolov
 * Implemented IOCTL_SERIAL_GET_MODEM_CONTROL
 *
 * Revision 1.18  2006/01/10 09:44:04  vfrolov
 * Added ability to enable/disable dump
 * Added tracing of HoldReasons, WaitForImmediate, AmountInOutQueue for SERIAL_STATUS
 *
 * Revision 1.17  2005/12/06 13:01:54  vfrolov
 * Implemented IOCTL_SERIAL_GET_DTRRTS
 *
 * Revision 1.16  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.15  2005/11/30 16:04:12  vfrolov
 * Implemented IOCTL_SERIAL_GET_STATS and IOCTL_SERIAL_CLEAR_STATS
 *
 * Revision 1.14  2005/09/28 10:06:42  vfrolov
 * Implemented IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION
 *
 * Revision 1.13  2005/09/13 08:55:41  vfrolov
 * Disabled modem status tracing by default
 *
 * Revision 1.12  2005/09/09 15:21:32  vfrolov
 * Added additional flushing for saved strings
 *
 * Revision 1.11  2005/09/06 06:58:20  vfrolov
 * Added SERIAL_STATUS.Errors tracing
 * Added version tracing
 *
 * Revision 1.10  2005/08/30 13:12:04  vfrolov
 * Disabled IOCTL_SERIAL_GET_MODEMSTATUS tracing by default
 *
 * Revision 1.9  2005/08/25 07:48:39  vfrolov
 * Changed type of code names from wchar to char
 * Fixed HandFlow tracing
 *
 * Revision 1.8  2005/08/23 15:28:26  vfrolov
 * Added build timestamp
 *
 * Revision 1.7  2005/07/14 13:51:09  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.6  2005/07/01 11:03:50  vfrolov
 * Included <stdarg.h>
 *
 * Revision 1.5  2005/05/19 08:23:41  vfrolov
 * Fixed data types
 *
 * Revision 1.4  2005/05/13 16:58:03  vfrolov
 * Implemented IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.3  2005/02/28 12:10:08  vfrolov
 * Log skipped lines to trace file (was to syslog)
 * Fixed missing trace file close
 *
 * Revision 1.2  2005/02/01 16:39:35  vfrolov
 * Added AnsiStrCopyCommStatus()
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"

#if DBG

#include "version.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 4

/********************************************************************/
#include <stdarg.h>

#include "trace.h"
#include "strutils.h"
/********************************************************************/
#define TRACE_ERROR_LIMIT 10
#define TRACE_BUF_SIZE 256
#define TRACE_BUFS_NUM 4
/********************************************************************/
typedef struct _TRACE_BUFFER {
  BOOLEAN busy;
  CHAR buf[TRACE_BUF_SIZE];
} TRACE_BUFFER, *PTRACE_BUFFER;
/********************************************************************/
#define TRACE_ENABLE_IRP       0x00000001
#define TRACE_ENABLE_DUMP      0x00000002

#define TRACE_ENABLE_ALL       0xFFFFFFFF

static struct {
  ULONG read;
  ULONG write;
  ULONG getTimeouts;
  ULONG setTimeouts;
  ULONG getCommStatus;
  ULONG getModemStatus;
  ULONG modemStatus;
} traceEnable;
/********************************************************************/
static int errorCount;
static WCHAR traceFileNameBuf[256];
static UNICODE_STRING traceFileName;
static PDRIVER_OBJECT pDrvObj;
static KSPIN_LOCK strOldLock;
static KSPIN_LOCK bufsLock;
static TRACE_BUFFER traceBufs[TRACE_BUFS_NUM];
static LONG skippedTraces;
/********************************************************************/
#define TRACE_FILE_OK (traceFileName.Buffer != NULL)
/********************************************************************/

VOID QueryRegistryTrace(IN PUNICODE_STRING pRegistryPath)
{
  NTSTATUS status;
  UNICODE_STRING traceRegistryPath;
  RTL_QUERY_REGISTRY_TABLE queryTable[2];

  status = STATUS_SUCCESS;

  RtlInitUnicodeString(&traceRegistryPath, NULL);
  StrAppendStr(&status, &traceRegistryPath, pRegistryPath->Buffer, pRegistryPath->Length);
  StrAppendStr0(&status, &traceRegistryPath, L"\\Trace");

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"QueryRegistryTrace FAIL");
    return;
  }

  RtlZeroMemory(queryTable, sizeof(queryTable));

  traceFileName.Length = 0;
  traceFileName.MaximumLength = sizeof(traceFileNameBuf);
  traceFileName.Buffer = traceFileNameBuf;

  queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  queryTable[0].Name          = L"TraceFile";
  queryTable[0].EntryContext  = &traceFileName;

  status = RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      traceRegistryPath.Buffer,
      queryTable,
      NULL,
      NULL);

  StrFree(&traceRegistryPath);

  if (!NT_SUCCESS(status))
    RtlInitUnicodeString(&traceFileName, NULL);
}

VOID QueryRegistryTraceEnable(IN PUNICODE_STRING pRegistryPath)
{
  NTSTATUS status;
  UNICODE_STRING traceRegistryPath;
  RTL_QUERY_REGISTRY_TABLE queryTable[8];
  ULONG zero = 0;

  RtlZeroMemory(&traceEnable, sizeof(traceEnable));

  status = STATUS_SUCCESS;

  RtlInitUnicodeString(&traceRegistryPath, NULL);
  StrAppendStr(&status, &traceRegistryPath, pRegistryPath->Buffer, pRegistryPath->Length);
  StrAppendStr0(&status, &traceRegistryPath, L"\\Trace\\Enable");

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"QueryRegistryTraceEnable FAIL");
    return;
  }

  RtlZeroMemory(queryTable, sizeof(queryTable));

  queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[0].Name          = L"Read";
  queryTable[0].EntryContext  = &traceEnable.read;
  queryTable[0].DefaultType   = REG_DWORD;
  queryTable[0].DefaultData   = &zero;
  queryTable[0].DefaultLength = sizeof(ULONG);

  queryTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[1].Name          = L"Write";
  queryTable[1].EntryContext  = &traceEnable.write;
  queryTable[1].DefaultType   = REG_DWORD;
  queryTable[1].DefaultData   = &zero;
  queryTable[1].DefaultLength = sizeof(ULONG);

  queryTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[2].Name          = L"GetTimeouts";
  queryTable[2].EntryContext  = &traceEnable.getTimeouts;
  queryTable[2].DefaultType   = REG_DWORD;
  queryTable[2].DefaultData   = &zero;
  queryTable[2].DefaultLength = sizeof(ULONG);

  queryTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[3].Name          = L"SetTimeouts";
  queryTable[3].EntryContext  = &traceEnable.setTimeouts;
  queryTable[3].DefaultType   = REG_DWORD;
  queryTable[3].DefaultData   = &zero;
  queryTable[3].DefaultLength = sizeof(ULONG);

  queryTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[4].Name          = L"GetCommStatus";
  queryTable[4].EntryContext  = &traceEnable.getCommStatus;
  queryTable[4].DefaultType   = REG_DWORD;
  queryTable[4].DefaultData   = &zero;
  queryTable[4].DefaultLength = sizeof(ULONG);

  queryTable[5].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[5].Name          = L"GetModemStatus";
  queryTable[5].EntryContext  = &traceEnable.getModemStatus;
  queryTable[5].DefaultType   = REG_DWORD;
  queryTable[5].DefaultData   = &zero;
  queryTable[5].DefaultLength = sizeof(ULONG);

  queryTable[6].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  queryTable[6].Name          = L"ModemStatus";
  queryTable[6].EntryContext  = &traceEnable.modemStatus;
  queryTable[6].DefaultType   = REG_DWORD;
  queryTable[6].DefaultData   = &zero;
  queryTable[6].DefaultLength = sizeof(ULONG);

  status = RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      traceRegistryPath.Buffer,
      queryTable,
      NULL,
      NULL);

  StrFree(&traceRegistryPath);
}
/********************************************************************/

PTRACE_BUFFER AllocTraceBuf()
{
  PTRACE_BUFFER pBuf;
  KIRQL oldIrql;
  int i;

  pBuf = NULL;

  KeAcquireSpinLock(&bufsLock, &oldIrql);
  for (i = 0 ; i < TRACE_BUFS_NUM ; i++) {
    if (!traceBufs[i].busy) {
      traceBufs[i].busy = TRUE;
      pBuf = &traceBufs[i];
      break;
    }
  }
  KeReleaseSpinLock(&bufsLock, oldIrql);

  if (!pBuf)
    InterlockedIncrement(&skippedTraces);

  return pBuf;
}

VOID FreeTraceBuf(PTRACE_BUFFER pBuf)
{
  HALT_UNLESS(pBuf);
  pBuf->busy = FALSE;
}
/********************************************************************/

PCHAR AnsiStrCopyStr(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PCHAR pStr)
{
  SIZE_T len, size;
  PCHAR pStrTmp;

  pStrTmp = pStr;

  while (*(pStrTmp++))
    ;

  len = pStrTmp - pStr;
  size = *pSize;

  if (len > size)
    len = size;

  if (len) {
    RtlCopyMemory(pDestStr, pStr, len - 1);
    size -= len - 1;
    pDestStr += len - 1;
    *pDestStr = 0;
    *pSize = size;
  }
  return pDestStr;
}

PCHAR AnsiStrCopyStrW(
    PCHAR pDestStr,
    PSIZE_T pSize,
    PWCHAR pStr)
{
  SIZE_T len, size;
  PWCHAR pStrTmp;

  pStrTmp = pStr;

  while (*(pStrTmp++))
    ;

  len = pStrTmp - pStr;
  size = *pSize;

  if (len > size)
    len = size;

  if (len) {
    SIZE_T i;

    for (i = 0 ; i < len - 1 ; i++) {
      if ((pStr[i] & 0xFF00) == 0)
        pDestStr[i] = (CHAR)(pStr[i]);
      else
        pDestStr[i] = '?';
    }

    size -= len - 1;
    pDestStr += len - 1;
    *pDestStr = 0;
    *pSize = size;
  }
  return pDestStr;
}

PCHAR AnsiStrCopyNum(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN ULONG num,
    IN ULONG base,
    int width)
{
  CHAR buf[sizeof(num) * 8 + 1];
  PCHAR pBuf;

  if (!(base > 0 && base <= 36))
    base = 10;

  pBuf = buf + sizeof(buf);
  *(--pBuf) = 0;

  do {
    ULONG d = num % base;
    *(--pBuf) = (CHAR)(d + ((d < 10) ? '0' : ('A' - 10)));
    num /= base;
    width--;
  } while (num);

  while (width-- > 0)
    pDestStr = AnsiStrCopyStr(pDestStr, pSize, "0");

  return AnsiStrCopyStr(pDestStr, pSize, pBuf);
}

PCHAR _AnsiStrVaFormat(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PCHAR pFmt,
    va_list va)
{
  SIZE_T size;

  size = *pSize;

  while (*pFmt) {
    BOOLEAN format;
    BOOLEAN l;
    BOOLEAN fail;
    int width;
    CHAR ch;

    ch = *(pFmt++);

    if (ch != '%') {
      CHAR buf[2];

      buf[0] = ch;
      buf[1] = 0;
      pDestStr = AnsiStrCopyStr(pDestStr, &size, buf);
      continue;
    }

    fail = FALSE;
    format = TRUE;
    l = FALSE;
    width = 0;

    while (*pFmt && format) {
      ch = *(pFmt++);

      switch (ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          width = width*10 + ch - '0';
          break;
        case 'l':
          l = TRUE;
          break;
        case 's': {
          PCHAR pStr = va_arg(va, PCHAR);
          pDestStr = AnsiStrCopyStr(pDestStr, &size, pStr);
          format = FALSE;
          break;
        }
        case 'S': {
          PWCHAR pStr = va_arg(va, PWCHAR);
          pDestStr = AnsiStrCopyStrW(pDestStr, &size, pStr);
          format = FALSE;
          break;
        }
        case 'u': {
          ULONG n;
          if (l)
            n = va_arg(va, unsigned long);
          else
            n = va_arg(va, unsigned int);
          pDestStr = AnsiStrCopyNum(pDestStr, &size, n, 10, width);
          format = FALSE;
          break;
        }
        case 'x':
        case 'X': {
          ULONG n;
          if (l)
            n = va_arg(va, unsigned long);
          else
            n = va_arg(va, unsigned int);
          pDestStr = AnsiStrCopyNum(pDestStr, &size, n, 16, width);
          format = FALSE;
          break;
        }
        case '%':
          pDestStr = AnsiStrCopyStr(pDestStr, &size, "%");
          format = FALSE;
          break;
        default:
          fail = TRUE;
      }
      if (fail)
        break;
    }
    if (fail)
      break;
  }

  *pSize = size;
  return pDestStr;
}

PCHAR AnsiStrVaFormat(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PCHAR pFmt,
    va_list va)
{
  SIZE_T oldSize = *pSize;

  try {
    return _AnsiStrVaFormat(pDestStr, pSize, pFmt, va);
  } except (EXCEPTION_EXECUTE_HANDLER) {
    SysLog(pDrvObj, GetExceptionCode(), L"AnsiStrVaFormat EXCEPTION");
    *pSize = oldSize;
    return pDestStr;
  }
}

PCHAR AnsiStrFormat(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PCHAR pFmt,
    ...)
{
  va_list va;

  va_start(va, pFmt);
  pDestStr = AnsiStrVaFormat(pDestStr, pSize, pFmt, va);
  va_end(va);

  return pDestStr;
}
/********************************************************************/

PCHAR code2name(
    IN ULONG code,
    IN PCODE2NAME pTable)
{
  if (!pTable)
    return NULL;

  while (pTable->name) {
    if (pTable->code == code)
      return pTable->name;
    pTable++;
  }

  return NULL;
}

PCHAR AnsiStrCopyCode(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN ULONG code,
    IN PCODE2NAME pTable,
    IN PCHAR pPref,
    IN ULONG base)
{
  PCHAR pStr;

  pStr = code2name(code, pTable);

  if (pStr) {
    pDestStr = AnsiStrCopyStr(pDestStr, pSize, pStr);
  } else {
    if (pPref)
      pDestStr = AnsiStrCopyStr(pDestStr, pSize, pPref);
    pDestStr = AnsiStrCopyNum(pDestStr, pSize, code, base, 0);
  }

  return pDestStr;
}

PCHAR AnsiStrCopyMask(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PCODE2NAME pTable,
    IN ULONG mask)
{
  ULONG b, unknown;
  int count;

  pDestStr = AnsiStrCopyStr(pDestStr, pSize, "[");

  unknown = 0;
  count = 0;

  for (b = 1 ; b ; b <<= 1) {
    if ((mask & b) != 0) {
      PCHAR pStr;

      pStr = code2name(b, pTable);

      if (pStr) {
        if (count)
          pDestStr = AnsiStrCopyStr(pDestStr, pSize, "|");
        pDestStr = AnsiStrCopyStr(pDestStr, pSize, pStr);
        count++;
      } else {
        unknown |= b;
      }
    }
  }

  if (unknown) {
    if (count)
      pDestStr = AnsiStrCopyStr(pDestStr, pSize, "|");
    pDestStr = AnsiStrFormat(pDestStr, pSize, "0x%lX", (long)unknown);
  }

  return AnsiStrCopyStr(pDestStr, pSize, "]");
}

PCHAR AnsiStrCopyFields(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PFIELD2NAME pTable,
    IN ULONG mask)
{
  int count = 0;

  pDestStr = AnsiStrCopyStr(pDestStr, pSize, "[");

  if (pTable) {
    while (pTable->name) {
      ULONG m = (mask & pTable->mask);

      if (m == pTable->code) {
        mask &= ~pTable->mask;
        if (count)
          pDestStr = AnsiStrCopyStr(pDestStr, pSize, "|");
        pDestStr = AnsiStrCopyStr(pDestStr, pSize, pTable->name);
        count++;
      }
      pTable++;
    }
  }

  if (mask) {
    if (count)
      pDestStr = AnsiStrCopyStr(pDestStr, pSize, "|");
    pDestStr = AnsiStrFormat(pDestStr, pSize, "0x%lX", (long)mask);
  }

  return AnsiStrCopyStr(pDestStr, pSize, "]");
}

PCHAR AnsiStrCopyDump(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PVOID pData,
    IN SIZE_T length)
{
#define DUMP_MAX 16
  CHAR bufA[DUMP_MAX + 1];
  SIZE_T i;

  pDestStr = AnsiStrFormat(pDestStr, pSize, "%lu:", (long)length);

  for (i = 0 ; i < length && i < DUMP_MAX ; i++) {
    UCHAR c = *(((PUCHAR)pData) + i);

    bufA[i] = (CHAR)((c >= 0x20 && c < 0x7F) ? c : '.');

    pDestStr = AnsiStrFormat(pDestStr, pSize, " %02X", (int)c);
  }

  bufA[i] = 0;

  return AnsiStrFormat(pDestStr, pSize, " * %s *", bufA);
}
/********************************************************************/

PCHAR AnsiStrCopyHead(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead)
{
  if (pDevExt) {
    pDestStr = AnsiStrFormat(pDestStr, pSize, "%S/", pDevExt->portName);
    pDestStr = AnsiStrCopyCode(pDestStr, pSize, pDevExt->doType, codeNameTableDoType, NULL, 10);
  }
  if (pHead)
    pDestStr = AnsiStrFormat(pDestStr, pSize, pDevExt ? " %s" : "%s", pHead);

  return pDestStr;
}

PCHAR AnsiStrCopyHandFlow(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_HANDFLOW pHandFlow)
{
  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " Hand");
  pDestStr = AnsiStrCopyFields(pDestStr, pSize,
      codeNameTableControlHandShake,
      pHandFlow->ControlHandShake);

  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " Flow");
  pDestStr = AnsiStrCopyFields(pDestStr, pSize,
      codeNameTableFlowReplace,
      pHandFlow->FlowReplace);

  pDestStr = AnsiStrFormat(pDestStr, pSize, " XonLim=%lu XoffLim=%lu",
      (long)pHandFlow->XonLimit, (long)pHandFlow->XoffLimit);

  return pDestStr;
}

PCHAR AnsiStrCopyTimeouts(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_TIMEOUTS pTimeouts)
{
  return AnsiStrFormat(pDestStr, pSize,
      " Read[Interval=%lu Multiplier=%lu Constant=%lu]"
      " Write[Multiplier=%lu Constant=%lu]",
      (long)pTimeouts->ReadIntervalTimeout,
      (long)pTimeouts->ReadTotalTimeoutMultiplier,
      (long)pTimeouts->ReadTotalTimeoutConstant,
      (long)pTimeouts->WriteTotalTimeoutMultiplier,
      (long)pTimeouts->WriteTotalTimeoutConstant);
}

PCHAR AnsiStrCopyChars(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_CHARS pChars)
{
  return AnsiStrFormat(pDestStr, pSize,
      " EofChar=0x%X ErrorChar=0x%X BreakChar=0x%X"
      " EventChar=0x%X XonChar=0x%X XoffChar=0x%X",
      (int)pChars->EofChar, (int)pChars->ErrorChar, (int)pChars->BreakChar,
      (int)pChars->EventChar, (int)pChars->XonChar, (int)pChars->XoffChar);
}

PCHAR AnsiStrCopyLineControl(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_LINE_CONTROL pLineControl)
{
  return AnsiStrFormat(pDestStr, pSize,
      " StopBits=%u Parity=%u WordLength=%u",
      (int)pLineControl->StopBits,
      (int)pLineControl->Parity,
      (int)pLineControl->WordLength);
}

PCHAR AnsiStrCopyBaudRate(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_BAUD_RATE pBaudRate)
{
  return AnsiStrFormat(pDestStr, pSize, " BaudRate=%lu", (long)pBaudRate->BaudRate);
}

PCHAR AnsiStrCopyQueueSize(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_QUEUE_SIZE pQueueSize)
{
  return AnsiStrFormat(pDestStr, pSize,
      " InSize=%lu OutSize=%lu",
      (long)pQueueSize->InSize, (long)pQueueSize->OutSize);
}

PCHAR AnsiStrCopyCommStatus(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIAL_STATUS pCommStatus)
{
  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " {");

  if (pCommStatus->Errors) {
    pDestStr = AnsiStrCopyStr(pDestStr, pSize, " Errors");
    pDestStr = AnsiStrCopyMask(pDestStr, pSize,
        codeNameTableErrors, pCommStatus->Errors);
  }

  if (pCommStatus->HoldReasons) {
    pDestStr = AnsiStrCopyStr(pDestStr, pSize, " HoldReasons");
    pDestStr = AnsiStrCopyMask(pDestStr, pSize,
        codeNameTableHoldReasons, pCommStatus->HoldReasons);
  }

  if (pCommStatus->WaitForImmediate)
    pDestStr = AnsiStrCopyStr(pDestStr, pSize, " WaitForImmediate=TRUE");

  if (pCommStatus->AmountInInQueue)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " AmountInInQueue=%lu", (long)pCommStatus->AmountInInQueue);

  if (pCommStatus->AmountInOutQueue)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " AmountInOutQueue=%lu", (long)pCommStatus->AmountInOutQueue);

  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " }");
  return pDestStr;
}

PCHAR AnsiStrCopyPerfStats(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PSERIALPERF_STATS pPerfStats)
{
  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " {");

  if (pPerfStats->ReceivedCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " ReceivedCount=%lu", (long)pPerfStats->ReceivedCount);

  if (pPerfStats->TransmittedCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " TransmittedCount=%lu", (long)pPerfStats->TransmittedCount);

  if (pPerfStats->FrameErrorCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " FrameErrorCount=%lu", (long)pPerfStats->FrameErrorCount);

  if (pPerfStats->SerialOverrunErrorCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " SerialOverrunErrorCount=%lu", (long)pPerfStats->SerialOverrunErrorCount);

  if (pPerfStats->BufferOverrunErrorCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " BufferOverrunErrorCount=%lu", (long)pPerfStats->BufferOverrunErrorCount);

  if (pPerfStats->ParityErrorCount)
    pDestStr = AnsiStrFormat(pDestStr, pSize,
        " ParityErrorCount=%lu", (long)pPerfStats->ParityErrorCount);

  pDestStr = AnsiStrCopyStr(pDestStr, pSize, " }");
  return pDestStr;
}
/********************************************************************/

VOID GetTimeFields(PTIME_FIELDS pTimeFields)
{
  LARGE_INTEGER systemTime;

  KeQuerySystemTime(&systemTime);

#ifndef _WDMDDK_
  {
    LARGE_INTEGER localTime;
    ExSystemTimeToLocalTime(&systemTime, &localTime);
    systemTime = localTime;
  }
#else
  #define LOCAL_TIME_SHIFT (3*60*60)
  systemTime.QuadPart += (LONGLONG)(10000)*1000*LOCAL_TIME_SHIFT;
#endif

  RtlTimeToTimeFields(&systemTime, pTimeFields);
}

PCHAR AnsiStrCopyTimeFields(
    PCHAR pDestStr,
    PSIZE_T pSize,
    IN PTIME_FIELDS pTimeFields)
{
  return AnsiStrFormat(pDestStr, pSize,
      "%04u/%02u/%02u %02u:%02u:%02u.%03u",
      (unsigned)pTimeFields->Year,
      (unsigned)pTimeFields->Month,
      (unsigned)pTimeFields->Day,
      (unsigned)pTimeFields->Hour,
      (unsigned)pTimeFields->Minute,
      (unsigned)pTimeFields->Second,
      (unsigned)pTimeFields->Milliseconds);
}
/********************************************************************/

NTSTATUS TraceWrite(
    IN PVOID pIoObject,
    IN HANDLE handle,
    IN PCHAR pStr)
{
  NTSTATUS status;
  ANSI_STRING str;
  IO_STATUS_BLOCK ioStatusBlock;

  RtlInitAnsiString(&str, pStr);

  status = ZwWriteFile(
      handle,
      NULL,
      NULL,
      NULL,
      &ioStatusBlock,
      str.Buffer,
      str.Length,
      NULL,
      NULL);

  if (!NT_SUCCESS(status)) {
    errorCount++;
    SysLog(pIoObject, status, L"TraceWrite ZwWriteFile FAIL");
  }

  return status;
}

VOID TraceOutput(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pStr)
{
  NTSTATUS status;
  PVOID pIoObject;
  HANDLE handle;
  OBJECT_ATTRIBUTES objectAttributes;
  IO_STATUS_BLOCK ioStatusBlock;

  static CHAR strOld[500];
  static LONG strOldBusyInd = 0;
  static LONG strOldFreeInd = 0;

  if (errorCount > TRACE_ERROR_LIMIT) {
    if (errorCount < (TRACE_ERROR_LIMIT + 100)) {
      errorCount += 100;
      SysLog(pDrvObj, STATUS_SUCCESS, L"Trace disabled");
    }
    return;
  }

  if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
    SIZE_T size;
    KIRQL oldIrql;
    PCHAR pDestStr;
    TIME_FIELDS timeFields;

    if (!pStr)
      return;

    GetTimeFields(&timeFields);

    KeAcquireSpinLock(&strOldLock, &oldIrql);

    size = sizeof(strOld) - strOldFreeInd;
    pDestStr = strOld + (sizeof(strOld) - size);

    pDestStr = AnsiStrCopyTimeFields(pDestStr, &size, &timeFields);
    pDestStr = AnsiStrFormat(pDestStr, &size, " *%u* %s\r\n", (unsigned)KeGetCurrentIrql(), pStr);
    HALT_UNLESS3(size > 0, strOldFreeInd, strOldBusyInd, sizeof(strOld));

    if (size == 1) {
      pDestStr -= 6;
      size += 6;
      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ...\r\n");
    }

    strOldFreeInd = (LONG)(sizeof(strOld) - size);

    KeReleaseSpinLock(&strOldLock, oldIrql);

    return;
  }

  if (pDevExt)
    pIoObject = pDevExt->pDevObj;
  else
    pIoObject = pDrvObj;

  HALT_UNLESS(TRACE_FILE_OK);
  InitializeObjectAttributes(&objectAttributes, &traceFileName, 0, NULL, NULL);

  status = ZwCreateFile(
      &handle,
      SYNCHRONIZE | FILE_APPEND_DATA,
      &objectAttributes,
      &ioStatusBlock,
      NULL,
      FILE_ATTRIBUTE_NORMAL,
      FILE_SHARE_READ|FILE_SHARE_WRITE,
      FILE_OPEN_IF,
      FILE_SYNCHRONOUS_IO_NONALERT,
      NULL,
      0);

  if (NT_SUCCESS(status)) {
    PTRACE_BUFFER pBuf;

    pBuf = AllocTraceBuf();

    if (pBuf) {
      PCHAR pDestStr;
      SIZE_T size;
      LONG skipped;

      size = TRACE_BUF_SIZE;
      pDestStr = pBuf->buf;

      while (strOldFreeInd) {
        SIZE_T lenBuf;
        KIRQL oldIrql;

        KeAcquireSpinLock(&strOldLock, &oldIrql);
        lenBuf = strOldFreeInd - strOldBusyInd;
        if (lenBuf) {
          if (lenBuf > size - 1)
            lenBuf = size - 1;
          RtlCopyMemory(pDestStr, &strOld[strOldBusyInd], lenBuf);
          pDestStr[lenBuf] = 0;
          strOldBusyInd += (LONG)lenBuf;
          HALT_UNLESS3(strOldBusyInd <= strOldFreeInd, strOldFreeInd, strOldBusyInd, lenBuf);
          if (strOldBusyInd == strOldFreeInd)
            strOldFreeInd = strOldBusyInd = 0;
        }
        KeReleaseSpinLock(&strOldLock, oldIrql);

        if (lenBuf)
          TraceWrite(pIoObject, handle, pBuf->buf);
      }

      skipped = InterlockedExchange(&skippedTraces, 0);

      if (skipped) {
        SIZE_T tmp_size = size;

        AnsiStrFormat(pDestStr, &tmp_size, "*** skipped %lu lines ***\r\n", (long)skipped);
        TraceWrite(pIoObject, handle, pBuf->buf);
      }

      if (pStr) {
        TIME_FIELDS timeFields;

        GetTimeFields(&timeFields);

        pDestStr = AnsiStrCopyTimeFields(pDestStr, &size, &timeFields);
        pDestStr = AnsiStrFormat(pDestStr, &size, " %s\r\n", pStr);

        TraceWrite(pIoObject, handle, pBuf->buf);
      }

      FreeTraceBuf(pBuf);
    }

    status = ZwClose(handle);

    if (!NT_SUCCESS(status)) {
      errorCount++;
      SysLog(pIoObject, status, L"TraceOutput ZwClose FAIL");
    }
  }
  else {
    errorCount++;
    SysLog(pIoObject, status, L"TraceOutput ZwCreateFile FAIL");
  }
}
/********************************************************************/

VOID TraceF(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pFmt,
    ...)
{
  PTRACE_BUFFER pBuf;
  PCHAR pDestStr;
  SIZE_T size;
  va_list va;

  pBuf = AllocTraceBuf();
  if (!pBuf)
    return;
  size = TRACE_BUF_SIZE;
  pDestStr = pBuf->buf;

  pDestStr = AnsiStrCopyHead(pDestStr, &size, pDevExt, NULL);
  pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");

  va_start(va, pFmt);
  pDestStr = AnsiStrVaFormat(pDestStr, &size, pFmt, va);
  va_end(va);

  TraceOutput(pDevExt, pBuf->buf);
  FreeTraceBuf(pBuf);
}
/********************************************************************/

VOID TraceOpen(
    IN PDRIVER_OBJECT _pDrvObj,
    IN PUNICODE_STRING pRegistryPath)
{
  pDrvObj = _pDrvObj;

  KeInitializeSpinLock(&strOldLock);
  KeInitializeSpinLock(&bufsLock);
  skippedTraces = 0;
  RtlZeroMemory(traceBufs, sizeof(traceBufs));

  RtlInitUnicodeString(&traceFileName, NULL);
  errorCount = 0;

  QueryRegistryTrace(pRegistryPath);
  QueryRegistryTraceEnable(pRegistryPath);

  if (TRACE_FILE_OK) {
    UNICODE_STRING msg;
    NTSTATUS status;

    status = STATUS_SUCCESS;

    RtlInitUnicodeString(&msg, NULL);
    StrAppendStr0(&status, &msg, L"Trace enabled. See ");
    StrAppendStr(&status, &msg, traceFileName.Buffer, traceFileName.Length);

    if (NT_SUCCESS(status))
      SysLog(pDrvObj, status, msg.Buffer);

    StrFree(&msg);

    TraceF(NULL, "===== BEGIN =====");
    TraceF(NULL, "VERSION " C0C_VERSION_STR " (" __DATE__ " " __TIME__ ")");

    if (errorCount) {
      RtlInitUnicodeString(&traceFileName, NULL);
      SysLog(pDrvObj, STATUS_SUCCESS, L"Trace disabled");
    }
  }
}

VOID TraceClose()
{
  if (!TRACE_FILE_OK)
    return;

  TraceF(NULL, "===== END =====");

  RtlInitUnicodeString(&traceFileName, NULL);
}

VOID Trace0(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr)
{
  if (!TRACE_FILE_OK || !pStr)
    return;

  TraceF(pDevExt, "%S", pStr);
}

VOID Trace00(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr1,
    IN PWCHAR pStr2)
{
  if (!TRACE_FILE_OK || !pStr1 || !pStr2)
    return;

  TraceF(pDevExt, "%S%S", pStr1, pStr2);
}

VOID TraceCode(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG code,
    IN PNTSTATUS pStatus)
{
  PTRACE_BUFFER pBuf;
  PCHAR pDestStr;
  SIZE_T size;

  if (!TRACE_FILE_OK)
    return;

  pBuf = AllocTraceBuf();
  if (!pBuf)
    return;
  size = TRACE_BUF_SIZE;
  pDestStr = pBuf->buf;

  pDestStr = AnsiStrCopyHead(pDestStr, &size, pDevExt, pHead);
  pDestStr = AnsiStrCopyCode(pDestStr, &size, code, pTable, "0x", 16);

  if (pStatus) {
    pDestStr = AnsiStrCopyStr(pDestStr, &size, ", status=");
    pDestStr = AnsiStrCopyCode(pDestStr, &size, *pStatus, codeNameTableStatus, "0x", 16);
  }

  TraceOutput(pDevExt, pBuf->buf);
  FreeTraceBuf(pBuf);
}

VOID TraceMask(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG mask)
{
  PTRACE_BUFFER pBuf;
  PCHAR pDestStr;
  SIZE_T size;

  if (!TRACE_FILE_OK)
    return;

  pBuf = AllocTraceBuf();
  if (!pBuf)
    return;
  size = TRACE_BUF_SIZE;
  pDestStr = pBuf->buf;

  pDestStr = AnsiStrCopyHead(pDestStr, &size, pDevExt, pHead);
  pDestStr = AnsiStrCopyMask(pDestStr, &size, pTable, mask);

  TraceOutput(pDevExt, pBuf->buf);
  FreeTraceBuf(pBuf);
}

VOID TraceModemStatus(IN PC0C_IO_PORT pIoPort)
{
  if (!TRACE_FILE_OK)
    return;

  if (!traceEnable.modemStatus)
    return;

  TraceMask(
      (PC0C_COMMON_EXTENSION)pIoPort->pDevExt,
      "ModemStatus",
      codeNameTableModemStatus,
      pIoPort->modemStatus);
}

VOID TraceIrp(
    IN PCHAR pHead,
    IN PIRP pIrp,
    IN PNTSTATUS pStatus,
    IN ULONG flags)
{
  PTRACE_BUFFER pBuf;
  PCHAR pDestStr;
  SIZE_T size;
  PIO_STACK_LOCATION pIrpStack;
  PC0C_COMMON_EXTENSION pDevExt;
  PVOID pSysBuf;
  ULONG_PTR inform;
  ULONG major;
  ULONG enableMask;

  if (!TRACE_FILE_OK)
    return;

  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
  major = pIrpStack->MajorFunction;
  enableMask = TRACE_ENABLE_ALL;

  switch (major) {
    case IRP_MJ_WRITE:
      enableMask = traceEnable.write;
      break;
    case IRP_MJ_READ:
      enableMask = traceEnable.read;
      break;
    case IRP_MJ_DEVICE_CONTROL:
      switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_SERIAL_GET_TIMEOUTS:
          enableMask = traceEnable.getTimeouts;
          break;
        case IOCTL_SERIAL_SET_TIMEOUTS:
          enableMask = traceEnable.setTimeouts;
          break;
        case IOCTL_SERIAL_GET_COMMSTATUS:
          enableMask = traceEnable.getCommStatus;
          break;
        case IOCTL_SERIAL_GET_MODEMSTATUS:
          enableMask = traceEnable.getModemStatus;
          break;
      }
      break;
  }

  pDevExt = pIrpStack->DeviceObject->DeviceExtension;

  if (!(enableMask & TRACE_ENABLE_IRP)) {
    TraceOutput(pDevExt, NULL);
    return;
  }

  pBuf = AllocTraceBuf();
  if (!pBuf)
    return;
  size = TRACE_BUF_SIZE;
  pDestStr = pBuf->buf;

  pDestStr = AnsiStrCopyHead(pDestStr, &size, pDevExt, pHead);

  switch (major) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_PNP:
    case IRP_MJ_POWER:
    case IRP_MJ_SYSTEM_CONTROL:
      break;
    default:
      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size,
          major, codeNameTableIrpMj, "IRP_MJ_", 10);
  }

  pSysBuf = pIrp->AssociatedIrp.SystemBuffer;
  inform = pIrp->IoStatus.Information;

  switch (major) {
    case IRP_MJ_WRITE:
    case IRP_MJ_READ:
      if (flags & TRACE_FLAG_PARAMS) {
        ULONG length;

        if (major == IRP_MJ_WRITE)
          length = pIrpStack->Parameters.Write.Length;
        else
          length = pIrpStack->Parameters.Read.Length;
        pDestStr = AnsiStrFormat(pDestStr, &size, " length=%lu", (long)length);
      }
      if (flags & TRACE_FLAG_RESULTS) {
        pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
        if (enableMask & TRACE_ENABLE_DUMP)
          pDestStr = AnsiStrCopyDump(pDestStr, &size, pSysBuf, inform);
        else
          pDestStr = AnsiStrFormat(pDestStr, &size, "%lu:", (long)inform);
      }
      break;
    case IRP_MJ_DEVICE_CONTROL: {
      ULONG code = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
      ULONG inLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTableIoctl, "IOCTL_", 16);

      switch (code) {
        case IOCTL_SERIAL_GET_MODEMSTATUS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTableModemStatus, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_GET_MODEM_CONTROL:
        case IOCTL_SERIAL_GET_DTRRTS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTableModemControl, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_SET_WAIT_MASK:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTableWaitMask, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_GET_WAIT_MASK:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTableWaitMask, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_WAIT_ON_MASK:
          if ((flags & TRACE_FLAG_PARAMS) && pDevExt->doType == C0C_DOTYPE_FP) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size, codeNameTableWaitMask,
                ((PC0C_FDOPORT_EXTENSION)pDevExt)->pIoPortLocal->waitMask);
          }
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTableWaitMask, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_PURGE:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(ULONG)) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyMask(pDestStr, &size,
                codeNameTablePurgeMask, *((PULONG)pSysBuf));
          }
          break;
        case IOCTL_SERIAL_SET_HANDFLOW:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_HANDFLOW))
            pDestStr = AnsiStrCopyHandFlow(pDestStr, &size, (PSERIAL_HANDFLOW)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_HANDFLOW:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_HANDFLOW))
            pDestStr = AnsiStrCopyHandFlow(pDestStr, &size, (PSERIAL_HANDFLOW)pSysBuf);
          break;
        case IOCTL_SERIAL_SET_TIMEOUTS:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_TIMEOUTS))
            pDestStr = AnsiStrCopyTimeouts(pDestStr, &size, (PSERIAL_TIMEOUTS)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_TIMEOUTS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_TIMEOUTS))
            pDestStr = AnsiStrCopyTimeouts(pDestStr, &size, (PSERIAL_TIMEOUTS)pSysBuf);
          break;
        case IOCTL_SERIAL_SET_CHARS:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_CHARS))
            pDestStr = AnsiStrCopyChars(pDestStr, &size, (PSERIAL_CHARS)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_CHARS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_CHARS))
            pDestStr = AnsiStrCopyChars(pDestStr, &size, (PSERIAL_CHARS)pSysBuf);
          break;
        case IOCTL_SERIAL_SET_LINE_CONTROL:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_LINE_CONTROL))
            pDestStr = AnsiStrCopyLineControl(pDestStr, &size, (PSERIAL_LINE_CONTROL)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_LINE_CONTROL:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_LINE_CONTROL))
            pDestStr = AnsiStrCopyLineControl(pDestStr, &size, (PSERIAL_LINE_CONTROL)pSysBuf);
          break;
        case IOCTL_SERIAL_SET_BAUD_RATE:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_BAUD_RATE))
            pDestStr = AnsiStrCopyBaudRate(pDestStr, &size, (PSERIAL_BAUD_RATE)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_BAUD_RATE:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_BAUD_RATE))
            pDestStr = AnsiStrCopyBaudRate(pDestStr, &size, (PSERIAL_BAUD_RATE)pSysBuf);
          break;
        case IOCTL_SERIAL_SET_QUEUE_SIZE:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(SERIAL_QUEUE_SIZE))
            pDestStr = AnsiStrCopyQueueSize(pDestStr, &size, (PSERIAL_QUEUE_SIZE)pSysBuf);
          break;
        case IOCTL_SERIAL_GET_COMMSTATUS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIAL_STATUS))
            pDestStr = AnsiStrCopyCommStatus(pDestStr, &size, (PSERIAL_STATUS)pSysBuf);
          break;
        case IOCTL_SERIAL_LSRMST_INSERT:
          if ((flags & TRACE_FLAG_PARAMS) && inLength >= sizeof(UCHAR))
            pDestStr = AnsiStrFormat(pDestStr, &size, " escapeChar=0x%02X", (int)(*(PUCHAR)pSysBuf & 0xFF));
          break;
        case IOCTL_SERIAL_GET_STATS:
          if ((flags & TRACE_FLAG_RESULTS) && inform >= sizeof(SERIALPERF_STATS))
            pDestStr = AnsiStrCopyPerfStats(pDestStr, &size, (PSERIALPERF_STATS)pSysBuf);
          break;
        case IOCTL_SERIAL_IMMEDIATE_CHAR:
          if (flags & TRACE_FLAG_RESULTS) {
            pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
            pDestStr = AnsiStrCopyDump(pDestStr, &size, pSysBuf, inform);
          }
          break;
      }
      break;
    }
    case IRP_MJ_PNP: {
      ULONG code = pIrpStack->MinorFunction;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTablePnp, "PNP_", 10);

      switch (code) {
        case IRP_MN_QUERY_ID:
          pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
          pDestStr = AnsiStrCopyCode(pDestStr, &size,
              pIrpStack->Parameters.QueryId.IdType,
              codeNameTableBusQuery, "BusQuery", 10);
          if (flags & TRACE_FLAG_RESULTS && pIrp->IoStatus.Information) {
            pDestStr = AnsiStrFormat(pDestStr, &size,
                " Information: \"%S\"",
                (PWCHAR)pIrp->IoStatus.Information);
          }
          break;
        case IRP_MN_QUERY_DEVICE_TEXT:
          pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
          pDestStr = AnsiStrCopyCode(pDestStr, &size,
              pIrpStack->Parameters.QueryDeviceText.DeviceTextType,
              codeNameTableDeviceText, "DeviceText", 10);
          if (flags & TRACE_FLAG_RESULTS && pIrp->IoStatus.Information) {
            pDestStr = AnsiStrFormat(pDestStr, &size,
                " Information: \"%S\"",
                (PWCHAR)pIrp->IoStatus.Information);
          }
          break;
        case IRP_MN_QUERY_DEVICE_RELATIONS:
          pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
          pDestStr = AnsiStrCopyCode(pDestStr, &size,
              pIrpStack->Parameters.QueryDeviceRelations.Type,
              codeNameTableRelations, "Relations", 10);
          if (flags & TRACE_FLAG_RESULTS) {
            if (pIrp->IoStatus.Information)
              pDestStr = AnsiStrFormat(pDestStr, &size, " Count=%u",
                  (unsigned)((PDEVICE_RELATIONS)pIrp->IoStatus.Information)->Count);
          }
          break;
        case IRP_MN_QUERY_INTERFACE:
          pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
          pDestStr = AnsiStrFormat(pDestStr, &size,
              " GUID: %8lX-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X",
              (long)pIrpStack->Parameters.QueryInterface.InterfaceType->Data1,
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data2,
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data3,
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[0],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[1],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[2],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[3],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[4],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[5],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[6],
              (int)pIrpStack->Parameters.QueryInterface.InterfaceType->Data4[7]);
          break;
      }
      break;
    }
    case IRP_MJ_POWER: {
      ULONG code = pIrpStack->MinorFunction;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTablePower, "POWER_", 10);
      break;
    }
    case IRP_MJ_SYSTEM_CONTROL: {
      ULONG code = pIrpStack->MinorFunction;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTableWmi, "WMI_", 10);
      break;
    }
    case IRP_MJ_QUERY_INFORMATION: {
      ULONG code = pIrpStack->Parameters.QueryFile.FileInformationClass;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTableFileInformationClass, NULL, 10);
      break;
    }
    case IRP_MJ_SET_INFORMATION: {
      ULONG code = pIrpStack->Parameters.SetFile.FileInformationClass;

      pDestStr = AnsiStrCopyStr(pDestStr, &size, " ");
      pDestStr = AnsiStrCopyCode(pDestStr, &size, code, codeNameTableFileInformationClass, NULL, 10);
      break;
    }
  }

  if (pStatus) {
    pDestStr = AnsiStrCopyStr(pDestStr, &size, ", status=");
    pDestStr = AnsiStrCopyCode(pDestStr, &size, *pStatus, codeNameTableStatus, "0x", 16);
  }

  TraceOutput(pDevExt, pBuf->buf);
  FreeTraceBuf(pBuf);
}
/********************************************************************/

#else /* DBG */
  #pragma warning(disable:4206) // nonstandard extension used : translation unit is empty
#endif /* DBG */
