/*
 * $Id$
 *
 * Copyright (c) 2004-2009 Vyacheslav Frolov
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
 * Revision 1.14  2009/05/22 14:25:39  vfrolov
 * Optimized for trace disabled mode
 *
 * Revision 1.13  2008/12/02 16:10:09  vfrolov
 * Separated tracing and debuging
 *
 * Revision 1.12  2008/05/04 09:51:45  vfrolov
 * Implemented HiddenMode option
 *
 * Revision 1.11  2007/02/21 16:52:34  vfrolov
 * Added tracing of IRP_MJ_POWER with more details
 *
 * Revision 1.10  2006/08/23 12:56:20  vfrolov
 * Added codeNameTableWmi
 *
 * Revision 1.9  2006/05/19 15:02:03  vfrolov
 * Implemented IOCTL_SERIAL_GET_MODEM_CONTROL
 *
 * Revision 1.8  2006/01/10 09:32:20  vfrolov
 * Added codeNameTableHoldReasons[]
 *
 * Revision 1.7  2005/12/06 13:01:54  vfrolov
 * Implemented IOCTL_SERIAL_GET_DTRRTS
 *
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
 */

#ifndef _TRACE_H_
#define _TRACE_H_

#if ENABLE_TRACING

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

extern struct _TRACE_DATA *pTraceData;
#define TRACE_FILE_OK (pTraceData != NULL)

VOID TraceOpen(
    IN PDRIVER_OBJECT _pDrvObj,
    IN PUNICODE_STRING pRegistryPath);

VOID TraceClose();

VOID InternalTrace0(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr);

VOID InternalTrace00(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PWCHAR pStr1,
    IN PWCHAR pStr2);

VOID InternalTraceCode(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG code,
    IN PNTSTATUS pStatus);

VOID InternalTraceMask(
    IN PC0C_COMMON_EXTENSION pDevExt,
    IN PCHAR pHead,
    IN PCODE2NAME pTable,
    IN ULONG mask);

VOID InternalTraceModemStatus(IN PC0C_IO_PORT pIoPort);

VOID InternalTraceIrp(
    IN PCHAR pHead,
    IN PIRP pIrp,
    IN PNTSTATUS pStatus,
    IN ULONG flags);

#define Trace0(a1, a2)                 if (TRACE_FILE_OK) InternalTrace0(a1, a2); else
#define Trace00(a1, a2, a3)            if (TRACE_FILE_OK) InternalTrace00(a1, a2, a3); else
#define TraceCode(a1, a2, a3, a4, a5)  if (TRACE_FILE_OK) InternalTraceCode(a1, a2, a3, a4, a5); else
#define TraceMask(a1, a2, a3, a4)      if (TRACE_FILE_OK) InternalTraceMask(a1, a2, a3, a4); else
#define TraceModemStatus(a1)           if (TRACE_FILE_OK) InternalTraceModemStatus(a1); else
#define TraceIrp(a1, a2, a3, a4)       if (TRACE_FILE_OK) InternalTraceIrp(a1, a2, a3, a4); else

CODE2NAME codeNameTableWaitMask[];
CODE2NAME codeNameTablePurgeMask[];
CODE2NAME codeNameTableIoctl[];
CODE2NAME codeNameTablePnp[];
CODE2NAME codeNameTablePower[];
CODE2NAME codeNameTablePowerType[];
CODE2NAME codeNameTableSystemPowerState[];
CODE2NAME codeNameTableDevicePowerState[];
CODE2NAME codeNameTablePowerAction[];
CODE2NAME codeNameTableWmi[];
CODE2NAME codeNameTableDoType[];
CODE2NAME codeNameTableModemStatus[];
CODE2NAME codeNameTableModemControl[];
CODE2NAME codeNameTableStatus[];
CODE2NAME codeNameTableIrpMj[];
CODE2NAME codeNameTableRelations[];
CODE2NAME codeNameTableBusQuery[];
CODE2NAME codeNameTableDeviceText[];
CODE2NAME codeNameTableErrors[];
CODE2NAME codeNameTableHoldReasons[];
CODE2NAME codeNameTableFileInformationClass[];
CODE2NAME codeNameTableShowPort[];

FIELD2NAME codeNameTableControlHandShake[];
FIELD2NAME codeNameTableFlowReplace[];

#else /* ENABLE_TRACING */

#define TraceOpen(a1, a2)
#define TraceClose()
#define Trace0(a1, a2)
#define Trace00(a1, a2, a3)
#define TraceCode(a1, a2, a3, a4, a5)
#define TraceMask(a1, a2, a3, a4)
#define TraceModemStatus(a1)
#define TraceIrp(a1, a2, a3, a4)

#endif /* ENABLE_TRACING */

#endif /* _TRACE_H_ */
