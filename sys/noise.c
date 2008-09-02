/*
 * $Id$
 *
 * Copyright (c) 2008 Vyacheslav Frolov
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
 * Revision 1.3  2008/09/02 07:37:34  vfrolov
 * Fixed bits in BreakError()
 *
 * Revision 1.2  2008/09/01 16:45:12  vfrolov
 * Fixed bits in BreakError()
 *
 * Revision 1.1  2008/06/26 13:37:10  vfrolov
 * Implemented noise emulation
 *
 */

#include "precomp.h"
#include "noise.h"
#include "rand.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 0xF

/********************************************************************/
VOID ParityError(PC0C_IO_PORT pReadIoPort, PUCHAR pLsr)
{
  *pLsr |= 0x04;  /* parity error */
  pReadIoPort->errors |= SERIAL_ERROR_PARITY;
  pReadIoPort->perfStats.ParityErrorCount++;
}
/********************************************************************/
VOID FrameError(PC0C_IO_PORT pReadIoPort, PUCHAR pLsr)
{
  *pLsr |= 0x08;  /* framing error */
  pReadIoPort->errors |= SERIAL_ERROR_FRAMING;
  pReadIoPort->perfStats.FrameErrorCount++;
}
/********************************************************************/
VOID BreakError(PC0C_IO_PORT pReadIoPort, PUCHAR pLsr)
{
  *pLsr |= 0x18;  /* break interrupt indicator & framing error */
  pReadIoPort->errors |= SERIAL_ERROR_BREAK;

  if (pReadIoPort->lineControl.Parity == ODD_PARITY || pReadIoPort->lineControl.Parity == MARK_PARITY)
    *pLsr |= 0x04;  /* parity error */
}
/********************************************************************/
UCHAR GarbageChar(PC0C_IO_PORT pWriteIoPort, PC0C_IO_PORT pReadIoPort, PUCHAR pLsr)
{
  if (pWriteIoPort->lineControl.Parity != NO_PARITY && RAND()%2 > 0)
    ParityError(pReadIoPort, pLsr);

  if (RAND()%2 > 0)
    FrameError(pReadIoPort, pLsr);

  return (UCHAR)RAND();
}
/********************************************************************/
VOID BrokeChar(PC0C_IO_PORT pWriteIoPort, PC0C_IO_PORT pReadIoPort, PUCHAR pChar, PUCHAR pLsr)
{
  if (pWriteIoPort->brokeChars) {
    pWriteIoPort->brokeChars--;
    *pChar = GarbageChar(pWriteIoPort, pReadIoPort, pLsr);
  }
  else
  if (pWriteIoPort->brokeCharsProbability > (RAND()%C0C_PROBABILITY_ONE)) {
    int r = (int)(RAND()%(
                      + 1  /* start bit */
                      + 1  /* stop bit(s) */
                      + pWriteIoPort->lineControl.WordLength
                      + ((pWriteIoPort->lineControl.Parity != NO_PARITY) ? 1 : 0)
                      ));

    if (r-- == 0) {
      /* start bit error */

      pWriteIoPort->brokeChars = (short)(RAND()%512);  /* some of subsequent chars should be broken */
      *pChar = GarbageChar(pWriteIoPort, pReadIoPort, pLsr);
    }
    else
    if (r-- == 0) {
      /* stop bit error */

      pWriteIoPort->brokeChars = (short)(RAND()%512);  /* some of subsequent chars should be broken */
      FrameError(pReadIoPort, pLsr);
    }
    else
    if (r < pWriteIoPort->lineControl.WordLength) {
      /* data bit error */

      *pChar ^= (1 << r);

      if (pWriteIoPort->lineControl.Parity == ODD_PARITY || pWriteIoPort->lineControl.Parity == EVEN_PARITY)
        ParityError(pReadIoPort, pLsr);
    }
    else {
      /* parity bit error */

      ParityError(pReadIoPort, pLsr);
    }
  }
}
/********************************************************************/
SIZE_T GetBrokenChars(ULONG brokeCharsProbability, SIZE_T chars)
{
  SIZE_T brokeChars = 0;

  while (chars--) {
    if (brokeCharsProbability > (RAND()%C0C_PROBABILITY_ONE))
      brokeChars++;
  }

  return brokeChars;
}
/********************************************************************/
