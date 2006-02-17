/*
 * $Id$
 *
 * Copyright (c) 2004-2006 Vyacheslav Frolov
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

NTSTATUS FdoPortOpen(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  LIST_ENTRY queueToComplete;
  PUCHAR pBase;
  ULONG size;
  KIRQL oldIrql;

  if (InterlockedIncrement(&pDevExt->openCount) != 1) {
    InterlockedDecrement(&pDevExt->openCount);
    return STATUS_ACCESS_DENIED;
  }

  if (!pDevExt->pIoPortRemote->pDevExt) {
    InterlockedDecrement(&pDevExt->openCount);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  switch (MmQuerySystemSize()) {
  case MmLargeSystem:
    size = 4096;
    pBase = (PUCHAR)ExAllocatePool(NonPagedPool, size);
    if (pBase)
      break;
  case MmMediumSystem:
    size = 1024;
    pBase = (PUCHAR)ExAllocatePool(NonPagedPool, size);
    if (pBase)
      break;
  case MmSmallSystem:
    size = 128;
    pBase = (PUCHAR)ExAllocatePool(NonPagedPool, size);
    if (pBase)
      break;
  default:
    size = 0;
    pBase = NULL;
  }

  InitializeListHead(&queueToComplete);

#if DBG
  if (pDevExt->pIoPortLocal->amountInWriteQueue) {
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"!!!WARNING!!! amountInWriteQueue != 0");
  }
#endif /* DBG */

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

  InitBuffer(&pDevExt->pIoPortLocal->readBuf, pBase, size);

  pDevExt->pIoPortLocal->tryWrite = FALSE;
  pDevExt->pIoPortLocal->errors = 0;
  pDevExt->pIoPortLocal->waitMask = 0;
  pDevExt->pIoPortLocal->eventMask = 0;
  pDevExt->pIoPortLocal->escapeChar = 0;
  RtlZeroMemory(&pDevExt->pIoPortLocal->perfStats, sizeof(pDevExt->pIoPortLocal->perfStats));
  pDevExt->handFlow.XoffLimit = size >> 3;
  pDevExt->handFlow.XonLimit = size >> 1;

  SetHandFlow(pDevExt, NULL, &queueToComplete);

  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);

  return STATUS_SUCCESS;
}

NTSTATUS FdoPortClose(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;

  InitializeListHead(&queueToComplete);

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

  pDevExt->pIoPortLocal->flipXoffLimit = FALSE;
  SetModemStatus(pDevExt->pIoPortRemote, 0, C0C_MSB_CTS | C0C_MSB_DSR, &queueToComplete);
  FreeBuffer(&pDevExt->pIoPortLocal->readBuf);
  SetBreakHolding(pDevExt->pIoPortLocal, FALSE);

  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);

  InterlockedDecrement(&pDevExt->openCount);

  return STATUS_SUCCESS;
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

  if (!NT_SUCCESS(status))
    TraceIrp("c0cOpen", pIrp, &status, TRACE_FLAG_RESULTS);

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS c0cClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

  TraceIrp("--- Close ---", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortClose((PC0C_FDOPORT_EXTENSION)pDevExt);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
  }

  pIrp->IoStatus.Information = 0;

  if (!NT_SUCCESS(status))
    TraceIrp("c0cClose", pIrp, &status, TRACE_FLAG_RESULTS);

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS c0cCleanup(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

  TraceIrp("c0cCleanup", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    FdoPortCancelQueues((PC0C_FDOPORT_EXTENSION)pDevExt);

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
