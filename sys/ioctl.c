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
 * Revision 1.26  2006/08/23 13:16:50  vfrolov
 * Moved code for IOCTL_SERIAL_GET_PROPERTIES to commprop.c
 *
 * Revision 1.25  2006/07/17 09:58:21  vfrolov
 * Added #if DBG
 *
 * Revision 1.24  2006/06/23 11:44:52  vfrolov
 * Mass replacement pDevExt by pIoPort
 *
 * Revision 1.23  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.22  2006/05/19 15:02:03  vfrolov
 * Implemented IOCTL_SERIAL_GET_MODEM_CONTROL
 *
 * Revision 1.21  2006/05/18 09:38:10  vfrolov
 * Implemented SERIAL_TX_WAITING_XOFF_SENT
 *
 * Revision 1.20  2006/05/18 08:00:30  vfrolov
 * Implemented SERIAL_RX_WAITING_FOR_DSR
 *
 * Revision 1.19  2006/05/17 15:31:14  vfrolov
 * Implemented SERIAL_TRANSMIT_TOGGLE
 *
 * Revision 1.18  2006/04/14 15:57:51  vfrolov
 * Fixed XON char sending delay after SERIAL_PURGE_RXCLEAR
 *   and IOCTL_SERIAL_SET_QUEUE_SIZE
 *
 * Revision 1.17  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
 * Revision 1.16  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.15  2005/12/28 10:01:59  vfrolov
 * Added stub for IOCTL_SERIAL_SET_XON
 *
 * Revision 1.14  2005/12/06 13:01:54  vfrolov
 * Implemented IOCTL_SERIAL_GET_DTRRTS
 *
 * Revision 1.13  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.12  2005/11/30 16:04:12  vfrolov
 * Implemented IOCTL_SERIAL_GET_STATS and IOCTL_SERIAL_CLEAR_STATS
 *
 * Revision 1.11  2005/11/29 16:16:46  vfrolov
 * Removed FdoPortCancelQueue()
 *
 * Revision 1.10  2005/11/29 12:33:21  vfrolov
 * Changed SetModemStatus() to ability set and clear bits simultaneously
 *
 * Revision 1.9  2005/11/28 12:57:16  vfrolov
 * Moved some C0C_BUFFER code to bufutils.c
 *
 * Revision 1.8  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.7  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.6  2005/07/14 12:29:23  vfrolov
 * Fixed BSOD on IRP_MJ_READ after IOCTL_SERIAL_SET_QUEUE_SIZE
 *
 * Revision 1.5  2005/05/19 08:23:41  vfrolov
 * Fixed data types
 *
 * Revision 1.4  2005/05/14 17:07:02  vfrolov
 * Implemented SERIAL_LSRMST_MST insertion
 *
 * Revision 1.3  2005/05/13 16:58:03  vfrolov
 * Implemented IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.2  2005/02/01 16:47:57  vfrolov
 * Implemented SERIAL_PURGE_RXCLEAR and IOCTL_SERIAL_GET_COMMSTATUS
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
#include "commprop.h"

