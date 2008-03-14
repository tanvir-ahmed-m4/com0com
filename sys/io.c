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
 * Revision 1.37  2008/03/14 15:28:39  vfrolov
 * Implemented ability to get paired port settings with
 * extended IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.36  2007/09/12 12:32:53  vfrolov
 * Fixed TX buffer
 *
 * Revision 1.35  2007/07/20 08:00:22  vfrolov
 * Implemented TX buffer
 *
 * Revision 1.34  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.33  2007/06/09 08:49:47  vfrolov
 * Improved baudrate emulation
 *
 * Revision 1.32  2007/06/07 14:51:47  vfrolov
 * Added check for NULL of pIrpXoffCounter
 *
 * Revision 1.31  2007/06/04 15:24:32  vfrolov
 * Fixed open reject just after close in exclusiveMode
 *
 * Revision 1.30  2007/02/20 12:05:11  vfrolov
 * Implemented IOCTL_SERIAL_XOFF_COUNTER
 * Fixed cancel and timeout routines
 *
 * Revision 1.29  2006/11/27 11:58:27  vfrolov
 * Fixed unexpected completing all queued read requests when
 * completing the first one
 *
 * Revision 1.28  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.27  2006/05/17 15:31:14  vfrolov
 * Implemented SERIAL_TRANSMIT_TOGGLE
 *
 * Revision 1.26  2006/02/26 08:39:19  vfrolov
 * Added check for start/stop queue matching
 * Fixed delayed BREAK losts
 *
 * Revision 1.25  2006/02/21 13:42:11  vfrolov
 * Implemented SERIAL_BREAK_CHAR
 *
 * Revision 1.24  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
 * Revision 1.23  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.22  2005/12/06 13:04:32  vfrolov
 * Fixed data types
 *
 * Revision 1.21  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.20  2005/11/30 16:04:11  vfrolov
 * Implemented IOCTL_SERIAL_GET_STATS and IOCTL_SERIAL_CLEAR_STATS
 *
 * Revision 1.19  2005/11/29 12:33:21  vfrolov
 * Changed SetModemStatus() to ability set and clear bits simultaneously
 *
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
#include "handflow.h"
#include "../include/cncext.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 1

#define GET_REST_BUFFER(pIrp, done) \
    (((PUCHAR)(pIrp)->AssociatedIrp.SystemBuffer) + (done))

#define GET_REST_BUFFER_WRITE(pIrp, done) \
    ((IoGetCurrentIrpStackLocation(pIrp)->MajorFunction == IRP_MJ_DEVICE_CONTROL && \
      IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER) \
        ? &(((PSERIAL_XOFF_COUNTER)(pIrp)->AssociatedIrp.SystemBuffer)->XoffChar) + (done) \
        : GET_REST_BUFFER(pIrp, (done)))

typedef struct _RW_DATA {

  #define RW_DATA_TYPE_IRP   1
  #define RW_DATA_TYPE_CHR   2

  short type;

  union {
    struct {
      PIRP pIrp;
      NTSTATUS status;
    } irp;
    struct {
      UCHAR chr;

      #define RW_DATA_TYPE_CHR_NONE   0
      #define RW_DATA_TYPE_CHR_XCHR   1
      #define RW_DATA_TYPE_CHR_BREAK  2

      short type;

      BOOLEAN isChr;
    } chr;
  } data;
} RW_DATA, *PRW_DATA;

#define CAN_WRITE_RW_DATA_CHR(pIoPort, dataChar) \
  ( \
    ((dataChar).data.chr.type == RW_DATA_TYPE_CHR_XCHR && \
          ((pIoPort)->writeHolding & ~SERIAL_TX_WAITING_FOR_XON) == 0) || \
    ((dataChar).data.chr.type == RW_DATA_TYPE_CHR_BREAK) \
  ) \

ULONG GetWriteLength(IN PIRP pIrp)
{
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  switch (pIrpStack->MajorFunction) {
  case IRP_MJ_WRITE:
    return pIrpStack->Parameters.Write.Length;
  case IRP_MJ_DEVICE_CONTROL:
    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
    case IOCTL_SERIAL_XOFF_COUNTER:
      return sizeof(UCHAR);
    }
    break;
  }
  return 0;
}

VOID OnRxChars(
    PC0C_IO_PORT pReadIoPort,
    SIZE_T size,
    PC0C_FLOW_FILTER pFlowFilter,
    PLIST_ENTRY pQueueToComplete)
{
  PIRP pCurrent;

  SetXonXoffHolding(pReadIoPort, pFlowFilter->lastXonXoff);

  if (pFlowFilter->events & (C0C_FLOW_FILTER_EV_RXCHAR | C0C_FLOW_FILTER_EV_RXFLAG)) {
    if (pFlowFilter->events & C0C_FLOW_FILTER_EV_RXCHAR)
      pReadIoPort->eventMask |= SERIAL_EV_RXCHAR;

    if (pFlowFilter->events & C0C_FLOW_FILTER_EV_RXFLAG)
      pReadIoPort->eventMask |= SERIAL_EV_RXFLAG;

    if (pReadIoPort->eventMask)
      WaitComplete(pReadIoPort, pQueueToComplete);
  }

  pReadIoPort->perfStats.ReceivedCount += (ULONG)size;

  pCurrent = pReadIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent;

  if (pCurrent) {
    PIO_STACK_LOCATION pCurrentStack;

    pCurrentStack = IoGetCurrentIrpStackLocation(pCurrent);

    if (pCurrentStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
        pCurrentStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER &&
        pCurrent->IoStatus.Information)
    {
      if ((LONG)size < pReadIoPort->xoffCounter) {
        pReadIoPort->xoffCounter -= (LONG)size;
      } else {
        ShiftQueue(&pReadIoPort->irpQueues[C0C_QUEUE_WRITE]);
        CompleteIrp(pCurrent, STATUS_SUCCESS, pQueueToComplete);
      }
    }
  }
}

