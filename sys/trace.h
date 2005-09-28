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
 * Revision 1.6  2005/09/28 10:06:42  vfrolov
 * Implemented IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION
 *
 * Revision 1.5  2005/09/13 08:55:41  vfrolov
 * Disabled modem status tracing by default
 *
 * Revision 1.4  2005/09/06 06:49:39  vfrolov
 * Added codeNameTableErrors[]
 *
 * Revision 1.3  2005/08/25 07:48:39  vfrolov
 * Changed type of code names from wchar to char
 * Fixed HandFlow tracing
 *
 * Revision 1.2  2005/07/14 12:18:59  vfrolov
 * Defined HALT_UNLESS
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _TRACE_H_
#define _TRACE_H_

#if DBG

#define HALT_UNLESS3(exp, p1, p2, p3) \
  if (!(exp)) \
    KeBugCheckEx(0xDEADC0CD, (FILE_ID << 16) + __LINE__, p1, p2, p3)

#define HALT_UNLESS2(exp, p1, p2) HALT_UNLESS3(exp, p1, p2, 0)
#define HALT_UNLESS1(exp, p1)     HALT_UNLESS3(exp, p1, 0, 0)
#define HALT_UNLESS(exp)          HALT_UNLESS3(exp, 0, 0, 0)

#define TRACE_FLAG_PARAMS         0x0001
#define TRACE_FLAG_RESULTS        0x0002

typedef struct _CODE2NAME {
  ULONG code;
  PCHAR name;
} CODE2NAME, *PCODE2NAME;

typedef struct _FIELD2NAME {
  ULONG code;
  ULONG mask;
  PCHAR name;
} FIELD2NAME, *PFIELD2NAME;

VOID TraceOpen(
    IN PDRIVER_OBJECT _pDrvObj,
    IN PUNICODE_STRING pRegistryPath);

VOID TraceClose();

VOID Trace0(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr);

VOID Trace00(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr1,
    IN PWCHAR pStr2);

VOID TraceCode(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG code,
    IN PNTSTATUS pStatus);

VOID TraceMask(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG mask);

VOID TraceModemStatus(IN PC0C_IO_PORT pIoPort);

VOID TraceIrp(
    IN PCHAR pHead,
    IN PIRP pIrp,
    IN PNTSTATUS pStatus,
    IN ULONG flags);

CODE2NAME codeNameTableWaitMask[];
CODE2NAME codeNameTablePurgeMask[];
CODE2NAME codeNameTableIoctl[];
CODE2NAME codeNameTablePnp[];
CODE2NAME codeNameTablePower[];
CODE2NAME codeNameTableDoType[];
CODE2NAME codeNameTableModemStatus[];
CODE2NAME codeNameTableStatus[];
CODE2NAME codeNameTableIrpMj[];
CODE2NAME codeNameTableRelations[];
CODE2NAME codeNameTableBusQuery[];
CODE2NAME codeNameTableDeviceText[];
CODE2NAME codeNameTableErrors[];
CODE2NAME codeNameTableFileInformationClass[];

FIELD2NAME codeNameTableControlHandShake[];
FIELD2NAME codeNameTableFlowReplace[];

#else /* DBG */

#define HALT_UNLESS3(exp, p1, p2, p3)
#define HALT_UNLESS2(exp, p1, p2)
#define HALT_UNLESS1(exp, p1)
#define HALT_UNLESS(exp)
#define TraceOpen(a1, a2)
#define TraceClose()
#define Trace0(a1, a2)
#define Trace00(a1, a2, a3)
#define TraceCode(a1, a2, a3, a4, a5)
#define TraceMask(a1, a2, a3, a4)
#define TraceModemStatus(a1)
#define TraceIrp(a1, a2, a3, a4)

#endif /* DBG */

#endif /* _TRACE_H_ */
