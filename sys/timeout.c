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
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PC0C_IRP_QUEUE pQueue)
{
  KIRQL oldIrql;
  PIRP pIrp;

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

  pIrp = pQueue->pCurrent;

  if (pIrp) {
    PDRIVER_CANCEL pCancelRoutine;

    #pragma warning(push, 3)
    pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);
    #pragma warning(pop)

    if (pCancelRoutine) {
      ShiftQueue(pQueue);
      if (pQueue->pCurrent)
        FdoPortSetIrpTimeout(pDevExt, pQueue->pCurrent);
    } else {
      pIrp = NULL;
    }
  }

  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

  if (pIrp) {
    TraceIrp("timeout", pIrp, NULL, TRACE_FLAG_RESULTS);

    pIrp->IoStatus.Status = STATUS_TIMEOUT;
    IoCompleteRequest(pIrp, IO_SERIAL_INCREMENT);
  }
}

NTSTATUS SetReadTimeout(IN PC0C_FDOPORT_EXTENSION pDevExt, PIRP pIrp)
{
  SERIAL_TIMEOUTS timeouts;
  KIRQL oldIrql;
  BOOLEAN setTotal;
  ULONG multiplier;
  ULONG constant;
  PC0C_IRP_STATE pState;

  KeCancelTimer(&pDevExt->pIoPortLocal->timerReadTotal);
  KeCancelTimer(&pDevExt->pIoPortLocal->timerReadInterval);

  pState = GetIrpState(pIrp);
  HALT_UNLESS(pState);

  KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
  timeouts = pDevExt->timeouts;
  KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

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

      pDevExt->pIoPortLocal->timeoutInterval.QuadPart = 
          ((LONGLONG)timeouts.ReadIntervalTimeout) * -10000;

      if (pIrp->IoStatus.Information)
        KeSetTimer(
            &pDevExt->pIoPortLocal->timerReadInterval,
            pDevExt->pIoPortLocal->timeoutInterval,
            &pDevExt->pIoPortLocal->timerReadIntervalDpc);
    }
  }

  if (setTotal) {
    LARGE_INTEGER total;
    ULONG length;

    length = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length;

    total.QuadPart = ((LONGLONG)(UInt32x32To64(length, multiplier) + constant)) * -10000;

    KeSetTimer(
        &pDevExt->pIoPortLocal->timerReadTotal,
        total,
        &pDevExt->pIoPortLocal->timerReadTotalDpc);
  }

  return STATUS_PENDING;
}

NTSTATUS SetWriteTimeout(IN PC0C_FDOPORT_EXTENSION pDevExt, PIRP pIrp)
{
  SERIAL_TIMEOUTS timeouts;
  KIRQL oldIrql;
  BOOLEAN setTotal;
  ULONG multiplier;
  ULONG constant;

  KeCancelTimer(&pDevExt->pIoPortLocal->timerWriteTotal);

  KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
  timeouts = pDevExt->timeouts;
  KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

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

    length = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length;

    total.QuadPart = ((LONGLONG)(UInt32x32To64(length, multiplier) + constant)) * -10000;

    KeSetTimer(
        &pDevExt->pIoPortLocal->timerWriteTotal,
        total,
        &pDevExt->pIoPortLocal->timerWriteTotalDpc);
  }

  return STATUS_PENDING;
}

VOID TimeoutReadTotal(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_FDOPORT_EXTENSION pDevExt = (PC0C_FDOPORT_EXTENSION)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pDevExt, &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_READ]);
}

VOID TimeoutReadInterval(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_FDOPORT_EXTENSION pDevExt = (PC0C_FDOPORT_EXTENSION)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pDevExt, &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_READ]);
}

VOID TimeoutWriteTotal(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_FDOPORT_EXTENSION pDevExt = (PC0C_FDOPORT_EXTENSION)deferredContext;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  TimeoutRoutine(pDevExt, &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_WRITE]);
}

NTSTATUS FdoPortSetIrpTimeout(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    PIRP pIrp)
{
  switch (IoGetCurrentIrpStackLocation(pIrp)->MajorFunction) {
  case IRP_MJ_WRITE:
    return SetWriteTimeout(pDevExt, pIrp);
  case IRP_MJ_READ:
    return SetReadTimeout(pDevExt, pIrp);
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
