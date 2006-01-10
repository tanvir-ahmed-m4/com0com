/*
 * $Id$
 *
 * Copyright (c) 2005-2006 Vyacheslav Frolov
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
 * Revision 1.2  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.1  2005/08/23 15:30:22  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "delay.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 7

VOID WriteDelayRoutine(
    IN PKDPC pDpc,
    IN PVOID deferredContext,
    IN PVOID systemArgument1,
    IN PVOID systemArgument2)
{
  PC0C_FDOPORT_EXTENSION pDevExt;
  PC0C_ADAPTIVE_DELAY pWriteDelay;

  UNREFERENCED_PARAMETER(pDpc);
  UNREFERENCED_PARAMETER(systemArgument1);
  UNREFERENCED_PARAMETER(systemArgument2);

  pDevExt = (PC0C_FDOPORT_EXTENSION)deferredContext;
  pWriteDelay = pDevExt->pIoPortLocal->pWriteDelay;

  if (pWriteDelay) {
    LIST_ENTRY queueToComplete;
    KIRQL oldIrql;

    InitializeListHead(&queueToComplete);

    KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

    if (pWriteDelay->started) {
      NTSTATUS status;

      status = ReadWrite(
          pDevExt->pIoPortRemote, FALSE,
          pDevExt->pIoPortLocal, FALSE,
          &queueToComplete);

      if (status != STATUS_PENDING)
        StopWriteDelayTimer(pWriteDelay);
    }

    KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

    FdoPortCompleteQueue(&queueToComplete);
  }
}

SIZE_T GetWriteLimit(PC0C_ADAPTIVE_DELAY pWriteDelay)
{
  ULONGLONG curTime;
  ULONGLONG maxFrames;

  HALT_UNLESS(pWriteDelay);

  curTime = KeQueryInterruptTime();

  HALT_UNLESS(pWriteDelay->params.decibits_per_frame);

  maxFrames = ((curTime - pWriteDelay->startTime) * pWriteDelay->params.baudRate)/
          (pWriteDelay->params.decibits_per_frame * 1000000L);

  if (maxFrames < pWriteDelay->sentFrames)
    return 0;

  return (SIZE_T)(maxFrames - pWriteDelay->sentFrames);
}

NTSTATUS AllocWriteDelay(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  PC0C_ADAPTIVE_DELAY pWriteDelay;

  pWriteDelay = (PC0C_ADAPTIVE_DELAY)ExAllocatePool(NonPagedPool, sizeof(*pWriteDelay));

  if (!pWriteDelay)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(pWriteDelay, sizeof(*pWriteDelay));

  KeInitializeTimer(&pWriteDelay->timer);
  KeInitializeDpc(&pWriteDelay->timerDpc, WriteDelayRoutine, pDevExt);

  pDevExt->pIoPortLocal->pWriteDelay = pWriteDelay;

  return STATUS_SUCCESS;
}

VOID FreeWriteDelay(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  PC0C_ADAPTIVE_DELAY pWriteDelay;

  pWriteDelay = pDevExt->pIoPortLocal->pWriteDelay;

  if (pWriteDelay) {
    pDevExt->pIoPortLocal->pWriteDelay = NULL;
    StopWriteDelayTimer(pWriteDelay);
    ExFreePool(pWriteDelay);
  }
}

VOID SetWriteDelay(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  PC0C_ADAPTIVE_DELAY pWriteDelay;
  KIRQL oldIrql;
  C0C_DELAY_PARAMS params;
  SERIAL_LINE_CONTROL lineControl;

  pWriteDelay = pDevExt->pIoPortLocal->pWriteDelay;

  if (!pWriteDelay)
    return;

  KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

  KeAcquireSpinLockAtDpcLevel(&pDevExt->controlLock);
  lineControl = pDevExt->lineControl;
  params.baudRate = pDevExt->baudRate.BaudRate;
  KeReleaseSpinLockFromDpcLevel(&pDevExt->controlLock);

  /* Startbit + WordLength */
  params.decibits_per_frame = (1 + lineControl.WordLength) * 10;

  switch (lineControl.Parity) {
  case NO_PARITY:
    break;
  default:
  case ODD_PARITY:
  case EVEN_PARITY:
  case MARK_PARITY:
  case SPACE_PARITY:
    params.decibits_per_frame += 10;
    break;
  }

  switch (lineControl.StopBits) {
  default:
  case STOP_BIT_1:
    params.decibits_per_frame += 10;
    break;
  case STOP_BITS_1_5:
    params.decibits_per_frame += 15;
    break;
  case STOP_BITS_2:
    params.decibits_per_frame += 30;
    break;
  }

  if (pWriteDelay->params.baudRate != params.baudRate ||
      pWriteDelay->params.decibits_per_frame != params.decibits_per_frame)
  {
    pWriteDelay->params = params;
    if (pWriteDelay->started) {
      StopWriteDelayTimer(pWriteDelay);
      StartWriteDelayTimer(pWriteDelay);
    }
  }

  KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
}

VOID StartWriteDelayTimer(PC0C_ADAPTIVE_DELAY pWriteDelay)
{
  LARGE_INTEGER dueTime;
  ULONG period;
  ULONG intervals_100ns;

  if (pWriteDelay->started)
    return;

  if (!pWriteDelay->params.baudRate)
    return;

  pWriteDelay->startTime = KeQueryInterruptTime();
  pWriteDelay->sentFrames = 0;

  /* 100-nanosecond intervals per frame */
  intervals_100ns = (pWriteDelay->params.decibits_per_frame * 1000000L)/pWriteDelay->params.baudRate;

  if (!intervals_100ns)
    intervals_100ns = 1;

  period = intervals_100ns/10000;  /* 1-millisecond intervals per frame */

  dueTime.QuadPart = -(LONGLONG)intervals_100ns;

  if (!period)
    period = 1;

  KeSetTimerEx(
      &pWriteDelay->timer,
      dueTime, period,
      &pWriteDelay->timerDpc);

  pWriteDelay->started = TRUE;
}

VOID StopWriteDelayTimer(PC0C_ADAPTIVE_DELAY pWriteDelay)
{
  pWriteDelay->started = FALSE;
  KeCancelTimer(&pWriteDelay->timer);
  KeRemoveQueueDpc(&pWriteDelay->timerDpc);
}