NTSTATUS ReadFromBuffers(
    PIRP pIrp,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PC0C_ADAPTIVE_DELAY pWriteDelay,
    PSIZE_T pReadDone,
    PSIZE_T pSendDone)
{
  SIZE_T destRestLength;
  SIZE_T information;
  C0C_FLOW_FILTER flowFilter;
  SIZE_T sendDone;
  SIZE_T readDone;
  PUCHAR pDestRest;

  information = pIrp->IoStatus.Information;
  destRestLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length - information;

  if (destRestLength == 0)
    return STATUS_SUCCESS;

  pDestRest = GET_REST_BUFFER(pIrp, information);

  readDone =  ReadFromBuffer(&pReadIoPort->readBuf, pDestRest, destRestLength);

  if (readDone) {
    *pReadDone += readDone;
    information += readDone;
    destRestLength -= readDone;

    if (destRestLength == 0) {
      pIrp->IoStatus.Information = information;
      return STATUS_SUCCESS;
    }

    pDestRest += readDone;
  }

  FlowFilterInit(pReadIoPort, &flowFilter);

  readDone =  ReadFromTxBuffer(&pReadIoPort->readBuf,
                               &flowFilter,
                               pDestRest, destRestLength,
                               &pReadIoPort->pIoPortRemote->txBuf,
                               pWriteLimit ? *pWriteLimit : -1,
                               &sendDone);

  if (readDone) {
    *pReadDone += readDone;
    information += readDone;
    destRestLength -= readDone;
  }

  if (sendDone) {
    if (pWriteLimit)
      *pWriteLimit -= sendDone;

    if (pWriteDelay) {
      pWriteDelay->sentFrames += sendDone;
      pWriteDelay->idleCount = 0;
    }

    OnRxChars(pReadIoPort, sendDone, &flowFilter, pQueueToComplete);
    *pSendDone += sendDone;
  }

  pIrp->IoStatus.Information = information;

  if (destRestLength == 0)
    return STATUS_SUCCESS;

  return STATUS_PENDING;
}

VOID AlertOverrun(PC0C_IO_PORT pReadIoPort, PLIST_ENTRY pQueueToComplete)
{
  pReadIoPort->errors |= SERIAL_ERROR_QUEUEOVERRUN;

  if (pReadIoPort->handFlow.FlowReplace & SERIAL_ERROR_CHAR)
    WriteMandatoryToBuffer(&pReadIoPort->readBuf, pReadIoPort->specialChars.ErrorChar);

  if (pReadIoPort->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) {
    CancelQueue(&pReadIoPort->irpQueues[C0C_QUEUE_READ], pQueueToComplete);
    CancelQueue(&pReadIoPort->irpQueues[C0C_QUEUE_WRITE], pQueueToComplete);
  }
}

VOID SendTxBuffer(
    PC0C_IO_PORT pReadIoPort,
    PC0C_IO_PORT pWriteIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PC0C_ADAPTIVE_DELAY pWriteDelay,
    PSIZE_T pSendDone)
{
  SIZE_T sendDone;
  SIZE_T txLimit;
  C0C_FLOW_FILTER flowFilter;

  FlowFilterInit(pReadIoPort, &flowFilter);

  if (pWriteLimit)
    txLimit = *pWriteLimit;
  else
    txLimit = (SIZE_T)-1;

  if (pReadIoPort->emuOverrun) {
    SIZE_T overrun;

    sendDone = MoveFromTxBuffer(&pReadIoPort->readBuf,
                                &pWriteIoPort->txBuf, txLimit,
                                &flowFilter,
                                &overrun);

    if (overrun) {
      AlertOverrun(pReadIoPort, pQueueToComplete);
      pReadIoPort->perfStats.BufferOverrunErrorCount += (ULONG)overrun;
    }
  } else {
    sendDone = MoveFromTxBuffer(&pReadIoPort->readBuf,
                                &pWriteIoPort->txBuf, txLimit,
                                &flowFilter,
                                NULL);
  }

  if (sendDone) {
    if (pWriteLimit)
      *pWriteLimit -= sendDone;

    if (pWriteDelay) {
      pWriteDelay->sentFrames += sendDone;
      pWriteDelay->idleCount = 0;
    }

    OnRxChars(pReadIoPort, sendDone, &flowFilter, pQueueToComplete);
    *pSendDone += sendDone;
  }
}

