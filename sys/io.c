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
 * Revision 1.18  2005/11/29 08:35:13  vfrolov
 * Implemented SERIAL_EV_RX80FULL
 *
 * Revision 1.17  2005/11/25 08:59:39  vfrolov
 * Implemented SERIAL_EV_RXFLAG
 *
 * Revision 1.16  2005/09/14 13:14:47  vfrolov
 * Fixed possible tick loss
 *
 * Revision 1.15  2005/09/14 10:42:38  vfrolov
 * Implemented SERIAL_EV_TXEMPTY
 *
 * Revision 1.14  2005/09/13 14:56:16  vfrolov
 * Implemented IRP_MJ_FLUSH_BUFFERS
 *
 * Revision 1.13  2005/09/13 08:55:41  vfrolov
 * Disabled modem status tracing by default
 *
 * Revision 1.12  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.11  2005/08/26 08:35:05  vfrolov
 * Fixed unwanted interference to baudrate emulation by read operations
 *
 * Revision 1.10  2005/08/25 15:38:17  vfrolov
 * Some code moved from io.c to bufutils.c
 *
 * Revision 1.9  2005/08/25 08:25:40  vfrolov
 * Fixed data types
 *
 * Revision 1.8  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.7  2005/07/14 12:24:31  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.6  2005/05/19 08:23:41  vfrolov
 * Fixed data types
 *
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
 */

#include "precomp.h"
#include "timeout.h"
#include "delay.h"
#include "bufutils.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 1

#define GET_REST_BUFFER(pIrp, done) \
    (((PUCHAR)(pIrp)->AssociatedIrp.SystemBuffer) + done)

NTSTATUS ReadBuffer(PIRP pIrp, PC0C_BUFFER pBuf, PSIZE_T pReadDone)
{
  NTSTATUS status;
  SIZE_T readLength, information;
  SIZE_T readDone;

  readLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length;
  information = pIrp->IoStatus.Information;

  readDone =  ReadFromBuffer(pBuf, GET_REST_BUFFER(pIrp, information), readLength - information);

  if (readDone) {
    *pReadDone += readDone;
    information += readDone;
    pIrp->IoStatus.Information = information;
  }


  if (information == readLength)
    status = STATUS_SUCCESS;
  else
    status = STATUS_PENDING;

  return status;
}

VOID OnRxChars(PC0C_IO_PORT pReadIoPort, PVOID pBuf, SIZE_T size, PLIST_ENTRY pQueueToComplete)
{
  if (pReadIoPort->waitMask & (SERIAL_EV_RXCHAR | SERIAL_EV_RXFLAG)) {
    if (pReadIoPort->waitMask & SERIAL_EV_RXCHAR)
      pReadIoPort->eventMask |= SERIAL_EV_RXCHAR;

    if (pReadIoPort->waitMask & SERIAL_EV_RXFLAG) {
      PUCHAR p = (PUCHAR)pBuf;
      SIZE_T s = size;
      UCHAR c = pReadIoPort->pDevExt->specialChars.EventChar;

      while (s--) {
        if (c == *p++) {
          pReadIoPort->eventMask |= SERIAL_EV_RXFLAG;
          break;
        }
      }
    }

    if (pReadIoPort->eventMask)
      WaitComplete(pReadIoPort, pQueueToComplete);
  }
}

NTSTATUS WriteBuffer(
    PIRP pIrp,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PSIZE_T pWriteDone)
{
  NTSTATUS status;
  SIZE_T writeLength, information;
  SIZE_T writeDone;
  PVOID pWriteBuf;
  PC0C_BUFFER pBuf;
  SIZE_T length;

  information = pIrp->IoStatus.Information;

  pWriteBuf = GET_REST_BUFFER(pIrp, information);
  writeLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length;

  pBuf = &pReadIoPort->readBuf;
  length = writeLength - information;

  if (pWriteLimit && length > *pWriteLimit)
    length = *pWriteLimit;

  writeDone = WriteToBuffer(pBuf, pWriteBuf, length, pReadIoPort->escapeChar);

  if (writeDone) {
    *pWriteDone += writeDone;
    information += writeDone;
    pIrp->IoStatus.Information = information;

    if (pWriteLimit)
      *pWriteLimit -= writeDone;

    OnRxChars(pReadIoPort, pWriteBuf, writeDone, pQueueToComplete);
  }

  if (information == writeLength)
    status = STATUS_SUCCESS;
  else
    status = STATUS_PENDING;

  return status;
}

