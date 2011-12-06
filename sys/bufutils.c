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
 * Revision 1.19  2011/12/06 16:03:22  vfrolov
 * Added cleaning high data bits for less then 8 bit data
 * Added AllDataBits option to force 8 bit data
 *
 * Revision 1.18  2010/08/04 10:38:55  vfrolov
 * Minimized PREfast noise
 *
 * Revision 1.17  2008/10/30 07:54:37  vfrolov
 * Improved BREAK emulation
 *
 * Revision 1.16  2008/10/27 15:13:58  vfrolov
 * Fixed buffer overrun bug
 *
 * Revision 1.15  2008/09/12 10:07:50  vfrolov
 * Fixed LSR insertion
 *
 * Revision 1.14  2008/09/02 07:36:22  vfrolov
 * Added missing SERIAL_EV_BREAK
 *
 * Revision 1.13  2008/09/01 16:54:28  vfrolov
 * Replaced SERIAL_LSRMST_LSR_NODATA by SERIAL_LSRMST_LSR_DATA for BREAK
 *
 * Revision 1.12  2008/07/11 10:30:39  vfrolov
 * Added missing data available bit to LSR
 *
 * Revision 1.11  2008/06/26 13:37:10  vfrolov
 * Implemented noise emulation
 *
 * Revision 1.10  2007/10/05 07:34:21  vfrolov
 * Added missing *pOverrun initialization
 * Changed TX FIFO writing emulation to be interrupt driven
 *
 * Revision 1.9  2007/07/20 08:00:22  vfrolov
 * Implemented TX buffer
 *
 * Revision 1.8  2007/01/11 14:50:29  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
 * Revision 1.7  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.6  2006/05/17 15:28:03  vfrolov
 * Implemented SERIAL_DSR_SENSITIVITY
 *
 * Revision 1.5  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.4  2005/11/29 08:35:14  vfrolov
 * Implemented SERIAL_EV_RX80FULL
 *
 * Revision 1.3  2005/11/28 12:57:16  vfrolov
 * Moved some C0C_BUFFER code to bufutils.c
 *
 * Revision 1.2  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.1  2005/08/25 15:38:17  vfrolov
 * Some code moved from io.c to bufutils.c
 *
 */

#include "precomp.h"
#include "bufutils.h"
#include "noise.h"
#include "../include/cncext.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 8

/********************************************************************/
VOID CompactRawData(PC0C_RAW_DATA pRawData, SIZE_T writeDone)
{
  if (writeDone) {
    pRawData->size = (UCHAR)(pRawData->size - writeDone);

    if (pRawData->size) {
      HALT_UNLESS3((pRawData->size + writeDone) <= sizeof(pRawData->data),
          pRawData->size, writeDone, sizeof(pRawData->data));

      RtlMoveMemory(pRawData->data, pRawData->data + writeDone, pRawData->size);
    }
  }
}

SIZE_T AddRawData(PC0C_RAW_DATA pDstRawData, PUCHAR pSrc, SIZE_T length)
{
  SIZE_T len;

  if (!length)
    return 0;

  HALT_UNLESS2(pDstRawData->size <= sizeof(pDstRawData->data),
      pDstRawData->size, sizeof(pDstRawData->data));

  len = sizeof(pDstRawData->data) - pDstRawData->size;

  if (!len)
    return 0;

  if (len > length)
    len = length;

  HALT_UNLESS3((pDstRawData->size + len) <= sizeof(pDstRawData->data),
      pDstRawData->size, len, sizeof(pDstRawData->data));

  RtlCopyMemory(pDstRawData->data + pDstRawData->size, pSrc, len);
  pDstRawData->size = (UCHAR)(pDstRawData->size + len);

  return len;
}

NTSTATUS MoveRawData(PC0C_RAW_DATA pDstRawData, PC0C_RAW_DATA pSrcRawData)
{
  CompactRawData(pSrcRawData, AddRawData(pDstRawData, pSrcRawData->data, pSrcRawData->size));

  return pSrcRawData->size ? STATUS_PENDING : STATUS_SUCCESS;
}
/********************************************************************/
#define C0C_FLOW_FILTER_FL_AUTO_TRANSMIT   0x01
#define C0C_FLOW_FILTER_FL_NULL_STRIPPING  0x02
#define C0C_FLOW_FILTER_FL_IGNORE_RECEIVED 0x04