VOID WriteToBuffers(
    PRW_DATA pDataWrite,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PC0C_ADAPTIVE_DELAY pWriteDelay,
    PSIZE_T pWriteDone,
    PSIZE_T pSendDone)
{
  SIZE_T writeLength, information;
  SIZE_T sendDone, writeDone;
  C0C_FLOW_FILTER flowFilter;
  PUCHAR pWriteBuf;
  SIZE_T length;
  SIZE_T sendLength;
  BOOLEAN isBreak;

  isBreak = FALSE;

  if (pDataWrite->type == RW_DATA_TYPE_IRP) {
    PIRP pIrp = pDataWrite->data.irp.pIrp;

    information = pIrp->IoStatus.Information;
    pWriteBuf = GET_REST_BUFFER_WRITE(pIrp, information);
    writeLength = GetWriteLength(pIrp);
  } else {
    HALT_UNLESS1(pDataWrite->type == RW_DATA_TYPE_CHR, pDataWrite->type);

    information = 0;
    pWriteBuf = &pDataWrite->data.chr.chr;
    writeLength = pDataWrite->data.chr.isChr ? 1 : 0;
    if (pDataWrite->data.chr.type == RW_DATA_TYPE_CHR_BREAK)
      isBreak = TRUE;
  }

  length = writeLength - information;

  if (pWriteLimit && length > *pWriteLimit)
    sendLength = *pWriteLimit;
  else
    sendLength = length;

  FlowFilterInit(pReadIoPort, &flowFilter);

  if (pReadIoPort->emuOverrun) {
    SIZE_T overrun;

    sendDone = WriteToBuffer(&pReadIoPort->readBuf, pWriteBuf, sendLength, &flowFilter, &overrun);

    if (overrun) {
      AlertOverrun(pReadIoPort, pQueueToComplete);
      pReadIoPort->perfStats.BufferOverrunErrorCount += (ULONG)overrun;
    }
  } else {
    sendDone = WriteToBuffer(&pReadIoPort->readBuf, pWriteBuf, sendLength, &flowFilter, NULL);
  }

  HALT_UNLESS3(sendDone <= sendLength, pDataWrite->type, sendDone, sendLength);

  writeDone = sendDone;

  if (writeDone < length)
    writeDone += WriteToTxBuffer(&pReadIoPort->pIoPortRemote->txBuf, pWriteBuf + writeDone, length - writeDone);

  HALT_UNLESS3(writeDone <= length, pDataWrite->type, writeDone, length);

  if (writeDone) {
    information += writeDone;

    if (pDataWrite->type == RW_DATA_TYPE_IRP) {
      pDataWrite->data.irp.pIrp->IoStatus.Information = information;
      pReadIoPort->pIoPortRemote->amountInWriteQueue -= (ULONG)writeDone;
    }

    if (!isBreak) {
      pReadIoPort->pIoPortRemote->perfStats.TransmittedCount += (ULONG)writeDone;
      *pWriteDone += writeDone;
    }
  }

  if (sendDone) {
    if (pWriteLimit)
      *pWriteLimit -= sendDone;

    if (pWriteDelay) {
      pWriteDelay->sentFrames += sendDone;
      pWriteDelay->idleCount = 0;
    }

    OnRxChars(pReadIoPort, sendDone, &flowFilter, pQueueToComplete);

    if (!isBreak)
      *pSendDone += sendDone;
  }

  if (information == writeLength) {
    if (pDataWrite->type == RW_DATA_TYPE_IRP) {
      pDataWrite->data.irp.status = STATUS_SUCCESS;
    } else {
      HALT_UNLESS1(pDataWrite->type == RW_DATA_TYPE_CHR, pDataWrite->type);

      pDataWrite->data.chr.isChr = FALSE;
    }
  }
}

VOID ReadWriteDirect(
    PIRP pIrpRead,
    PRW_DATA pDataWrite,
    PNTSTATUS pStatusRead,
    PC0C_IO_PORT pReadIoPort,
    PLIST_ENTRY pQueueToComplete,
    PSIZE_T pWriteLimit,
    PC0C_ADAPTIVE_DELAY pWriteDelay,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone)
{
  SIZE_T readDone, writeDone;
  SIZE_T writeLength, readLength;
  C0C_FLOW_FILTER flowFilter;
  PVOID pWriteBuf, pReadBuf;
  BOOLEAN isBreak;

  isBreak = FALSE;

  pReadBuf = GET_REST_BUFFER(pIrpRead, pIrpRead->IoStatus.Information);
  readLength = IoGetCurrentIrpStackLocation(pIrpRead)->Parameters.Read.Length
                                                - pIrpRead->IoStatus.Information;

  if (pDataWrite->type == RW_DATA_TYPE_IRP) {
    PIRP pIrpWrite = pDataWrite->data.irp.pIrp;

    pWriteBuf = GET_REST_BUFFER_WRITE(pIrpWrite, pIrpWrite->IoStatus.Information);
    writeLength = GetWriteLength(pIrpWrite) - pIrpWrite->IoStatus.Information;
  } else {
    HALT_UNLESS1(pDataWrite->type == RW_DATA_TYPE_CHR, pDataWrite->type);

    pWriteBuf = &pDataWrite->data.chr.chr;
    writeLength = pDataWrite->data.chr.isChr ? 1 : 0;
    if (pDataWrite->data.chr.type == RW_DATA_TYPE_CHR_BREAK)
      isBreak = TRUE;
  }

  FlowFilterInit(pReadIoPort, &flowFilter);

  CopyCharsWithEscape(
      &pReadIoPort->readBuf, &flowFilter,
      pReadBuf, readLength,
      pWriteBuf, (pWriteLimit && writeLength > *pWriteLimit) ? *pWriteLimit : writeLength,
      &readDone, &writeDone);

  pIrpRead->IoStatus.Information += readDone;

  if (readDone == readLength)
    *pStatusRead = STATUS_SUCCESS;

  if (writeDone == writeLength) {
    if (pDataWrite->type == RW_DATA_TYPE_IRP) {
      pDataWrite->data.irp.status = STATUS_SUCCESS;
    } else {
      HALT_UNLESS1(pDataWrite->type == RW_DATA_TYPE_CHR, pDataWrite->type);

      pDataWrite->data.chr.isChr = FALSE;
    }
  }

  if (writeDone) {
    if (pWriteLimit)
      *pWriteLimit -= writeDone;

    if (pWriteDelay) {
      pWriteDelay->sentFrames += writeDone;
      pWriteDelay->idleCount = 0;
    }

    if (pDataWrite->type == RW_DATA_TYPE_IRP) {
      pDataWrite->data.irp.pIrp->IoStatus.Information += writeDone;
      pReadIoPort->pIoPortRemote->amountInWriteQueue -= (ULONG)writeDone;
    }

    OnRxChars(pReadIoPort, writeDone, &flowFilter, pQueueToComplete);

    if (!isBreak) {
      pReadIoPort->pIoPortRemote->perfStats.TransmittedCount += (ULONG)writeDone;
      *pWriteDone += writeDone;
    }
  }

  *pReadDone += readDone;
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

    if (*ppCancelRoutine) {
#if DBG
      HALT_UNLESS(!pQueue->started);
      pQueue->started = TRUE;
#endif /* DBG */
      return pIrp;
    }

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
  PC0C_IRP_STATE pState;

#if DBG
  HALT_UNLESS(pQueue->started);
  pQueue->started = FALSE;
#endif /* DBG */

  pIrp = pQueue->pCurrent;

  pState = GetIrpState(pIrp);
  HALT_UNLESS(pState);

  if (status == STATUS_PENDING && done) {
    if ((pState->flags & C0C_IRP_FLAG_WAIT_ONE) != 0) {
      status = STATUS_SUCCESS;
    }
    else
    if (first && (pState->flags & C0C_IRP_FLAG_INTERVAL_TIMEOUT) != 0) {
      SetIntervalTimeout(pIoPort);
    }
  }

  if (status == STATUS_PENDING && (pState->flags & C0C_IRP_FLAG_EXPIRED) != 0) {
    status = STATUS_TIMEOUT;
  }
  else
  if (!first && status == STATUS_PENDING) {
    status = SetIrpTimeout(pIoPort, pIrp);
  }

  HALT_UNLESS(pCancelRoutine);

  if (status == STATUS_PENDING) {
    #pragma warning(push, 3)
    IoSetCancelRoutine(pIrp, pCancelRoutine);
    #pragma warning(pop)
    if (pIrp->Cancel) {
      ShiftQueue(pQueue);
      CompleteIrp(pIrp, STATUS_CANCELLED, pQueueToComplete);
      return STATUS_CANCELLED;
    }
  } else {
    ShiftQueue(pQueue);
    pIrp->IoStatus.Status = status;
    InsertTailList(pQueueToComplete, &pIrp->Tail.Overlay.ListEntry);
  }

  return status;
}

