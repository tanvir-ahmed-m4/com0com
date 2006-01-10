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
 * Revision 1.1  2006/01/10 10:12:05  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "handflow.h"
#include "bufutils.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 9

NTSTATUS SetHandFlow(
    PC0C_FDOPORT_EXTENSION pDevExt,
    PSERIAL_HANDFLOW pHandFlow,
    PLIST_ENTRY pQueueToComplete)
{
  ULONG bits;
  PC0C_BUFFER pReadBuf;
  PSERIAL_HANDFLOW pNewHandFlow;

  pReadBuf = &pDevExt->pIoPortLocal->readBuf;

  if (pHandFlow) {
    if ((pDevExt->pIoPortLocal->escapeChar && (pHandFlow->FlowReplace & SERIAL_ERROR_CHAR)) ||
        ((SIZE_T)pHandFlow->XonLimit > C0C_BUFFER_SIZE(pReadBuf)) ||
        ((SIZE_T)pHandFlow->XoffLimit > C0C_BUFFER_SIZE(pReadBuf)))
    {
      return STATUS_INVALID_PARAMETER;
    }

    pNewHandFlow = pHandFlow;
  } else {
    pNewHandFlow = &pDevExt->handFlow;
  }

  if ((pNewHandFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) == 0)
    SetXonXoffHolding(pDevExt->pIoPortLocal, C0C_XCHAR_ON);

  bits = 0;

  if (!pHandFlow ||
      (pDevExt->handFlow.FlowReplace & SERIAL_RTS_MASK) !=
          (pHandFlow->FlowReplace & SERIAL_RTS_MASK))
  {
    if ((pNewHandFlow->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_CONTROL)
      bits |= C0C_MSB_CTS;
  }

  if (!pHandFlow ||
      (pDevExt->handFlow.ControlHandShake & SERIAL_DTR_MASK) !=
          (pHandFlow->ControlHandShake & SERIAL_DTR_MASK))
  {
    if ((pNewHandFlow->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_CONTROL)
      bits |= C0C_MSB_DSR;
  }

  if (pHandFlow)
    pDevExt->handFlow = *pHandFlow;

  if (bits)
    SetModemStatus(pDevExt->pIoPortRemote, bits, bits, pQueueToComplete);

  SetLimit(pDevExt->pIoPortRemote->pDevExt);
  SetLimit(pDevExt);
  UpdateHandFlow(pDevExt, TRUE, pQueueToComplete);

  if (pDevExt->pIoPortRemote->tryWrite) {
    ReadWrite(
        pDevExt->pIoPortLocal, FALSE,
        pDevExt->pIoPortRemote, FALSE,
        pQueueToComplete);
  }

  if (pDevExt->pIoPortLocal->tryWrite) {
    ReadWrite(
        pDevExt->pIoPortRemote, FALSE,
        pDevExt->pIoPortLocal, FALSE,
        pQueueToComplete);
  }

  return STATUS_SUCCESS;
}

VOID UpdateHandFlow(
    PC0C_FDOPORT_EXTENSION pDevExt,
    BOOLEAN freed,
    PLIST_ENTRY pQueueToComplete)
{
  ULONG bits, mask;
  PC0C_BUFFER pReadBuf;
  PSERIAL_HANDFLOW pHandFlowLocal, pHandFlowRemote;
  PC0C_IO_PORT pIoPortLocal;

  pIoPortLocal = pDevExt->pIoPortLocal;
  pHandFlowLocal = &pDevExt->handFlow;
  pHandFlowRemote = &pDevExt->pIoPortRemote->pDevExt->handFlow;
  pReadBuf = &pIoPortLocal->readBuf;

  bits = mask = 0;

  if (pIoPortLocal->flipXoffLimit) {
    if (!freed && C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pHandFlowLocal->XoffLimit)) {
      pIoPortLocal->flipXoffLimit = FALSE;

      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE)
        mask |= C0C_MSB_CTS;

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE)
        mask |= C0C_MSB_DSR;

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPortLocal->sendXonXoff = C0C_XCHAR_OFF;
        pIoPortLocal->tryWrite = TRUE;
      }
    }
  } else {
    if (freed && C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pHandFlowLocal->XonLimit) {
      pIoPortLocal->flipXoffLimit = TRUE;

      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {
        bits |= C0C_MSB_CTS;
        mask |= C0C_MSB_CTS;
      }

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
        bits |= C0C_MSB_DSR;
        mask |= C0C_MSB_DSR;
      }

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPortLocal->sendXonXoff = C0C_XCHAR_ON;
        pIoPortLocal->tryWrite = TRUE;
      }
    }
  }

  if (mask)
    SetModemStatus(pDevExt->pIoPortRemote, bits, mask, pQueueToComplete);
}

