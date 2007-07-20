/*
 * $Id$
 *
 * Copyright (c) 2005-2007 Vyacheslav Frolov
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
 * Revision 1.6  2007/07/20 08:00:22  vfrolov
 * Implemented TX buffer
 *
 * Revision 1.5  2006/05/17 15:28:03  vfrolov
 * Implemented SERIAL_DSR_SENSITIVITY
 *
 * Revision 1.4  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
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

#ifndef _C0C_BUFUTILS_H_
#define _C0C_BUFUTILS_H_

typedef struct _C0C_FLOW_FILTER {
  #define C0C_FLOW_FILTER_AUTO_TRANSMIT      0x01
  #define C0C_FLOW_FILTER_EV_RXCHAR          0x02
  #define C0C_FLOW_FILTER_EV_RXFLAG          0x04
  #define C0C_FLOW_FILTER_NULL_STRIPPING     0x08
  #define C0C_FLOW_FILTER_IGNORE_RECEIVED    0x10

  UCHAR flags;
  UCHAR xonChar;
  UCHAR xoffChar;
  UCHAR eventChar;
  UCHAR escapeChar;

  UCHAR events;
  UCHAR lastXonXoff;
} C0C_FLOW_FILTER, *PC0C_FLOW_FILTER;


NTSTATUS MoveRawData(PC0C_RAW_DATA pDstRawData, PC0C_RAW_DATA pSrcRawData);
VOID FlowFilterInit(PC0C_IO_PORT pIoPort, PC0C_FLOW_FILTER pFlowFilter);
VOID CopyCharsWithEscape(
    PC0C_BUFFER pBuf,
    PC0C_FLOW_FILTER pFlowFilter,
    PUCHAR pReadBuf, SIZE_T readLength,
    PUCHAR pWriteBuf, SIZE_T writeLength,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone);
SIZE_T ReadFromBuffer(PC0C_BUFFER pBuf, PVOID pRead, SIZE_T readLength);
SIZE_T WriteToBuffer(
    PC0C_BUFFER pBuf,
    PVOID pWrite,
    SIZE_T writeLength,
    PC0C_FLOW_FILTER pFlowFilter,
    PSIZE_T pOverrun);
VOID WriteMandatoryToBuffer(PC0C_BUFFER pBuf, UCHAR mandatoryChar);
NTSTATUS WriteRawDataToBuffer(PC0C_RAW_DATA pRawData, PC0C_BUFFER pBuf);
SIZE_T WriteRawData(PC0C_RAW_DATA pRawData, PNTSTATUS pStatus, PVOID pReadBuf, SIZE_T readLength);
BOOLEAN SetNewBufferBase(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size);
VOID PurgeBuffer(PC0C_BUFFER pBuf);
VOID InitBuffer(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size);
VOID FreeBuffer(PC0C_BUFFER pBuf);
VOID SetBufferLimit(PC0C_BUFFER pBuf, SIZE_T limit);
SIZE_T ReadFromTxBuffer(
    PC0C_BUFFER pBuf,
    PC0C_FLOW_FILTER pFlowFilter,
    PVOID pRead, SIZE_T readLength,
    PC0C_TX_BUFFER pTxBuf, SIZE_T txLimit,
    PSIZE_T pWriteDone);
SIZE_T WriteToTxBuffer(
    PC0C_TX_BUFFER pTxBuf,
    PVOID pWrite,
    SIZE_T writeLength);
SIZE_T MoveFromTxBuffer(
    PC0C_BUFFER pBuf,
    PC0C_TX_BUFFER pTxBuf,
    SIZE_T txLimit,
    PC0C_FLOW_FILTER pFlowFilter,
    PSIZE_T pOverrun);
VOID SetTxBuffer(PC0C_TX_BUFFER pTxBuf, SIZE_T size, BOOLEAN cleanFifo);
VOID FreeTxBuffer(PC0C_TX_BUFFER pTxBuf);

#endif /* _C0C_BUFUTILS_H_ */