VOID AlertOverrun(PC0C_IO_PORT pReadIoPort, PLIST_ENTRY pQueueToComplete)
{
  pReadIoPort->errors |= SERIAL_ERROR_QUEUEOVERRUN;

  if (pReadIoPort->pDevExt->handFlow.FlowReplace & SERIAL_ERROR_CHAR)
    WriteMandatoryToBuffer(&pReadIoPort->readBuf, pReadIoPort->pDevExt->specialChars.ErrorChar);

  if (pReadIoPort->pDevExt->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) {
    CancelQueue(&pReadIoPort->irpQueues[C0C_QUEUE_READ], pQueueToComplete);
    CancelQueue(&pReadIoPort->irpQueues[C0C_QUEUE_WRITE], pQueueToComplete);
  }
}

NTSTATUS WriteOverrun(
    PIRP pIrp,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PSIZE_T pWriteDone)
{
  NTSTATUS status;
  SIZE_T writeLength, information;
  SIZE_T writeDone;
  PVOID pWriteBuf;

  information = pIrp->IoStatus.Information;

  pWriteBuf = GET_REST_BUFFER(pIrp, information);
  writeLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length;

  writeDone = writeLength - information;

  if (pWriteLimit && writeDone > *pWriteLimit)
    writeDone = *pWriteLimit;

  if (writeDone) {
    *pWriteDone += writeDone;
    information += writeDone;
    pIrp->IoStatus.Information = information;

    if (pWriteLimit)
      *pWriteLimit -= writeDone;

    AlertOverrun(pReadIoPort, pQueueToComplete);
    OnRxChars(pReadIoPort, pWriteBuf, writeDone, pQueueToComplete);
  }

  if (information == writeLength)
    status = STATUS_SUCCESS;
  else
    status = STATUS_PENDING;

  return status;
}

VOID ReadWriteDirect(
    PIRP pIrpRead,
    PIRP pIrpWrite,
    PNTSTATUS pStatusRead,
    PNTSTATUS pStatusWrite,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone)
{
  SIZE_T readDone, writeDone;
  SIZE_T writeLength, readLength;
  PVOID pWriteBuf, pReadBuf;

  pReadBuf = GET_REST_BUFFER(pIrpRead, pIrpRead->IoStatus.Information);
  readLength = IoGetCurrentIrpStackLocation(pIrpRead)->Parameters.Read.Length
                                                - pIrpRead->IoStatus.Information;

  pWriteBuf = GET_REST_BUFFER(pIrpWrite, pIrpWrite->IoStatus.Information);
  writeLength = IoGetCurrentIrpStackLocation(pIrpWrite)->Parameters.Write.Length
                                                - pIrpWrite->IoStatus.Information;

  CopyCharsWithEscape(
      &pReadIoPort->readBuf, pReadIoPort->escapeChar,
      pReadBuf, readLength,
      pWriteBuf, (pWriteLimit && writeLength > *pWriteLimit) ? *pWriteLimit : writeLength,
      &readDone, &writeDone);

  if (pWriteLimit)
    *pWriteLimit -= writeDone;

  pIrpRead->IoStatus.Information += readDone;
  pIrpWrite->IoStatus.Information += writeDone;

  if (readDone == readLength)
    *pStatusRead = STATUS_SUCCESS;
  if (writeDone == writeLength)
    *pStatusWrite = STATUS_SUCCESS;

  if (writeDone)
    OnRxChars(pReadIoPort, pWriteBuf, writeDone, pQueueToComplete);

  *pReadDone += readDone;
  *pWriteDone += writeDone;
}

