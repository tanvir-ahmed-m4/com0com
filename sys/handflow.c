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
 * Revision 1.6  2006/05/19 12:16:19  vfrolov
 * Implemented SERIAL_XOFF_CONTINUE
 *
 * Revision 1.5  2006/05/17 15:31:14  vfrolov
 * Implemented SERIAL_TRANSMIT_TOGGLE
 *
 * Revision 1.4  2006/04/05 07:22:15  vfrolov
 * Replaced flipXoffLimit flag by writeHoldingRemote to correct handFlow changing
 *
 * Revision 1.3  2006/03/15 13:49:15  vfrolov
 * Fixed [1446861] Problems with setting DCB.fOutxCtsFlow and DCB.fRtsControl
 * Thanks to Brad (bdwade100 at users.sourceforge.net)
 *
 * Revision 1.2  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
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
  PC0C_IO_PORT pIoPortLocal;
  ULONG bits, mask;
  PC0C_BUFFER pReadBuf;
  PSERIAL_HANDFLOW pNewHandFlow;
  BOOLEAN setModemStatusHolding;

  pIoPortLocal = pDevExt->pIoPortLocal;
  pReadBuf = &pIoPortLocal->readBuf;

  if (pHandFlow) {
    if ((pIoPortLocal->escapeChar && (pHandFlow->FlowReplace & SERIAL_ERROR_CHAR)) ||
        ((SIZE_T)pHandFlow->XonLimit > C0C_BUFFER_SIZE(pReadBuf)) ||
        ((SIZE_T)pHandFlow->XoffLimit > C0C_BUFFER_SIZE(pReadBuf)))
    {
      return STATUS_INVALID_PARAMETER;
    }

    pNewHandFlow = pHandFlow;
  } else {
    pNewHandFlow = &pDevExt->handFlow;
  }

  // Set local side
  if (pHandFlow &&
      ((pDevExt->handFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) != 0) &&
      ((pHandFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) == 0))
  {
    SetXonXoffHolding(pIoPortLocal, C0C_XCHAR_ON);
  }

  if (!pHandFlow ||
      (pDevExt->handFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) !=
          (pHandFlow->ControlHandShake & SERIAL_OUT_HANDSHAKEMASK))
  {
    setModemStatusHolding = TRUE;
  } else {
    setModemStatusHolding = FALSE;
  }

  // Set remote side
  bits = mask = 0;

  if (!pHandFlow ||
      (pDevExt->handFlow.FlowReplace & SERIAL_RTS_MASK) !=
          (pHandFlow->FlowReplace & SERIAL_RTS_MASK))
  {
    switch (pNewHandFlow->FlowReplace & SERIAL_RTS_MASK) {
    case 0:
      pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
      mask |= C0C_MSB_CTS; // Turn off CTS on remote side if RTS is disabled
      break;
    case SERIAL_RTS_CONTROL:
      pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
      bits |= C0C_MSB_CTS;
      mask |= C0C_MSB_CTS;
      break;
    case SERIAL_RTS_HANDSHAKE:
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_CTS;
        mask |= C0C_MSB_CTS;
      }
      else
      if (pIoPortLocal->writeHoldingRemote & SERIAL_TX_WAITING_FOR_CTS) {
        if (C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pNewHandFlow->XonLimit) {
          pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
          bits |= C0C_MSB_CTS;
          mask |= C0C_MSB_CTS;
        }
      }
      else {
        bits |= C0C_MSB_CTS;
        mask |= C0C_MSB_CTS;
      }
    }
  }

  if (!pHandFlow ||
      (pDevExt->handFlow.ControlHandShake & SERIAL_DTR_MASK) !=
          (pHandFlow->ControlHandShake & SERIAL_DTR_MASK))
  {
    switch (pNewHandFlow->ControlHandShake & SERIAL_DTR_MASK) {
    case 0:
      pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
      mask |= C0C_MSB_DSR; // Turn off DSR on remote side if DTR is disabled
      break;
    case SERIAL_DTR_CONTROL:
      pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
      bits |= C0C_MSB_DSR;
      mask |= C0C_MSB_DSR;
      break;
    case SERIAL_DTR_HANDSHAKE:
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_DSR;
        mask |= C0C_MSB_DSR;
      }
      else
      if (pIoPortLocal->writeHoldingRemote & SERIAL_TX_WAITING_FOR_DSR) {
        if (C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pNewHandFlow->XonLimit) {
          pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
          bits |= C0C_MSB_DSR;
          mask |= C0C_MSB_DSR;
        }
      }
      else {
        bits |= C0C_MSB_DSR;
        mask |= C0C_MSB_DSR;
      }
    }
  }

  if (!pHandFlow ||
      (pDevExt->handFlow.FlowReplace & SERIAL_AUTO_RECEIVE) !=
          (pHandFlow->FlowReplace & SERIAL_AUTO_RECEIVE))
  {
    if (pNewHandFlow->FlowReplace & SERIAL_AUTO_RECEIVE) {
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_XON;
        if ((pNewHandFlow->FlowReplace & SERIAL_XOFF_CONTINUE) == 0)
          pIoPortLocal->writeHolding |= SERIAL_TX_WAITING_FOR_XON;
        pIoPortLocal->sendXonXoff = C0C_XCHAR_OFF;
        pIoPortLocal->tryWrite = TRUE;
      }
    }
    else
    if (pIoPortLocal->writeHoldingRemote & SERIAL_TX_WAITING_FOR_XON) {
      pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_XON;
      pIoPortLocal->writeHolding &= ~SERIAL_TX_WAITING_FOR_XON;
      if (pIoPortLocal->sendXonXoff != C0C_XCHAR_OFF) {
        // XOFF was sent so send XON
        pIoPortLocal->sendXonXoff = C0C_XCHAR_ON;
        pIoPortLocal->tryWrite = TRUE;
      } else {
        // XOFF still was not sent so cancel it
        pIoPortLocal->sendXonXoff = 0;
      }
    }
  }

  if (pHandFlow)
    pDevExt->handFlow = *pHandFlow;

  if (setModemStatusHolding)
    SetModemStatusHolding(pIoPortLocal);

  if (mask)
    SetModemStatus(pDevExt->pIoPortRemote, bits, mask, pQueueToComplete);

  UpdateTransmitToggle(pDevExt, pQueueToComplete);

  SetLimit(pDevExt->pIoPortRemote->pDevExt);
  SetLimit(pDevExt);

  if (pDevExt->pIoPortRemote->tryWrite) {
    ReadWrite(
        pIoPortLocal, FALSE,
        pDevExt->pIoPortRemote, FALSE,
        pQueueToComplete);
  }

  if (pIoPortLocal->tryWrite) {
    ReadWrite(
        pDevExt->pIoPortRemote, FALSE,
        pIoPortLocal, FALSE,
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

  if (!pIoPortLocal->writeHoldingRemote) {
    if (!freed && C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pHandFlowLocal->XoffLimit)) {
      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_CTS;
        mask |= C0C_MSB_CTS;
      }

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_DSR;
        mask |= C0C_MSB_DSR;
      }

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPortLocal->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_XON;
        if ((pHandFlowLocal->FlowReplace & SERIAL_XOFF_CONTINUE) == 0)
          pIoPortLocal->writeHolding |= SERIAL_TX_WAITING_FOR_XON;
        pIoPortLocal->sendXonXoff = C0C_XCHAR_OFF;
        pIoPortLocal->tryWrite = TRUE;
      }
    }
  } else {
    if (freed && C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pHandFlowLocal->XonLimit) {
      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {
        pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
        bits |= C0C_MSB_CTS;
        mask |= C0C_MSB_CTS;
      }

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
        pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
        bits |= C0C_MSB_DSR;
        mask |= C0C_MSB_DSR;
      }

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPortLocal->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_XON;
        pIoPortLocal->writeHolding &= ~SERIAL_TX_WAITING_FOR_XON;
        pIoPortLocal->sendXonXoff = C0C_XCHAR_ON;
        pIoPortLocal->tryWrite = TRUE;
      }
    }
  }

  if (mask)
    SetModemStatus(pDevExt->pIoPortRemote, bits, mask, pQueueToComplete);
}

