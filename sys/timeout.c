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
 * Revision 1.8  2007/02/20 12:05:11  vfrolov
 * Implemented IOCTL_SERIAL_XOFF_COUNTER
 * Fixed cancel and timeout routines
 *
 * Revision 1.7  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.6  2006/06/08 11:33:35  vfrolov
 * Fixed bugs with amountInWriteQueue
 *
 * Revision 1.5  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.4  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.3  2005/08/16 16:36:33  vfrolov
 * Hidden timeout functions
 *
 * Revision 1.2  2005/07/14 13:51:09  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "timeout.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 3

VOID TimeoutRoutine(
    PC0C_IO_PORT pIoPort,
    IN PC0C_IRP_QUEUE pQueue)
{
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;

  InitializeListHead(&queueToComplete);

  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  if (pQueue->pCurrent) {
    PC0C_IRP_STATE pState;

    pState = GetIrpState(pQueue->pCurrent);
    HALT_UNLESS(pState);

    pState->flags |= C0C_IRP_FLAG_EXPIRED;

    if (pState->iQueue == C0C_QUEUE_WRITE)
      ReadWrite(pIoPort->pIoPortRemote, FALSE, pIoPort, FALSE, &queueToComplete);
    else
      ReadWrite(pIoPort, FALSE, pIoPort->pIoPortRemote, FALSE, &queueToComplete);
  }

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);
}

NTSTATUS SetReadTimeout(PC0C_IO_PORT pIoPort, PIRP pIrp)
{
  SERIAL_TIMEOUTS timeouts;
  BOOLEAN setTotal;
  ULONG multiplier;
  ULONG constant;
  PC0C_IRP_STATE pState;
  PC0C_FDOPORT_EXTENSION pDevExt;

  KeCancelTimer(&pIoPort->timerReadTotal);
  KeCancelTimer(&pIoPort->timerReadInterval);

  pState = GetIrpState(pIrp);
  HALT_UNLESS(pState);

  pDevExt = pIoPort->pDevExt;
  HALT_UNLESS(pDevExt);

  KeAcquireSpinLockAtDpcLevel(&pDevExt->controlLock);
  timeouts = pDevExt->timeouts;
  KeReleaseSpinLockFromDpcLevel(&pDevExt->controlLock);

  if (timeouts.ReadIntervalTimeout == MAXULONG &&
      !timeouts.ReadTotalTimeoutMultiplier &&
      !timeouts.ReadTotalTimeoutConstant) {
    return STATUS_SUCCESS;
  }

  setTotal = FALSE;
  multiplier = 0;
  constant = 0;

  if (timeouts.ReadIntervalTimeout == MAXULONG &&
      timeouts.ReadTotalTimeoutMultiplier == MAXULONG &&
      timeouts.ReadTotalTimeoutConstant < MAXULONG &&
      timeouts.ReadTotalTimeoutConstant > 0) {

    if (pIrp->IoStatus.Information) {
      return STATUS_SUCCESS;
    }

    pState->flags |= C0C_IRP_FLAG_WAIT_ONE;

    setTotal = TRUE;
    multiplier = 0;
    constant = timeouts.ReadTotalTimeoutConstant;
  } else {
    if (timeouts.ReadTotalTimeoutMultiplier || timeouts.ReadTotalTimeoutConstant) {
      setTotal = TRUE;
      multiplier = timeouts.ReadTotalTimeoutMultiplier;
      constant = timeouts.ReadTotalTimeoutConstant;
    }

    if (timeouts.ReadIntervalTimeout) {
      pState->flags |= C0C_IRP_FLAG_INTERVAL_TIMEOUT;

      pIoPort->timeoutInterval.QuadPart =
          ((LONGLONG)timeouts.ReadIntervalTimeout) * -10000;

      if (pIrp->IoStatus.Information)
        SetIntervalTimeout(pIoPort);
    }
  }

  if (setTotal) {
    LARGE_INTEGER total;
    ULONG length;

    length = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length;

    total.QuadPart = ((LONGLONG)(UInt32x32To64(length, multiplier) + constant)) * -10000;

    KeSetTimer(
        &pIoPort->timerReadTotal,
        total,
        &pIoPort->timerReadTotalDpc);
  }

  return STATUS_PENDING;
}

NTSTATUS SetWriteTimeout(PC0C_IO_PORT pIoPort, PIRP pIrp)
{
  SERIAL_TIMEOUTS timeouts;
  BOOLEAN setTotal;
  ULONG multiplier;
  ULONG constant;
  PC0C_FDOPORT_EXTENSION pDevExt;

  KeCancelTimer(&pIoPort->timerWriteTotal);

  pDevExt = pIoPort->pDevExt;
  HALT_UNLESS(pDevExt);

  KeAcquireSpinLockAtDpcLevel(&pDevExt->controlLock);
  timeouts = pDevExt->timeouts;
  KeReleaseSpinLockFromDpcLevel(&pDevExt->controlLock);

  setTotal = FALSE;
  multiplier = 0;
  constant = 0;

  if (timeouts.WriteTotalTimeoutMultiplier || timeouts.WriteTotalTimeoutConstant) {
    setTotal = TRUE;
    multiplier = timeouts.WriteTotalTimeoutMultiplier;
    constant = timeouts.WriteTotalTimeoutConstant;
  }

  if (setTotal) {
    LARGE_INTEGER total;
    ULONG length;

    length = GetWriteLength(pIrp);

    total.QuadPart = ((LONGLONG)(UInt32x32To64(length, multiplier) + constant)) * -10000;

    KeSetTimer(
        &pIoPort->timerWriteTotal,
        total,
        &pIoPort->timerWriteTotalDpc);
  }

  return STATUS_PENDING;
}

