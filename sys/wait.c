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

NTSTATUS StartIrpWaitOnMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PLIST_ENTRY pQueueToComplete)
{
  UNREFERENCED_PARAMETER(pQueueToComplete);

  if (!pDevExt->pIoPortLocal->waitMask)
    return STATUS_INVALID_PARAMETER;

  if (pDevExt->pIoPortLocal->eventMask) {
    PIRP pIrp;

    pIrp = pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_WAIT].pCurrent;

    *((PULONG)pIrp->AssociatedIrp.SystemBuffer) = pDevExt->pIoPortLocal->eventMask;
    pDevExt->pIoPortLocal->eventMask = 0;
    pIrp->IoStatus.Information = sizeof(ULONG);
    return STATUS_SUCCESS;
  }

  return STATUS_PENDING;
}

NTSTATUS FdoPortWaitOnMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack)
{
  if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
    return STATUS_BUFFER_TOO_SMALL;

  return FdoPortStartIrp(pDevExt, pIrp, C0C_QUEUE_WAIT, StartIrpWaitOnMask);
}

NTSTATUS FdoPortSetWaitMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack)
{
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;
  PULONG pSysBuf;

  if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
    return STATUS_BUFFER_TOO_SMALL;

  pSysBuf = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

  if (*pSysBuf & ~(
      SERIAL_EV_RXCHAR   |
      SERIAL_EV_RXFLAG   |
      SERIAL_EV_TXEMPTY  |
      SERIAL_EV_CTS      |
      SERIAL_EV_DSR      |
      SERIAL_EV_RLSD     |
      SERIAL_EV_BREAK    |
      SERIAL_EV_ERR      |
      SERIAL_EV_RING     |
      SERIAL_EV_PERR     |
      SERIAL_EV_RX80FULL |
      SERIAL_EV_EVENT1   |
      SERIAL_EV_EVENT2
      ))
    return STATUS_INVALID_PARAMETER;

  InitializeListHead(&queueToComplete);

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
  FdoPortIo(
      C0C_IO_TYPE_WAIT_COMPLETE,
      &pDevExt->pIoPortLocal->eventMask,
      pDevExt->pIoPortLocal,
      &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_WAIT],
      &queueToComplete);

  pDevExt->pIoPortLocal->waitMask = *pSysBuf;
  pDevExt->pIoPortLocal->eventMask = 0;

  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
  FdoPortCompleteQueue(&queueToComplete);

  return STATUS_SUCCESS;
}

NTSTATUS FdoPortGetWaitMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack)
{
  KIRQL oldIrql;
  PULONG pSysBuf;

  if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
    return STATUS_BUFFER_TOO_SMALL;

  pSysBuf = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
  *pSysBuf = pDevExt->pIoPortLocal->waitMask;
  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

  pIrp->IoStatus.Information = sizeof(ULONG);

  return STATUS_SUCCESS;
}

VOID WaitComplete(
    IN PC0C_IO_PORT pIoPort,
    PLIST_ENTRY pQueueToComplete)
{
  if (pIoPort->eventMask) {
    FdoPortIo(
        C0C_IO_TYPE_WAIT_COMPLETE,
        &pIoPort->eventMask,
        pIoPort,
        &pIoPort->irpQueues[C0C_QUEUE_WAIT],
        pQueueToComplete);
  }
}
