/*
 * $Id$
 *
 * Copyright (c) 2004-2011 Vyacheslav Frolov
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
 * Revision 1.9  2011/12/15 06:17:12  vfrolov
 * Removed usage undocumented PDRIVER_OBJECT->Type
 *
 * Revision 1.8  2010/08/09 06:02:40  vfrolov
 * Eliminated accessing undocumented structure members
 *
 * Revision 1.7  2010/08/04 10:38:55  vfrolov
 * Minimized PREfast noise
 *
 * Revision 1.6  2006/08/23 13:13:53  vfrolov
 * Moved c0cSystemControlDispatch() to wmi.c
 *
 * Revision 1.5  2005/09/28 10:06:42  vfrolov
 * Implemented IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION
 *
 * Revision 1.4  2005/09/13 14:56:16  vfrolov
 * Implemented IRP_MJ_FLUSH_BUFFERS
 *
 * Revision 1.3  2005/07/13 16:12:36  vfrolov
 * Added c0cGlobal struct for global driver's data
 *
 * Revision 1.2  2005/05/12 07:41:27  vfrolov
 * Added ability to change the port names
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "strutils.h"

C0C_GLOBAL c0cGlobal;

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDrvObj, IN PUNICODE_STRING pRegistryPath)
{
  NTSTATUS status;

  KeInitializeSpinLock(&c0cGlobal.listFdoBusLock);
  InitializeListHead(&c0cGlobal.listFdoBus);

  c0cGlobal.pDrvObj = pDrvObj;

  status = STATUS_SUCCESS;

  RtlInitUnicodeString(&c0cGlobal.registryPath, NULL);
  StrAppendStr(&status, &c0cGlobal.registryPath, pRegistryPath->Buffer, pRegistryPath->Length);

  if (!NT_SUCCESS(status)) {
    SysLogDrv(pDrvObj, status, L"DriverEntry FAIL");
    return status;
  }

  TraceOpen(pDrvObj, pRegistryPath);

  pDrvObj->DriverUnload                                  = c0cUnload;
  pDrvObj->DriverExtension->AddDevice                    = c0cAddDevice;

  pDrvObj->MajorFunction[IRP_MJ_CREATE]                  = c0cOpen;
  pDrvObj->MajorFunction[IRP_MJ_CLOSE]                   = c0cClose;
  pDrvObj->MajorFunction[IRP_MJ_CLEANUP]                 = c0cCleanup;
  pDrvObj->MajorFunction[IRP_MJ_FLUSH_BUFFERS]           = c0cWrite;
  pDrvObj->MajorFunction[IRP_MJ_WRITE]                   = c0cWrite;
  pDrvObj->MajorFunction[IRP_MJ_READ]                    = c0cRead;
  pDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = c0cIoControl;
  pDrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = c0cInternalIoControl;
  pDrvObj->MajorFunction[IRP_MJ_QUERY_INFORMATION]       = c0cFileInformation;
  pDrvObj->MajorFunction[IRP_MJ_SET_INFORMATION]         = c0cFileInformation;
  pDrvObj->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = c0cSystemControlDispatch;
  pDrvObj->MajorFunction[IRP_MJ_PNP]                     = c0cPnpDispatch;
  pDrvObj->MajorFunction[IRP_MJ_POWER]                   = c0cPowerDispatch;

  return STATUS_SUCCESS;
}

VOID c0cUnload(IN PDRIVER_OBJECT pDrvObj)
{
  UNREFERENCED_PARAMETER(pDrvObj);

  StrFree(&c0cGlobal.registryPath);

  TraceClose();
}

NTSTATUS c0cInternalIoControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;

  UNREFERENCED_PARAMETER(pDevObj);

  status = STATUS_INVALID_DEVICE_REQUEST;

  TraceIrp("c0cInternalIoControl", pIrp, &status, TRACE_FLAG_PARAMS);

  pIrp->IoStatus.Information = 0;
  pIrp->IoStatus.Status = status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}