VOID SetXoffCounterTimeout(
    PC0C_IO_PORT pIoPort,
    PIRP pIrp)
{
  PSERIAL_XOFF_COUNTER pXoffCounter;
  LARGE_INTEGER total;

  KeCancelTimer(&pIoPort->timerWriteTotal);

  pXoffCounter = (PSERIAL_XOFF_COUNTER)pIrp->AssociatedIrp.SystemBuffer;

  if (pXoffCounter->Timeout) {
    total.QuadPart = ((LONGLONG)pXoffCounter->Timeout) * -10000;

    KeSetTimer(
        &pIoPort->timerWriteTotal,
        total,
        &pIoPort->timerWriteTotalDpc);
  }
}

VOID TimeoutReadTotal(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_IO_PORT pIoPort = (PC0C_IO_PORT)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pIoPort, &pIoPort->irpQueues[C0C_QUEUE_READ]);
}

VOID TimeoutReadInterval(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_IO_PORT pIoPort = (PC0C_IO_PORT)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pIoPort, &pIoPort->irpQueues[C0C_QUEUE_READ]);
}

VOID TimeoutWriteTotal(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_IO_PORT pIoPort = (PC0C_IO_PORT)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pIoPort, &pIoPort->irpQueues[C0C_QUEUE_WRITE]);
}

VOID AllocTimeouts(PC0C_IO_PORT pIoPort)
{
  KeInitializeTimer(&pIoPort->timerReadTotal);
  KeInitializeTimer(&pIoPort->timerReadInterval);
  KeInitializeTimer(&pIoPort->timerWriteTotal);

  KeInitializeDpc(&pIoPort->timerReadTotalDpc, TimeoutReadTotal, pIoPort);
  KeInitializeDpc(&pIoPort->timerReadIntervalDpc, TimeoutReadInterval, pIoPort);
  KeInitializeDpc(&pIoPort->timerWriteTotalDpc, TimeoutWriteTotal, pIoPort);
}

VOID FreeTimeouts(PC0C_IO_PORT pIoPort)
{
    KeCancelTimer(&pIoPort->timerReadTotal);
    KeCancelTimer(&pIoPort->timerReadInterval);
    KeCancelTimer(&pIoPort->timerWriteTotal);

    KeRemoveQueueDpc(&pIoPort->timerReadTotalDpc);
    KeRemoveQueueDpc(&pIoPort->timerReadIntervalDpc);
    KeRemoveQueueDpc(&pIoPort->timerWriteTotalDpc);
}

VOID SetIntervalTimeout(PC0C_IO_PORT pIoPort)
{
  KeSetTimer(&pIoPort->timerReadInterval, pIoPort->timeoutInterval, &pIoPort->timerReadIntervalDpc);
}

NTSTATUS SetIrpTimeout(
    PC0C_IO_PORT pIoPort,
    PIRP pIrp)
{
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  switch (pIrpStack->MajorFunction) {
  case IRP_MJ_WRITE:
    return SetWriteTimeout(pIoPort, pIrp);
  case IRP_MJ_DEVICE_CONTROL:
    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SERIAL_XOFF_COUNTER:
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
      return SetWriteTimeout(pIoPort, pIrp);
    }
    break;
  case IRP_MJ_READ:
    return SetReadTimeout(pIoPort, pIrp);
  }

  return STATUS_PENDING;
}

NTSTATUS FdoPortSetTimeouts(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack)
{
  KIRQL oldIrql;
  PSERIAL_TIMEOUTS pSysBuf;

  if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_TIMEOUTS))
    return STATUS_BUFFER_TOO_SMALL;

  pSysBuf = (PSERIAL_TIMEOUTS)pIrp->AssociatedIrp.SystemBuffer;

  if (pSysBuf->ReadIntervalTimeout == MAXULONG &&
      pSysBuf->ReadTotalTimeoutMultiplier == MAXULONG &&
      pSysBuf->ReadTotalTimeoutConstant == MAXULONG)
    return STATUS_INVALID_PARAMETER;

  KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
  pDevExt->timeouts = *pSysBuf;
  KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

  return STATUS_SUCCESS;
}

NTSTATUS FdoPortGetTimeouts(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack)
{
  KIRQL oldIrql;
  PSERIAL_TIMEOUTS pSysBuf;

  if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_TIMEOUTS))
    return STATUS_BUFFER_TOO_SMALL;

  pSysBuf = (PSERIAL_TIMEOUTS)pIrp->AssociatedIrp.SystemBuffer;

  KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
  *pSysBuf = pDevExt->timeouts;
  KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

  pIrp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

  return STATUS_SUCCESS;
}