VOID UpdateTransmitToggle(
    PC0C_FDOPORT_EXTENSION pDevExt,
    PLIST_ENTRY pQueueToComplete)
{
  if ((pDevExt->handFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {
    ULONG bits;
    PC0C_IO_PORT pIoPortLocal;

    pIoPortLocal = pDevExt->pIoPortLocal;

    if ((pIoPortLocal->writeHolding & SERIAL_TX_WAITING_ON_BREAK) == 0 &&
        (pIoPortLocal->sendXonXoff || pIoPortLocal->irpQueues[C0C_QUEUE_WRITE].pCurrent))
    {
      bits = C0C_MSB_CTS;
    } else {
      bits = 0;
    }

    SetModemStatus(pDevExt->pIoPortRemote, bits, C0C_MSB_CTS, pQueueToComplete);
  }
}

VOID SetLimit(PC0C_FDOPORT_EXTENSION pDevExt)
{
  PC0C_BUFFER pReadBuf;
  SIZE_T limit;
  PSERIAL_HANDFLOW pHandFlowLocal, pHandFlowRemote;

  pHandFlowLocal = &pDevExt->handFlow;
  pHandFlowRemote = &pDevExt->pIoPortRemote->pDevExt->handFlow;
  pReadBuf = &pDevExt->pIoPortLocal->readBuf;

  /* DCD = DSR */
  #define C0C_DSR_HANDSHAKE (SERIAL_DSR_HANDSHAKE|SERIAL_DCD_HANDSHAKE)

  if ((((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) &&
                             (pHandFlowRemote->ControlHandShake & SERIAL_CTS_HANDSHAKE)) ||
      (((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) &&
                             (pHandFlowRemote->ControlHandShake & C0C_DSR_HANDSHAKE)))
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

VOID SetBreakHolding(PC0C_IO_PORT pIoPort, BOOLEAN on)
{
  if (on) {
    if ((pIoPort->writeHolding & SERIAL_TX_WAITING_ON_BREAK) == 0) {
      pIoPort->writeHolding |= SERIAL_TX_WAITING_ON_BREAK;
      pIoPort->sendBreak = TRUE;
      pIoPort->tryWrite = TRUE;
    }
  } else {
    if (pIoPort->writeHolding & SERIAL_TX_WAITING_ON_BREAK) {
      pIoPort->writeHolding &= ~SERIAL_TX_WAITING_ON_BREAK;
      pIoPort->sendBreak = FALSE;

      if ((!(pIoPort->writeHolding & ~SERIAL_TX_WAITING_FOR_XON) && pIoPort->sendXonXoff) ||
          !pIoPort->writeHolding && pIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent)
      {
        pIoPort->tryWrite = TRUE;
      }
    }
  }
}
