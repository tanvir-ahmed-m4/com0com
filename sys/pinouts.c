/*
 * $Id$
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * Revision 1.1  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 *
 */

#include "precomp.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 0xE

VOID SetModemControl(
    IN PC0C_IO_PORT pIoPort,
    IN UCHAR bits,
    IN UCHAR mask,
    PLIST_ENTRY pQueueToComplete)
{
  UCHAR modemControlOld;
  UCHAR modemControlNew;
  UCHAR modemControlChanged;

  modemControlOld = modemControlNew = pIoPort->modemControl;

  modemControlNew |= bits & mask;
  modemControlNew &= ~(~bits & mask);

  modemControlChanged = modemControlOld ^ modemControlNew;

  if (modemControlChanged) {
    UCHAR bitsStatusRemote, maskStatusRemote;
    UCHAR bitsStatusLocal, maskStatusLocal;

    pIoPort->modemControl = modemControlNew;

    bitsStatusRemote = maskStatusRemote = 0;
    bitsStatusLocal = maskStatusLocal = 0;

    #define SET_PIN_OUTS(pin) \
    if (modemControlChanged & C0C_MCR_##pin) { \
      maskStatusRemote |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].remote.positive | \
                          pIoPort->pinOuts[C0C_PIN_OUTS_##pin].remote.negative; \
      maskStatusLocal |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].local.positive | \
                         pIoPort->pinOuts[C0C_PIN_OUTS_##pin].local.negative; \
      if (modemControlNew & C0C_MCR_##pin) { \
        bitsStatusRemote |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].remote.positive; \
        bitsStatusLocal |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].local.positive; \
      } else { \
        bitsStatusRemote |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].remote.negative; \
        bitsStatusLocal |= pIoPort->pinOuts[C0C_PIN_OUTS_##pin].local.negative; \
      } \
    }

    SET_PIN_OUTS(RTS);
    SET_PIN_OUTS(DTR);
    SET_PIN_OUTS(OUT1);

    #undef SET_PIN_OUTS

    if (maskStatusRemote)
      SetModemStatus(pIoPort->pIoPortRemote, bitsStatusRemote, maskStatusRemote, pQueueToComplete);

    if (maskStatusLocal)
      SetModemStatus(pIoPort, bitsStatusLocal, maskStatusLocal, pQueueToComplete);
  }
}

VOID PinWire(
    PC0C_IO_PORT pIoPortLocal,
    PC0C_IO_PORT pIoPortRemote,
    ULONG pinSrc,
    UCHAR pinDst,
    PUCHAR pModemStatus)
{
  switch (pinSrc) {
    case C0C_PIN_ON:
      *pModemStatus |= pinDst;
      break;
    case C0C_PIN_ON|C0C_PIN_NEGATIVE:
      *pModemStatus &= ~pinDst;
      break;

    #define CASE_PIN(pin) \
    case C0C_PIN_R##pin: \
      pIoPortRemote->pinOuts[C0C_PIN_OUTS_##pin].remote.positive |= pinDst; \
      if (pIoPortRemote->modemControl & C0C_MCR_##pin) \
        *pModemStatus |= pinDst; \
      else \
        *pModemStatus &= ~pinDst; \
      break; \
    case C0C_PIN_L##pin: \
      pIoPortLocal->pinOuts[C0C_PIN_OUTS_##pin].local.positive |= pinDst; \
      if (pIoPortLocal->modemControl & C0C_MCR_##pin) \
        *pModemStatus |= pinDst; \
      else \
        *pModemStatus &= ~pinDst; \
      break; \
    case C0C_PIN_R##pin|C0C_PIN_NEGATIVE: \
      pIoPortRemote->pinOuts[C0C_PIN_OUTS_##pin].remote.negative |= pinDst; \
      if (pIoPortRemote->modemControl & C0C_MCR_##pin) \
        *pModemStatus &= ~pinDst; \
      else \
        *pModemStatus |= pinDst; \
      break; \
    case C0C_PIN_L##pin|C0C_PIN_NEGATIVE: \
      pIoPortLocal->pinOuts[C0C_PIN_OUTS_##pin].local.negative |= pinDst; \
      if (pIoPortLocal->modemControl & C0C_MCR_##pin) \
        *pModemStatus &= ~pinDst; \
      else \
        *pModemStatus |= pinDst; \
      break;

    CASE_PIN(RTS)
    CASE_PIN(DTR)
    CASE_PIN(OUT1)

    #undef CASE_PIN
  }
}

VOID PinMap(
    IN PC0C_IO_PORT pIoPort,
    IN ULONG pinCTS,
    IN ULONG pinDSR,
    IN ULONG pinDCD,
    IN ULONG pinRI)
{
  PC0C_IO_PORT pIoPortRemote;
  int i;

  for (i = 0 ; i < sizeof(pIoPort->pinOuts)/sizeof(pIoPort->pinOuts[0]) ; i++)
    RtlZeroMemory(&pIoPort->pinOuts[i].local, sizeof(pIoPort->pinOuts[i].local));

  pIoPortRemote = pIoPort->pIoPortRemote;

  for (i = 0 ; i < sizeof(pIoPortRemote->pinOuts)/sizeof(pIoPortRemote->pinOuts[0]) ; i++)
    RtlZeroMemory(&pIoPortRemote->pinOuts[i].remote, sizeof(pIoPortRemote->pinOuts[i].remote));

  if (!pinCTS)
    pinCTS = C0C_PIN_RRTS;
  PinWire(pIoPort, pIoPortRemote, pinCTS, C0C_MSB_CTS, &pIoPort->modemStatus);

  if (!pinDSR)
    pinDSR = C0C_PIN_RDTR;
  PinWire(pIoPort, pIoPortRemote, pinDSR, C0C_MSB_DSR, &pIoPort->modemStatus);

  if (!pinDCD)
    pinDCD = C0C_PIN_RDTR;
  PinWire(pIoPort, pIoPortRemote, pinDCD, C0C_MSB_RLSD, &pIoPort->modemStatus);

  if (!pinRI)
    pinRI = C0C_PIN_ON|C0C_PIN_NEGATIVE;
  PinWire(pIoPort, pIoPortRemote, pinRI, C0C_MSB_RING, &pIoPort->modemStatus);
}
