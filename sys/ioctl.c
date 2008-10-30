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
 * Revision 1.40  2008/10/30 07:54:37  vfrolov
 * Improved BREAK emulation
 *
 * Revision 1.39  2008/09/12 10:07:50  vfrolov
 * Fixed LSR insertion
 *
 * Revision 1.38  2008/08/19 12:40:58  vfrolov
 * Replaces C0CE_INSERT_ENABLE_LSR_NBI (insertion on BREAK OFF)
 * by C0CE_INSERT_ENABLE_LSR_BI (insertion on BREAK change)
 *
 * Revision 1.37  2008/07/11 10:38:00  vfrolov
 * Added nonstandard ability to enable LSR insertion on BREAK OFF
 *
 * Revision 1.36  2008/06/10 11:32:35  vfrolov
 * Fixed break interrupt indicator for C0CE_INSERT_IOCTL_GET
 * Added parameter checking for IOCTL_SERIAL_SET_LINE_CONTROL
 *
 * Revision 1.35  2008/04/08 10:36:16  vfrolov
 * Implemented ability to set individual pins with extended
 * IOCTL_SERIAL_SET_MODEM_CONTROL and IOCTL_SERIAL_GET_MODEM_CONTROL
 *
 * Revision 1.34  2008/03/14 15:28:39  vfrolov
 * Implemented ability to get paired port settings with
 * extended IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.33  2007/09/17 14:31:06  vfrolov
 * Implemented pseudo pin OPEN
 *
 * Revision 1.32  2007/07/09 11:15:42  vfrolov
 * Implemented IOCTL_SERIAL_SET_MODEM_CONTROL
 *
 * Revision 1.31  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.30  2007/06/01 08:36:26  vfrolov
 * Changed parameter type for SetWriteDelay()
 *
 * Revision 1.29  2007/02/20 12:05:11  vfrolov
 * Implemented IOCTL_SERIAL_XOFF_COUNTER
 * Fixed cancel and timeout routines
 *
 * Revision 1.28  2007/01/15 16:07:12  vfrolov
 * Fixed non zero Information for IOCTL_SERIAL_PURGE and IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.27  2007/01/11 14:50:29  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
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
#include "../include/cncext.h"

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

          SetModemControl(
            pIoPortLocal,
            code == IOCTL_SERIAL_SET_RTS ? C0C_MCR_RTS : 0,
            C0C_MCR_RTS,
            &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite || pIoPortLocal->tryWrite) {
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

          SetModemControl(
            pIoPortLocal,
            code == IOCTL_SERIAL_SET_DTR ? C0C_MCR_DTR : 0,
            C0C_MCR_DTR,
            &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite || pIoPortLocal->tryWrite) {
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
    case IOCTL_SERIAL_SET_MODEM_CONTROL: {
      LIST_ENTRY queueToComplete;
      UCHAR mask;
      PUCHAR pSysBuf;
      ULONG InputBufferLength;

      InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

      if (InputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;

      if (InputBufferLength >= (sizeof(ULONG) + sizeof(ULONG) + C0CE_SIGNATURE_SIZE) &&
          RtlEqualMemory(pSysBuf + sizeof(ULONG) + sizeof(ULONG), C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE))
      {
        mask = C0C_MCR_MASK & (UCHAR)*((PULONG)pSysBuf + 1);
      } else {
        mask = C0C_MCR_MASK;
      }

      InitializeListHead(&queueToComplete);

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      SetModemControl(pIoPortLocal, (UCHAR)*(PULONG)pSysBuf, mask, &queueToComplete);

      if (pIoPortLocal->pIoPortRemote->tryWrite || pIoPortLocal->tryWrite) {
        ReadWrite(
            pIoPortLocal, FALSE,
            pIoPortLocal->pIoPortRemote, FALSE,
            &queueToComplete);
      }

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_GET_MODEM_CONTROL:
    case IOCTL_SERIAL_GET_DTRRTS: {
      ULONG modemControl;
      PUCHAR pSysBuf;
      ULONG OutputBufferLength;

      OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

      if (OutputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      modemControl = pIoPortLocal->modemControl;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);

      pSysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;

      if (code == IOCTL_SERIAL_GET_DTRRTS) {
        modemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;
      } else {
        ULONG InputBufferLength;

        InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

        if (OutputBufferLength >= (sizeof(ULONG) + C0CE_SIGNATURE_SIZE) &&
            InputBufferLength >= C0CE_SIGNATURE_SIZE &&
            RtlEqualMemory(pSysBuf, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE))
        {
          RtlCopyMemory(pSysBuf + sizeof(PULONG), C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE);

          if (OutputBufferLength > (sizeof(ULONG) + C0CE_SIGNATURE_SIZE)) {
            RtlZeroMemory(pSysBuf + sizeof(ULONG) + C0CE_SIGNATURE_SIZE,
                          OutputBufferLength - (sizeof(ULONG) + C0CE_SIGNATURE_SIZE));
          }

          pIrp->IoStatus.Information = OutputBufferLength;
        } else {
          pIrp->IoStatus.Information = sizeof(ULONG);
        }

        modemControl &= C0C_MCR_MASK;
      }

      *(PULONG)pSysBuf = modemControl;

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

      SetBreakHolding(pIoPortLocal, TRUE, &queueToComplete);
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

      SetBreakHolding(pIoPortLocal, FALSE, &queueToComplete);
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
    case IOCTL_SERIAL_XOFF_COUNTER:
      status = FdoPortXoffCounter(pIoPortLocal, pIrp, pIrpStack);
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
      status = FdoPortSetTimeouts(pIoPortLocal, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_GET_TIMEOUTS:
      status = FdoPortGetTimeouts(pIoPortLocal, pIrp, pIrpStack);
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
      ULONG Information;
      ULONG optsAndBits;
      UCHAR escapeChar;
      PUCHAR pSysBuf;
      ULONG InputBufferLength;
      BOOLEAN extended;

      InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

      if (InputBufferLength < sizeof(UCHAR)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      Information = 0;
      pSysBuf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;
      escapeChar = *pSysBuf;

      if (InputBufferLength >= (sizeof(UCHAR) + C0CE_SIGNATURE_SIZE + sizeof(ULONG)) &&
          RtlEqualMemory(pSysBuf + 1, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE))
      {
        extended = TRUE;
        optsAndBits = *(ULONG *)(pSysBuf + 1 + C0CE_SIGNATURE_SIZE);

        #define C0CE_INSERT_OPTS ( \
            C0CE_INSERT_IOCTL_GET| \
            C0CE_INSERT_IOCTL_RXCLEAR)

        #define C0CE_INSERT_BITS ( \
            C0CE_INSERT_ENABLE_LSR| \
            C0CE_INSERT_ENABLE_MST| \
            C0CE_INSERT_ENABLE_RBR| \
            C0CE_INSERT_ENABLE_RLC| \
            C0CE_INSERT_ENABLE_LSR_BI)

        #define C0CE_INSERT_CAPS (C0CE_INSERT_OPTS|C0CE_INSERT_BITS)

        if (optsAndBits == C0CE_INSERT_IOCTL_CAPS) {
          optsAndBits = 0;

          Information += C0CE_SIGNATURE_SIZE + sizeof(ULONG);

          if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < Information) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
          }

          RtlCopyMemory(pSysBuf, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE);
          *(ULONG *)(pSysBuf + C0CE_SIGNATURE_SIZE) = C0CE_INSERT_CAPS;
        } else {
          if (optsAndBits & ~C0CE_INSERT_CAPS) {
            status = STATUS_INVALID_PARAMETER;
            break;
          }

          if (optsAndBits & C0CE_INSERT_IOCTL_GET) {
            if (optsAndBits & (C0CE_INSERT_ENABLE_LSR|C0CE_INSERT_ENABLE_LSR_BI))
              Information += sizeof(UCHAR)*2 + sizeof(UCHAR);
            if (optsAndBits & C0CE_INSERT_ENABLE_MST)
              Information += sizeof(UCHAR)*2 + sizeof(UCHAR);
            if (optsAndBits & C0CE_INSERT_ENABLE_RBR)
              Information += sizeof(UCHAR)*2 + sizeof(ULONG);
            if (optsAndBits & C0CE_INSERT_ENABLE_RLC)
              Information += sizeof(UCHAR)*2 + sizeof(UCHAR)*3;
          }

          if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < Information) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
          }
        }
      } else {
        extended = FALSE;
        optsAndBits = (C0CE_INSERT_ENABLE_LSR|C0CE_INSERT_ENABLE_MST);
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);

      if (escapeChar && ((escapeChar == pIoPortLocal->specialChars.XoffChar) ||
                         (escapeChar == pIoPortLocal->specialChars.XonChar) ||
                         (pIoPortLocal->handFlow.FlowReplace & SERIAL_ERROR_CHAR)))
      {
        status = STATUS_INVALID_PARAMETER;
        KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
        break;
      }

      pIoPortLocal->insertMask = optsAndBits & C0CE_INSERT_BITS;
      pIoPortLocal->escapeChar = escapeChar;

      if (extended) {
        LIST_ENTRY queueToComplete;
        PC0C_IO_PORT pIoPortRemote;

        InitializeListHead(&queueToComplete);
        pIoPortRemote = pIoPortLocal->pIoPortRemote;

        if (optsAndBits & C0CE_INSERT_IOCTL_GET) {
          if (optsAndBits & (C0CE_INSERT_ENABLE_LSR|C0CE_INSERT_ENABLE_LSR_BI)) {
            UCHAR lsr = 0;

            if (C0C_TX_BUFFER_THR_EMPTY(&pIoPortLocal->txBuf)) {
              lsr |= 0x20;  /* transmit holding register empty */

              if (C0C_TX_BUFFER_EMPTY(&pIoPortLocal->txBuf))
                lsr |= 0x40;  /* transmit holding register empty and line is idle */
            }

            if ((optsAndBits & C0CE_INSERT_ENABLE_LSR_BI) != 0 && pIoPortLocal->rcvdBreak)
              lsr |= 0x10;  /* break interrupt indicator */

            *pSysBuf++ = escapeChar;
            *pSysBuf++ = SERIAL_LSRMST_LSR_NODATA;
            *pSysBuf++ = lsr;
          }

          if (optsAndBits & C0CE_INSERT_ENABLE_MST) {
            *pSysBuf++ = escapeChar;
            *pSysBuf++ = SERIAL_LSRMST_MST;
            *pSysBuf++ = pIoPortLocal->modemStatus;
          }

          if (optsAndBits & C0CE_INSERT_ENABLE_RBR) {
            *pSysBuf++ = escapeChar;
            *pSysBuf++ = C0CE_INSERT_RBR;
            *(ULONG *)pSysBuf = pIoPortRemote->baudRate.BaudRate;
            pSysBuf += sizeof(ULONG);
          }

          if (optsAndBits & C0CE_INSERT_ENABLE_RLC) {
            *pSysBuf++ = escapeChar;
            *pSysBuf++ = C0CE_INSERT_RLC;
            *pSysBuf++ = pIoPortRemote->lineControl.WordLength;
            *pSysBuf++ = pIoPortRemote->lineControl.Parity;
            *pSysBuf++ = pIoPortRemote->lineControl.StopBits;
          }
        }

        pIrp->IoStatus.Information = Information;

        if (optsAndBits & C0CE_INSERT_IOCTL_RXCLEAR) {
          PurgeBuffer(&pIoPortLocal->readBuf);
          UpdateHandFlow(pIoPortLocal, TRUE, &queueToComplete);
          if (pIoPortLocal->tryWrite || pIoPortRemote->tryWrite) {
            ReadWrite(
                pIoPortLocal, FALSE,
                pIoPortRemote, FALSE,
                &queueToComplete);
          }
        }

        KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
        FdoPortCompleteQueue(&queueToComplete);

        TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
        break;
      }

      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    }
    case IOCTL_SERIAL_SET_LINE_CONTROL: {
      PSERIAL_LINE_CONTROL pLineControl;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pLineControl = (PSERIAL_LINE_CONTROL)pIrp->AssociatedIrp.SystemBuffer;

      switch (pLineControl->WordLength) {
        case 5:
        case 6:
        case 7:
        case 8:
          break;
        default:
          status = STATUS_INVALID_PARAMETER;
      }

      switch (pLineControl->Parity) {
        case NO_PARITY:
        case ODD_PARITY:
        case EVEN_PARITY:
        case MARK_PARITY:
        case SPACE_PARITY:
          break;
        default:
          status = STATUS_INVALID_PARAMETER;
      }

      switch (pLineControl->StopBits) {
        case STOP_BIT_1:
        case STOP_BITS_1_5:
        case STOP_BITS_2:
          break;
        default:
          status = STATUS_INVALID_PARAMETER;
      }

      if (status == STATUS_INVALID_PARAMETER)
        break;

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      if (pIoPortLocal->lineControl.StopBits != pLineControl->StopBits ||
          pIoPortLocal->lineControl.Parity != pLineControl->Parity ||
          pIoPortLocal->lineControl.WordLength != pLineControl->WordLength)
      {
        PC0C_IO_PORT pIoPortRemote;

        pIoPortLocal->lineControl = *pLineControl;
        SetWriteDelay(pIoPortLocal);

        pIoPortRemote = pIoPortLocal->pIoPortRemote;

        if (pIoPortRemote->escapeChar && (pIoPortRemote->insertMask & C0CE_INSERT_ENABLE_RLC)) {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          InsertRemoteLc(pIoPortRemote, &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite || pIoPortLocal->tryWrite) {
            ReadWrite(
                pIoPortLocal, FALSE,
                pIoPortLocal->pIoPortRemote, FALSE,
                &queueToComplete);
          }

          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
          break;
        }
      }
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    }
    case IOCTL_SERIAL_GET_LINE_CONTROL:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PSERIAL_LINE_CONTROL)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->lineControl;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_BAUD_RATE: {
      PSERIAL_BAUD_RATE pBaudRate;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pBaudRate = (PSERIAL_BAUD_RATE)pIrp->AssociatedIrp.SystemBuffer;

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      if (pIoPortLocal->baudRate.BaudRate != pBaudRate->BaudRate) {
        PC0C_IO_PORT pIoPortRemote;

        pIoPortLocal->baudRate = *pBaudRate;
        SetWriteDelay(pIoPortLocal);

        pIoPortRemote = pIoPortLocal->pIoPortRemote;

        if (pIoPortRemote->escapeChar && (pIoPortRemote->insertMask & C0CE_INSERT_ENABLE_RBR)) {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          InsertRemoteBr(pIoPortRemote, &queueToComplete);

          if (pIoPortLocal->pIoPortRemote->tryWrite || pIoPortLocal->tryWrite) {
            ReadWrite(
                pIoPortLocal, FALSE,
                pIoPortLocal->pIoPortRemote, FALSE,
                &queueToComplete);
          }

          KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
          break;
        }
      }
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      break;
    }
    case IOCTL_SERIAL_GET_BAUD_RATE:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PSERIAL_BAUD_RATE)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->baudRate;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
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
        pBase = C0C_ALLOCATE_POOL_WITH_QUOTA(NonPagedPool, pSysBuf->InSize);
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

      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      *(PSERIALPERF_STATS)pIrp->AssociatedIrp.SystemBuffer = pIoPortLocal->perfStats;
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIALPERF_STATS);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_CLEAR_STATS:
      KeAcquireSpinLock(pIoPortLocal->pIoLock, &oldIrql);
      RtlZeroMemory(&pIoPortLocal->perfStats, sizeof(pIoPortLocal->perfStats));
      KeReleaseSpinLock(pIoPortLocal->pIoLock, oldIrql);
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