VOID StartXoffCounter(PC0C_IO_PORT pIoPortWrite, PLIST_ENTRY pQueueToComplete)
{
  PIRP pIrpXoffCounter;
  PC0C_IRP_STATE pState;
  PC0C_IRP_QUEUE pQueue;

  pQueue = &pIoPortWrite->irpQueues[C0C_QUEUE_WRITE];
  HALT_UNLESS(!pQueue->started);

  pIrpXoffCounter = pQueue->pCurrent;
  HALT_UNLESS(pIrpXoffCounter);

  ShiftQueue(pQueue);

  while (pQueue->pCurrent) {
    PIRP pCurrent = pQueue->pCurrent;

    if (IoGetCurrentIrpStackLocation(pCurrent)->MajorFunction != IRP_MJ_FLUSH_BUFFERS) {
      CompleteIrp(pIrpXoffCounter, STATUS_SERIAL_MORE_WRITES, pQueueToComplete);
      return;
    }

    ShiftQueue(pQueue);
    CompleteIrp(pCurrent, STATUS_SUCCESS, pQueueToComplete);
  }

  pIoPortWrite->xoffCounter = ((PSERIAL_XOFF_COUNTER)pIrpXoffCounter->AssociatedIrp.SystemBuffer)->Counter;

  pState = GetIrpState(pIrpXoffCounter);
  HALT_UNLESS(pState);

  pQueue->pCurrent = pIrpXoffCounter;
  pState->flags |= C0C_IRP_FLAG_IS_CURRENT;

  SetXoffCounterTimeout(pIoPortWrite, pIrpXoffCounter);
}

NTSTATUS FdoPortIo(
    short ioType,
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

  first = TRUE;

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
    case C0C_IO_TYPE_CLOSE_COMPLETE:
      InterlockedDecrement(&pIoPort->pDevExt->openCount);
      pIrpCurrent->IoStatus.Information = 0;
      statusCurrent = STATUS_SUCCESS;
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
      UpdateHandFlow(pIoPort, FALSE, pQueueToComplete);
      break;
    }
  }
  return status;
}

VOID InsertLsrMst(
    PC0C_IO_PORT pIoPortRead,
    BOOLEAN isMST,
    UCHAR value,
    PLIST_ENTRY pQueueToComplete)
{
  C0C_RAW_DATA insertData;

  insertData.size = 3;
  insertData.data[0] = pIoPortRead->escapeChar;
  insertData.data[1] = (UCHAR)(isMST ? SERIAL_LSRMST_MST : SERIAL_LSRMST_LSR_NODATA);
  insertData.data[2] = value;

  if (FdoPortIo(
      C0C_IO_TYPE_INSERT,
      &insertData,
      pIoPortRead,
      &pIoPortRead->irpQueues[C0C_QUEUE_READ],
      pQueueToComplete) == STATUS_PENDING)
  {
    AlertOverrun(pIoPortRead, pQueueToComplete);
    Trace00((PC0C_COMMON_EXTENSION)pIoPortRead->pDevExt,
        L"WARNING: Lost SERIAL_LSRMST_", isMST ? L"MST" : L"LSR_NODATA");
  }
}