VOID FlowFilterInit(PC0C_IO_PORT pIoPort, PC0C_FLOW_FILTER pFlowFilter)
{
  PSERIAL_HANDFLOW pHandFlow;

  RtlZeroMemory(pFlowFilter, sizeof(*pFlowFilter));

  pFlowFilter->pIoPort = pIoPort;

  pHandFlow = &pIoPort->handFlow;

  if ((pHandFlow->ControlHandShake & SERIAL_DSR_SENSITIVITY) &&
      (pIoPort->modemStatus & C0C_MSB_DSR) == 0)
  {
    pFlowFilter->flags |= C0C_FLOW_FILTER_FL_IGNORE_RECEIVED;
  }

  if (pHandFlow->FlowReplace & SERIAL_AUTO_TRANSMIT)
    pFlowFilter->flags |= C0C_FLOW_FILTER_FL_AUTO_TRANSMIT;

  if (pHandFlow->FlowReplace & SERIAL_NULL_STRIPPING)
    pFlowFilter->flags |= C0C_FLOW_FILTER_FL_NULL_STRIPPING;
}

VOID CopyCharsWithEscape(
    PC0C_BUFFER pBuf,
    PC0C_FLOW_FILTER pFlowFilter,
    PUCHAR pReadBuf, SIZE_T readLength,
    PUCHAR pWriteBuf, SIZE_T writeLength,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone)
{
  SIZE_T readDone;
  SIZE_T writeDone;

/********************************************************************
 * Special cases if pFlowFilter != NULL:
 *
 *   pReadBuf == NULL && readLength == 0
 *                               - skip writing to pReadBuf
 *
 *   pWriteBuf == NULL           - emulate writeLength noise impulses
 *
 ********************************************************************/

  HALT_UNLESS3(pReadBuf || (pFlowFilter && !readLength), pReadBuf != NULL, pFlowFilter != NULL, readLength);
  HALT_UNLESS2(pWriteBuf || pFlowFilter, pReadBuf != NULL, pFlowFilter != NULL);

  readDone = 0;

  if (pBuf->insertData.size && (readLength || pReadBuf == NULL)) {
    SIZE_T length = pBuf->insertData.size;

    HALT_UNLESS2(length <= sizeof(pBuf->insertData.data),
        length, sizeof(pBuf->insertData.data));

    if (pReadBuf) {
      if (length > readLength)
        length = readLength;

      RtlCopyMemory(pReadBuf, pBuf->insertData.data, length);
      pReadBuf += length;
      readLength -= length;
    }

    readDone += length;
    CompactRawData(&pBuf->insertData, length);
  }

  if (!pFlowFilter) {
    writeDone = writeLength < readLength ? writeLength : readLength;

    if (writeDone) {
      RtlCopyMemory(pReadBuf, pWriteBuf, writeDone);
      readDone += writeDone;
    }
  }
  else {
    PC0C_IO_PORT pIoPort = pFlowFilter->pIoPort;
    PC0C_IO_PORT pIoPortRemote = pIoPort->pIoPortRemote;
    UCHAR dataMask;

    {
      static const UCHAR masks[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
      UCHAR wordLength = 8;

      if (!pIoPort->allDataBits && wordLength > pIoPort->lineControl.WordLength)
        wordLength = pIoPort->lineControl.WordLength;

      if (!pIoPortRemote->allDataBits && wordLength > pIoPortRemote->lineControl.WordLength)
        wordLength = pIoPortRemote->lineControl.WordLength;

      dataMask = masks[wordLength];
    }

    for (writeDone = 0 ; writeDone < writeLength && (readLength || pReadBuf == NULL) ; writeDone++) {
      UCHAR curChar;
      UCHAR lsr = 0;

      if (pWriteBuf) {
        if (pIoPortRemote->sendBreak) {
          pIoPortRemote->sendBreak = FALSE;
          pIoPort->rcvdBreak = TRUE;
          BreakError(pIoPort, &lsr);
          pFlowFilter->events |= SERIAL_EV_BREAK;

          curChar = 0x00;  /* -----------____________ */
                           /* iiiiiiiiiiis00000000bbb */
                           /* dddddddddddt\______/rrr */
                           /* llllllllllla  0x00  eee */
                           /* eeeeeeeeeeer        aaa */
                           /*            t        kkk */
        } else {
          if ((pIoPortRemote->writeHolding & SERIAL_TX_WAITING_ON_BREAK) == 0) {
            if (pIoPortRemote->brokeChars) {
              pIoPortRemote->brokeChars--;
              curChar = GarbageChar(pIoPortRemote, pIoPort, &lsr);
            } else {
              curChar = *pWriteBuf++;

              if (pIoPortRemote->brokeCharsProbability > 0)
                BrokeChar(pIoPortRemote, pIoPort, &curChar, &lsr);
            }
          } else {
            if (pIoPortRemote->brokeCharsProbability <= 0 ||
                GetBrokenChars(pIoPortRemote->brokeCharsProbability, 1) == 0)
            {
              continue;
            }

            BreakError(pIoPort, &lsr);
            pFlowFilter->events |= SERIAL_EV_BREAK;

            curChar = 0x00;  /* __________-____________ */
                             /* bbbbbbbbbbss00000000bbb */
                             /* rrrrrrrrrrtt\______/rrr */
                             /* eeeeeeeeeeoa  0x00  eee */
                             /* aaaaaaaaaapr        aaa */
                             /* kkkkkkkkkk t        kkk */
          }
        }
      } else {
        if (pIoPort->rcvdBreak) {
          /* emulate one noise impulse for BREAK state */

          BreakError(pIoPort, &lsr);
          pFlowFilter->events |= SERIAL_EV_BREAK;

          curChar = 0x00;  /* __________-____________ */
                           /* bbbbbbbbbbss00000000bbb */
                           /* rrrrrrrrrrtt\______/rrr */
                           /* eeeeeeeeeeoa  0x00  eee */
                           /* aaaaaaaaaapr        aaa */
                           /* kkkkkkkkkk t        kkk */
        } else {
          /* emulate one noise impulse for IDLE state */

          curChar = 0xFF;  /* ----------_------------ */
                           /* iiiiiiiiiis11111111siii */
                           /* ddddddddddt\______/tddd */
                           /* lllllllllla  0xFF  olll */
                           /* eeeeeeeeeer        peee */
                           /*           t             */
        }
      }

      curChar &= dataMask;
      pFlowFilter->rxCount++;

      if (lsr) {
        BOOLEAN noCurChar = FALSE;
        BOOLEAN isBreak = ((lsr & 0x10) != 0);
        pFlowFilter->events |= SERIAL_EV_ERR;

        if (pIoPort->escapeChar &&
            ((pIoPort->insertMask & C0CE_INSERT_ENABLE_LSR) != 0 ||
             (isBreak && (pIoPort->insertMask & C0CE_INSERT_ENABLE_LSR_BI) != 0)))
        {
          UCHAR buf[4];
          SIZE_T length = sizeof(buf);
          SIZE_T len;

          lsr |= 0x81;    /* errornous data in FIFO and data available */

          if (C0C_TX_BUFFER_THR_EMPTY(&pIoPort->txBuf)) {
            lsr |= 0x20;  /* transmit holding register empty */

            if (C0C_TX_BUFFER_EMPTY(&pIoPort->txBuf))
              lsr |= 0x40;  /* transmit holding register empty and line is idle */
          }

          if ((pIoPort->insertMask & C0CE_INSERT_ENABLE_LSR_BI) != 0 && pIoPort->rcvdBreak)
            lsr |= 0x10;  /* break interrupt indicator */

          buf[0] = pIoPort->escapeChar;
          buf[1] = SERIAL_LSRMST_LSR_DATA;
          buf[2] = lsr;
          buf[3] = curChar;

          noCurChar = TRUE;

          if (length > readLength)
            len = readLength;
          else
            len = length;

          if (len) {
            RtlCopyMemory(pReadBuf, buf, len);
            pReadBuf += len;
            readDone += len;
            readLength -= len;
            length -= len;
          }

          if (length) {
            if (pReadBuf == NULL) {
              readDone += length;
            } else {
#if DBG
              SIZE_T done =
#endif /* DBG */
              AddRawData(&pBuf->insertData, buf + len, length);
#if DBG
              HALT_UNLESS2(done == length, done, length);
#endif /* DBG */
            }
          }
        }

        if (isBreak) {
          if (pIoPort->handFlow.FlowReplace & SERIAL_BREAK_CHAR) {
            UCHAR errorChar = pIoPort->specialChars.BreakChar;

            if (!readLength) {
              if (pReadBuf == NULL) {
                readDone++;
              } else {
#if DBG
                SIZE_T done =
#endif /* DBG */
                AddRawData(&pBuf->insertData, &errorChar, sizeof(errorChar));
#if DBG
                HALT_UNLESS1(done == sizeof(errorChar), done);
#endif /* DBG */
              }
            } else {
              *pReadBuf++ = errorChar;
              readLength--;
              readDone++;
            }
          }
        }
        else
        if (pIoPort->handFlow.FlowReplace & SERIAL_ERROR_CHAR) {
          UCHAR errorChar = pIoPort->specialChars.ErrorChar;

          if (!readLength) {
            if (pReadBuf == NULL) {
              readDone++;
            } else {
#if DBG
              SIZE_T done =
#endif /* DBG */
              AddRawData(&pBuf->insertData, &errorChar, sizeof(errorChar));
#if DBG
              HALT_UNLESS1(done == sizeof(errorChar), done);
#endif /* DBG */
            }
          } else {
            *pReadBuf++ = errorChar;
            readLength--;
            readDone++;
          }
        }

        if (noCurChar)
          continue;
      }

      if (pFlowFilter->flags & C0C_FLOW_FILTER_FL_IGNORE_RECEIVED) {
      }
      else
      if (!curChar && (pFlowFilter->flags & C0C_FLOW_FILTER_FL_NULL_STRIPPING)) {
      }
      else
      if ((pFlowFilter->flags & C0C_FLOW_FILTER_FL_AUTO_TRANSMIT) &&
          (curChar == pIoPort->specialChars.XoffChar || curChar == pIoPort->specialChars.XonChar))
      {
        if (curChar == pIoPort->specialChars.XoffChar)
          pFlowFilter->lastXonXoff = C0C_XCHAR_OFF;
        else
          pFlowFilter->lastXonXoff = C0C_XCHAR_ON;
      }
      else {
          if (!readLength) {
            if (pReadBuf == NULL) {
              readDone++;
            } else {
#if DBG
              SIZE_T done =
#endif /* DBG */
              AddRawData(&pBuf->insertData, &curChar, sizeof(curChar));
#if DBG
              HALT_UNLESS1(done == sizeof(curChar), done);
#endif /* DBG */
            }
          } else {
            *pReadBuf++ = curChar;
            readLength--;
            readDone++;
          }

          pFlowFilter->events |= SERIAL_EV_RXCHAR;

          if ((pIoPort->waitMask & SERIAL_EV_RXFLAG) &&
              curChar == pIoPort->specialChars.EventChar)
          {
            pFlowFilter->events |= SERIAL_EV_RXFLAG;
          }

          if (pIoPort->escapeChar && curChar == pIoPort->escapeChar) {
            curChar = SERIAL_LSRMST_ESCAPE;

            if (!readLength) {
              if (pReadBuf == NULL) {
                readDone++;
              } else {
#if DBG
                SIZE_T done =
#endif /* DBG */
                AddRawData(&pBuf->insertData, &curChar, sizeof(curChar));
#if DBG
                HALT_UNLESS1(done == sizeof(curChar), done);
#endif /* DBG */
              }
            } else {
              *pReadBuf++ = curChar;
              readLength--;
              readDone++;
            }
          }
      }
    }
  }

  *pReadDone = readDone;
  *pWriteDone = writeDone;
}

SIZE_T ReadFromBuffer(PC0C_BUFFER pBuf, PVOID pRead, SIZE_T readLength)
{
  PUCHAR pReadBuf = (PUCHAR)pRead;

  while (readLength) {
    SIZE_T length, writeLength;
    PUCHAR pWriteBuf;

    if (!pBuf->busy) {
      if (pBuf->insertData.size) {
        length = pBuf->insertData.size;

        HALT_UNLESS2(length <= sizeof(pBuf->insertData.data),
            length, sizeof(pBuf->insertData.data));

        if (length > readLength)
          length = readLength;

        RtlCopyMemory(pReadBuf, pBuf->insertData.data, length);
        pReadBuf += length;
        readLength -= length;
        CompactRawData(&pBuf->insertData, length);
        if (!readLength)
          break;
      }
      break;
    }

    HALT_UNLESS(pBuf->pBase);

    pWriteBuf = pBuf->pBusy;

    writeLength = pBuf->pFree <= pWriteBuf ?
        pBuf->pEnd - pWriteBuf : pBuf->busy;

    length = writeLength < readLength ? writeLength : readLength;

    RtlCopyMemory(pReadBuf, pWriteBuf, length);

    pBuf->busy -= length;
    pBuf->pBusy += length;
    if (pBuf->pBusy == pBuf->pEnd)
      pBuf->pBusy = pBuf->pBase;

    pReadBuf += length;
    readLength -= length;
  }

  return pReadBuf - (PUCHAR)pRead;
}

SIZE_T WriteToBuffer(
    PC0C_BUFFER pBuf,
    PVOID pWrite,
    SIZE_T writeLength,
    PC0C_FLOW_FILTER pFlowFilter,
    PSIZE_T pOverrun)
{
  SIZE_T writeDoneTotal = 0;
  PUCHAR pWriteBuf = (PUCHAR)pWrite;

  if (pOverrun)
    *pOverrun = 0;

  while (writeLength) {
    SIZE_T readDone, writeDone;
    SIZE_T readLength;
    PUCHAR pReadBuf;

    if (pBuf->limit <= pBuf->busy) {
      if (pOverrun) {
        if (pBuf->busy >= C0C_BUFFER_SIZE(pBuf)) {
          CopyCharsWithEscape(
              pBuf, pFlowFilter,
              NULL, 0,
              pWriteBuf, writeLength,
              pOverrun, &writeDone);

          writeDoneTotal += writeDone;
        }
      }
      break;
    }

    pReadBuf = pBuf->pFree;

    readLength = pBuf->pBusy <= pReadBuf ?
        pBuf->pEnd - pReadBuf : pBuf->pBusy - pReadBuf;

    if (readLength > (pBuf->limit - pBuf->busy))
      readLength = pBuf->limit - pBuf->busy;

    CopyCharsWithEscape(
        pBuf, pFlowFilter,
        pReadBuf, readLength,
        pWriteBuf, writeLength,
        &readDone, &writeDone);

    pBuf->busy += readDone;
    pBuf->pFree += readDone;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    writeDoneTotal += writeDone;
    writeLength -= writeDone;

    if (pWriteBuf)
      pWriteBuf += writeDone;
  }

  return writeDoneTotal;
}

VOID WriteMandatoryToBuffer(PC0C_BUFFER pBuf, UCHAR mandatoryChar)
{
  if (C0C_BUFFER_SIZE(pBuf) <= pBuf->busy) {
    if (pBuf->pBase) {
      if (pBuf->pFree == pBuf->pBase)
        *(pBuf->pEnd - 1) = mandatoryChar;
      else
        *(pBuf->pFree - 1) = mandatoryChar;
    }
  } else {
    pBuf->busy++;
    *pBuf->pFree = mandatoryChar;
    if (++pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;
  }
}

NTSTATUS WriteRawDataToBuffer(PC0C_RAW_DATA pRawData, PC0C_BUFFER pBuf)
{
  NTSTATUS status;

  if (!pBuf->pBase)
    return STATUS_PENDING;

  status = STATUS_PENDING;

  for (;;) {
    SIZE_T readDone, writeDone;
    SIZE_T writeLength, readLength;
    PVOID pWriteBuf, pReadBuf;

    writeLength = pRawData->size;

    if (!writeLength) {
      status = STATUS_SUCCESS;
      break;
    }

    HALT_UNLESS2(writeLength <= sizeof(pRawData->data),
        writeLength, sizeof(pRawData->data));

    pWriteBuf = pRawData->data;

    if (C0C_BUFFER_SIZE(pBuf) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pReadBuf = pBuf->pFree;

    CopyCharsWithEscape(
        pBuf, NULL,
        pReadBuf, readLength,
        pWriteBuf, writeLength,
        &readDone, &writeDone);

    pBuf->busy += readDone;
    pBuf->pFree += readDone;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    CompactRawData(pRawData, writeDone);
  }

  return status;
}

SIZE_T WriteRawData(PC0C_RAW_DATA pRawData, PNTSTATUS pStatus, PVOID pReadBuf, SIZE_T readLength)
{
  SIZE_T length, writeLength;
  PVOID pWriteBuf;

  pWriteBuf = pRawData->data;
  writeLength = pRawData->size;

  HALT_UNLESS2(writeLength <= sizeof(pRawData->data),
      writeLength, sizeof(pRawData->data));

  length = writeLength < readLength ? writeLength : readLength;

  RtlCopyMemory(pReadBuf, pWriteBuf, length);

  CompactRawData(pRawData, length);

  if (!pRawData->size)
    *pStatus = STATUS_SUCCESS;

  return length;
}

VOID InitBufferBase(PC0C_BUFFER pBuf, __drv_aliasesMem PUCHAR pBase, SIZE_T size)
{
  pBuf->pBase = pBase;
  pBuf->pEnd = pBuf->pBase + size;
  pBuf->limit = size;
  pBuf->size80 = (size*4 + 4)/5;
}

BOOLEAN SetNewBufferBase(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size)
{
  C0C_BUFFER newBuf;

  if (size <= C0C_BUFFER_SIZE(pBuf)) {
    C0C_FREE_POOL(pBase);
    return FALSE;
  }

  InitBufferBase(&newBuf, pBase, size);

  newBuf.pFree = newBuf.pBusy = newBuf.pBase;
  newBuf.busy = 0;

  if (pBuf->pBase) {
    while (pBuf->busy) {
      SIZE_T length;

      length = pBuf->pFree <= pBuf->pBusy ?
          pBuf->pEnd - pBuf->pBusy : pBuf->busy;

      RtlCopyMemory(newBuf.pFree, pBuf->pBusy, length);

      pBuf->busy -= length;
      pBuf->pBusy += length;
      if (pBuf->pBusy == pBuf->pEnd)
        pBuf->pBusy = pBuf->pBase;

      newBuf.busy += length;
      newBuf.pFree += length;
    }

    C0C_FREE_POOL(pBuf->pBase);
  }

  newBuf.insertData = pBuf->insertData;

  *pBuf = newBuf;

  return TRUE;
}

VOID PurgeBuffer(PC0C_BUFFER pBuf)
{
  pBuf->pFree = pBuf->pBusy = pBuf->pBase;
  pBuf->busy = 0;
  pBuf->insertData.size = 0;
}

VOID InitBuffer(PC0C_BUFFER pBuf, __drv_aliasesMem PUCHAR pBase, SIZE_T size)
{
  RtlZeroMemory(pBuf, sizeof(*pBuf));
  InitBufferBase(pBuf, pBase, size);
  PurgeBuffer(pBuf);
}

VOID FreeBuffer(PC0C_BUFFER pBuf)
{
  if (pBuf->pBase)
    C0C_FREE_POOL(pBuf->pBase);

  RtlZeroMemory(pBuf, sizeof(*pBuf));
}

VOID SetBufferLimit(PC0C_BUFFER pBuf, SIZE_T limit)
{
  if (limit > C0C_BUFFER_SIZE(pBuf))
    limit = C0C_BUFFER_SIZE(pBuf);

  pBuf->limit = limit;
}
/********************************************************************/
SIZE_T ReadFromTxBuffer(
    PC0C_BUFFER pBuf,
    PC0C_FLOW_FILTER pFlowFilter,
    PVOID pRead, SIZE_T readLength,
    PC0C_TX_BUFFER pTxBuf, SIZE_T txLimit,
    PSIZE_T pWriteDone)
{
  PUCHAR pReadBuf = (PUCHAR)pRead;

  *pWriteDone = 0;

  while (readLength) {
    SIZE_T writeLength;
    SIZE_T readDone, writeDone;
    PUCHAR pWriteBuf;

    pWriteBuf = pTxBuf->pBusy;

    if (pTxBuf->busy) {
      writeLength = pTxBuf->pFree <= pWriteBuf ?
          pTxBuf->pEnd - pWriteBuf : pTxBuf->busy;

      if (writeLength > txLimit)
        writeLength = txLimit;
    } else {
      writeLength = 0;
    }

    CopyCharsWithEscape(pBuf, pFlowFilter,
                       pReadBuf, readLength,
                       pWriteBuf, writeLength,
                       &readDone, &writeDone);

    pTxBuf->busy -= writeDone;
    pTxBuf->pBusy += writeDone;
    if (pTxBuf->pBusy == pTxBuf->pEnd)
      pTxBuf->pBusy = pTxBuf->pBase;

    *pWriteDone += writeDone;

    if (readDone == 0)
      break;

    pReadBuf += readDone;
    readLength -= readDone;
    txLimit -= writeDone;
  }

  return pReadBuf - (PUCHAR)pRead;
}

SIZE_T WriteToTxBuffer(
    PC0C_TX_BUFFER pTxBuf,
    PVOID pWrite,
    SIZE_T writeLength)
{
  PUCHAR pWriteBuf;
  SIZE_T sizeTxBuf;

  /* Writing to TX FIFO is interrupt driven so we can write only if it's empty */

  if (!C0C_TX_BUFFER_THR_EMPTY(pTxBuf))
    return 0;

  sizeTxBuf = C0C_TX_BUFFER_SIZE(pTxBuf);

  /* Write not more then TxFIFO registry value */

  if (writeLength > sizeTxBuf - 1)
    writeLength = sizeTxBuf - 1;

  pWriteBuf = (PUCHAR)pWrite;

  while (writeLength && pTxBuf->busy < sizeTxBuf) {
    SIZE_T readLength;
    PUCHAR pReadBuf;

    pReadBuf = pTxBuf->pFree;

    readLength = pTxBuf->pBusy <= pReadBuf ?
        pTxBuf->pEnd - pReadBuf : pTxBuf->pBusy - pReadBuf;

    if (readLength > writeLength)
      readLength = writeLength;

    RtlCopyMemory(pReadBuf, pWriteBuf, readLength);

    pTxBuf->busy += readLength;
    pTxBuf->pFree += readLength;
    if (pTxBuf->pFree == pTxBuf->pEnd)
      pTxBuf->pFree = pTxBuf->pBase;

    pWriteBuf += readLength;
    writeLength -= readLength;
  }

  return pWriteBuf - (PUCHAR)pWrite;
}

SIZE_T MoveFromTxBuffer(
    PC0C_BUFFER pBuf,
    PC0C_TX_BUFFER pTxBuf,
    SIZE_T txLimit,
    PC0C_FLOW_FILTER pFlowFilter,
    PSIZE_T pOverrun)
{
  SIZE_T done = 0;

  if (pOverrun)
    *pOverrun = 0;

  while (pTxBuf->busy) {
    SIZE_T writeLength;
    SIZE_T writeDone;
    PUCHAR pWriteBuf;

    pWriteBuf = pTxBuf->pBusy;

    writeLength = pTxBuf->pFree <= pWriteBuf ?
        pTxBuf->pEnd - pWriteBuf : pTxBuf->busy;

    if (writeLength > txLimit)
      writeLength = txLimit;

    if (writeLength == 0)
      break;

    if (pOverrun) {
      SIZE_T overrun;

      writeDone = WriteToBuffer(pBuf, pWriteBuf, writeLength, pFlowFilter, &overrun);

      *pOverrun += overrun;
    } else {
      writeDone = WriteToBuffer(pBuf, pWriteBuf, writeLength, pFlowFilter, NULL);
    }

    if (writeDone == 0)
      break;

    pTxBuf->busy -= writeDone;
    pTxBuf->pBusy += writeDone;
    if (pTxBuf->pBusy == pTxBuf->pEnd)
      pTxBuf->pBusy = pTxBuf->pBase;

    done += writeDone;
    txLimit -= writeDone;
  }

  return done;
}

VOID SetTxBuffer(PC0C_TX_BUFFER pTxBuf, SIZE_T size, BOOLEAN cleanFifo)
{
  PUCHAR pBase;

  UNREFERENCED_PARAMETER(cleanFifo);

  size = sizeof(pTxBuf->leastBuf) - 1;

  size += 1;  /* add shift register */
  pBase = pTxBuf->leastBuf;

  pTxBuf->pFree = pTxBuf->pBusy = pTxBuf->pBase = pBase;
  pTxBuf->pEnd = pTxBuf->pBase + size;
  pTxBuf->busy = 0;
}

VOID FreeTxBuffer(PC0C_TX_BUFFER pTxBuf)
{
  if (pTxBuf->pBase && pTxBuf->pBase != pTxBuf->leastBuf)
    C0C_FREE_POOL(pTxBuf->pBase);

  RtlZeroMemory(pTxBuf, sizeof(*pTxBuf));
}
/********************************************************************/
