/*
 * $Id$
 *
 * Copyright (c) 2006-2008 Vyacheslav Frolov
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
 * Revision 1.5  2008/03/14 15:28:39  vfrolov
 * Implemented ability to get paired port settings with
 * extended IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.4  2007/06/01 16:22:40  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.3  2006/10/27 12:44:14  vfrolov
 * Fixed typecasting
 *
 * Revision 1.2  2006/10/17 06:54:37  vfrolov
 * Disabled SERIAL_PORT_WMI_HW_GUID for binary compatibility with
 * both W2K and WXP
 *
 * Revision 1.1  2006/08/23 13:09:55  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include <wmidata.h>
#include <wmilib.h>
#pragma warning(push, 3)
#include <wmistr.h>
#pragma warning(pop)
#include "strutils.h"
#include "commprop.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 0xB

GUID guidWmiPortName       = SERIAL_PORT_WMI_NAME_GUID;
GUID guidWmiPortComm       = SERIAL_PORT_WMI_COMM_GUID;
#ifdef ALLOW_WMI_HW_GUID
GUID guidWmiPortHW         = SERIAL_PORT_WMI_HW_GUID;
#endif /* ALLOW_WMI_HW_GUID */
GUID guidWmiPortPerf       = SERIAL_PORT_WMI_PERF_GUID;
GUID guidWmiPortProperties = SERIAL_PORT_WMI_PROPERTIES_GUID;

enum COC_GUID_INDEX {
  COC_WMI_PORT_NAME,
  COC_WMI_PORT_COMM,
#ifdef ALLOW_WMI_HW_GUID
  COC_WMI_PORT_HW,
#endif /* ALLOW_WMI_HW_GUID */
  COC_WMI_PORT_PERF,
  COC_WMI_PORT_PROPERTIES,

  COC_WMI_LIST_SIZE
};

WMIGUIDREGINFO guidWmiList[COC_WMI_LIST_SIZE] = {
  {&guidWmiPortName,       1, 0},
  {&guidWmiPortComm,       1, 0},
#ifdef ALLOW_WMI_HW_GUID
  {&guidWmiPortHW,         1, 0},
#endif /* ALLOW_WMI_HW_GUID */
  {&guidWmiPortPerf,       1, 0},
  {&guidWmiPortProperties, 1, 0},
};

NTSTATUS QueryWmiRegInfo(
    IN PDEVICE_OBJECT pDevObj,
    OUT PULONG pRegFlags,
    OUT PUNICODE_STRING pInstanceName,
    OUT PUNICODE_STRING *ppRegistryPath,
    OUT PUNICODE_STRING pMofResourceName,
    OUT PDEVICE_OBJECT *ppPhDevObj)
{
  PC0C_FDOPORT_EXTENSION pDevExt = (PC0C_FDOPORT_EXTENSION)pDevObj->DeviceExtension;

  UNREFERENCED_PARAMETER(pInstanceName);
  UNREFERENCED_PARAMETER(pMofResourceName);

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiRegInfo");

  *ppRegistryPath = &c0cGlobal.registryPath;

  *pRegFlags = WMIREG_FLAG_INSTANCE_PDO;
  *ppPhDevObj = pDevExt->pIoPortLocal->pPhDevObj;

  return STATUS_SUCCESS;
}

