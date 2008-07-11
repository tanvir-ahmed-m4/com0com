/*
 * $Id$
 *
 * Copyright (c) 2005-2008 Vyacheslav Frolov
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
 * Revision 1.9  2008/07/11 10:38:00  vfrolov
 * Added nonstandard ability to enable LSR insertion on BREAK OFF
 *
 * Revision 1.8  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.7  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
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
 */

#include "precomp.h"
#include "handflow.h"
#include "bufutils.h"
#include "../include/cncext.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 9

NTSTATUS SetHandFlow(
    PC0C_IO_PORT pIoPort,
    PSERIAL_HANDFLOW pHandFlow,
    PLIST_ENTRY pQueueToComplete)
{
  UCHAR bits, mask;
  PC0C_BUFFER pReadBuf;
  PSERIAL_HANDFLOW pNewHandFlow;
  BOOLEAN setModemStatusHolding;

  pReadBuf = &pIoPort->readBuf;

  if (pHandFlow) {
    if ((pIoPort->escapeChar && (pHandFlow->FlowReplace & SERIAL_ERROR_CHAR)) ||
        ((SIZE_T)pHandFlow->XonLimit > C0C_BUFFER_SIZE(pReadBuf)) ||
        ((SIZE_T)pHandFlow->XoffLimit > C0C_BUFFER_SIZE(pReadBuf)))
    {
      return STATUS_INVALID_PARAMETER;
    }

    pNewHandFlow = pHandFlow;
  } else {
    pNewHandFlow = &pIoPort->handFlow;
  }

  // Set local side
  if (pHandFlow &&
      ((pIoPort->handFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) != 0) &&
      ((pHandFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) == 0))
  {
    SetXonXoffHolding(pIoPort, C0C_XCHAR_ON);
  }

  if (!pHandFlow ||
      (pIoPort->handFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) !=
          (pHandFlow->ControlHandShake & SERIAL_OUT_HANDSHAKEMASK))
  {
    setModemStatusHolding = TRUE;
  } else {
    setModemStatusHolding = FALSE;
  }

  // Set remote side
  bits = mask = 0;

  if (!pHandFlow ||
      (pIoPort->handFlow.FlowReplace & SERIAL_RTS_MASK) !=
          (pHandFlow->FlowReplace & SERIAL_RTS_MASK))
  {
    switch (pNewHandFlow->FlowReplace & SERIAL_RTS_MASK) {
    case 0:
      pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
      mask |= C0C_MCR_RTS;
      break;
    case SERIAL_RTS_CONTROL:
      pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
      bits |= C0C_MCR_RTS;
      mask |= C0C_MCR_RTS;
      break;
    case SERIAL_RTS_HANDSHAKE:
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_CTS;
        mask |= C0C_MCR_RTS;
      }
      else
      if (pIoPort->writeHoldingRemote & SERIAL_TX_WAITING_FOR_CTS) {
        if (C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pNewHandFlow->XonLimit) {
          pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
          bits |= C0C_MCR_RTS;
          mask |= C0C_MCR_RTS;
        }
      }
      else {
        bits |= C0C_MCR_RTS;
        mask |= C0C_MCR_RTS;
      }
    }
  }

  if (!pHandFlow ||
      (pIoPort->handFlow.ControlHandShake & SERIAL_DTR_MASK) !=
          (pHandFlow->ControlHandShake & SERIAL_DTR_MASK))
  {
    switch (pNewHandFlow->ControlHandShake & SERIAL_DTR_MASK) {
    case 0:
      pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
      mask |= C0C_MCR_DTR;
      break;
    case SERIAL_DTR_CONTROL:
      pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
      bits |= C0C_MCR_DTR;
      mask |= C0C_MCR_DTR;
      break;
    case SERIAL_DTR_HANDSHAKE:
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_DSR;
        mask |= C0C_MCR_DTR;
      }
      else
      if (pIoPort->writeHoldingRemote & SERIAL_TX_WAITING_FOR_DSR) {
        if (C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pNewHandFlow->XonLimit) {
          pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
          bits |= C0C_MCR_DTR;
          mask |= C0C_MCR_DTR;
        }
      }
      else {
        bits |= C0C_MCR_DTR;
        mask |= C0C_MCR_DTR;
      }
    }
  }

  if (!pHandFlow ||
      (pIoPort->handFlow.FlowReplace & SERIAL_AUTO_RECEIVE) !=
          (pHandFlow->FlowReplace & SERIAL_AUTO_RECEIVE))
  {
    if (pNewHandFlow->FlowReplace & SERIAL_AUTO_RECEIVE) {
      if (C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pNewHandFlow->XoffLimit)) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_XON;
        if ((pNewHandFlow->FlowReplace & SERIAL_XOFF_CONTINUE) == 0)
          pIoPort->writeHolding |= SERIAL_TX_WAITING_FOR_XON;
        pIoPort->sendXonXoff = C0C_XCHAR_OFF;
        pIoPort->tryWrite = TRUE;
      }
    }
    else
    if (pIoPort->writeHoldingRemote & SERIAL_TX_WAITING_FOR_XON) {
      pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_XON;
      pIoPort->writeHolding &= ~SERIAL_TX_WAITING_FOR_XON;
      if (pIoPort->sendXonXoff != C0C_XCHAR_OFF) {
        // XOFF was sent so send XON
        pIoPort->sendXonXoff = C0C_XCHAR_ON;
        pIoPort->tryWrite = TRUE;
      } else {
        // XOFF still was not sent so cancel it
        pIoPort->sendXonXoff = 0;
      }
    }
  }

  if (pHandFlow)
    pIoPort->handFlow = *pHandFlow;

  if (setModemStatusHolding)
    SetModemStatusHolding(pIoPort);

  if (mask)
    SetModemControl(pIoPort, bits, mask, pQueueToComplete);

  UpdateTransmitToggle(pIoPort, pQueueToComplete);

  SetLimit(pIoPort->pIoPortRemote);
  SetLimit(pIoPort);

  if (pIoPort->pIoPortRemote->tryWrite) {
    ReadWrite(
        pIoPort, FALSE,
        pIoPort->pIoPortRemote, FALSE,
        pQueueToComplete);
  }

  if (pIoPort->tryWrite) {
    ReadWrite(
        pIoPort->pIoPortRemote, FALSE,
        pIoPort, FALSE,
        pQueueToComplete);
  }

  return STATUS_SUCCESS;
}

