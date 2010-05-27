/*
 * $Id$
 *
 * Copyright (c) 2004-2010 Vyacheslav Frolov
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
 * Revision 1.10  2010/05/27 11:16:46  vfrolov
 * Added ability to put the port to the Ports class
 *
 * Revision 1.9  2007/11/23 08:58:48  vfrolov
 * Added UniqueID capability
 *
 * Revision 1.8  2007/06/04 15:24:33  vfrolov
 * Fixed open reject just after close in exclusiveMode
 *
 * Revision 1.7  2007/06/01 16:22:40  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.6  2007/01/11 14:50:29  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
 * Revision 1.5  2006/06/23 07:37:24  vfrolov
 * Disabled usage pDevExt after deleting device
 * Added check of openCount to IRP_MN_QUERY_REMOVE_DEVICE
 *
 * Revision 1.4  2005/07/14 13:51:07  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.3  2005/06/28 12:25:34  vfrolov
 * Implemented IRP_MN_QUERY_CAPABILITIES and IRP_MN_QUERY_BUS_INFORMATION for PdoPortPnp()
 *
 * Revision 1.2  2005/05/17 15:06:18  vfrolov
 * Fixed type cast
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include <initguid.h>
#include "strutils.h"
#include "showport.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 5

/*
 * {E74D3627-7582-48a6-8B0B-ED60CE908A51}
 */
DEFINE_GUID(GUID_C0C_BUS_TYPE,
    0xe74d3627, 0x7582, 0x48a6, 0x8b, 0xb, 0xed, 0x60, 0xce, 0x90, 0x8a, 0x51);

NTSTATUS FdoBusPnp(
    IN PC0C_FDOBUS_EXTENSION pDevExt,
    IN PIRP                  pIrp)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
  ULONG minorFunction = pIrpStack->MinorFunction;
  PDEVICE_OBJECT pLowDevObj = pDevExt->pLowDevObj; // IRP_MN_REMOVE_DEVICE deletes *pDevExt!

  status = STATUS_SUCCESS;

  switch (minorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    if (pIrpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {
      ULONG countPdos, countRelations;
      PDEVICE_RELATIONS pRelationsPrev, pRelations;
      int i;

      countPdos = 0;
      for (i = 0 ; i < 2 ; i++) {
        if (pDevExt->childs[i].pDevExt)
          countPdos++;
      }

      if (!countPdos)
        break;

      pRelationsPrev = (PDEVICE_RELATIONS)pIrp->IoStatus.Information;
      countRelations = pRelationsPrev ? pRelationsPrev->Count : 0;

      pRelations = (PDEVICE_RELATIONS)C0C_ALLOCATE_POOL(PagedPool,
        sizeof(DEVICE_RELATIONS) + ((countPdos + countRelations - 1) * sizeof (PDEVICE_OBJECT)));

      if (!pRelations) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        break;
      }

      if (countRelations)
        RtlCopyMemory(pRelations->Objects, pRelationsPrev->Objects,
                                      countRelations * sizeof (PDEVICE_OBJECT));

      for (i = 0 ; i < 2 ; i++) {
        PC0C_PDOPORT_EXTENSION  pPhDevExt;

        pPhDevExt = pDevExt->childs[i].pDevExt;

        if (pPhDevExt) {
          if (!pDevExt->childs[i].ioPort.pDevExt) {
            UNICODE_STRING portRegistryPath;
            UNICODE_STRING portName;

            RtlInitUnicodeString(&portRegistryPath, NULL);
            StrAppendPortParametersRegistryPath(&status, &portRegistryPath, pPhDevExt->portName);

            RtlInitUnicodeString(&portName, NULL);
            StrAppendParameterPortName(&status, &portName, portRegistryPath.Buffer);

            if (NT_SUCCESS(status) && portName.Length &&
                _wcsicmp(C0C_PORT_NAME_COMCLASS, portName.Buffer) == 0)
            {
              pDevExt->childs[i].ioPort.isComClass = TRUE;
              Trace0((PC0C_COMMON_EXTENSION)pPhDevExt, L"Port class set to COM");
            } else {
              pDevExt->childs[i].ioPort.isComClass = FALSE;
              Trace0((PC0C_COMMON_EXTENSION)pPhDevExt, L"Port class set to CNC");
            }

            status = STATUS_SUCCESS;
          }

          pRelations->Objects[countRelations++] = pPhDevExt->pDevObj;
          ObReferenceObject(pPhDevExt->pDevObj);
        }
      }

      pRelations->Count = countRelations;

      if (pRelationsPrev)
        C0C_FREE_POOL(pRelationsPrev);

      pIrp->IoStatus.Information = (ULONG_PTR)pRelations;
      pIrp->IoStatus.Status = STATUS_SUCCESS;
    }
    break;
  case IRP_MN_REMOVE_DEVICE:
    RemoveFdoBus(pDevExt);
    pDevExt = NULL;
    break;
  }

  if (status == STATUS_SUCCESS) {
    TraceIrp("FdoBusPnp", pIrp, NULL, TRACE_FLAG_RESULTS);

    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pLowDevObj, pIrp);

    TraceCode((PC0C_COMMON_EXTENSION)pDevExt, "PNP ", codeNameTablePnp, minorFunction, &status);
  } else {
    TraceIrp("PNP", pIrp, &status, TRACE_FLAG_RESULTS);

    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}