NTSTATUS QueryWmiDataBlock(
    IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp,
    IN ULONG guidIndex,
    IN ULONG instanceIndex,
    IN ULONG instanceCount,
    IN OUT PULONG pInstanceLengthArray,
    IN ULONG bufSize,
    OUT PUCHAR pBuf)
{
  NTSTATUS status;
  ULONG size;
  KIRQL oldIrql;
  PC0C_FDOPORT_EXTENSION pDevExt = (PC0C_FDOPORT_EXTENSION)pDevObj->DeviceExtension;
  PC0C_IO_PORT pIoPort = pDevExt->pIoPortLocal;

  UNREFERENCED_PARAMETER(instanceIndex);
  UNREFERENCED_PARAMETER(instanceCount);

  switch(guidIndex) {
  case COC_WMI_PORT_NAME:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock PORT_NAME");

    size = (ULONG)wcslen(pDevExt->portName) * sizeof(pDevExt->portName[0]);

    if (bufSize < (size + sizeof(USHORT))) {
      size += sizeof(USHORT);
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    *(USHORT *)pBuf = (USHORT)size;
    RtlCopyMemory(pBuf + sizeof(USHORT), pDevExt->portName, size);

    size += sizeof(USHORT);
    status = STATUS_SUCCESS;
    break;
  case COC_WMI_PORT_COMM:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock PORT_COMM");

    size = sizeof(SERIAL_WMI_COMM_DATA);

    if (bufSize < size) {
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

    ((PSERIAL_WMI_COMM_DATA)pBuf)->BaudRate = pIoPort->baudRate.BaudRate;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->BitsPerByte = pIoPort->lineControl.WordLength;

    ((PSERIAL_WMI_COMM_DATA)pBuf)->ParityCheckEnable = TRUE;

    switch (pIoPort->lineControl.Parity) {
    default:
    case NO_PARITY:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->Parity = SERIAL_WMI_PARITY_NONE;
      ((PSERIAL_WMI_COMM_DATA)pBuf)->ParityCheckEnable = FALSE;
      break;
    case ODD_PARITY:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->Parity = SERIAL_WMI_PARITY_ODD;
      break;
    case EVEN_PARITY:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->Parity = SERIAL_WMI_PARITY_EVEN;
      break;
    case MARK_PARITY:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->Parity = SERIAL_WMI_PARITY_MARK;
      break;
    case SPACE_PARITY:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->Parity = SERIAL_WMI_PARITY_SPACE;
      break;
    }

    switch (pIoPort->lineControl.StopBits) {
    default:
    case STOP_BIT_1:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->StopBits = SERIAL_WMI_STOP_1;
      break;
    case STOP_BITS_1_5:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->StopBits = SERIAL_WMI_STOP_1_5;
      break;
    case STOP_BITS_2:
      ((PSERIAL_WMI_COMM_DATA)pBuf)->StopBits = SERIAL_WMI_STOP_2;
      break;
    }

    ((PSERIAL_WMI_COMM_DATA)pBuf)->XoffCharacter = pIoPort->specialChars.XoffChar;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->XoffXmitThreshold = pIoPort->handFlow.XoffLimit;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->XonCharacter = pIoPort->specialChars.XonChar;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->XonXmitThreshold = pIoPort->handFlow.XonLimit;

    KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

    ((PSERIAL_WMI_COMM_DATA)pBuf)->MaximumBaudRate = 128L * 1024L;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->MaximumOutputBufferSize = (ULONG)-1L;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->MaximumInputBufferSize = (ULONG)-1L;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->Support16BitMode = FALSE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SupportDTRDSR = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SupportIntervalTimeouts = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SupportParityCheck = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SupportRTSCTS = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SupportXonXoff = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableBaudRate = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableDataBits = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableFlowControl = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableParity = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableParityCheck = TRUE;
    ((PSERIAL_WMI_COMM_DATA)pBuf)->SettableStopBits = TRUE;

    ((PSERIAL_WMI_COMM_DATA)pBuf)->IsBusy = (BOOLEAN)pDevExt->openCount;

    status = STATUS_SUCCESS;
    break;
#ifdef ALLOW_WMI_HW_GUID
  /*
   * W2K and WXP have different SERIAL_WMI_HW_DATA structures
   * so we don't allow SERIAL_PORT_WMI_HW_GUID by default.
   * Define ALLOW_WMI_HW_GUID if you need it.
   */
  case COC_WMI_PORT_HW:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock PORT_HW");

    size = sizeof(SERIAL_WMI_HW_DATA);

    if (bufSize < size) {
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    RtlZeroMemory((PSERIAL_WMI_HW_DATA)pBuf, size);

    status = STATUS_SUCCESS;
    break;
#endif /* ALLOW_WMI_HW_GUID */
  case COC_WMI_PORT_PERF:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock PORT_PERF");

    size = sizeof(SERIAL_WMI_PERF_DATA);

    if (bufSize < size) {
      status = STATUS_BUFFER_TOO_SMALL;
      break;
    }

    KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);

    ((PSERIAL_WMI_PERF_DATA)pBuf)->ReceivedCount = pIoPort->perfStats.ReceivedCount;
    ((PSERIAL_WMI_PERF_DATA)pBuf)->TransmittedCount = pIoPort->perfStats.TransmittedCount;
    ((PSERIAL_WMI_PERF_DATA)pBuf)->FrameErrorCount = pIoPort->perfStats.FrameErrorCount;
    ((PSERIAL_WMI_PERF_DATA)pBuf)->SerialOverrunErrorCount = pIoPort->perfStats.SerialOverrunErrorCount;
    ((PSERIAL_WMI_PERF_DATA)pBuf)->BufferOverrunErrorCount = pIoPort->perfStats.BufferOverrunErrorCount;
    ((PSERIAL_WMI_PERF_DATA)pBuf)->ParityErrorCount = pIoPort->perfStats.ParityErrorCount;

    KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

    status = STATUS_SUCCESS;
    break;
  case COC_WMI_PORT_PROPERTIES:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock PORT_PROPERTIES");

    status = GetCommProp(pDevExt, pBuf, bufSize, &size);
    break;
  default:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"QueryWmiDataBlock ???");
    size = 0;
    status = STATUS_WMI_GUID_NOT_FOUND;
  }

  if (status == STATUS_SUCCESS)
    *pInstanceLengthArray = size;

  status = WmiCompleteRequest(pDevObj, pIrp, status, size, IO_NO_INCREMENT);

  return status;
}

NTSTATUS SetWmiDataBlock(
    IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp,
    IN ULONG guidIndex,
    IN ULONG instanceIndex,
    IN ULONG bufSize,
    IN PUCHAR pBuf)
{
  NTSTATUS status;
  ULONG size = 0;

  UNREFERENCED_PARAMETER(guidIndex);
  UNREFERENCED_PARAMETER(instanceIndex);
  UNREFERENCED_PARAMETER(bufSize);
  UNREFERENCED_PARAMETER(pBuf);

  Trace0((PC0C_COMMON_EXTENSION)pDevObj->DeviceExtension, L"SetWmiDataBlock");

  status = STATUS_WMI_GUID_NOT_FOUND;

  status = WmiCompleteRequest(pDevObj, pIrp, status, size, IO_NO_INCREMENT);

  return status;
}

NTSTATUS SetWmiDataItem(
    IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp,
    IN ULONG guidIndex,
    IN ULONG instanceIndex,
    IN ULONG dataItemId,
    IN ULONG bufSize,
    IN PUCHAR pBuf)
{
  NTSTATUS status;
  ULONG size = 0;

  UNREFERENCED_PARAMETER(guidIndex);
  UNREFERENCED_PARAMETER(instanceIndex);
  UNREFERENCED_PARAMETER(dataItemId);
  UNREFERENCED_PARAMETER(bufSize);
  UNREFERENCED_PARAMETER(pBuf);

  Trace0((PC0C_COMMON_EXTENSION)pDevObj->DeviceExtension, L"SetWmiDataItem");

  status = STATUS_WMI_GUID_NOT_FOUND;

  status = WmiCompleteRequest(pDevObj, pIrp, status, size, IO_NO_INCREMENT);

  return status;
}

WMILIB_CONTEXT wmiLibContext = {
  COC_WMI_LIST_SIZE,
  guidWmiList,

  QueryWmiRegInfo,
  QueryWmiDataBlock,
  SetWmiDataBlock,
  SetWmiDataItem,
  NULL /*ExecuteWmiMethod*/,
  NULL /*WmiFunctionControl*/
};

NTSTATUS FdoPortWmi(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp)
{
  NTSTATUS status;
  SYSCTL_IRP_DISPOSITION disposition;

  status = WmiSystemControl(&wmiLibContext, pDevExt->pDevObj, pIrp, &disposition);

  switch(disposition) {
  case IrpProcessed:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"IrpProcessed");
    break;
  case IrpNotCompleted:
    TraceCode((PC0C_COMMON_EXTENSION)pDevExt, "IrpNotCompleted ", NULL,
        (ULONG)(ULONG_PTR)IoGetCurrentIrpStackLocation(pIrp)->Parameters.WMI.DataPath, &status);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    break;
  case IrpForward:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"IrpForward");
    goto forward;
  case IrpNotWmi:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"IrpNotWmi");
    goto forward;
  default:
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Irp???");
    goto forward;
  forward:
    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pDevExt->pLowDevObj, pIrp);
  }

  return status;
}

NTSTATUS c0cSystemControlDispatch(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

#if DBG
  ULONG code = IoGetCurrentIrpStackLocation(pIrp)->MinorFunction;
#endif /* DBG */

  HALT_UNLESS2(IoGetCurrentIrpStackLocation(pIrp)->MajorFunction == IRP_MJ_SYSTEM_CONTROL,
      IoGetCurrentIrpStackLocation(pIrp)->MajorFunction,
      IoGetCurrentIrpStackLocation(pIrp)->MinorFunction);

  TraceIrp("c0cSystemControlDispatch", pIrp, NULL, TRACE_FLAG_PARAMS);

  status = STATUS_NO_SUCH_DEVICE;

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortWmi((PC0C_FDOPORT_EXTENSION)pDevExt, pIrp);
    break;
  case C0C_DOTYPE_FB:
    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(((PC0C_COMMON_FDO_EXTENSION)pDevExt)->pLowDevObj, pIrp);
    break;
  case C0C_DOTYPE_PP:
    status = STATUS_NOT_SUPPORTED;
  default:
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

#if DBG
  if (status != STATUS_SUCCESS)
    TraceCode(pDevExt, "WMI_", codeNameTableWmi, code, &status);
#endif /* DBG */

  return status;
}
