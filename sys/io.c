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
 * Revision 1.5  2005/05/14 17:07:02  vfrolov
 * Implemented SERIAL_LSRMST_MST insertion
 *
 * Revision 1.4  2005/05/13 16:58:03  vfrolov
 * Implemented IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.3  2005/05/13 06:32:16  vfrolov
 * Implemented SERIAL_EV_RXCHAR
 *
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

VOID CompactRawData(PC0C_RAW_DATA pRawData, ULONG writeDone)
{
  if (writeDone) {
    pRawData->size = (UCHAR)(pRawData->size - writeDone);

    if (pRawData->size) {
      ASSERT((pRawData->size + writeDone) <= sizeof(pRawData->data));

      RtlMoveMemory(pRawData->data, pRawData->data + writeDone, pRawData->size);
    }
  }
}

NTSTATUS MoveRawData(PC0C_RAW_DATA pDstRawData, PC0C_RAW_DATA pSrcRawData)
{
  int free;

  if (!pSrcRawData->size)
    return STATUS_SUCCESS;

  free = sizeof(pDstRawData->data) - pDstRawData->size;

  if (free > 0) {
    ULONG length;

    if (free > pSrcRawData->size)
      length = pSrcRawData->size;
    else
      length = free;

    ASSERT((pDstRawData->size + length) <= sizeof(pDstRawData->data));

    RtlCopyMemory(pDstRawData->data + pDstRawData->size, pSrcRawData->data, length);
    pDstRawData->size = (UCHAR)(pDstRawData->size + length);
    CompactRawData(pSrcRawData, length);
  }
  return pSrcRawData->size ? STATUS_PENDING : STATUS_SUCCESS;
}