NTSTATUS PdoPortQueryId(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp,
    IN PIO_STACK_LOCATION     pIrpStack)
{
  NTSTATUS status;
  PWCHAR pIDs = NULL;

  switch (pIrpStack->Parameters.QueryId.IdType) {
  case BusQueryDeviceID:
    status = DupStrW(&pIDs, C0C_PORT_DEVICE_ID, FALSE);
    break;
  case BusQueryHardwareIDs:
    status = DupStrW(
        &pIDs,
        (pDevExt->pIoPortLocal && pDevExt->pIoPortLocal->isComClass) ?
            C0C_PORT_HARDWARE_IDS_COMCLASS :
            C0C_PORT_HARDWARE_IDS_CNCCLASS,
        TRUE);
    break;
  case BusQueryCompatibleIDs:
    status = DupStrW(&pIDs, L"\0", TRUE);
    break;
  case BusQueryInstanceID:
    status = DupStrW(&pIDs, pDevExt->portName, FALSE);
    break;
  default:
    status = pIrp->IoStatus.Status;
  }

  pIrp->IoStatus.Information = (ULONG_PTR)pIDs;

  return status;
}

NTSTATUS PdoPortQueryCaps(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp,
    IN PIO_STACK_LOCATION     pIrpStack)
{
  PDEVICE_CAPABILITIES pCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

  UNREFERENCED_PARAMETER(pDevExt);
  UNREFERENCED_PARAMETER(pIrp);

  if (pCaps->Version != 1 || pCaps->Size < sizeof(DEVICE_CAPABILITIES))
    return STATUS_UNSUCCESSFUL;

  pCaps->UniqueID = TRUE;

  return STATUS_SUCCESS;
}

NTSTATUS PdoPortQueryDevText(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp,
    IN PIO_STACK_LOCATION     pIrpStack)
{
  NTSTATUS status;

  status = STATUS_SUCCESS;

  switch (pIrpStack->Parameters.QueryDeviceText.DeviceTextType) {
  case DeviceTextDescription:
    if (!pIrp->IoStatus.Information) {
      UNICODE_STRING portText;

      RtlInitUnicodeString(&portText, NULL);
      StrAppendStr0(&status, &portText, L"Port ");
      StrAppendStr0(&status, &portText, pDevExt->portName);

      if (NT_SUCCESS(status))
        pIrp->IoStatus.Information = (ULONG_PTR)portText.Buffer;
    }
    break;
  case DeviceTextLocationInformation:
    if (!pIrp->IoStatus.Information) {
      UNICODE_STRING portText;

      RtlInitUnicodeString(&portText, NULL);
      StrAppendStr0(&status, &portText, pDevExt->portName);

      if (NT_SUCCESS(status))
        pIrp->IoStatus.Information = (ULONG_PTR)portText.Buffer;
    }
    break;
  default:
    status = pIrp->IoStatus.Status;
  }

  return status;
}

NTSTATUS PdoPortBusInfo(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp)
{
  PPNP_BUS_INFORMATION pBusInfo;

  pBusInfo = (PPNP_BUS_INFORMATION)C0C_ALLOCATE_POOL(PagedPool, sizeof(PNP_BUS_INFORMATION));

  if (!pBusInfo)
    return STATUS_INSUFFICIENT_RESOURCES;

  pBusInfo->BusTypeGuid = GUID_C0C_BUS_TYPE;
  pBusInfo->LegacyBusType = PNPBus;
  pBusInfo->BusNumber = pDevExt->pBusExt->portNum;

  pIrp->IoStatus.Information = (ULONG_PTR)pBusInfo;

  return STATUS_SUCCESS;
}

