/*
 * $Id$
 *
 * Copyright (c) 2005 Vyacheslav Frolov
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
 * Revision 1.2  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.1  2005/08/25 15:38:17  vfrolov
 * Some code moved from io.c to bufutils.c
 *
 */

#ifndef _C0C_BUFUTILS_H_
#define _C0C_BUFUTILS_H_

NTSTATUS MoveRawData(PC0C_RAW_DATA pDstRawData, PC0C_RAW_DATA pSrcRawData);
VOID CopyCharsWithEscape(
    PC0C_BUFFER pBuf, UCHAR escapeChar,
    PUCHAR pReadBuf, SIZE_T readLength,
    PUCHAR pWriteBuf, SIZE_T writeLength,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone);
SIZE_T ReadFromBuffer(PC0C_BUFFER pBuf, PVOID pRead, SIZE_T readLength);
SIZE_T WriteToBuffer(PC0C_BUFFER pBuf, PVOID pWrite, SIZE_T writeLength, UCHAR escapeChar);
VOID WriteMandatoryToBuffer(PC0C_BUFFER pBuf, UCHAR mandatoryChar);
NTSTATUS WriteRawDataToBuffer(PC0C_RAW_DATA pRawData, PC0C_BUFFER pBuf);
SIZE_T WriteRawData(PC0C_RAW_DATA pRawData, PNTSTATUS pStatus, PVOID pReadBuf, SIZE_T readLength);

#endif /* _C0C_BUFUTILS_H_ */
