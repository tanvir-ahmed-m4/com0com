/*
 * $Id$
 *
 * Copyright (c) 2004-2009 Vyacheslav Frolov
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
 * Revision 1.19  2009/09/21 08:49:56  vfrolov
 * Added missing removing from queue
 * (Thanks Kirill Bagrinovsky)
 *
 * Revision 1.18  2007/06/20 10:37:47  vfrolov
 * Fixed double decrementing of amountInWriteQueue on CANCEL
 *
 * Revision 1.17  2007/06/04 15:24:33  vfrolov
 * Fixed open reject just after close in exclusiveMode
 *
 * Revision 1.16  2007/02/20 12:05:11  vfrolov
 * Implemented IOCTL_SERIAL_XOFF_COUNTER
 * Fixed cancel and timeout routines
 *
 * Revision 1.15  2007/01/22 17:05:16  vfrolov
 * Added missing IoMarkIrpPending()
 *
 * Revision 1.14  2007/01/15 16:09:16  vfrolov
 * Fixed non zero Information for IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.13  2006/06/28 13:52:09  vfrolov
 * Fixed double-release of spin lock
 *
 * Revision 1.12  2006/06/23 11:44:52  vfrolov
 * Mass replacement pDevExt by pIoPort
 *
 * Revision 1.11  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.10  2006/06/08 11:33:35  vfrolov
 * Fixed bugs with amountInWriteQueue
 *
 * Revision 1.9  2006/05/17 15:31:14  vfrolov
 * Implemented SERIAL_TRANSMIT_TOGGLE
 *
 * Revision 1.8  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.7  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.6  2005/11/29 16:16:46  vfrolov
 * Removed FdoPortCancelQueue()
 *
 * Revision 1.5  2005/09/13 14:56:16  vfrolov
 * Implemented IRP_MJ_FLUSH_BUFFERS
 *
 * Revision 1.4  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.3  2005/08/24 12:50:40  vfrolov
 * Fixed IRP processing order
 *
 * Revision 1.2  2005/07/14 13:51:09  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include "handflow.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 2

PC0C_IRP_STATE GetIrpState(IN PIRP pIrp)
{
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  switch (pIrpStack->MajorFunction) {
  case IRP_MJ_WRITE:
    return (PC0C_IRP_STATE)&pIrpStack->Parameters.Write.Key;
  case IRP_MJ_READ:
    return (PC0C_IRP_STATE)&pIrpStack->Parameters.Read.Key;
  case IRP_MJ_DEVICE_CONTROL:
    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SERIAL_WAIT_ON_MASK:
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
    case IOCTL_SERIAL_XOFF_COUNTER:
      return (PC0C_IRP_STATE)&pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    }
    break;
  case IRP_MJ_FLUSH_BUFFERS:
  case IRP_MJ_CLOSE:
    return (PC0C_IRP_STATE)&pIrpStack->Parameters.Others.Argument1;
  }

  return NULL;
}

VOID ShiftQueue(PC0C_IRP_QUEUE pQueue)
{
  if (pQueue->pCurrent) {
    PC0C_IRP_STATE pState;

    pState = GetIrpState(pQueue->pCurrent);

    HALT_UNLESS(pState);

    pQueue->pCurrent = NULL;
    pState->flags &= ~C0C_IRP_FLAG_IS_CURRENT;
  }

  if (!IsListEmpty(&pQueue->queue)) {
    PC0C_IRP_STATE pState;
    PIRP pIrp;
    PLIST_ENTRY pListEntry;

    pListEntry = RemoveHeadList(&pQueue->queue);
    pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

    pState = GetIrpState(pIrp);

    HALT_UNLESS(pState);

    pQueue->pCurrent = pIrp;
    pState->flags &= ~C0C_IRP_FLAG_IN_QUEUE;
    pState->flags |= C0C_IRP_FLAG_IS_CURRENT;
  }
}

VOID CancelRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  LIST_ENTRY queueToComplete;
  PC0C_IO_PORT pIoPort;
  PC0C_IRP_STATE pState;
  KIRQL oldIrql;
  PC0C_IRP_QUEUE pQueue;

  IoReleaseCancelSpinLock(pIrp->CancelIrql);

  pIoPort = FDO_PORT_TO_IO_PORT(pDevObj);
  pState = GetIrpState(pIrp);
  HALT_UNLESS(pState);

  pQueue = &pIoPort->irpQueues[pState->iQueue];

  InitializeListHead(&queueToComplete);

  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  if (pState->flags & C0C_IRP_FLAG_IN_QUEUE) {
    RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
    pState->flags &= ~C0C_IRP_FLAG_IN_QUEUE;
  }

  pIrp->IoStatus.Status = STATUS_CANCELLED;
  InsertTailList(&queueToComplete, &pIrp->Tail.Overlay.ListEntry);

  if (pState->flags & C0C_IRP_FLAG_IS_CURRENT) {
    ShiftQueue(pQueue);

    if (pState->iQueue == C0C_QUEUE_WRITE)
      ReadWrite(pIoPort->pIoPortRemote, FALSE, pIoPort, FALSE, &queueToComplete);
  }

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);
}

VOID CompleteIrp(PIRP pIrp, NTSTATUS status, PLIST_ENTRY pQueueToComplete)
{
  PDRIVER_CANCEL pCancelRoutine;

  #pragma warning(push, 3)
  pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);
  #pragma warning(pop)

  if (pCancelRoutine) {
    pIrp->IoStatus.Status = status;
    InsertTailList(pQueueToComplete, &pIrp->Tail.Overlay.ListEntry);
  }
}

VOID CancelQueue(PC0C_IRP_QUEUE pQueue, PLIST_ENTRY pQueueToComplete)
{
  while (pQueue->pCurrent) {
    PIRP pIrp;

    pIrp = pQueue->pCurrent;
    ShiftQueue(pQueue);

    CompleteIrp(pIrp, STATUS_CANCELLED, pQueueToComplete);
  }
}

VOID FdoPortCancelQueues(IN PC0C_IO_PORT pIoPort)
{
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;
  int i;

  InitializeListHead(&queueToComplete);
  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  for (i = 0 ; i < C0C_QUEUE_SIZE ; i++)
    CancelQueue(&pIoPort->irpQueues[i], &queueToComplete);

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);
  FdoPortCompleteQueue(&queueToComplete);
}

VOID FdoPortCompleteQueue(IN PLIST_ENTRY pQueueToComplete)
{
  while (!IsListEmpty(pQueueToComplete)) {
    PIRP pIrp;
    PC0C_IRP_STATE pState;
    PLIST_ENTRY pListEntry;
    PIO_STACK_LOCATION pIrpStack;

    pListEntry = RemoveHeadList(pQueueToComplete);
    pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    if (pIrp->IoStatus.Status == STATUS_TIMEOUT && pIrp->IoStatus.Information &&
        pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)
    {
      pIrp->IoStatus.Status = STATUS_SERIAL_COUNTER_TIMEOUT;
    }

    TraceIrp("complete", pIrp, &pIrp->IoStatus.Status, TRACE_FLAG_RESULTS);

    pState = GetIrpState(pIrp);
    HALT_UNLESS(pState);

    if (pState->iQueue == C0C_QUEUE_WRITE) {
      KIRQL oldIrql;
      PC0C_IO_PORT pIoPort;

      pIoPort = FDO_PORT_TO_IO_PORT(IoGetCurrentIrpStackLocation(pIrp)->DeviceObject);

      KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);
      pIoPort->amountInWriteQueue -=
          GetWriteLength(pIrp) - (ULONG)pIrp->IoStatus.Information;
      KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);
    }

    if (pIrp->IoStatus.Status == STATUS_CANCELLED ||
        (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR ||
                pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)))
    {
      pIrp->IoStatus.Information = 0;
    }

    IoCompleteRequest(pIrp, IO_SERIAL_INCREMENT);
  }
}

NTSTATUS NoPending(IN PIRP pIrp, NTSTATUS status)
{
  PDRIVER_CANCEL pCancelRoutine;

  #pragma warning(push, 3)
  pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);
  #pragma warning(pop)

  if (!pCancelRoutine) {
    IoMarkIrpPending(pIrp);
    return STATUS_PENDING;
  }

  return status;
}

NTSTATUS StartIrp(
    PC0C_IO_PORT pIoPort,
    PIRP pIrp,
    PC0C_IRP_STATE pState,
    PC0C_IRP_QUEUE pQueue,
    PLIST_ENTRY pQueueToComplete,
    PC0C_FDOPORT_START_ROUTINE pStartRoutine)
{
  NTSTATUS status;

  pQueue->pCurrent = pIrp;
  pState->flags |= C0C_IRP_FLAG_IS_CURRENT;

  if (pState->iQueue == C0C_QUEUE_WRITE) {
    ULONG length = GetWriteLength(pIrp);

    if (length) {
      pIoPort->amountInWriteQueue += length;
      UpdateTransmitToggle(pIoPort, pQueueToComplete);
    }
  }

  status = pStartRoutine(pIoPort, pQueueToComplete);

  if (status == STATUS_PENDING) {
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);
  } else {
    status = NoPending(pIrp, status);

    if (status != STATUS_PENDING) {
      PIO_STACK_LOCATION pIrpStack;

      if (pState->iQueue == C0C_QUEUE_WRITE) {
        pIoPort->amountInWriteQueue -=
            GetWriteLength(pIrp) - (ULONG)pIrp->IoStatus.Information;
      }

      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

      if (status == STATUS_CANCELLED ||
          (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
              (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR ||
                  pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)))
      {
        pIrp->IoStatus.Information = 0;
      }
    }

    if (pQueue->pCurrent == pIrp)
      ShiftQueue(pQueue);
  }

  return status;
}

NTSTATUS FdoPortStartIrp(
    IN PC0C_IO_PORT pIoPort,
    IN PIRP pIrp,
    IN UCHAR iQueue,
    IN PC0C_FDOPORT_START_ROUTINE pStartRoutine)
{
  NTSTATUS status;
  LIST_ENTRY queueToComplete;
  KIRQL oldIrql;
  PC0C_IRP_QUEUE pQueue;
  PC0C_IRP_STATE pState;

  InitializeListHead(&queueToComplete);
  pState = GetIrpState(pIrp);

  HALT_UNLESS(pState);

  pState->flags = 0;
  pState->iQueue = iQueue;

  pQueue = &pIoPort->irpQueues[iQueue];

  KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

  #pragma warning(push, 3)
  IoSetCancelRoutine(pIrp, CancelRoutine);
  #pragma warning(pop)

  if (pIrp->Cancel) {
    status = NoPending(pIrp, STATUS_CANCELLED);
  } else {
    if (!pQueue->pCurrent) {
      status = StartIrp(pIoPort, pIrp, pState, pQueue, &queueToComplete, pStartRoutine);
    } else {
      PIO_STACK_LOCATION pIrpStack;
      PIO_STACK_LOCATION pCurrentStack;

      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
      pCurrentStack = IoGetCurrentIrpStackLocation(pQueue->pCurrent);

      if (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
          pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_WAIT_ON_MASK)
      {
        status = NoPending(pIrp, STATUS_INVALID_PARAMETER);
      }
      else
      if (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
          pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR)
      {
        if (pCurrentStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            pCurrentStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR)
        {
          status = NoPending(pIrp, STATUS_INVALID_PARAMETER);
        } else {
          PC0C_IRP_STATE pCurrentState;

          pCurrentState = GetIrpState(pQueue->pCurrent);

          HALT_UNLESS(pCurrentState);

          pCurrentState->flags &= ~C0C_IRP_FLAG_IS_CURRENT;
          InsertHeadList(&pQueue->queue, &pQueue->pCurrent->Tail.Overlay.ListEntry);
          pCurrentState->flags |= C0C_IRP_FLAG_IN_QUEUE;

          status = StartIrp(pIoPort, pIrp, pState, pQueue, &queueToComplete, pStartRoutine);
        }
      }
      else {
        InsertTailList(&pQueue->queue, &pIrp->Tail.Overlay.ListEntry);
        pState->flags |= C0C_IRP_FLAG_IN_QUEUE;

        if (pState->iQueue == C0C_QUEUE_WRITE) {
          pIoPort->amountInWriteQueue += GetWriteLength(pIrp);
        }

        if (pCurrentStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            pCurrentStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER &&
            pQueue->pCurrent->IoStatus.Information)
        {
          if (pIrpStack->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
            pState->flags &= ~C0C_IRP_FLAG_IN_QUEUE;
            status = NoPending(pIrp, STATUS_SUCCESS);
          } else {
            PIRP pIrpXoffCounter = pQueue->pCurrent;

            ShiftQueue(pQueue);
            CompleteIrp(pIrpXoffCounter, STATUS_SERIAL_MORE_WRITES, &queueToComplete);

            status = StartIrp(pIoPort, pIrp, pState, pQueue, &queueToComplete, pStartRoutine);
          }
        } else {
          pIrp->IoStatus.Status = STATUS_PENDING;
          IoMarkIrpPending(pIrp);
          status = STATUS_PENDING;
        }
      }
    }
  }

  KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

  FdoPortCompleteQueue(&queueToComplete);

  return status;
}
