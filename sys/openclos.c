/*
 * $Id$
 *
 * Copyright (c) 2004-2008 Vyacheslav Frolov
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
 * Revision 1.21  2008/03/14 15:28:39  vfrolov
 * Implemented ability to get paired port settings with
 * extended IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.20  2007/09/17 14:31:06  vfrolov
 * Implemented pseudo pin OPEN
 *
 * Revision 1.19  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.18  2007/06/04 15:24:32  vfrolov
 * Fixed open reject just after close in exclusiveMode
 *
 * Revision 1.17  2007/06/01 16:22:40  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.16  2007/01/11 14:50:29  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
 * Revision 1.15  2006/06/23 11:44:52  vfrolov
 * Mass replacement pDevExt by pIoPort
 *
 * Revision 1.14  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.13  2006/06/08 11:33:35  vfrolov
 * Fixed bugs with amountInWriteQueue
 *
 * Revision 1.12  2006/04/05 07:22:15  vfrolov
 * Replaced flipXoffLimit flag by writeHoldingRemote to correct handFlow changing
 *
 * Revision 1.11  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
 * Revision 1.10  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.9  2005/11/30 16:04:12  vfrolov
 * Implemented IOCTL_SERIAL_GET_STATS and IOCTL_SERIAL_CLEAR_STATS
 *
 * Revision 1.8  2005/11/29 12:33:21  vfrolov
 * Changed SetModemStatus() to ability set and clear bits simultaneously
 *
 * Revision 1.7  2005/11/28 12:57:16  vfrolov
 * Moved some C0C_BUFFER code to bufutils.c
 *
 * Revision 1.6  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.5  2005/05/14 17:07:02  vfrolov
 * Implemented SERIAL_LSRMST_MST insertion
 *
 * Revision 1.4  2005/05/13 16:58:03  vfrolov
 * Implemented IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.3  2005/02/01 16:51:51  vfrolov
 * Used C0C_BUFFER_PURGE()
 *
 * Revision 1.2  2005/02/01 08:37:55  vfrolov
 * Changed SetModemStatus() to set multiple bits
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "handflow.h"
#include "bufutils.h"
#include "strutils.h"
#include "timeout.h"

NTSTATUS FdoPortOpen(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  LIST_ENTRY queueToComplete;
  PUCHAR pBase;
  ULONG size;
  KIRQL oldIrql;
  PC0C_IO_PORT pIoPort;

  if (InterlockedIncrement(&pDevExt->openCount) != 1) {
    InterlockedDecrement(&pDevExt->openCount);
    return STATUS_ACCESS_DENIED;
  }

  pIoPort = pDevExt->pIoPortLocal;

  if (pIoPort->plugInMode && !pIoPort->pIoPortRemote->isOpen) {
    InterlockedDecrement(&pDevExt->openCount);
    return STATUS_ACCESS_DENIED;
  }

  if (pIoPort->exclusiveMode)
    IoInvalidateDeviceRelations(pIoPort->pPhDevObj, BusRelations);

  switch (MmQuerySystemSize()) {
  case MmLargeSystem:
    size = 4096;
    pBase = (PUCHAR)C0C_ALLOCATE_POOL(NonPagedPool, size);
    if (pBase)
      break;
  case MmMediumSystem:
    size = 1024;
    pBase = (PUCHAR)C0C_ALLOCATE_POOL(NonPagedPool, size);
    if (pBase)
      break;
  case MmSmallSystem:
    size = 128;
    pBase = (PUCHAR)C0C_ALLOCATE_POOL(NonPagedPool, size);
    if (pBase)
      break;
  default:
    size = 0;
    pBase = NULL;
  }

  InitializeListHead(&queueToComplete);

#if DBG
  if (pIoPort->amountInWriteQueue) {
    NTSTATUS status;
    UNICODE_STRING msg;

    status = STATUS_SUCCESS;
    RtlInitUnicodeString(&msg, NULL);
    StrAppendStr0(&status, &msg, L"!!!WARNING!!! amountInWriteQueue = ");
    StrAppendNum(&status, &msg, pIoPort->amountInWriteQueue, 10);

    Trace0((PC0C_COMMON_EXTENSION)pDevExt, msg.Buffer);

    StrFree(&msg);
  }
#endif /* DBG */

  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  InitBuffer(&pIoPort->readBuf, pBase, size);

  pIoPort->amountInWriteQueue = 0;
  pIoPort->tryWrite = FALSE;
  pIoPort->errors = 0;
  pIoPort->waitMask = 0;
  pIoPort->eventMask = 0;
  RtlZeroMemory(&pIoPort->perfStats, sizeof(pIoPort->perfStats));
  pIoPort->handFlow.XoffLimit = size >> 3;
  pIoPort->handFlow.XonLimit = size >> 1;

  SetHandFlow(pIoPort, NULL, &queueToComplete);
  SetModemControl(pIoPort, C0C_MCR_OPEN, C0C_MCR_OPEN, &queueToComplete);

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

  pIoPort->isOpen = TRUE;

  if (pIoPort->pIoPortRemote->plugInMode)
    IoInvalidateDeviceRelations(pIoPort->pIoPortRemote->pPhDevObj, BusRelations);

  FdoPortCompleteQueue(&queueToComplete);

  return STATUS_SUCCESS;
}