VOID InsertChar(
    PC0C_IO_PORT pIoPortRead,
    UCHAR value,
    PLIST_ENTRY pQueueToComplete)
{
  C0C_RAW_DATA insertData;

  insertData.size = 1;
  insertData.data[0] = value;

  if (FdoPortIo(
      C0C_IO_TYPE_INSERT,
      &insertData,
      pIoPortRead,
      &pIoPortRead->irpQueues[C0C_QUEUE_READ],
      pQueueToComplete) == STATUS_PENDING)
  {
    AlertOverrun(pIoPortRead, pQueueToComplete);
    Trace0((PC0C_COMMON_EXTENSION)pIoPortRead->pDevExt, L"WARNING: Lost char");
  }
}

VOID InsertRemoteBr(
    PC0C_IO_PORT pIoPortRead,
    PLIST_ENTRY pQueueToComplete)
{
  C0C_RAW_DATA insertData;

  insertData.size = 2 + sizeof(ULONG);
  insertData.data[0] = pIoPortRead->escapeChar;
  insertData.data[1] = C0CE_INSERT_RBR;
  *(ULONG *)&insertData.data[2] = pIoPortRead->pIoPortRemote->baudRate.BaudRate;

  if (FdoPortIo(
      C0C_IO_TYPE_INSERT,
      &insertData,
      pIoPortRead,
      &pIoPortRead->irpQueues[C0C_QUEUE_READ],
      pQueueToComplete) == STATUS_PENDING)
  {
    AlertOverrun(pIoPortRead, pQueueToComplete);
    Trace0((PC0C_COMMON_EXTENSION)pIoPortRead->pDevExt, L"WARNING: Lost C0CE_INSERT_RBR");
  }
}

VOID InsertRemoteLc(
    PC0C_IO_PORT pIoPortRead,
    PLIST_ENTRY pQueueToComplete)
{
  C0C_RAW_DATA insertData;

  insertData.size = 5;
  insertData.data[0] = pIoPortRead->escapeChar;
  insertData.data[1] = C0CE_INSERT_RLC;
  insertData.data[2] = pIoPortRead->pIoPortRemote->lineControl.WordLength;
  insertData.data[3] = pIoPortRead->pIoPortRemote->lineControl.Parity;
  insertData.data[4] = pIoPortRead->pIoPortRemote->lineControl.StopBits;

  if (FdoPortIo(
      C0C_IO_TYPE_INSERT,
      &insertData,
      pIoPortRead,
      &pIoPortRead->irpQueues[C0C_QUEUE_READ],
      pQueueToComplete) == STATUS_PENDING)
  {
    AlertOverrun(pIoPortRead, pQueueToComplete);
    Trace0((PC0C_COMMON_EXTENSION)pIoPortRead->pDevExt, L"WARNING: Lost C0CE_INSERT_RLC");
  }
}

