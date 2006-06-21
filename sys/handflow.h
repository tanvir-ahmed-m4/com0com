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
 * Revision 1.4  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.3  2006/05/17 15:31:14  vfrolov
 * Implemented SERIAL_TRANSMIT_TOGGLE
 *
 * Revision 1.2  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
 * Revision 1.1  2006/01/10 10:12:05  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_FLOW_H_
#define _C0C_FLOW_H_

NTSTATUS SetHandFlow(
    PC0C_IO_PORT pIoPort,
    PSERIAL_HANDFLOW pHandFlow,
    PLIST_ENTRY pQueueToComplete);

VOID UpdateHandFlow(
    PC0C_IO_PORT pIoPort,
    BOOLEAN freed,
    PLIST_ENTRY pQueueToComplete);

VOID UpdateTransmitToggle(
    PC0C_IO_PORT pIoPort,
    PLIST_ENTRY pQueueToComplete);

VOID SetLimit(PC0C_IO_PORT pIoPort);
VOID SetModemStatusHolding(PC0C_IO_PORT pIoPort);
VOID SetXonXoffHolding(PC0C_IO_PORT pIoPort, short xonXoff);
VOID SetBreakHolding(PC0C_IO_PORT pIoPort, BOOLEAN on);

#endif /* _C0C_FLOW_H_ */
