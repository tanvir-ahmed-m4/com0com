/*
 * $Id$
 *
 * Copyright (c) 2004-2010 Vyacheslav Frolov
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
 * Revision 1.5  2010/05/27 11:06:23  vfrolov
 * Added StrAppendPortParametersRegistryPath() and StrAppendParameterPortName()
 *
 * Revision 1.4  2006/11/03 13:13:26  vfrolov
 * CopyStrW() now gets size in characters (not in bytes)
 *
 * Revision 1.3  2006/03/27 09:37:28  vfrolov
 * Added StrAppendDeviceProperty()
 *
 * Revision 1.2  2005/05/17 15:07:36  vfrolov
 * Fixed parameter type typo
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#ifndef _C0C_STRUTILS_H_
#define _C0C_STRUTILS_H_

NTSTATUS CopyStrW(OUT PWCHAR pDestStr, IN LONG size, IN PWCHAR pStr);

NTSTATUS DupStrW(OUT PWCHAR *ppDestStr, IN PWCHAR pStr, IN BOOLEAN multiStr);
VOID StrFree(IN OUT PUNICODE_STRING  pDest);
BOOLEAN StrFreeBad(NTSTATUS status, IN OUT PUNICODE_STRING  pDest);
VOID StrAppendStr(
    PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING  pDest,
    IN PWCHAR pSrc,
    IN USHORT lenSrc);
VOID StrAppendStr0(PNTSTATUS pStatus, IN OUT PUNICODE_STRING  pDest, IN PWCHAR pSrc);
VOID StrAppendNum(
    PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN ULONG num,
    IN ULONG base);
VOID StrAppendDeviceProperty(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PDEVICE_OBJECT pDevObj,
    IN DEVICE_REGISTRY_PROPERTY deviceProperty);
VOID StrAppendPortParametersRegistryPath(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PWCHAR pPhPortName);
VOID StrAppendParameterPortName(
    IN OUT PNTSTATUS pStatus,
    IN OUT PUNICODE_STRING pDest,
    IN PWCHAR pPortParametersRegistryPath);

#endif /* _C0C_STRUTILS_H_ */