NTSTATUS TryReadWrite(
    PC0C_IO_PORT pIoPortRead,
    BOOLEAN startRead,
    PC0C_IO_PORT pIoPortWrite,
    BOOLEAN startWrite,
    PLIST_ENTRY pQueueToComplete)
{
/*
 * With the real serial ports the data goes this way:
 *
 * ---------software--------|------------hardware---------------
 * TX QUEUE ------------------> TX FIFO --> TX shift register -
 *                                                             |
 * RX QUEUE <-- RX buffer <--- RX FIFO <-- RX shift register <-
 * -------------------------|-----------------------------------
 *
 * From application view point it's equivalent to:
 *
 * -------------------------------------------------------------
 * TX QUEUE ------------------> TX buffer ---------------------
 *                                                             |
 * RX QUEUE <-- RX buffer <------------------------------------
 * -------------------------------------------------------------
 *
 * Here the data goes by following steps:
 *
 *  - Move data from RX buffer to RX QUEUE while RX QUEUE is not empty;
 *  - Move shifted data from TX buffer to RX QUEUE while RX QUEUE is not empty;
 *  - Move shifted data from TX buffer to RX buffer if RX QUEUE is empty or
 *    move shifted data from TX QUEUE to RX QUEUE while RX QUEUE is not empty;
 *  - Move shifted data from TX QUEUE to RX buffer;
 *  - Move not shifted data from TX QUEUE to TX buffer while TX buffer not full;
 *
 */
  NTSTATUS status;
  SIZE_T readBufBusyBeg, readBufBusyEnd;

  RW_DATA dataIrpRead;
  PC0C_IRP_QUEUE pQueueRead;
  BOOLEAN firstRead;
  PDRIVER_CANCEL pCancelRoutineRead;
  SIZE_T doneRead;

  RW_DATA dataChar;

  RW_DATA dataIrpWrite;
  PC0C_IRP_QUEUE pQueueWrite;
  BOOLEAN firstWrite;
  PDRIVER_CANCEL pCancelRoutineWrite;
  SIZE_T doneWrite;
  SIZE_T doneSend;

  PC0C_ADAPTIVE_DELAY pWriteDelay;
  SIZE_T writeLimit;
  PSIZE_T pWriteLimit;

  dataIrpRead.type = RW_DATA_TYPE_IRP;
  dataChar.type = RW_DATA_TYPE_CHR;
  dataIrpWrite.type = RW_DATA_TYPE_IRP;

  pQueueRead = &pIoPortRead->irpQueues[C0C_QUEUE_READ];
  pQueueWrite = &pIoPortWrite->irpQueues[C0C_QUEUE_WRITE];
  pWriteDelay = pIoPortWrite->pWriteDelay;
  status = STATUS_SUCCESS;

  if (pWriteDelay) {
    if (C0C_TX_BUFFER_BUSY(&pIoPortWrite->txBuf) ||
        pQueueWrite->pCurrent ||
        pIoPortWrite->sendBreak ||
        pIoPortWrite->sendXonXoff)
    {
      StartWriteDelayTimer(pWriteDelay);
      writeLimit = GetWriteLimit(pWriteDelay);
    } else {
      writeLimit = 0;
    }

    pWriteLimit = &writeLimit;
  } else {
    pWriteLimit = NULL;
  }

  doneSend = 0;
  readBufBusyBeg = C0C_BUFFER_BUSY(&pIoPortRead->readBuf);

  /******************************************************************************
   * Prepare RX QUEUE                                                           *
   ******************************************************************************/

  /* get first pIrpRead */

  dataIrpRead.data.irp.status = STATUS_PENDING;
  doneRead = 0;
  firstRead = TRUE;

  if (startRead) {
    dataIrpRead.data.irp.pIrp = pQueueRead->pCurrent;
    pCancelRoutineRead = NULL;
  } else {
    dataIrpRead.data.irp.pIrp = StartCurrentIrp(pQueueRead, &pCancelRoutineRead, &firstRead);
  }

  /******************************************************************************
   * Move data from RX buffer to RX QUEUE while RX QUEUE is not empty           *
   * Move shifted data from TX buffer to RX QUEUE while RX QUEUE is not empty   *
   ******************************************************************************/

  while (dataIrpRead.data.irp.pIrp) {
    dataIrpRead.data.irp.status = ReadFromBuffers(dataIrpRead.data.irp.pIrp,
                                                  pIoPortRead,
                                                  pQueueToComplete,
                                                  pWriteLimit,
                                                  pWriteDelay,
                                                  &doneRead, &doneSend);

    if (dataIrpRead.data.irp.status == STATUS_PENDING)
      break;

    if (startRead && firstRead) {
      status = dataIrpRead.data.irp.status;
      ShiftQueue(pQueueRead);
    } else {
      StopCurrentIrp(dataIrpRead.data.irp.status, pCancelRoutineRead, firstRead,
                     doneRead, pIoPortRead, pQueueRead, pQueueToComplete);
    }

    /* get next pIrpRead */

    doneRead = 0;
    firstRead = FALSE;

    dataIrpRead.data.irp.pIrp =
        StartCurrentIrp(pQueueRead, &pCancelRoutineRead, &firstRead);
  }

  /******************************************************************************
   * Move shifted data from TX buffer to RX buffer if RX QUEUE is empty         *
   ******************************************************************************/

  if (!dataIrpRead.data.irp.pIrp)
    SendTxBuffer(pIoPortRead, pIoPortWrite, pQueueToComplete, pWriteLimit, pWriteDelay, &doneSend);

  /******************************************************************************
   * Prepare TX QUEUE                                                           *
   ******************************************************************************/

  /* get special char to send */

  if (pIoPortWrite->sendBreak) {
    /* get BREAK char */

    dataChar.data.chr.type = RW_DATA_TYPE_CHR_BREAK;
    dataChar.data.chr.chr = 0;
    dataChar.data.chr.isChr = TRUE;
  } else {
    /* get XON or XOFF char */

    switch (pIoPortWrite->sendXonXoff) {
    case C0C_XCHAR_ON:
      dataChar.data.chr.type = RW_DATA_TYPE_CHR_XCHR;
      dataChar.data.chr.chr = pIoPortWrite->specialChars.XonChar;
      dataChar.data.chr.isChr = TRUE;
      break;
    case C0C_XCHAR_OFF:
      dataChar.data.chr.type = RW_DATA_TYPE_CHR_XCHR;
      dataChar.data.chr.chr = pIoPortWrite->specialChars.XoffChar;
      dataChar.data.chr.isChr = TRUE;
      break;
    default:
      dataChar.data.chr.type = RW_DATA_TYPE_CHR_NONE;
      dataChar.data.chr.isChr = FALSE;
    }
  }

  /* get first pIrpWrite */

  doneWrite = 0;
  firstWrite = TRUE;

  if(startWrite) {
    dataIrpWrite.data.irp.pIrp = pQueueWrite->pCurrent;
    pCancelRoutineWrite = NULL;
  } else {
    dataIrpWrite.data.irp.pIrp =
        StartCurrentIrp(pQueueWrite, &pCancelRoutineWrite, &firstWrite);
  }

  /******************************************************************************
   * Move shifted data from TX QUEUE to RX QUEUE while RX QUEUE is not empty    *
   ******************************************************************************/

  while (dataIrpRead.data.irp.pIrp) {
    if (dataChar.data.chr.isChr) {
      if (!pWriteLimit || *pWriteLimit) {
        if (CAN_WRITE_RW_DATA_CHR(pIoPortWrite, dataChar)) {
          if (dataIrpRead.data.irp.status == STATUS_PENDING) {
            ReadWriteDirect(dataIrpRead.data.irp.pIrp,
                            &dataChar,
                            &dataIrpRead.data.irp.status,
                            pIoPortRead,
                            pQueueToComplete,
                            pWriteLimit,
                            pWriteDelay,
                            &doneRead, &doneSend);
          }
        }
        else
        if (pWriteDelay) {
          pWriteDelay->sentFrames += *pWriteLimit;
          *pWriteLimit = 0;
        }
      }
    }

    while (dataIrpWrite.data.irp.pIrp) {
      PIO_STACK_LOCATION pIrpStackWrite = IoGetCurrentIrpStackLocation(dataIrpWrite.data.irp.pIrp);

      if (pIrpStackWrite->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {
        dataIrpWrite.data.irp.status = STATUS_SUCCESS;
      } else {
        dataIrpWrite.data.irp.status = STATUS_PENDING;

        if (!pWriteLimit || *pWriteLimit) {
          if (!pIoPortWrite->writeHolding) {
            if (dataIrpRead.data.irp.status == STATUS_PENDING) {
              SIZE_T done = 0;

              ReadWriteDirect(dataIrpRead.data.irp.pIrp,
                              &dataIrpWrite,
                              &dataIrpRead.data.irp.status,
                              pIoPortRead,
                              pQueueToComplete,
                              pWriteLimit,
                              pWriteDelay,
                              &doneRead, &done);

              if (done) {
                doneWrite += done;
                doneSend += done;
              }
            }
          }
          else
          if (pWriteDelay) {
            pWriteDelay->sentFrames += *pWriteLimit;
            *pWriteLimit = 0;
          }
        }
      }

      if (dataIrpWrite.data.irp.status == STATUS_PENDING)
        break;

      /* stop current pIrpWrite */

      if (dataIrpWrite.data.irp.status == STATUS_SUCCESS &&
          pIrpStackWrite->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
          pIrpStackWrite->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)
      {
        if(startWrite && firstWrite)
          status = STATUS_PENDING;
        else
          StopCurrentIrp(STATUS_PENDING, pCancelRoutineWrite, firstWrite,
                         doneWrite, pIoPortWrite, pQueueWrite, pQueueToComplete);

        if (dataIrpWrite.data.irp.pIrp == pQueueWrite->pCurrent) {
          if (doneWrite)
            StartXoffCounter(pIoPortWrite, pQueueToComplete);

          if (dataIrpWrite.data.irp.pIrp == pQueueWrite->pCurrent) {
            dataIrpWrite.data.irp.pIrp = NULL;
            break;
          }
        }
      } else {
        if(startWrite && firstWrite) {
          status = dataIrpWrite.data.irp.status;
          ShiftQueue(pQueueWrite);
        } else {
          StopCurrentIrp(dataIrpWrite.data.irp.status, pCancelRoutineWrite, firstWrite,
                         doneWrite, pIoPortWrite, pQueueWrite, pQueueToComplete);
        }
      }

      /* get next pIrpWrite */

      doneWrite = 0;
      firstWrite = FALSE;

      dataIrpWrite.data.irp.pIrp =
          StartCurrentIrp(pQueueWrite, &pCancelRoutineWrite, &firstWrite);
    }

    if (startRead && firstRead) {
      if (dataIrpRead.data.irp.status == STATUS_PENDING)
        dataIrpRead.data.irp.status =
            SetIrpTimeout(pIoPortRead, dataIrpRead.data.irp.pIrp);

      status = dataIrpRead.data.irp.status;

      if (dataIrpRead.data.irp.status != STATUS_PENDING)
        ShiftQueue(pQueueRead);
    } else {
      dataIrpRead.data.irp.status = StopCurrentIrp(
          dataIrpRead.data.irp.status, pCancelRoutineRead, firstRead,
          doneRead, pIoPortRead, pQueueRead, pQueueToComplete);
    }

    /* get next pIrpRead */

    if (dataIrpRead.data.irp.status != STATUS_PENDING) {
      dataIrpRead.data.irp.status = STATUS_PENDING;
      doneRead = 0;
      firstRead = FALSE;
      dataIrpRead.data.irp.pIrp =
          StartCurrentIrp(pQueueRead, &pCancelRoutineRead, &firstRead);
    } else {
      dataIrpRead.data.irp.pIrp = NULL;
    }
  }

  /******************************************************************************
   * Move shifted data from TX QUEUE to RX buffer                               *
   * Move not shifted data from TX QUEUE to TX buffer while TX buffer not full  *
   ******************************************************************************/

  if (dataChar.data.chr.isChr) {
    if (CAN_WRITE_RW_DATA_CHR(pIoPortWrite, dataChar)) {
      SIZE_T done = 0;

      WriteToBuffers(&dataChar, pIoPortRead,
                     pQueueToComplete, pWriteLimit, pWriteDelay, &done, &doneSend);
    }
    else
    if (pWriteDelay) {
      pWriteDelay->sentFrames += *pWriteLimit;
      *pWriteLimit = 0;
    }
  }

  if (!dataChar.data.chr.isChr) {
    /* complete special char sending */

    switch (dataChar.data.chr.type) {
    case RW_DATA_TYPE_CHR_XCHR:
      pIoPortWrite->sendXonXoff = 0;
      break;
    case RW_DATA_TYPE_CHR_BREAK:
      if (pIoPortWrite->sendBreak) {
        pIoPortWrite->sendBreak = FALSE;

        pIoPortRead->errors |= SERIAL_ERROR_BREAK;
        pIoPortRead->eventMask |= pIoPortRead->waitMask & (SERIAL_EV_BREAK | SERIAL_EV_ERR);

        if (pIoPortRead->eventMask)
          WaitComplete(pIoPortRead, pQueueToComplete);

        if (pIoPortRead->escapeChar && (pIoPortRead->insertMask & C0CE_INSERT_ENABLE_LSR)) {
          UCHAR lsr = 0x10;  /* break interrupt indicator */

          if (!pIoPortRead->amountInWriteQueue || pIoPortRead->writeHolding)
            lsr |= 0x60;  /* transmit holding register empty and transmitter empty indicators */

          InsertLsrMst(pIoPortRead, FALSE,  lsr, pQueueToComplete);
        }
        if (pIoPortRead->handFlow.FlowReplace & SERIAL_BREAK_CHAR)
          InsertChar(pIoPortRead, pIoPortRead->specialChars.BreakChar, pQueueToComplete);
      }
      break;
    }
  }

  while (dataIrpWrite.data.irp.pIrp) {
    PIO_STACK_LOCATION pIrpStackWrite = IoGetCurrentIrpStackLocation(dataIrpWrite.data.irp.pIrp);

    if (pIrpStackWrite->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {
      dataIrpWrite.data.irp.status = STATUS_SUCCESS;
    } else {
      dataIrpWrite.data.irp.status = STATUS_PENDING;

      if (!pIoPortWrite->writeHolding) {
        WriteToBuffers(&dataIrpWrite, pIoPortRead,
                       pQueueToComplete, pWriteLimit, pWriteDelay, &doneWrite, &doneSend);
      }
      else
      if (pWriteDelay) {
        pWriteDelay->sentFrames += *pWriteLimit;
        *pWriteLimit = 0;
      }
    }

    /* stop current pIrpWrite */

    if (dataIrpWrite.data.irp.status == STATUS_SUCCESS &&
        pIrpStackWrite->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
        pIrpStackWrite->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)
    {
      if(startWrite && firstWrite)
        status = STATUS_PENDING;
      else
        StopCurrentIrp(STATUS_PENDING, pCancelRoutineWrite, firstWrite,
                       doneWrite, pIoPortWrite, pQueueWrite, pQueueToComplete);

      if (dataIrpWrite.data.irp.pIrp == pQueueWrite->pCurrent) {
        if (doneWrite)
          StartXoffCounter(pIoPortWrite, pQueueToComplete);

        if (dataIrpWrite.data.irp.pIrp == pQueueWrite->pCurrent)
          break;
      }
    } else {
      if(startWrite && firstWrite) {
        if (dataIrpWrite.data.irp.status == STATUS_PENDING)
          dataIrpWrite.data.irp.status =
              SetIrpTimeout(pIoPortWrite, dataIrpWrite.data.irp.pIrp);

        status = dataIrpWrite.data.irp.status;

        if (dataIrpWrite.data.irp.status != STATUS_PENDING)
          ShiftQueue(pQueueWrite);
      } else {
        dataIrpWrite.data.irp.status =
            StopCurrentIrp(dataIrpWrite.data.irp.status, pCancelRoutineWrite, firstWrite,
                           doneWrite, pIoPortWrite, pQueueWrite, pQueueToComplete);
      }
    }

    /* get next pIrpWrite */

    if (dataIrpWrite.data.irp.status != STATUS_PENDING) {
      doneWrite = 0;
      firstWrite = FALSE;
      dataIrpWrite.data.irp.pIrp = StartCurrentIrp(pQueueWrite, &pCancelRoutineWrite, &firstWrite);
    } else {
      dataIrpWrite.data.irp.pIrp = NULL;
    }
  }

  /******************************************************************************/

  readBufBusyEnd = C0C_BUFFER_BUSY(&pIoPortRead->readBuf);

  if (readBufBusyBeg > readBufBusyEnd) {
    UpdateHandFlow(pIoPortRead, TRUE, pQueueToComplete);
  }
  else
  if (readBufBusyBeg < readBufBusyEnd) {
    if ((pIoPortRead->waitMask & SERIAL_EV_RX80FULL) &&
        readBufBusyEnd > pIoPortRead->readBuf.size80 &&
        readBufBusyBeg <= pIoPortRead->readBuf.size80)
    {
      pIoPortRead->eventMask |= SERIAL_EV_RX80FULL;
      WaitComplete(pIoPortRead, pQueueToComplete);
    }

    UpdateHandFlow(pIoPortRead, FALSE, pQueueToComplete);
  }

  if ((pIoPortWrite->waitMask & SERIAL_EV_TXEMPTY) && doneSend &&
      !pQueueWrite->pCurrent && C0C_TX_BUFFER_THR_EMPTY(&pIoPortWrite->txBuf))
  {
    pIoPortWrite->eventMask |= SERIAL_EV_TXEMPTY;
    WaitComplete(pIoPortWrite, pQueueToComplete);
  }

  UpdateTransmitToggle(pIoPortWrite, pQueueToComplete);

  return status;
}

NTSTATUS ReadWrite(
    PC0C_IO_PORT pIoPortRead,
    BOOLEAN startRead,
    PC0C_IO_PORT pIoPortWrite,
    BOOLEAN startWrite,
    PLIST_ENTRY pQueueToComplete)
{
  NTSTATUS status;

  pIoPortWrite->tryWrite = FALSE;

  status = TryReadWrite(
      pIoPortRead, startRead,
      pIoPortWrite, startWrite,
      pQueueToComplete);

  while (pIoPortWrite->tryWrite || pIoPortRead->tryWrite) {
    if (!pIoPortWrite->tryWrite) {
      PC0C_IO_PORT pIoPortTmp;

      pIoPortTmp = pIoPortRead;
      pIoPortRead = pIoPortWrite;
      pIoPortWrite = pIoPortTmp;
    }

    pIoPortWrite->tryWrite = FALSE;

    TryReadWrite(
        pIoPortRead, FALSE,
        pIoPortWrite, FALSE,
        pQueueToComplete);
  }

  return status;
}

VOID SetModemStatus(
    IN PC0C_IO_PORT pIoPort,
    IN UCHAR bits,
    IN UCHAR mask,
    PLIST_ENTRY pQueueToComplete)
{
  UCHAR modemStatusOld;
  UCHAR modemStatusNew;
  UCHAR modemStatusChanged;

  modemStatusOld = modemStatusNew = pIoPort->modemStatus;

  modemStatusNew |= bits & mask;
  modemStatusNew &= ~(~bits & mask);

  modemStatusChanged = modemStatusOld ^ modemStatusNew;

  if (modemStatusChanged) {
    pIoPort->modemStatus = modemStatusNew;

    TraceModemStatus(pIoPort);

    SetModemStatusHolding(pIoPort);

    if (pIoPort->escapeChar && (pIoPort->insertMask & C0CE_INSERT_ENABLE_MST))
      InsertLsrMst(pIoPort, TRUE,
          (UCHAR)(pIoPort->modemStatus | (modemStatusChanged >> 4)), pQueueToComplete);

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