VOID InsertDirect(
    PC0C_RAW_DATA pRawData,
    PIRP pIrpRead,
    PNTSTATUS pStatusWrite,
    PNTSTATUS pStatusRead,
    PSIZE_T pReadDone)
{
  SIZE_T readDone;
  SIZE_T readLength;
  PVOID pReadBuf;

  pReadBuf = GET_REST_BUFFER(pIrpRead, pIrpRead->IoStatus.Information);
  readLength = IoGetCurrentIrpStackLocation(pIrpRead)->Parameters.Read.Length
                                                - pIrpRead->IoStatus.Information;

  readDone = WriteRawData(pRawData, pStatusWrite, pReadBuf, readLength);

  pIrpRead->IoStatus.Information += readDone;

  if (readDone == readLength)
    *pStatusRead = STATUS_SUCCESS;

  *pReadDone += readDone;
}

PIRP StartCurrentIrp(PC0C_IRP_QUEUE pQueue, PDRIVER_CANCEL *ppCancelRoutine, PBOOLEAN pFirst)
{
  while (pQueue->pCurrent) {
    PIRP pIrp;

    pIrp = pQueue->pCurrent;

    #pragma warning(push, 3)
    *ppCancelRoutine = IoSetCancelRoutine(pIrp, NULL);
    #pragma warning(pop)

    if (*ppCancelRoutine)
      return pIrp;

    ShiftQueue(pQueue);
    *pFirst = FALSE;
  }
  return NULL;
}

NTSTATUS StopCurrentIrp(
    NTSTATUS status,
    PDRIVER_CANCEL pCancelRoutine,
    BOOLEAN first,
    SIZE_T done,
    PC0C_IO_PORT pIoPort,
    PC0C_IRP_QUEUE pQueue,
    PLIST_ENTRY pQueueToComplete)
{
  PIRP pIrp;

  pIrp = pQueue->pCurrent;

  if (status == STATUS_PENDING && done) {
    PC0C_IRP_STATE pState;

    pState = GetIrpState(pIrp);
    HALT_UNLESS(pState);

    if ((pState->flags & C0C_IRP_FLAG_WAIT_ONE) != 0) {
      status = STATUS_SUCCESS;
    }
    else
    if (first && (pState->flags & C0C_IRP_FLAG_INTERVAL_TIMEOUT) != 0) {
      SetIntervalTimeout(pIoPort);
    }
  }

  if (!first && status == STATUS_PENDING)
    status = FdoPortSetIrpTimeout(pIoPort->pDevExt, pIrp);

  HALT_UNLESS(pCancelRoutine);

  if (status == STATUS_PENDING) {
    #pragma warning(push, 3)
    IoSetCancelRoutine(pIrp, pCancelRoutine);
    #pragma warning(pop)
    if (pIrp->Cancel) {
      #pragma warning(push, 3)
      pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);
      #pragma warning(pop)

      if (pCancelRoutine) {
        ShiftQueue(pQueue);
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;
        InsertTailList(pQueueToComplete, &pIrp->Tail.Overlay.ListEntry);
        return STATUS_CANCELLED;
      }
    }
  } else {
    ShiftQueue(pQueue);
    pIrp->IoStatus.Status = status;
    InsertTailList(pQueueToComplete, &pIrp->Tail.Overlay.ListEntry);
  }

  return status;
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
  BOOLEAN firstCurrent;
  PIRP pIrpCurrent;
  PDRIVER_CANCEL pCancelRoutineCurrent;
  SIZE_T done;

  first = TRUE;
  done = 0;

  status = STATUS_PENDING;

  for (firstCurrent = TRUE ; (pIrpCurrent = StartCurrentIrp(pQueue, &pCancelRoutineCurrent, &firstCurrent)) != NULL ; firstCurrent = FALSE) {
    NTSTATUS statusCurrent;
    SIZE_T doneCurrent;

    statusCurrent = STATUS_PENDING;
    doneCurrent = 0;

    switch (ioType) {
    case C0C_IO_TYPE_WAIT_COMPLETE:
      HALT_UNLESS(pParam);
      *((PULONG)pIrpCurrent->AssociatedIrp.SystemBuffer) = *((PULONG)pParam);
      *((PULONG)pParam) = 0;
      pIrpCurrent->IoStatus.Information = sizeof(ULONG);
      statusCurrent = STATUS_SUCCESS;
      break;
    case C0C_IO_TYPE_INSERT:
      HALT_UNLESS(pParam);
      InsertDirect((PC0C_RAW_DATA)pParam, pIrpCurrent, &status, &statusCurrent, &doneCurrent);
      break;
    }

    statusCurrent = StopCurrentIrp(statusCurrent,
                                   pCancelRoutineCurrent,
                                   firstCurrent,
                                   doneCurrent,
                                   pIoPort,
                                   pQueue,
                                   pQueueToComplete);

    if (statusCurrent == STATUS_PENDING)
      break;
  }

  if (status == STATUS_PENDING) {
    switch (ioType) {
    case C0C_IO_TYPE_INSERT:
      HALT_UNLESS(pParam);
      status = WriteRawDataToBuffer((PC0C_RAW_DATA)pParam, &pIoPort->readBuf);
      if (status == STATUS_PENDING && !pIoPort->emuOverrun)
        status = MoveRawData(&pIoPort->readBuf.insertData, (PC0C_RAW_DATA)pParam);
      break;
    }
  }
  return status;
}

