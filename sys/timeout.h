/*
 * $Id$
 *
 * Copyright (c) 2004-2007 Vyacheslav Frolov
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
 * Revision 1.6  2007/06/04 15:24:33  vfrolov
 * Fixed open reject just after close in exclusiveMode
 *
 * Revision 1.5  2007/02/20 12:05:11  vfrolov
 * Implemented IOCTL_SERIAL_XOFF_COUNTER
 * Fixed cancel and timeout routines
 *
 * Revision 1.4  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.3  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.2  2005/08/16 16:36:33  vfrolov
 * Hidden timeout functions
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _C0C_TIMEOUT_H_
#define _C0C_TIMEOUT_H_

VOID AllocTimeouts(PC0C_IO_PORT pIoPort);
VOID FreeTimeouts(PC0C_IO_PORT pIoPort);

VOID SetIntervalTimeout(PC0C_IO_PORT pIoPort);

VOID SetCloseTimeout(PC0C_IO_PORT pIoPort);

VOID SetXoffCounterTimeout(
    PC0C_IO_PORT pIoPort,
    PIRP pIrp);

NTSTATUS SetIrpTimeout(
    PC0C_IO_PORT pIoPort,
    PIRP pIrp);

NTSTATUS FdoPortSetTimeouts(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

NTSTATUS FdoPortGetTimeouts(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

#endif /* _C0C_TIMEOUT_H_ */
