/*
 * $Id$
 *
 * Copyright (c) 2006 Vyacheslav Frolov
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
 * Revision 1.1  2006/08/23 13:09:15  vfrolov
 * Initial revision
 *
 *
 */

#include "precomp.h"
#include "commprop.h"

NTSTATUS GetCommProp(
    PC0C_FDOPORT_EXTENSION pDevExt,
    PVOID pBuf,
    ULONG bufSize,
    PULONG pSize)
{
  *pSize = sizeof(SERIAL_COMMPROP);

  if (bufSize < sizeof(SERIAL_COMMPROP))
    return STATUS_BUFFER_TOO_SMALL;

  RtlZeroMemory(pBuf, sizeof(SERIAL_COMMPROP));

  ((PSERIAL_COMMPROP)pBuf)->PacketLength = sizeof(SERIAL_COMMPROP);
  ((PSERIAL_COMMPROP)pBuf)->PacketVersion = 2;
  ((PSERIAL_COMMPROP)pBuf)->ServiceMask = SERIAL_SP_SERIALCOMM;
  ((PSERIAL_COMMPROP)pBuf)->MaxTxQueue = 0;
  ((PSERIAL_COMMPROP)pBuf)->MaxRxQueue = 0;

  ((PSERIAL_COMMPROP)pBuf)->MaxBaud = SERIAL_BAUD_USER;

  ((PSERIAL_COMMPROP)pBuf)->SettableBaud =
    SERIAL_BAUD_075          |
    SERIAL_BAUD_110          |
    SERIAL_BAUD_134_5        |
    SERIAL_BAUD_150          |
    SERIAL_BAUD_300          |
    SERIAL_BAUD_600          |
    SERIAL_BAUD_1200         |
    SERIAL_BAUD_1800         |
    SERIAL_BAUD_2400         |
    SERIAL_BAUD_4800         |
    SERIAL_BAUD_7200         |
    SERIAL_BAUD_9600         |
    SERIAL_BAUD_14400        |
    SERIAL_BAUD_19200        |
    SERIAL_BAUD_38400        |
    SERIAL_BAUD_56K          |
    SERIAL_BAUD_128K         |
    SERIAL_BAUD_115200       |
    SERIAL_BAUD_57600        |
    SERIAL_BAUD_USER;

  ((PSERIAL_COMMPROP)pBuf)->ProvSubType = SERIAL_SP_RS232;

  ((PSERIAL_COMMPROP)pBuf)->ProvCapabilities =
    SERIAL_PCF_DTRDSR        |
    SERIAL_PCF_RTSCTS        |
    SERIAL_PCF_CD            |
    SERIAL_PCF_PARITY_CHECK  |
    SERIAL_PCF_XONXOFF       |
    SERIAL_PCF_SETXCHAR      |
    SERIAL_PCF_TOTALTIMEOUTS |
    SERIAL_PCF_INTTIMEOUTS;

  ((PSERIAL_COMMPROP)pBuf)->SettableParams =
    SERIAL_SP_PARITY         |
    SERIAL_SP_BAUD           |
    SERIAL_SP_DATABITS       |
    SERIAL_SP_STOPBITS       |
    SERIAL_SP_HANDSHAKING    |
    SERIAL_SP_PARITY_CHECK   |
    SERIAL_SP_CARRIER_DETECT;

  ((PSERIAL_COMMPROP)pBuf)->SettableData =
    SERIAL_DATABITS_5        |
    SERIAL_DATABITS_6        |
    SERIAL_DATABITS_7        |
    SERIAL_DATABITS_8;

  ((PSERIAL_COMMPROP)pBuf)->SettableStopParity =
    SERIAL_STOPBITS_10       |
    SERIAL_STOPBITS_15       |
    SERIAL_STOPBITS_20       |
    SERIAL_PARITY_NONE       |
    SERIAL_PARITY_ODD        |
    SERIAL_PARITY_EVEN       |
    SERIAL_PARITY_MARK       |
    SERIAL_PARITY_SPACE;

  ((PSERIAL_COMMPROP)pBuf)->CurrentTxQueue = 0;
  ((PSERIAL_COMMPROP)pBuf)->CurrentRxQueue = (ULONG)C0C_BUFFER_SIZE(&pDevExt->pIoPortLocal->readBuf);

  return STATUS_SUCCESS;
}