NTSTATUS ReadWrite(
    PC0C_IO_PORT pIoPortRead,
    PC0C_IRP_QUEUE pQueueRead,
    BOOLEAN startRead,
    PC0C_IO_PORT pIoPortWrite,
    PC0C_IRP_QUEUE pQueueWrite,
    BOOLEAN startWrite,
    PLIST_ENTRY pQueueToComplete)
{
  NTSTATUS status;
  SIZE_T readBufBusyBeg, readBufBusyEnd;
  BOOLEAN firstRead;
  PC0C_ADAPTIVE_DELAY pWriteDelay;
  SIZE_T writeLimit;
  PSIZE_T pWriteLimit;

  pWriteDelay = pIoPortWrite->pWriteDelay;

  if (pWriteDelay) {
    if (pQueueWrite->pCurrent) {
      StartWriteDelayTimer(pWriteDelay);
      writeLimit = GetWriteLimit(pWriteDelay);
      status = STATUS_PENDING;
    } else {
      writeLimit = 0;
      status = STATUS_SUCCESS;
    }

    pWriteLimit = &writeLimit;
  } else {
    status = STATUS_SUCCESS;
    pWriteLimit = NULL;
  }

  readBufBusyBeg = C0C_BUFFER_BUSY(&pIoPortRead->readBuf);

  for (firstRead = TRUE ;; firstRead = FALSE) {
    NTSTATUS statusRead;
    PDRIVER_CANCEL pCancelRoutineRead;
    PIRP pIrpRead;
    SIZE_T doneRead;
    BOOLEAN firstWrite, wasWrite;

    if (startRead) {
      pIrpRead = pQueueRead->pCurrent;
      pCancelRoutineRead = NULL;
    } else {
      pIrpRead = StartCurrentIrp(pQueueRead, &pCancelRoutineRead, &firstRead);
    }

    doneRead = 0;

    if (pIrpRead)
      statusRead = ReadBuffer(pIrpRead, &pIoPortRead->readBuf, &doneRead);
    else
      statusRead = STATUS_SUCCESS;

    wasWrite = FALSE;

    for (firstWrite = TRUE ;; firstWrite = FALSE) {
      NTSTATUS statusWrite;
      PDRIVER_CANCEL pCancelRoutineWrite;
      PIRP pIrpWrite;
      SIZE_T doneWrite;

      if(startWrite) {
        pIrpWrite = firstWrite ? pQueueWrite->pCurrent : NULL;
        pCancelRoutineWrite = NULL;
      } else {
        pIrpWrite = StartCurrentIrp(pQueueWrite, startWrite ? NULL : &pCancelRoutineWrite, &firstWrite);
      }

      if (!pIrpWrite) {
        if (wasWrite && pIoPortWrite->waitMask & SERIAL_EV_TXEMPTY) {
          pIoPortWrite->eventMask |= SERIAL_EV_TXEMPTY;
          WaitComplete(pIoPortWrite, pQueueToComplete);
        }
        break;
      }

      statusWrite = STATUS_PENDING;
      doneWrite = 0;

      if (IoGetCurrentIrpStackLocation(pIrpWrite)->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {
        pIrpWrite->IoStatus.Information = 0;
        statusWrite = STATUS_SUCCESS;
      }
      else
      if (!pWriteLimit || *pWriteLimit) {
        if (statusRead == STATUS_PENDING)
          ReadWriteDirect(
              pIrpRead, pIrpWrite,
              &statusRead, &statusWrite,
              pIoPortRead,
              pQueueToComplete,
              pWriteLimit,
              &doneRead, &doneWrite);

        if (statusWrite == STATUS_PENDING) {
          statusWrite = WriteBuffer(pIrpWrite, pIoPortRead, pQueueToComplete, pWriteLimit, &doneWrite);

          if (pIoPortRead->emuOverrun && !pIrpRead && statusWrite == STATUS_PENDING)
            statusWrite = WriteOverrun(pIrpWrite, pIoPortRead, pQueueToComplete, pWriteLimit, &doneWrite);
        }

        if (doneWrite) {
          wasWrite = TRUE;

          if (pWriteDelay)
            pWriteDelay->sentFrames += doneWrite;
        }
      }

      if (startWrite) {
        status = statusWrite;
        if (status == STATUS_PENDING)
          status = FdoPortSetIrpTimeout(pIoPortWrite->pDevExt, pIrpWrite);
        continue;
      }

      statusWrite = StopCurrentIrp(statusWrite,
                                   pCancelRoutineWrite,
                                   firstWrite,
                                   doneWrite,
                                   pIoPortWrite,
                                   pQueueWrite,
                                   pQueueToComplete);

      if (statusWrite == STATUS_PENDING)
        break;
    }

    if (!pIrpRead)
      break;

    if (startRead) {
      status = statusRead;
      if (status == STATUS_PENDING)
        status = FdoPortSetIrpTimeout(pIoPortRead->pDevExt, pIrpRead);
      break;
    }

    statusRead = StopCurrentIrp(statusRead,
                                pCancelRoutineRead,
                                firstRead,
                                doneRead,
                                pIoPortRead,
                                pQueueRead,
                                pQueueToComplete);

    if (statusRead == STATUS_PENDING)
      break;
  }

  readBufBusyEnd = C0C_BUFFER_BUSY(&pIoPortRead->readBuf);

  if (readBufBusyBeg != readBufBusyEnd) {
    if ((pIoPortRead->waitMask & SERIAL_EV_RX80FULL) &&
        readBufBusyEnd > pIoPortRead->readBuf.size80 &&
        readBufBusyBeg <= pIoPortRead->readBuf.size80)
    {
      pIoPortRead->eventMask |= SERIAL_EV_RX80FULL;
      WaitComplete(pIoPortRead, pQueueToComplete);
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

  modemStatusChanged = modemStatusOld ^ pIoPort->modemStatus;

  if (modemStatusChanged) {
    TraceModemStatus(pIoPort);

    if (pIoPort->escapeChar) {
      NTSTATUS status;
      C0C_RAW_DATA insertData;

      insertData.size = 3;
      insertData.data[0] = pIoPort->escapeChar;
      insertData.data[1] = SERIAL_LSRMST_MST;
      insertData.data[2] = (UCHAR)(pIoPort->modemStatus | (modemStatusChanged >> 4));

      status = FdoPortIo(
          C0C_IO_TYPE_INSERT,
          &insertData,
          pIoPort,
          &pIoPort->irpQueues[C0C_QUEUE_READ],
          pQueueToComplete);

      if (status == STATUS_PENDING) {
        AlertOverrun(pIoPort, pQueueToComplete);
        Trace0((PC0C_COMMON_EXTENSION)pIoPort->pDevExt, L"WARNING: Lost SERIAL_LSRMST_MST");
      }
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