VOID UpdateHandFlow(
    PC0C_IO_PORT pIoPort,
    BOOLEAN freed,
    PLIST_ENTRY pQueueToComplete)
{
  UCHAR bits, mask;
  PC0C_BUFFER pReadBuf;
  PSERIAL_HANDFLOW pHandFlowLocal, pHandFlowRemote;

  pHandFlowLocal = &pIoPort->handFlow;
  pHandFlowRemote = &pIoPort->pIoPortRemote->handFlow;
  pReadBuf = &pIoPort->readBuf;

  bits = mask = 0;

  if (!pIoPort->writeHoldingRemote) {
    if (!freed && C0C_BUFFER_BUSY(pReadBuf) > (C0C_BUFFER_SIZE(pReadBuf) - pHandFlowLocal->XoffLimit)) {
      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_CTS;
        mask |= C0C_MCR_RTS;
      }

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_DSR;
        mask |= C0C_MCR_DTR;
      }

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPort->writeHoldingRemote |= SERIAL_TX_WAITING_FOR_XON;
        if ((pHandFlowLocal->FlowReplace & SERIAL_XOFF_CONTINUE) == 0)
          pIoPort->writeHolding |= SERIAL_TX_WAITING_FOR_XON;
        pIoPort->sendXonXoff = C0C_XCHAR_OFF;
        pIoPort->tryWrite = TRUE;
      }
    }
  } else {
    if (freed && C0C_BUFFER_BUSY(pReadBuf) <= (SIZE_T)pHandFlowLocal->XonLimit) {
      if ((pHandFlowLocal->FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {
        pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_CTS;
        bits |= C0C_MCR_RTS;
        mask |= C0C_MCR_RTS;
      }

      if ((pHandFlowLocal->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
        pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_DSR;
        bits |= C0C_MCR_DTR;
        mask |= C0C_MCR_DTR;
      }

      if (pHandFlowLocal->FlowReplace & SERIAL_AUTO_RECEIVE) {
        pIoPort->writeHoldingRemote &= ~SERIAL_TX_WAITING_FOR_XON;
        pIoPort->writeHolding &= ~SERIAL_TX_WAITING_FOR_XON;
        pIoPort->sendXonXoff = C0C_XCHAR_ON;
        pIoPort->tryWrite = TRUE;
      }
    }
  }

  if (mask)
    SetModemControl(pIoPort, bits, mask, pQueueToComplete);
}

VOID UpdateTransmitToggle(
    PC0C_IO_PORT pIoPort,
    PLIST_ENTRY pQueueToComplete)
{
  if ((pIoPort->handFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {
    UCHAR bits;

    if ((pIoPort->writeHolding & SERIAL_TX_WAITING_ON_BREAK) == 0 &&
        (pIoPort->sendXonXoff || pIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent))
    {
      bits = C0C_MCR_RTS;
    } else {
      bits = 0;
    }

    SetModemControl(pIoPort, bits, C0C_MCR_RTS, pQueueToComplete);
  }
}

VOID SetLimit(PC0C_IO_PORT pIoPort)
{
  PC0C_BUFFER pReadBuf;
  SIZE_T limit;
  PSERIAL_HANDFLOW pHandFlowLocal, pHandFlowRemote;

  pHandFlowLocal = &pIoPort->handFlow;
  pHandFlowRemote = &pIoPort->pIoPortRemote->handFlow;
  pReadBuf = &pIoPort->readBuf;

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

  pHandFlow = &pIoPort->handFlow;
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

VOID SetBreakHolding(PC0C_IO_PORT pIoPort, BOOLEAN on, PLIST_ENTRY pQueueToComplete)
{
  if (on) {
    if ((pIoPort->writeHolding & SERIAL_TX_WAITING_ON_BREAK) == 0) {
      pIoPort->writeHolding |= SERIAL_TX_WAITING_ON_BREAK;
      pIoPort->sendBreak = TRUE;
      pIoPort->tryWrite = TRUE;
    }
  } else {
    if (pIoPort->writeHolding & SERIAL_TX_WAITING_ON_BREAK) {
      PC0C_IO_PORT pIoPortRead;

      pIoPort->writeHolding &= ~SERIAL_TX_WAITING_ON_BREAK;
      pIoPort->sendBreak = FALSE;

      pIoPortRead = pIoPort->pIoPortRemote;

      if (pIoPortRead->escapeChar && (pIoPortRead->insertMask & C0CE_INSERT_ENABLE_LSR_NBI)) {
        UCHAR lsr = 0;

        if (C0C_TX_BUFFER_THR_EMPTY(&pIoPortRead->txBuf)) {
          lsr |= 0x20;  /* transmit holding register empty */

          if (C0C_TX_BUFFER_EMPTY(&pIoPortRead->txBuf))
            lsr |= 0x40;  /* transmit holding register empty and line is idle */
        }

        InsertLsrMst(pIoPortRead, FALSE,  lsr, pQueueToComplete);
      }

      if ((!(pIoPort->writeHolding & ~SERIAL_TX_WAITING_FOR_XON) && pIoPort->sendXonXoff) ||
          (!pIoPort->writeHolding && pIoPort->irpQueues[C0C_QUEUE_WRITE].pCurrent))
      {
        pIoPort->tryWrite = TRUE;
      }
    }
  }
}
