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
 * Revision 1.7  2008/12/02 16:21:11  vfrolov
 * Separated tracing and debuging
 *
 * Revision 1.6  2006/06/23 11:44:52  vfrolov
 * Mass replacement pDevExt by pIoPort
 *
 * Revision 1.5  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.4  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.3  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.2  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"

NTSTATUS StartIrpRead(
    IN PC0C_IO_PORT pIoPort,
    IN PLIST_ENTRY pQueueToComplete)
{
  return ReadWrite(
      pIoPort, TRUE,
      pIoPort->pIoPortRemote, FALSE,
      pQueueToComplete);
}

NTSTATUS FdoPortRead(IN PC0C_IO_PORT pIoPort, IN PIRP pIrp)
{
  NTSTATUS status;

  pIrp->IoStatus.Information = 0;

  if ((pIoPort->handFlow.ControlHandShake & SERIAL_ERROR_ABORT) && pIoPort->errors) {
    status = STATUS_CANCELLED;
  } else {
    if (IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length)
      status = FdoPortStartIrp(pIoPort, pIrp, C0C_QUEUE_READ, StartIrpRead);
    else
      status = STATUS_SUCCESS;
  }

  if (status != STATUS_PENDING) {
    TraceIrp("FdoPortRead", pIrp, &status, TRACE_FLAG_RESULTS);
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}

NTSTATUS c0cRead(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;

#if ENABLE_TRACING
  ULONG code = IoGetCurrentIrpStackLocation(pIrp)->MajorFunction;
#endif /* ENABLE_TRACING */

  TraceIrp("c0cRead", pIrp, NULL, TRACE_FLAG_PARAMS);

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FP:
    status = FdoPortRead(((PC0C_FDOPORT_EXTENSION)pDevExt)->pIoPortLocal, pIrp);
    break;
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

#if ENABLE_TRACING
  if (!NT_SUCCESS(status))
    TraceCode(pDevExt, "IRP_MJ_", codeNameTableIrpMj, code, &status);
#endif /* ENABLE_TRACING */

  return status;
}
