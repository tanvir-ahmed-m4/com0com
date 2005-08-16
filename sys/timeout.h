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

VOID InitializeTimeoutDpc(IN PC0C_FDOPORT_EXTENSION pDevExt);

NTSTATUS FdoPortSetIrpTimeout(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
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
