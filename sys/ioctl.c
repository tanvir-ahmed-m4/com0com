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
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "timeout.h"

NTSTATUS FdoPortIoCtl(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
  ULONG code = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
  KIRQL oldIrql;

  status = STATUS_SUCCESS;
  pIrp->IoStatus.Information = 0;

  switch (code) {
    case IOCTL_SERIAL_SET_RTS:
    case IOCTL_SERIAL_CLR_RTS:
      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
      switch (pDevExt->handFlow.FlowReplace & SERIAL_RTS_MASK) {
        case SERIAL_RTS_HANDSHAKE:
        case SERIAL_TRANSMIT_TOGGLE:
          KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
          status = STATUS_INVALID_PARAMETER;
          break;
        default: {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          SetModemStatus(
            pDevExt->pIoPortRemote,
            C0C_MSB_CTS,
            (BOOLEAN)(code == IOCTL_SERIAL_SET_RTS));

          WaitComplete(pDevExt->pIoPortRemote, &queueToComplete);

          KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
        }
      }
      break;
    case IOCTL_SERIAL_SET_DTR:
    case IOCTL_SERIAL_CLR_DTR:
      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
      switch (pDevExt->handFlow.ControlHandShake & SERIAL_DTR_MASK) {
        case SERIAL_DTR_HANDSHAKE:
          KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
          status = STATUS_INVALID_PARAMETER;
          break;
        default: {
          LIST_ENTRY queueToComplete;

          InitializeListHead(&queueToComplete);

          SetModemStatus(
            pDevExt->pIoPortRemote,
            C0C_MSB_DSR,
            (BOOLEAN)(code == IOCTL_SERIAL_SET_DTR));

          WaitComplete(pDevExt->pIoPortRemote, &queueToComplete);

          KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
          FdoPortCompleteQueue(&queueToComplete);
        }
      }
      break;
    case IOCTL_SERIAL_GET_MODEMSTATUS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
      *(PULONG)pIrp->AssociatedIrp.SystemBuffer = pDevExt->pIoPortLocal->modemStatus;
      KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(ULONG);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_WAIT_MASK:
      status = FdoPortSetWaitMask(pDevExt, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_GET_WAIT_MASK:
      status = FdoPortGetWaitMask(pDevExt, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_WAIT_ON_MASK:
      status = FdoPortWaitOnMask(pDevExt, pIrp, pIrpStack);
      break;
    case IOCTL_SERIAL_PURGE: {
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

      if (*pSysBuf & SERIAL_PURGE_RXABORT)
        FdoPortCancelQueue(pDevExt, &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_READ]);

      if (*pSysBuf & SERIAL_PURGE_TXABORT)
        FdoPortCancelQueue(pDevExt, &pDevExt->pIoPortLocal->irpQueues[C0C_QUEUE_WRITE]);

      pIrp->IoStatus.Information = sizeof(ULONG);
      break;
    }
    case IOCTL_SERIAL_GET_COMMSTATUS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      *(PSERIAL_STATUS)pIrp->AssociatedIrp.SystemBuffer = pDevExt->commStatus;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      pIrp->IoStatus.Information = sizeof(SERIAL_STATUS);
      break;
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
          (pSysBuf->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_MASK
          ) {
        status = STATUS_INVALID_PARAMETER;
        break;
      }

      InitializeListHead(&queueToComplete);

      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

      pDevExt->handFlow = *pSysBuf;
      UpdateHandFlow(pDevExt, &queueToComplete);

      KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
      FdoPortCompleteQueue(&queueToComplete);
      break;
    }
    case IOCTL_SERIAL_GET_HANDFLOW:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_HANDFLOW)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);
      *(PSERIAL_HANDFLOW)pIrp->AssociatedIrp.SystemBuffer = pDevExt->handFlow;
      KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);
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
    case IOCTL_SERIAL_SET_CHARS:
      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_CHARS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      pDevExt->specialChars = *(PSERIAL_CHARS)pIrp->AssociatedIrp.SystemBuffer;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
      break;
    case IOCTL_SERIAL_GET_CHARS:
      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_CHARS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      *(PSERIAL_CHARS)pIrp->AssociatedIrp.SystemBuffer = pDevExt->specialChars;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);

      pIrp->IoStatus.Information = sizeof(SERIAL_CHARS);

      TraceIrp("FdoPortIoCtl", pIrp, &status, TRACE_FLAG_RESULTS);
      break;
    case IOCTL_SERIAL_SET_LINE_CONTROL:
      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      KeAcquireSpinLock(&pDevExt->controlLock, &oldIrql);
      pDevExt->lineControl = *(PSERIAL_LINE_CONTROL)pIrp->AssociatedIrp.SystemBuffer;
      KeReleaseSpinLock(&pDevExt->controlLock, oldIrql);
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
      PSERIAL_COMMPROP pSysBuf;

      if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_COMMPROP)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      pSysBuf = (PSERIAL_COMMPROP)pIrp->AssociatedIrp.SystemBuffer;

      RtlZeroMemory(pSysBuf, sizeof(SERIAL_COMMPROP));

      pSysBuf->PacketLength = sizeof(SERIAL_COMMPROP);
      pSysBuf->PacketVersion = 2;
      pSysBuf->ServiceMask = SERIAL_SP_SERIALCOMM;
      pSysBuf->MaxTxQueue = 0;
      pSysBuf->MaxRxQueue = 0;

      pSysBuf->MaxBaud = SERIAL_BAUD_USER;

      pSysBuf->SettableBaud =
        SERIAL_BAUD_075          |
        SERIAL_BAUD_110          |
        SERIAL_BAUD_134_5        |
        SERIAL_BAUD_150          |
        SERIAL_BAUD_300          |
        SERIAL_BAUD_600          |
        SERIAL_BAUD_1200         |
        SERIAL_BAUD_1800         |
        SERIAL_BAUD_2400         |
        SERIAL_BAUD_4800         |
        SERIAL_BAUD_7200         |
        SERIAL_BAUD_9600         |
        SERIAL_BAUD_14400        |
        SERIAL_BAUD_19200        |
        SERIAL_BAUD_38400        |
        SERIAL_BAUD_56K          |
        SERIAL_BAUD_128K         |
        SERIAL_BAUD_115200       |
        SERIAL_BAUD_57600;

      pSysBuf->ProvSubType = SERIAL_SP_RS232;

      pSysBuf->ProvCapabilities =
        SERIAL_PCF_DTRDSR        |
        SERIAL_PCF_RTSCTS        |
        SERIAL_PCF_CD            |
        SERIAL_PCF_PARITY_CHECK  |
        SERIAL_PCF_XONXOFF       |
        SERIAL_PCF_SETXCHAR      |
        SERIAL_PCF_TOTALTIMEOUTS |
        SERIAL_PCF_INTTIMEOUTS;

      pSysBuf->SettableParams =
        SERIAL_SP_PARITY         |
        SERIAL_SP_BAUD           |
        SERIAL_SP_DATABITS       |
        SERIAL_SP_STOPBITS       |
        SERIAL_SP_HANDSHAKING    |
        SERIAL_SP_PARITY_CHECK   |
        SERIAL_SP_CARRIER_DETECT;

      pSysBuf->SettableData =
        SERIAL_DATABITS_5        |
        SERIAL_DATABITS_6        |
        SERIAL_DATABITS_7        |
        SERIAL_DATABITS_8;

      pSysBuf->SettableStopParity =
        SERIAL_STOPBITS_10       |
        SERIAL_STOPBITS_15       |
        SERIAL_STOPBITS_20       |
        SERIAL_PARITY_NONE       |
        SERIAL_PARITY_ODD        |
        SERIAL_PARITY_EVEN       |
        SERIAL_PARITY_MARK       |
        SERIAL_PARITY_SPACE;

      pSysBuf->CurrentTxQueue = 0;
      pSysBuf->CurrentRxQueue =
        pDevExt->pIoPortLocal->readBuf.pEnd - pDevExt->pIoPortLocal->readBuf.pBase;

      pIrp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
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
      C0C_BUFFER readBufNew;
      PC0C_BUFFER pReadBuf;

      if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_QUEUE_SIZE)) {
        status = STATUS_BUFFER_TOO_SMALL;
        break;
      }

      if (pSysBuf->InSize <=
        (ULONG)(pDevExt->pIoPortLocal->readBuf.pEnd - pDevExt->pIoPortLocal->readBuf.pBase)) {
        status = STATUS_SUCCESS;
        break;
      }

      try {
        readBufNew.pBase = ExAllocatePoolWithQuota(NonPagedPool, pSysBuf->InSize);
      } except (EXCEPTION_EXECUTE_HANDLER) {
        readBufNew.pBase = NULL;
        status = GetExceptionCode();
      }

      if (!readBufNew.pBase)
        break;

      readBufNew.pFree = readBufNew.pBusy = readBufNew.pBase;
      readBufNew.pEnd = readBufNew.pBase + pSysBuf->InSize;
      readBufNew.busy = 0;

      pReadBuf = &pDevExt->pIoPortLocal->readBuf;

      KeAcquireSpinLock(pDevExt->pIoLock, &oldIrql);

      if (pReadBuf->pBase) {
        while (pReadBuf->busy) {
          ULONG length;

          length = pReadBuf->pFree <= pReadBuf->pBusy ?
              pReadBuf->pEnd - pReadBuf->pBusy : pReadBuf->busy;

          RtlCopyMemory(readBufNew.pFree, pReadBuf->pBusy, length);

          pReadBuf->busy -= length;
          pReadBuf->pBusy += length;
          if (pReadBuf->pBusy == pReadBuf->pEnd)
            pReadBuf->pBusy = pReadBuf->pBase;

          readBufNew.busy += length;
          readBufNew.pFree += length;
        }

        ExFreePool(pReadBuf->pBase);
      }

      *pReadBuf = readBufNew;

      KeReleaseSpinLock(pDevExt->pIoLock, oldIrql);

      status = STATUS_SUCCESS;
      break;
    }
    default:
      status = STATUS_INVALID_PARAMETER;
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

  if (status != STATUS_SUCCESS)
    TraceCode(pDevExt, "IOCTL_", codeNameTableIoctl, code, &status);

  return status;
}
