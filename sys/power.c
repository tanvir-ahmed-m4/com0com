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

NTSTATUS PdoPortPower(
    IN PC0C_PDOPORT_EXTENSION pDevExt,
    IN PIRP                   pIrp,
    IN PIO_STACK_LOCATION     pIrpStack)
{
  NTSTATUS status;
  POWER_STATE_TYPE    powerType = pIrpStack->Parameters.Power.Type;
  POWER_STATE         powerState = pIrpStack->Parameters.Power.State;

  switch (pIrpStack->MinorFunction) {
  case IRP_MN_SET_POWER:
    switch (powerType) {
    case DevicePowerState:
      PoSetPowerState(pDevExt->pDevObj, powerType, powerState);
      status = STATUS_SUCCESS;
      break;
    case SystemPowerState:
      status = STATUS_SUCCESS;
      break;
    default:
      status = STATUS_NOT_SUPPORTED;
      break;
    }
    break;
  case IRP_MN_QUERY_POWER:
    status = STATUS_SUCCESS;
    break;
  case IRP_MN_WAIT_WAKE:
  case IRP_MN_POWER_SEQUENCE:
  default:
    status = STATUS_NOT_SUPPORTED;
    break;
  }

  if (status != STATUS_NOT_SUPPORTED)
    pIrp->IoStatus.Status = status;

  PoStartNextPowerIrp(pIrp);
  status = pIrp->IoStatus.Status;
  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return status;
}

NTSTATUS c0cPowerDispatch(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
  NTSTATUS status;
  PC0C_COMMON_EXTENSION pDevExt = pDevObj->DeviceExtension;
  PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

  TraceIrp("c0cPowerDispatch", pIrp, NULL, TRACE_FLAG_PARAMS);

  status = STATUS_NO_SUCH_DEVICE;

  switch (pDevExt->doType) {
  case C0C_DOTYPE_FB:
  case C0C_DOTYPE_FP:
    PoStartNextPowerIrp(pIrp);
    IoSkipCurrentIrpStackLocation(pIrp);
    status = PoCallDriver(((PC0C_COMMON_FDO_EXTENSION)pDevExt)->pLowDevObj, pIrp);
    break;
  case C0C_DOTYPE_PP:
    status = PdoPortPower((PC0C_PDOPORT_EXTENSION)pDevExt, pIrp, pIrpStack);
    break;
  default:
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
  }

  return status;
}
