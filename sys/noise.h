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
 * Revision 1.2  2008/10/30 07:54:37  vfrolov
 * Improved BREAK emulation
 *
 * Revision 1.1  2008/06/26 13:37:10  vfrolov
 * Implemented noise emulation
 *
 */

#ifndef _C0C_NOISE_H_
#define _C0C_NOISE_H_

VOID BreakError(PC0C_IO_PORT pReadIoPort, PUCHAR pLsr);
UCHAR GarbageChar(PC0C_IO_PORT pWriteIoPort, PC0C_IO_PORT pReadIoPort, PUCHAR pLsr);
VOID BrokeChar(PC0C_IO_PORT pWriteIoPort, PC0C_IO_PORT pReadIoPort, PUCHAR pChar, PUCHAR pLsr);
SIZE_T GetBrokenChars(ULONG brokeCharsProbability, SIZE_T chars);

#endif /* _C0C_NOISE_H_ */