NTSTATUS PdoPortPnp(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  switch (pIrpStack->MinorFunction) {
  case IRP_MN_QUERY_ID:
    status = PdoPortQueryId(pDevExt, pIrp, pIrpStack);
    break;
  case IRP_MN_QUERY_CAPABILITIES:
    status = PdoPortQueryCaps(pDevExt, pIrp, pIrpStack);
    break;
  case IRP_MN_QUERY_DEVICE_TEXT:
    status = PdoPortQueryDevText(pDevExt, pIrp, pIrpStack);
    break;
  case IRP_MN_QUERY_BUS_INFORMATION:
    status = PdoPortBusInfo(pDevExt, pIrp);
    break;
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    switch (pIrpStack->Parameters.QueryDeviceRelations.Type) {
    case RemovalRelations:
      status = STATUS_SUCCESS;
      break;
    case BusRelations:
    case EjectionRelations:
    case PowerRelations:
    case TargetDeviceRelation:
    default:
      status = pIrp->IoStatus.Status;
    }
    break;
  case IRP_MN_REMOVE_DEVICE:
  case IRP_MN_START_DEVICE:
  case IRP_MN_STOP_DEVICE:
  case IRP_MN_QUERY_STOP_DEVICE:
  case IRP_MN_CANCEL_STOP_DEVICE:
  case IRP_MN_QUERY_REMOVE_DEVICE:
  case IRP_MN_CANCEL_REMOVE_DEVICE:
  case IRP_MN_SURPRISE_REMOVAL:
  case IRP_MN_EJECT:
    status = STATUS_SUCCESS;
    break;
  default:
    status = pIrp->IoStatus.Status;
  }

  TraceIrp("PNP", pIrp, &status, TRACE_FLAG_RESULTS);

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS FdoPortPnp(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp)
{
  NTSTATUS status;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
  ULONG minorFunction = pIrpStack->MinorFunction;
  PDEVICE_OBJECT pLowDevObj = pDevExt->pLowDevObj; // IRP_MN_REMOVE_DEVICE deletes *pDevExt!

  status = STATUS_SUCCESS;

  switch (minorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS: {
    LIST_ENTRY queueToComplete;
    KIRQL oldIrql;
    PC0C_IO_PORT pIoPort = pDevExt->pIoPortLocal;

    if ((pIoPort->exclusiveMode && pIoPort->isOpen) ||
        (pIoPort->plugInMode && !pIoPort->pIoPortRemote->isOpen))
    {
      HidePort(pDevExt);
    } else {
      ShowPort(pDevExt);
    }

    /* complete pending CLOSE IRPs */

    InitializeListHead(&queueToComplete);

    KeAcquireSpinLock(pIoPort->pIoLock, &oldIrql);
    FdoPortIo(C0C_IO_TYPE_CLOSE_COMPLETE,
              NULL,
              pIoPort,
              &pIoPort->irpQueues[C0C_QUEUE_CLOSE],
              &queueToComplete);
    KeReleaseSpinLock(pIoPort->pIoLock, oldIrql);

    FdoPortCompleteQueue(&queueToComplete);
    break;
  }
  case IRP_MN_QUERY_REMOVE_DEVICE:
    if (pDevExt->openCount)
      status = STATUS_DEVICE_BUSY;
    break;
  case IRP_MN_REMOVE_DEVICE:
    RemoveFdoPort(pDevExt);
    pDevExt = NULL;
    break;
  }

  if (status == STATUS_SUCCESS) {
    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pLowDevObj, pIrp);

    TraceCode((PC0C_COMMON_EXTENSION)pDevExt, "PNP ", codeNameTablePnp, minorFunction, &status);
  } else {
    TraceIrp("PNP", pIrp, &status, TRACE_FLAG_RESULTS);

    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}

NTSTATUS c0cPnpDispatch(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

  HALT_UNLESS2(IoGetCurrentIrpStackLocation(pIrp)->MajorFunction == IRP_MJ_PNP,
      IoGetCurrentIrpStackLocation(pIrp)->MajorFunction,
      IoGetCurrentIrpStackLocation(pIrp)->MinorFunction);

  TraceIrp("PNP", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FB:
    status = FdoBusPnp((PC0C_FDOBUS_EXTENSION)pDevExt, pIrp);
    break;
  case C0C_DOTYPE_PP:
    status = PdoPortPnp((PC0C_PDOPORT_EXTENSION)pDevExt, pIrp);
    break;
  case C0C_DOTYPE_FP:
    status = FdoPortPnp((PC0C_FDOPORT_EXTENSION)pDevExt, pIrp);
    break;
  default:
    status = STATUS_NO_SUCH_DEVICE;
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}
