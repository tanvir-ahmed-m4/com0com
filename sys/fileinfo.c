/*
 * $Id$
 *
 * Copyright (c) 2005-2006 Vyacheslav Frolov
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
 * Revision 1.2  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.1  2005/09/28 10:06:42  vfrolov
 * Implemented IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION
 *
 *
 */

#include "precomp.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 0xA

NTSTATUS FdoPortQueryInformation(PC0C_IO_PORT pIoPortLocal, IN PIRP pIrp)
{
  NTSTATUS status;

  if ((pIoPortLocal->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) && pIoPortLocal->errors) {
    pIrp->IoStatus.Information = 0;
    status = STATUS_CANCELLED;
  } else {
    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpStack->Parameters.QueryFile.FileInformationClass) {
    case FileStandardInformation:
      RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sizeof(FILE_STANDARD_INFORMATION));
      pIrp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
      status = STATUS_SUCCESS;
      break;
    case FilePositionInformation:
      RtlZeroMemory(pIrp->AssociatedIrp.SystemBuffer, sizeof(FILE_POSITION_INFORMATION));
      pIrp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
      status = STATUS_SUCCESS;
      break;
    default:
      pIrp->IoStatus.Information = 0;
      status = STATUS_INVALID_PARAMETER;
    }
  }

  TraceIrp("FdoPortQueryInformation", pIrp, &status, TRACE_FLAG_PARAMS);

  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS FdoPortSetInformation(PC0C_IO_PORT pIoPortLocal, IN PIRP pIrp)
{
  NTSTATUS status;

  if ((pIoPortLocal->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) && pIoPortLocal->errors) {
    status = STATUS_CANCELLED;
  } else {
    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpStack->Parameters.QueryFile.FileInformationClass) {
    case FileEndOfFileInformation:
    case FileAllocationInformation:
      status = STATUS_SUCCESS;
      break;
    default:
      status = STATUS_INVALID_PARAMETER;
    }
  }

  TraceIrp("FdoPortSetInformation", pIrp, &status, TRACE_FLAG_PARAMS);

  pIrp->IoStatus.Information = 0;
  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS c0cFileInformation(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;
  ULONG code = IoGetCurrentIrpStackLocation(pIrp)->MajorFunction;

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    if (code == IRP_MJ_QUERY_INFORMATION)
      status = FdoPortQueryInformation(((PC0C_FDOPORT_EXTENSION)pDevExt)->pIoPortLocal, pIrp);
    else
      status = FdoPortSetInformation(((PC0C_FDOPORT_EXTENSION)pDevExt)->pIoPortLocal, pIrp);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    TraceCode(pDevExt, "IRP_MJ_", codeNameTableIrpMj, code, &status);
  }

  return status;
}