NTSTATUS ReadBuffer(PIRP pIrp, PC0C_BUFFER pBuf)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack;

  status = STATUS_PENDING;
  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  for (;;) {
    ULONG length, writeLength, readLength;
    PUCHAR pWriteBuf, pReadBuf;

    readLength = pIrpStack->Parameters.Read.Length - pIrp->IoStatus.Information;

    if (!readLength) {
      status = STATUS_SUCCESS;
      break;
    }

    pReadBuf = GET_REST_BUFFER(pIrp);

    if (!pBuf->busy) {
      if (pBuf->escape) {
        pBuf->escape = FALSE;
        *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
        pIrp->IoStatus.Information++;
        readLength--;
        if (!readLength) {
          status = STATUS_SUCCESS;
          break;
        }
      }
      if (pBuf->insertData.size) {
        length = pBuf->insertData.size;

        if (length > readLength)
          length = readLength;

        RtlCopyMemory(pReadBuf, pBuf->insertData.data, length);
        pReadBuf += length;
        pIrp->IoStatus.Information += length;
        readLength -= length;
        CompactRawData(&pBuf->insertData, length);
        if (!readLength) {
          status = STATUS_SUCCESS;
          break;
        }
      }
      break;
    }

    ASSERT(pBuf->pBase);

    writeLength = pBuf->pFree <= pBuf->pBusy ?
        pBuf->pEnd - pBuf->pBusy : pBuf->busy;

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

VOID WaitCompleteRxChar(PC0C_IO_PORT pReadIoPort, PLIST_ENTRY pQueueToComplete)
{
  if (pReadIoPort->waitMask & SERIAL_EV_RXCHAR) {
    pReadIoPort->eventMask |= SERIAL_EV_RXCHAR;
    WaitComplete(pReadIoPort, pQueueToComplete);
  }
}

VOID CopyCharsWithEscape(
    PC0C_BUFFER pBuf, UCHAR escapeChar,
    PUCHAR pReadBuf, ULONG readLength,
    PUCHAR pWriteBuf, ULONG writeLength,
    PULONG pReadDone,
    PULONG pWriteDone)
{
  ULONG readDone;
  ULONG writeDone;

  readDone = 0;

  if (pBuf->escape && readLength) {
    pBuf->escape = FALSE;
    *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
    readDone++;
    readLength--;
  }

  if (pBuf->insertData.size && readLength) {
    ULONG length = pBuf->insertData.size;

    if (length > readLength)
      length = readLength;

    RtlCopyMemory(pReadBuf, pBuf->insertData.data, length);
    pReadBuf += length;
    readDone += length;
    readLength -= length;
    CompactRawData(&pBuf->insertData, length);
  }

  if (!escapeChar) {
    writeDone = writeLength < readLength ? writeLength : readLength;

    if (writeDone)
      RtlCopyMemory(pReadBuf, pWriteBuf, writeDone);

    readDone += writeDone;
  } else {
    writeDone = 0;

    while (writeLength--) {
      UCHAR curChar;

      if (!readLength--)
        break;

      curChar = *pWriteBuf++;
      writeDone++;
      *pReadBuf++ = curChar;
      readDone++;

      if (curChar == escapeChar) {
        if (!readLength--) {
          pBuf->escape = TRUE;
          break;
        }
        *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
        readDone++;
      }
    }
  }

  *pReadDone = readDone;
  *pWriteDone = writeDone;
}

NTSTATUS WriteBuffer(PIRP pIrp, PC0C_IO_PORT pReadIoPort, PLIST_ENTRY pQueueToComplete)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack;
  PC0C_BUFFER pBuf = &pReadIoPort->readBuf;

  if (!pBuf->pBase)
    return STATUS_PENDING;

  status = STATUS_PENDING;
  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  for (;;) {
    ULONG writeLength, readLength;
    PVOID pWriteBuf, pReadBuf;
    ULONG readDone, writeDone;

    writeLength = pIrpStack->Parameters.Write.Length - pIrp->IoStatus.Information;

    if (!writeLength) {
      status = STATUS_SUCCESS;
      break;
    }

    pWriteBuf = GET_REST_BUFFER(pIrp);

    if ((ULONG)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pReadBuf = pBuf->pFree;

    CopyCharsWithEscape(
        pBuf, pReadIoPort->escapeChar,
        pReadBuf, readLength,
        pWriteBuf, writeLength,
        &readDone, &writeDone);

    pBuf->busy += readDone;
    pBuf->pFree += readDone;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    pIrp->IoStatus.Information += writeDone;

    if (writeDone)
      WaitCompleteRxChar(pReadIoPort, pQueueToComplete);
  }

  return status;
}

NTSTATUS InsertBuffer(PC0C_RAW_DATA pRawData, PC0C_BUFFER pBuf)
{
  NTSTATUS status;

  if (!pBuf->pBase)
    return STATUS_PENDING;

  status = STATUS_PENDING;

  for (;;) {
    ULONG writeLength, readLength;
    PVOID pWriteBuf, pReadBuf;
    ULONG readDone, writeDone;

    writeLength = pRawData->size;

    if (!writeLength) {
      status = STATUS_SUCCESS;
      break;
    }

    pWriteBuf = pRawData->data;

    if ((ULONG)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pReadBuf = pBuf->pFree;

    CopyCharsWithEscape(
        pBuf, 0,
        pReadBuf, readLength,
        pWriteBuf, writeLength,
        &readDone, &writeDone);

    pBuf->busy += readDone;
    pBuf->pFree += readDone;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    CompactRawData(pRawData, writeDone);
  }

  if (status == STATUS_PENDING)
    status = MoveRawData(&pBuf->insertData, pRawData);

  return status;
}

VOID ReadWriteDirect(
    PIRP pIrpLocal,
    PIRP pIrpRemote,
    PNTSTATUS pStatusLocal,
    PNTSTATUS pStatusRemote,
    PC0C_IO_PORT pIoPortRemote,
    PLIST_ENTRY pQueueToComplete)
{
  PC0C_IO_PORT pReadIoPort;
  ULONG readDone, writeDone;
  PIRP pIrpRead, pIrpWrite;
  PNTSTATUS pStatusRead, pStatusWrite;
  ULONG writeLength, readLength;
  PVOID pWriteBuf, pReadBuf;

  if (IoGetCurrentIrpStackLocation(pIrpLocal)->MajorFunction == IRP_MJ_WRITE) {
    pIrpRead = pIrpRemote;
    pIrpWrite = pIrpLocal;
    pStatusRead = pStatusRemote;
    pStatusWrite = pStatusLocal;
    pReadIoPort = pIoPortRemote;
  } else {
    pIrpRead = pIrpLocal;
    pIrpWrite = pIrpRemote;
    pStatusRead = pStatusLocal;
    pStatusWrite = pStatusRemote;
    pReadIoPort = pIoPortRemote->pDevExt->pIoPortRemote;
  }

  pReadBuf = GET_REST_BUFFER(pIrpRead);
  readLength = IoGetCurrentIrpStackLocation(pIrpRead)->Parameters.Read.Length
                                                - pIrpRead->IoStatus.Information;
  pWriteBuf = GET_REST_BUFFER(pIrpWrite);
  writeLength = IoGetCurrentIrpStackLocation(pIrpWrite)->Parameters.Write.Length
                                                - pIrpWrite->IoStatus.Information;

  CopyCharsWithEscape(
      &pReadIoPort->readBuf, pReadIoPort->escapeChar,
      pReadBuf, readLength,
      pWriteBuf, writeLength,
      &readDone, &writeDone);

  pIrpRead->IoStatus.Information += readDone;
  pIrpWrite->IoStatus.Information += writeDone;

  if (readDone == readLength)
    *pStatusRead = STATUS_SUCCESS;
  if (writeDone == writeLength)
    *pStatusWrite = STATUS_SUCCESS;

  if (writeDone)
    WaitCompleteRxChar(pReadIoPort, pQueueToComplete);
}

VOID InsertDirect(
    PC0C_RAW_DATA pRawData,
    PIRP pIrpRead,
    PNTSTATUS pStatusWrite,
    PNTSTATUS pStatusRead,
    PC0C_BUFFER pBuf)
{
  ULONG readDone, writeDone;
  ULONG writeLength, readLength;
  PVOID pWriteBuf, pReadBuf;

  pReadBuf = GET_REST_BUFFER(pIrpRead);
  readLength = IoGetCurrentIrpStackLocation(pIrpRead)->Parameters.Read.Length
                                                - pIrpRead->IoStatus.Information;
  pWriteBuf = pRawData->data;
  writeLength = pRawData->size;

  CopyCharsWithEscape(
      pBuf, 0,
      pReadBuf, readLength,
      pWriteBuf, writeLength,
      &readDone, &writeDone);

  pIrpRead->IoStatus.Information += readDone;
  CompactRawData(pRawData, writeDone);

  if (readDone == readLength)
    *pStatusRead = STATUS_SUCCESS;
  if (writeDone == writeLength)
    *pStatusWrite = STATUS_SUCCESS;
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
        ReadWriteDirect((PIRP)pParam, pIrpCurrent, &status, &statusCurrent, pIoPort, pQueueToComplete);
      break;
    case C0C_IO_TYPE_WAIT_COMPLETE:
      ASSERT(pParam);
      *((PULONG)pIrpCurrent->AssociatedIrp.SystemBuffer) = *((PULONG)pParam);
      *((PULONG)pParam) = 0;
      pIrpCurrent->IoStatus.Information = sizeof(ULONG);
      statusCurrent = STATUS_SUCCESS;
      break;
    case C0C_IO_TYPE_INSERT:
      ASSERT(pParam);
      InsertDirect((PC0C_RAW_DATA)pParam, pIrpCurrent, &status, &statusCurrent, &pIoPort->readBuf);
      break;
    }

    if (IoGetCurrentIrpStackLocation(pIrpCurrent)->MajorFunction == IRP_MJ_WRITE &&
        statusCurrent == STATUS_PENDING)
      statusCurrent = WriteBuffer(pIrpCurrent, pIoPort->pDevExt->pIoPortRemote, pQueueToComplete);

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

  if (status == STATUS_PENDING) {
    switch (ioType) {
    case C0C_IO_TYPE_WRITE:
      ASSERT(pParam);
      status = WriteBuffer((PIRP)pParam, pIoPort, pQueueToComplete);
      break;
    case C0C_IO_TYPE_INSERT:
      ASSERT(pParam);
      status = InsertBuffer((PC0C_RAW_DATA)pParam, &pIoPort->readBuf);
      break;
    }
  }
  return status;
}

VOID SetModemStatus(
    IN PC0C_IO_PORT pIoPort,
    IN ULONG bits,
    IN BOOLEAN set,
    PLIST_ENTRY pQueueToComplete)
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

  if (modemStatusChanged) {
    if (pIoPort->escapeChar) {
      C0C_RAW_DATA insertData;

      insertData.size = 3;
      insertData.data[0] = pIoPort->escapeChar;
      insertData.data[1] = SERIAL_LSRMST_MST;
      insertData.data[2] = (UCHAR)(pIoPort->modemStatus | (modemStatusChanged >> 4));

      FdoPortIo(
          C0C_IO_TYPE_INSERT,
          &insertData,
          pIoPort,
          &pIoPort->irpQueues[C0C_QUEUE_READ],
          pQueueToComplete);
#if DBG
      if (insertData.size)
        Trace0((PC0C_COMMON_EXTENSION)pIoPort->pDevExt, L"WARNING: Lost SERIAL_LSRMST_MST");
#endif /* DBG */
    }

    if (modemStatusChanged & C0C_MSB_CTS)
      pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_CTS;

    if (modemStatusChanged & C0C_MSB_DSR)
      pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_DSR;

    if (modemStatusChanged & C0C_MSB_RING)
      pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_RING;

    if (modemStatusChanged & C0C_MSB_RLSD)
      pIoPort->eventMask |= pIoPort->waitMask & SERIAL_EV_RLSD;

    WaitComplete(pIoPort, pQueueToComplete);
  }
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
    SetModemStatus(pDevExt->pIoPortRemote, bits, TRUE, pQueueToComplete);
}
