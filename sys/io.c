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
 * Revision 1.2  2005/02/01 08:36:27  vfrolov
 * Changed SetModemStatus() to set multiple bits and set CD to DSR
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "timeout.h"

#define GET_REST_BUFFER(pIrp) \
    (((PUCHAR)(pIrp)->AssociatedIrp.SystemBuffer) + (pIrp)->IoStatus.Information)

NTSTATUS ReadBuffer(PIRP pIrp, PC0C_BUFFER pBuf)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack;

  if (!pBuf->pBase)
    return STATUS_PENDING;

  status = STATUS_PENDING;
  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  for (;;) {
    ULONG length, writeLength, readLength;
    PVOID pWriteBuf, pReadBuf;

    readLength = pIrpStack->Parameters.Read.Length - pIrp->IoStatus.Information;

    if (!readLength) {
      status = STATUS_SUCCESS;
      break;
    }

    if (!pBuf->busy)
      break;

    writeLength = pBuf->pFree <= pBuf->pBusy ?
        pBuf->pEnd - pBuf->pBusy : pBuf->busy;

    pReadBuf = GET_REST_BUFFER(pIrp);
    pWriteBuf = pBuf->pBusy;

    length = writeLength < readLength ? writeLength : readLength;

    RtlCopyMemory(pReadBuf, pWriteBuf, length);

    pBuf->busy -= length;
    pBuf->pBusy += length;
    if (pBuf->pBusy == pBuf->pEnd)
      pBuf->pBusy = pBuf->pBase;

    pIrp->IoStatus.Information += length;
  }

  return status;
}

NTSTATUS WriteBuffer(PIRP pIrp, PC0C_BUFFER pBuf)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack;

  if (!pBuf->pBase)
    return STATUS_PENDING;

  status = STATUS_PENDING;
  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  for (;;) {
    ULONG length, writeLength, readLength;
    PVOID pWriteBuf, pReadBuf;

    writeLength = pIrpStack->Parameters.Write.Length - pIrp->IoStatus.Information;

    if (!writeLength) {
      status = STATUS_SUCCESS;
      break;
    }

    if ((ULONG)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pWriteBuf = GET_REST_BUFFER(pIrp);
    pReadBuf = pBuf->pFree;

    length = writeLength < readLength ? writeLength : readLength;

    RtlCopyMemory(pReadBuf, pWriteBuf, length);

    pBuf->busy += length;
    pBuf->pFree += length;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    pIrp->IoStatus.Information += length;
  }

  return status;
}

VOID ReadWriteDirect(
    PIRP pIrpLocal,
    PIRP pIrpRemote,
    PNTSTATUS pStatusLocal,
    PNTSTATUS pStatusRemote)
{
  PIO_STACK_LOCATION pIrpStackLocal;
  PIO_STACK_LOCATION pIrpStackRemote;
  ULONG length, writeLength, readLength;
  PVOID pWriteBuf, pReadBuf;

  pIrpStackLocal = IoGetCurrentIrpStackLocation(pIrpLocal);
  pIrpStackRemote = IoGetCurrentIrpStackLocation(pIrpRemote);

  if (pIrpStackLocal->MajorFunction == IRP_MJ_WRITE) {
    pWriteBuf = GET_REST_BUFFER(pIrpLocal);
    writeLength = pIrpStackLocal->Parameters.Write.Length - pIrpLocal->IoStatus.Information;
    pReadBuf = GET_REST_BUFFER(pIrpRemote);
    readLength = pIrpStackRemote->Parameters.Read.Length - pIrpRemote->IoStatus.Information;

    if (writeLength <= readLength)
      *pStatusLocal = STATUS_SUCCESS;
    if (writeLength >= readLength)
      *pStatusRemote = STATUS_SUCCESS;
  } else {
    pReadBuf = GET_REST_BUFFER(pIrpLocal);
    readLength = pIrpStackLocal->Parameters.Read.Length - pIrpLocal->IoStatus.Information;
    pWriteBuf = GET_REST_BUFFER(pIrpRemote);
    writeLength = pIrpStackRemote->Parameters.Write.Length - pIrpRemote->IoStatus.Information;

    if (readLength <= writeLength)
      *pStatusLocal = STATUS_SUCCESS;
    if (readLength >= writeLength)
      *pStatusRemote = STATUS_SUCCESS;
  }

  length = writeLength < readLength ? writeLength : readLength;

  if (length)
    RtlCopyMemory(pReadBuf, pWriteBuf, length);

  pIrpRemote->IoStatus.Information += length;
  pIrpLocal->IoStatus.Information += length;
}