VOID SetLimit(PC0C_FDOPORT_EXTENSION pDevExt)
{
  PC0C_BUFFER pReadBuf;
  SIZE_T limit;
  PSERIAL_HANDFLOW pHandFlowLocal, pHandFlowRemote;

  pHandFlowLocal = &pDevExt->handFlow;
  pHandFlowRemote = &pDevExt->pIoPortRemote->pDevExt->handFlow;
  pReadBuf = &pDevExt->pIoPortLocal->readBuf;

  if ((((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) &&
                             (pHandFlowRemote->ControlHandShake & SERIAL_CTS_HANDSHAKE)) ||
      (((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) &&
                             (pHandFlowRemote->ControlHandShake & SERIAL_DSR_HANDSHAKE)))
  {
    limit = C0C_BUFFER_SIZE(pReadBuf) - pHandFlowLocal->XoffLimit + 1;
  } else {
    limit = C0C_BUFFER_SIZE(pReadBuf);
  }
  SetBufferLimit(pReadBuf, limit);
}

VOID SetModemStatusHolding(PC0C_IO_PORT pIoPort)
{
  ULONG writeHolding;
  PSERIAL_HANDFLOW pHandFlow;

  pHandFlow = &pIoPort->pDevExt->handFlow;
  writeHolding = pIoPort->writeHolding;

  if ((pHandFlow->ControlHandShake & SERIAL_CTS_HANDSHAKE) && !(pIoPort->modemStatus & C0C_MSB_CTS))
    writeHolding |= SERIAL_TX_WAITING_FOR_CTS;
  else
    writeHolding &= ~SERIAL_TX_WAITING_FOR_CTS;

  if ((pHandFlow->ControlHandShake & SERIAL_DSR_HANDSHAKE) && !(pIoPort->modemStatus & C0C_MSB_DSR))
    writeHolding |= SERIAL_TX_WAITING_FOR_DSR;
  else
    writeHolding &= ~SERIAL_TX_WAITING_FOR_DSR;

  if ((pHandFlow->ControlHandShake & SERIAL_DCD_HANDSHAKE) && !(pIoPort->modemStatus & C0C_MSB_RLSD))
    writeHolding |= SERIAL_TX_WAITING_FOR_DCD;
  else
    writeHolding &= ~SERIAL_TX_WAITING_FOR_DCD;

  if (!(writeHolding & ~SERIAL_TX_WAITING_FOR_XON) &&
      (pIoPort->writeHolding & ~SERIAL_TX_WAITING_FOR_XON))
  {
    if (pIoPort->sendXonXoff || (!writeHolding && pIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent))
      pIoPort->tryWrite = TRUE;
  }

  pIoPort->writeHolding = writeHolding;
}

VOID SetXonXoffHolding(PC0C_IO_PORT pIoPort, short xonXoff)
{
  switch (xonXoff) {
  case C0C_XCHAR_ON:
    if (pIoPort->writeHolding & SERIAL_TX_WAITING_FOR_XON) {
      pIoPort->writeHolding &= ~SERIAL_TX_WAITING_FOR_XON;

      if (!pIoPort->writeHolding && pIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent)
        pIoPort->tryWrite = TRUE;
    }
    break;
  case C0C_XCHAR_OFF:
    pIoPort->writeHolding |= SERIAL_TX_WAITING_FOR_XON;
    break;
  }
}
