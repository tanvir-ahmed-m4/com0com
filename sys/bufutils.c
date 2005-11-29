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

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 8

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

NTSTATUS MoveRawData(PC0C_RAW_DATA pDstRawData, PC0C_RAW_DATA pSrcRawData)
{
  SIZE_T free;

  if (!pSrcRawData->size)
    return STATUS_SUCCESS;

  HALT_UNLESS2(pDstRawData->size <= sizeof(pDstRawData->data),
      pDstRawData->size, sizeof(pDstRawData->data));

  free = sizeof(pDstRawData->data) - pDstRawData->size;

  if (free) {
    SIZE_T length;

    if (free > pSrcRawData->size)
      length = pSrcRawData->size;
    else
      length = free;

    HALT_UNLESS3((pDstRawData->size + length) <= sizeof(pDstRawData->data),
        pDstRawData->size, length, sizeof(pDstRawData->data));

    RtlCopyMemory(pDstRawData->data + pDstRawData->size, pSrcRawData->data, length);
    pDstRawData->size = (UCHAR)(pDstRawData->size + length);
    CompactRawData(pSrcRawData, length);
  }
  return pSrcRawData->size ? STATUS_PENDING : STATUS_SUCCESS;
}

VOID CopyCharsWithEscape(
    PC0C_BUFFER pBuf, UCHAR escapeChar,
    PUCHAR pReadBuf, SIZE_T readLength,
    PUCHAR pWriteBuf, SIZE_T writeLength,
    PSIZE_T pReadDone,
    PSIZE_T pWriteDone)
{
  SIZE_T readDone;
  SIZE_T writeDone;

  readDone = 0;

  if (pBuf->escape && readLength) {
    pBuf->escape = FALSE;
    *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
    readDone++;
    readLength--;
  }

  if (pBuf->insertData.size && readLength) {
    SIZE_T length = pBuf->insertData.size;

    HALT_UNLESS2(length <= sizeof(pBuf->insertData.data),
        length, sizeof(pBuf->insertData.data));

    if (length > readLength)
      length = readLength;

    RtlCopyMemory(pReadBuf, pBuf->insertData.data, length);
    pReadBuf += length;
    readDone += length;
    readLength -= length;
    CompactRawData(&pBuf->insertData, length);
  }

  if (!escapeChar) {
    writeDone = writeLength < readLength ? writeLength : readLength;

    if (writeDone) {
      RtlCopyMemory(pReadBuf, pWriteBuf, writeDone);
      readDone += writeDone;
    }
  } else {
    writeDone = 0;

    while (writeLength--) {
      UCHAR curChar;

      if (!readLength--)
        break;

      curChar = *pWriteBuf++;
      writeDone++;
      *pReadBuf++ = curChar;
      readDone++;

      if (curChar == escapeChar) {
        if (!readLength--) {
          pBuf->escape = TRUE;
          break;
        }
        *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
        readDone++;
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
    PVOID pWriteBuf;

    if (!pBuf->busy) {
      if (pBuf->escape) {
        pBuf->escape = FALSE;
        *pReadBuf++ = SERIAL_LSRMST_ESCAPE;
        readLength--;
        if (!readLength)
          break;
      }
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

    writeLength = pBuf->pFree <= pBuf->pBusy ?
        pBuf->pEnd - pBuf->pBusy : pBuf->busy;

    pWriteBuf = pBuf->pBusy;

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

SIZE_T WriteToBuffer(PC0C_BUFFER pBuf, PVOID pWrite, SIZE_T writeLength, UCHAR escapeChar)
{
  PUCHAR pWriteBuf = (PUCHAR)pWrite;

  while (writeLength) {
    SIZE_T readDone, writeDone;
    SIZE_T readLength;
    PVOID pReadBuf;

    if ((SIZE_T)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pReadBuf = pBuf->pFree;

    CopyCharsWithEscape(
        pBuf, escapeChar,
        pReadBuf, readLength,
        pWriteBuf, writeLength,
        &readDone, &writeDone);

    pBuf->busy += readDone;
    pBuf->pFree += readDone;
    if (pBuf->pFree == pBuf->pEnd)
      pBuf->pFree = pBuf->pBase;

    pWriteBuf += writeDone;
    writeLength -= writeDone;
  }

  return pWriteBuf - (PUCHAR)pWrite;
}

VOID WriteMandatoryToBuffer(PC0C_BUFFER pBuf, UCHAR mandatoryChar)
{
  if ((SIZE_T)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy) {
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

    if ((SIZE_T)(pBuf->pEnd - pBuf->pBase) <= pBuf->busy)
      break;

    readLength = pBuf->pBusy <= pBuf->pFree  ?
        pBuf->pEnd - pBuf->pFree : pBuf->pBusy - pBuf->pFree;

    pReadBuf = pBuf->pFree;

    CopyCharsWithEscape(
        pBuf, 0,
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

VOID InitBufferBase(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size)
{
  pBuf->pBase = pBase;
  pBuf->pEnd = pBuf->pBase + size;
  pBuf->size80 = (size*4 + 4)/5;
}

BOOLEAN SetNewBufferBase(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size)
{
  C0C_BUFFER newBuf;

  if (size <= C0C_BUFFER_SIZE(pBuf)) {
    ExFreePool(pBase);
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

    ExFreePool(pBuf->pBase);
  }

  newBuf.escape = pBuf->escape;
  newBuf.insertData = pBuf->insertData;

  *pBuf = newBuf;

  return TRUE;
}

VOID PurgeBuffer(PC0C_BUFFER pBuf)
{
  pBuf->pFree = pBuf->pBusy = pBuf->pBase;
  pBuf->busy = 0;
  pBuf->escape = FALSE;
  pBuf->insertData.size = 0;
}

VOID InitBuffer(PC0C_BUFFER pBuf, PUCHAR pBase, SIZE_T size)
{
  RtlZeroMemory(pBuf, sizeof(*pBuf));
  InitBufferBase(pBuf, pBase, size);
  PurgeBuffer(pBuf);
}

VOID FreeBuffer(PC0C_BUFFER pBuf)
{
  if (pBuf->pBase)
    ExFreePool(pBuf->pBase);

  RtlZeroMemory(pBuf, sizeof(*pBuf));
}