NTSTATUS StartIrpClose(
    IN PC0C_IO_PORT pIoPort,
    IN PLIST_ENTRY pQueueToComplete)
{
  UNREFERENCED_PARAMETER(pQueueToComplete);

  if (!pIoPort->exclusiveMode) {
    PIRP pIrp;

    InterlockedDecrement(&pIoPort->pDevExt->openCount);
    pIrp = pIoPort->irpQueues[C0C_QUEUE_CLOSE].pCurrent;
    pIrp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
  }

  IoInvalidateDeviceRelations(pIoPort->pPhDevObj, BusRelations);
  SetCloseTimeout(pIoPort);

  return STATUS_PENDING;
}

NTSTATUS FdoPortClose(IN PC0C_FDOPORT_EXTENSION pDevExt, IN PIRP pIrp)
{
  NTSTATUS status;
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;
  PC0C_IO_PORT pIoPort;

  pIoPort = pDevExt->pIoPortLocal;

  pIoPort->isOpen = FALSE;

  if (pIoPort->pIoPortRemote->plugInMode)
    IoInvalidateDeviceRelations(pIoPort->pIoPortRemote->pPhDevObj, BusRelations);

  InitializeListHead(&queueToComplete);

  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  pIoPort->escapeChar = 0;
  pIoPort->writeHoldingRemote = 0;
  pIoPort->sendXonXoff = 0;
  SetModemControl(pIoPort, 0, C0C_MCR_RTS | C0C_MCR_DTR | C0C_MCR_OPEN, &queueToComplete);
  FreeBuffer(&pIoPort->readBuf);
  SetBreakHolding(pIoPort, FALSE);

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);

  status = FdoPortStartIrp(pIoPort, pIrp, C0C_QUEUE_CLOSE, StartIrpClose);

  if (status != STATUS_PENDING) {
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}

NTSTATUS c0cOpen(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

  TraceIrp("--- Open ---", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortOpen((PC0C_FDOPORT_EXTENSION)pDevExt);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
  }

  pIrp->IoStatus.Information = 0;

#if DBG
  if (!NT_SUCCESS(status))
    TraceIrp("c0cOpen", pIrp, &status, TRACE_FLAG_RESULTS);
#endif /* DBG */

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS c0cClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

#if DBG
  ULONG code = IoGetCurrentIrpStackLocation(pIrp)->MajorFunction;
#endif /* DBG */

  TraceIrp("--- Close ---", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortClose((PC0C_FDOPORT_EXTENSION)pDevExt, pIrp);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

#if DBG
  if (status != STATUS_SUCCESS)
    TraceCode(pDevExt, "IRP_MJ_", codeNameTableIrpMj, code, &status);
#endif /* DBG */

  return status;
}

NTSTATUS c0cCleanup(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

  TraceIrp("c0cCleanup", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    FdoPortCancelQueues(((PC0C_FDOPORT_EXTENSION)pDevExt)->pIoPortLocal);

    status = STATUS_SUCCESS;
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
  }

  pIrp->IoStatus.Information = 0;

  TraceIrp("c0cCleanup", pIrp, &status, TRACE_FLAG_RESULTS);

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}