NTSTATUS FdoPortIoCtl(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
  ULONG code = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
  KIRQL oldIrql;
  PC0C_IO_PORT pIoPortLocal;

  pIrp->IoStatus.Information = 0;
  pIoPortLocal = pDevExt->pIoPortLocal;

  if ((pIoPortLocal->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) &&
      pIoPortLocal->errors && code != IOCTL_SERIAL_GET_COMMSTATUS)
  {
    status = STATUS_CANCELLED;
  } else {
    status = STATUS_SUCCESS;

    switch (code) {
    case IOCTL_SERIAL_SET_RTS:
    case IOCTL_SERIAL_CLR_RTS:
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      switch (pIoPortLocal->handFlow.FlowReplace & SERIAL_RTS_MASK) {
        case SERIAL_RTS_HANDSHAKE:
        case SERIAL_TRANSMIT_TOGGLE:
          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          status = STATUS_INVALID_PARAMETER;
          break;
        default: {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          SetModemStatus(
            pIoPortLocal->pIoPortRemote,
            code == IOCTL_SERIAL_SET_RTS ? C0C_MSB_CTS : 0,
            C0C_MSB_CTS,
            &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite) {
            ReadWrite(
                pIoPortLocal, FALSE,
                pIoPortLocal->pIoPortRemote, FALSE,
                &queueToComplete);
          }

          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
        }
      }
      break;
    case IOCTL_SERIAL_SET_DTR:
    case IOCTL_SERIAL_CLR_DTR:
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      switch (pIoPortLocal->handFlow.ControlHandShake & SERIAL_DTR_MASK) {
        case SERIAL_DTR_HANDSHAKE:
          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          status = STATUS_INVALID_PARAMETER;
          break;
        default: {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          SetModemStatus(
            pIoPortLocal->pIoPortRemote,
            code == IOCTL_SERIAL_SET_DTR ? C0C_MSB_DSR : 0,
            C0C_MSB_DSR,
            &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite) {
            ReadWrite(
                pIoPortLocal, FALSE,
                pIoPortLocal->pIoPortRemote, FALSE,
                &queueToComplete);
          }

          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
        }
      }
      break;
    case IOCTL_SERIAL_GET_MODEM_CONTROL:
    case IOCTL_SERIAL_GET_DTRRTS: {
      ULONG modemStatusRemote;

      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      modemStatusRemote = pIoPortLocal->pIoPortRemote->modemStatus;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);

      *(PULONG)pIrp->AssociatedIrp.SystemBuffer =
          ((modemStatusRemote & C0C_MSB_DSR) ? SERIAL_DTR_STATE : 0) |
          ((modemStatusRemote & C0C_MSB_CTS) ? SERIAL_RTS_STATE : 0);

      if (code == IOCTL_SERIAL_GET_MODEM_CONTROL)
        *(PULONG)pIrp->AssociatedIrp.SystemBuffer |= 0x8;

      pIrp->IoStatus.Information = sizeof(ULONG);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    }
    case IOCTL_SERIAL_SET_XON: {
      LIST_ENTRY queueToComplete;

      InitializeListHead(&queueToComplete);

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      SetXonXoffHolding(pIoPortLocal, C0C_XCHAR_ON);

      if (pIoPortLocal->tryWrite) {
        ReadWrite(
            pIoPortLocal, FALSE,
            pIoPortLocal->pIoPortRemote, FALSE,
            &queueToComplete);
      }
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_SET_XOFF:
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      SetXonXoffHolding(pIoPortLocal, C0C_XCHAR_OFF);
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    case IOCTL_SERIAL_SET_BREAK_ON: {
      LIST_ENTRY queueToComplete;

      InitializeListHead(&queueToComplete);

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      SetBreakHolding(pIoPortLocal, TRUE);
      UpdateTransmitToggle(pIoPortLocal, &queueToComplete);

      ReadWrite(
          pIoPortLocal, FALSE,
          pIoPortLocal->pIoPortRemote, FALSE,
          &queueToComplete);

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_SET_BREAK_OFF:{
      LIST_ENTRY queueToComplete;

      InitializeListHead(&queueToComplete);

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      SetBreakHolding(pIoPortLocal, FALSE);
      UpdateTransmitToggle(pIoPortLocal, &queueToComplete);

      if (pIoPortLocal->tryWrite || pIoPortLocal->pIoPortRemote->tryWrite) {
        ReadWrite(
            pIoPortLocal, FALSE,
            pIoPortLocal->pIoPortRemote, FALSE,
            &queueToComplete);
      }
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_GET_MODEMSTATUS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->modemStatus;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(ULONG);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_WAIT_MASK:
      status = FdoPortSetWaitMask(pIoPortLocal, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_GET_WAIT_MASK:
      status = FdoPortGetWaitMask(pIoPortLocal, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_WAIT_ON_MASK:
      status = FdoPortWaitOnMask(pIoPortLocal, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
      status = FdoPortImmediateChar(pIoPortLocal, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_PURGE: {
      LIST_ENTRY queueToComplete;
      PULONG pSysBuf;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

      if (*pSysBuf & ~(
          SERIAL_PURGE_TXABORT |
          SERIAL_PURGE_RXABORT |
          SERIAL_PURGE_TXCLEAR |
          SERIAL_PURGE_RXCLEAR
          )) {
        status = STATUS_INVALID_PARAMETER;
        break;
      }

      InitializeListHead(&queueToComplete);
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      if (*pSysBuf & SERIAL_PURGE_RXABORT)
        CancelQueue(&pIoPortLocal->irpQueues[C0C_QUEUE_READ], &queueToComplete);

      if (*pSysBuf & SERIAL_PURGE_TXABORT)
        CancelQueue(&pIoPortLocal->irpQueues[C0C_QUEUE_WRITE], &queueToComplete);

      if (*pSysBuf & SERIAL_PURGE_RXCLEAR) {
        PurgeBuffer(&pIoPortLocal->readBuf);
        UpdateHandFlow(pIoPortLocal, TRUE, &queueToComplete);
        if (pIoPortLocal->tryWrite || pIoPortLocal->pIoPortRemote->tryWrite) {
          ReadWrite(
              pIoPortLocal, FALSE,
              pIoPortLocal->pIoPortRemote, FALSE,
              &queueToComplete);
        }
      }

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);

      pIrp->IoStatus.Information = sizeof(ULONG);
      break;
    }
    case IOCTL_SERIAL_GET_COMMSTATUS: {
      PSERIAL_STATUS pSysBuf;
      PIRP pIrpWrite;

      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PSERIAL_STATUS)pIrp->AssociatedIrp.SystemBuffer;
      RtlZeroMemory(pSysBuf, sizeof(*pSysBuf));

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      pSysBuf->AmountInInQueue = (ULONG)C0C_BUFFER_BUSY(&pIoPortLocal->readBuf);

      pIrpWrite = pIoPortLocal->irpQueues[C0C_QUEUE_WRITE].pCurrent;

      if (pIrpWrite) {
        PIO_STACK_LOCATION pIrpStackWrite = IoGetCurrentIrpStackLocation(pIrpWrite);

        if (pIrpStackWrite->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            pIrpStackWrite->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR)
        {
          pSysBuf->WaitForImmediate = TRUE;
        }
      }

      pSysBuf->AmountInOutQueue = pIoPortLocal->amountInWriteQueue;
      pSysBuf->HoldReasons = pIoPortLocal->writeHolding;

      if ((pIoPortLocal->handFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) &&
          (pIoPortLocal->modemStatus & C0C_MSB_DSR) == 0)
      {
        pSysBuf->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;
      }

      if (pIoPortLocal->writeHoldingRemote & SERIAL_TX_WAITING_FOR_XON)
        pSysBuf->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;

      pSysBuf->Errors = pIoPortLocal->errors;
      pIoPortLocal->errors = 0;

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);

      pIrp->IoStatus.Information = sizeof(SERIAL_STATUS);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);

      break;
    }
    case IOCTL_SERIAL_SET_HANDFLOW: {
      LIST_ENTRY queueToComplete;
      PSERIAL_HANDFLOW pSysBuf;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_HANDFLOW)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PSERIAL_HANDFLOW)pIrp->AssociatedIrp.SystemBuffer;

      if (pSysBuf->ControlHandShake & SERIAL_CONTROL_INVALID ||
          pSysBuf->FlowReplace & SERIAL_FLOW_INVALID ||
          (pSysBuf->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_MASK ||
          pSysBuf->XonLimit < 0 ||
          pSysBuf->XoffLimit < 0)
      {
        status = STATUS_INVALID_PARAMETER;
        break;
      }

      InitializeListHead(&queueToComplete);
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      status = SetHandFlow(pIoPortLocal, pSysBuf, &queueToComplete);

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_GET_HANDFLOW:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_HANDFLOW)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PSERIAL_HANDFLOW)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->handFlow;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_TIMEOUTS:
      status = FdoPortSetTimeouts(pDevExt, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_GET_TIMEOUTS:
      status = FdoPortGetTimeouts(pDevExt, pIrp, pIrpStack);
      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_CHARS: {
      PSERIAL_CHARS pSysBuf;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_CHARS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PSERIAL_CHARS)pIrp->AssociatedIrp.SystemBuffer;

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      if (pIoPortLocal->escapeChar &&
            ((pIoPortLocal->escapeChar == pSysBuf->XoffChar) ||
                (pIoPortLocal->escapeChar == pSysBuf->XonChar)))
      {
        status = STATUS_INVALID_PARAMETER;
      }

      if (status == STATUS_SUCCESS)
        pIoPortLocal->specialChars = *pSysBuf;

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    }
    case IOCTL_SERIAL_GET_CHARS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_CHARS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PSERIAL_CHARS)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->specialChars;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);

      pIrp->IoStatus.Information = sizeof(SERIAL_CHARS);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_LSRMST_INSERT: {
      UCHAR escapeChar;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      escapeChar = *(PUCHAR)pIrp->AssociatedIrp.SystemBuffer;

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      if (escapeChar && ((escapeChar == pIoPortLocal->specialChars.XoffChar) ||
                         (escapeChar == pIoPortLocal->specialChars.XonChar) ||
                         (pIoPortLocal->handFlow.FlowReplace & SERIAL_ERROR_CHAR)))
      {
        status = STATUS_INVALID_PARAMETER;
      }

      if (status == STATUS_SUCCESS) {
        pIoPortLocal->escapeChar = escapeChar;
        pIrp->IoStatus.Information = sizeof(UCHAR);
      }

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    }
    case IOCTL_SERIAL_SET_LINE_CONTROL:
      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      pDevExt->lineControl = *(PSERIAL_LINE_CONTROL)pIrp->AssociatedIrp.SystemBuffer;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

      SetWriteDelay(pIoPortLocal);
      break;
    case IOCTL_SERIAL_GET_LINE_CONTROL:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      *(PSERIAL_LINE_CONTROL)pIrp->AssociatedIrp.SystemBuffer = pDevExt->lineControl;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_BAUD_RATE:
      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      pDevExt->baudRate = *(PSERIAL_BAUD_RATE)pIrp->AssociatedIrp.SystemBuffer;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

      SetWriteDelay(pIoPortLocal);
      break;
    case IOCTL_SERIAL_GET_BAUD_RATE:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      *(PSERIAL_BAUD_RATE)pIrp->AssociatedIrp.SystemBuffer = pDevExt->baudRate;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_GET_PROPERTIES: {
      ULONG size;

      status = GetCommProp(pDevExt,
                           pIrp->AssociatedIrp.SystemBuffer,
                           pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                           &size);

      if (status == STATUS_SUCCESS)
        pIrp->IoStatus.Information = size;

      break;
    }
    case IOCTL_SERIAL_CONFIG_SIZE:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }
      pIrp->IoStatus.Information = sizeof(ULONG);
      *(PULONG)pIrp->AssociatedIrp.SystemBuffer = 0;
      break;
    case IOCTL_SERIAL_SET_QUEUE_SIZE: {
      PSERIAL_QUEUE_SIZE pSysBuf = (PSERIAL_QUEUE_SIZE)pIrp->AssociatedIrp.SystemBuffer;
      LIST_ENTRY queueToComplete;
      PC0C_BUFFER pReadBuf;
      PUCHAR pBase;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_QUEUE_SIZE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pReadBuf = &pIoPortLocal->readBuf;

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      if (pSysBuf->InSize <= C0C_BUFFER_SIZE(pReadBuf)) {
        KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
        break;
      }
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);

      try {
        pBase = ExAllocatePoolWithQuota(NonPagedPool, pSysBuf->InSize);
      } except (EXCEPTION_EXECUTE_HANDLER) {
        pBase = NULL;
        status = GetExceptionCode();
      }

      if (!pBase)
        break;

      InitializeListHead(&queueToComplete);
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      if (SetNewBufferBase(pReadBuf, pBase, pSysBuf->InSize)) {
        pIoPortLocal->handFlow.XoffLimit = pSysBuf->InSize >> 3;
        pIoPortLocal->handFlow.XonLimit = pSysBuf->InSize >> 1;
        SetLimit(pIoPortLocal);
        UpdateHandFlow(pIoPortLocal, TRUE, &queueToComplete);
        if (pIoPortLocal->tryWrite || pIoPortLocal->pIoPortRemote->tryWrite) {
          ReadWrite(
              pIoPortLocal, FALSE,
              pIoPortLocal->pIoPortRemote, FALSE,
              &queueToComplete);
        }
      }

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_GET_STATS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIALPERF_STATS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      *(PSERIALPERF_STATS)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->perfStats;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIALPERF_STATS);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_CLEAR_STATS:
      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      RtlZeroMemory(&pIoPortLocal->perfStats, sizeof(pIoPortLocal->perfStats));
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      break;
    default:
      status = STATUS_INVALID_PARAMETER;
    }
  }

  if (status != STATUS_PENDING) {
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}

NTSTATUS c0cIoControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

#if DBG
  ULONG code = IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode;
#endif /* DBG */

  TraceIrp("c0cIoControl", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortIoCtl((PC0C_FDOPORT_EXTENSION)pDevExt, pIrp);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

#if DBG
  if (status != STATUS_SUCCESS)
    TraceCode(pDevExt, "IOCTL_", codeNameTableIoctl, code, &status);
#endif /* DBG */

  return status;
}