NTSTATUS FdoPortIo(
    int ioType,
    PVOID pParam,
    PC0C_IO_PORT pIoPort,
    PC0C_IRP_QUEUE pQueue,
    PLIST_ENTRY pQueueToComplete)
{
  NTSTATUS status;
  BOOLEAN first;

  if (ioType == C0C_IO_TYPE_READ) {
    ASSERT(pParam);
    status = ReadBuffer((PIRP)pParam, &pIoPort->pDevExt->pIoPortRemote->readBuf);
  } else {
    status = STATUS_PENDING;
  }

  for (first = TRUE ; pQueue->pCurrent ; first = FALSE) {
    PIRP pIrpCurrent;
    PDRIVER_CANCEL pCancelRoutineCurrent;
    PC0C_IRP_STATE pStateCurrent;
    NTSTATUS statusCurrent;

    statusCurrent = STATUS_PENDING;

    pIrpCurrent = pQueue->pCurrent;

    pStateCurrent = GetIrpState(pIrpCurrent);
    ASSERT(pStateCurrent);

    #pragma warning(push, 3)
    pCancelRoutineCurrent = IoSetCancelRoutine(pIrpCurrent, NULL);
    #pragma warning(pop)

    if (!pCancelRoutineCurrent) {
      ShiftQueue(pQueue);
      continue;
    }

    switch (ioType) {
    case C0C_IO_TYPE_READ:
    case C0C_IO_TYPE_WRITE:
      ASSERT(pParam);
      if (status == STATUS_PENDING)
        ReadWriteDirect((PIRP)pParam, pIrpCurrent, &status, &statusCurrent);
      break;
    case C0C_IO_TYPE_WAIT_COMPLETE:
      ASSERT(pParam);
      *((PULONG)pIrpCurrent->AssociatedIrp.SystemBuffer) = *((PULONG)pParam);
      *((PULONG)pParam) = 0;
      pIrpCurrent->IoStatus.Information = sizeof(ULONG);
      statusCurrent = STATUS_SUCCESS;
      break;
    }

    if (IoGetCurrentIrpStackLocation(pIrpCurrent)->MajorFunction == IRP_MJ_WRITE &&
        statusCurrent == STATUS_PENDING)
      statusCurrent = WriteBuffer(pIrpCurrent, &pIoPort->pDevExt->pIoPortRemote->readBuf);

    if (statusCurrent == STATUS_PENDING) {
      if ((pStateCurrent->flags & C0C_IRP_FLAG_WAIT_ONE) != 0) {
        statusCurrent = STATUS_SUCCESS;
      }
      else
      if ((pStateCurrent->flags & C0C_IRP_FLAG_INTERVAL_TIMEOUT) != 0) {
        KeSetTimer(
            &pIoPort->timerReadInterval,
            pIoPort->timeoutInterval,
            &pIoPort->timerReadIntervalDpc);
      }
    }

    if (statusCurrent == STATUS_PENDING && !first)
      statusCurrent = FdoPortSetIrpTimeout(pIoPort->pDevExt, pIrpCurrent);

    if (statusCurrent == STATUS_PENDING) {
      #pragma warning(push, 3)
      IoSetCancelRoutine(pIrpCurrent, pCancelRoutineCurrent);
      #pragma warning(pop)
      if (pIrpCurrent->Cancel) {
        #pragma warning(push, 3)
        pCancelRoutineCurrent = IoSetCancelRoutine(pIrpCurrent, NULL);
        #pragma warning(pop)

        if (pCancelRoutineCurrent) {
          ShiftQueue(pQueue);
          pIrpCurrent->IoStatus.Status = STATUS_CANCELLED;
          pIrpCurrent->IoStatus.Information = 0;
          InsertTailList(pQueueToComplete, &pIrpCurrent->Tail.Overlay.ListEntry);
          continue;
        }
      }
      break;
    }

    ShiftQueue(pQueue);
    pIrpCurrent->IoStatus.Status = statusCurrent;
    InsertTailList(pQueueToComplete, &pIrpCurrent->Tail.Overlay.ListEntry);
  }

  if (ioType == C0C_IO_TYPE_WRITE && status == STATUS_PENDING) {
    ASSERT(pParam);
    status = WriteBuffer((PIRP)pParam, &pIoPort->readBuf);
  }

  return status;
}

VOID SetModemStatus(
    IN PC0C_IO_PORT pIoPort,
    IN ULONG bits,
    IN BOOLEAN set)
{
  ULONG modemStatusOld;
  ULONG modemStatusChanged;

  modemStatusOld = pIoPort->modemStatus;

  if (bits & C0C_MSB_DSR)
    bits |= C0C_MSB_RLSD;    /* CD = DSR */

  if (set)
    pIoPort->modemStatus |= bits;
  else
    pIoPort->modemStatus &= ~bits;

  TraceMask(
    (PC0C_COMMON_EXTENSION)pIoPort->pDevExt,
    "ModemStatus",
    codeNameTableModemStatus,
    pIoPort->modemStatus);

  modemStatusChanged = modemStatusOld ^ pIoPort->modemStatus;

  if (modemStatusChanged & C0C_MSB_CTS)
    pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_CTS;

  if (modemStatusChanged & C0C_MSB_DSR)
    pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_DSR;

  if (modemStatusChanged & C0C_MSB_RING)
    pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_RING;

  if (modemStatusChanged & C0C_MSB_RLSD)
    pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_RLSD;
}

VOID UpdateHandFlow(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PLIST_ENTRY pQueueToComplete)
{
  ULONG bits = 0;

  switch (pDevExt->handFlow.FlowReplace & SERIAL_RTS_MASK) {
  case SERIAL_RTS_CONTROL:
  case SERIAL_RTS_HANDSHAKE:
  case SERIAL_TRANSMIT_TOGGLE:
    bits |= C0C_MSB_CTS;
  }

  switch (pDevExt->handFlow.ControlHandShake & SERIAL_DTR_MASK) {
  case SERIAL_DTR_CONTROL:
  case SERIAL_DTR_HANDSHAKE:
    bits |= C0C_MSB_DSR;
  }

  if (bits)
    SetModemStatus(pDevExt->pIoPortRemote, bits, TRUE);

  WaitComplete(pDevExt->pIoPortRemote, pQueueToComplete);
}
