/*
 * $Id$
 *
 * Copyright (c) 2004-2008 Vyacheslav Frolov
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
 * Revision 1.32  2008/03/14 15:28:39  vfrolov
 * Implemented ability to get paired port settings with
 * extended IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.31  2007/10/19 16:03:41  vfrolov
 * Added default values
 *
 * Revision 1.30  2007/09/26 10:12:13  vfrolov
 * Added checks of DeviceExtension for zero
 *
 * Revision 1.29  2007/07/20 08:00:22  vfrolov
 * Implemented TX buffer
 *
 * Revision 1.28  2007/07/03 14:35:17  vfrolov
 * Implemented pinout customization
 *
 * Revision 1.27  2007/06/05 12:15:08  vfrolov
 * Fixed memory leak
 *
 * Revision 1.26  2007/06/01 16:22:40  vfrolov
 * Implemented plug-in and exclusive modes
 *
 * Revision 1.25  2007/06/01 08:36:26  vfrolov
 * Changed parameter type for SetWriteDelay()
 *
 * Revision 1.24  2007/01/11 14:50:28  vfrolov
 * Pool functions replaced by
 *   C0C_ALLOCATE_POOL()
 *   C0C_ALLOCATE_POOL_WITH_QUOTA()
 *   C0C_FREE_POOL()
 *
 * Revision 1.23  2006/11/23 11:10:10  vfrolov
 * Strict usage fixed port numbers
 *
 * Revision 1.22  2006/11/03 13:13:26  vfrolov
 * CopyStrW() now gets size in characters (not in bytes)
 *
 * Revision 1.21  2006/11/02 16:04:50  vfrolov
 * Added using fixed port numbers
 *
 * Revision 1.20  2006/10/16 08:30:45  vfrolov
 * Added the device interface registration
 *
 * Revision 1.19  2006/10/13 10:22:22  vfrolov
 * Changed name of device object (for WMI)
 *
 * Revision 1.18  2006/10/10 15:18:15  vfrolov
 * Added PortName value setting for WMI
 *
 * Revision 1.17  2006/08/23 13:48:12  vfrolov
 * Implemented WMI functionality
 *
 * Revision 1.16  2006/06/23 11:44:52  vfrolov
 * Mass replacement pDevExt by pIoPort
 *
 * Revision 1.15  2006/06/21 16:23:57  vfrolov
 * Fixed possible BSOD after one port of pair removal
 *
 * Revision 1.14  2006/03/29 09:39:28  vfrolov
 * Fixed possible usage uninitialized portName
 *
 * Revision 1.13  2006/03/27 09:38:23  vfrolov
 * Utilized StrAppendDeviceProperty()
 *
 * Revision 1.12  2006/02/26 08:35:55  vfrolov
 * Added check for start/stop queue matching
 *
 * Revision 1.11  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.10  2005/09/27 16:41:01  vfrolov
 * Fixed DeviceType
 *
 * Revision 1.9  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.8  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.7  2005/08/16 16:36:33  vfrolov
 * Hidden timeout functions
 *
 * Revision 1.6  2005/07/14 13:51:08  vfrolov
 * Replaced ASSERT by HALT_UNLESS
 *
 * Revision 1.5  2005/07/13 16:12:36  vfrolov
 * Added c0cGlobal struct for global driver's data
 *
 * Revision 1.4  2005/06/28 12:17:12  vfrolov
 * Added pBusExt to C0C_PDOPORT_EXTENSION
 *
 * Revision 1.3  2005/05/20 12:06:05  vfrolov
 * Improved port numbering
 *
 * Revision 1.2  2005/05/12 07:41:27  vfrolov
 * Added ability to change the port names
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#include "precomp.h"
#include <initguid.h>
#include <ntddser.h>
#include "timeout.h"
#include "delay.h"
#include "bufutils.h"
#include "strutils.h"
#include "showport.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 6

NTSTATUS InitCommonExt(
    PC0C_COMMON_EXTENSION pDevExt,
    IN PDEVICE_OBJECT pDevObj,
    short doType,
    PWCHAR pPortName)
{
  pDevExt->pDevObj = pDevObj;
  pDevExt->doType = doType;
  return CopyStrW(pDevExt->portName, sizeof(pDevExt->portName)/sizeof(pDevExt->portName[0]), pPortName);
}

VOID RemoveFdoPort(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  if (pDevExt->pIoPortLocal) {
    FreeTimeouts(pDevExt->pIoPortLocal);
    FreeWriteDelay(pDevExt->pIoPortLocal);
    pDevExt->pIoPortLocal->plugInMode = FALSE;
    pDevExt->pIoPortLocal->exclusiveMode = FALSE;
    pDevExt->pIoPortLocal->pDevExt = NULL;
    FreeTxBuffer(&pDevExt->pIoPortLocal->txBuf);
  }

  if (!HidePort(pDevExt))
    SysLog(pDevExt->pDevObj, STATUS_UNSUCCESSFUL, L"RemoveFdoPort HidePort FAIL");

  if (pDevExt->symbolicLinkName.Buffer)
    RtlFreeUnicodeString(&pDevExt->symbolicLinkName);

  StrFree(&pDevExt->ntDeviceName);
  StrFree(&pDevExt->win32DeviceName);

  if (pDevExt->pLowDevObj)
    IoDetachDevice(pDevExt->pLowDevObj);

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"RemoveFdoPort");

  IoDeleteDevice(pDevExt->pDevObj);
}

NTSTATUS AddFdoPort(IN PDRIVER_OBJECT pDrvObj, IN PDEVICE_OBJECT pPhDevObj)
{
  NTSTATUS status;
  UNICODE_STRING portName;
  PDEVICE_OBJECT pNewDevObj;
  PC0C_FDOPORT_EXTENSION pDevExt;
  PC0C_PDOPORT_EXTENSION pPhDevExt;
  ULONG emuBR, emuOverrun, plugInMode, exclusiveMode;
  ULONG pinCTS, pinDSR, pinDCD, pinRI;
  UNICODE_STRING ntDeviceName;
  PWCHAR pPortName;

  status = STATUS_SUCCESS;
  pDevExt = NULL;
  RtlInitUnicodeString(&portName, NULL);
  RtlInitUnicodeString(&ntDeviceName, NULL);

  StrAppendDeviceProperty(&status, &ntDeviceName, pPhDevObj, DevicePropertyPhysicalDeviceObjectName);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoGetDeviceProperty FAIL");
    goto clean;
  }

  pPhDevExt = (PC0C_PDOPORT_EXTENSION)pPhDevObj->DeviceExtension;

  if (!pPhDevExt || pPhDevExt->doType != C0C_DOTYPE_PP) {
    status = STATUS_UNSUCCESSFUL;
    SysLog(pPhDevObj, status, L"AddFdoPort FAIL. Type  of PDO is not PP");
    goto clean;
  }

  Trace00((PC0C_COMMON_EXTENSION)pPhDevExt, L"AddFdoPort for ", ntDeviceName.Buffer);

  pPortName = pPhDevExt->portName;

  if (!*pPortName) {
    status = STATUS_UNSUCCESSFUL;
    SysLog(pPhDevObj, status, L"AddFdoPort FAIL. The PDO has invalid port name");
    goto clean;
  }

  {
    UNICODE_STRING portRegistryPath;

    RtlInitUnicodeString(&portRegistryPath, NULL);
    StrAppendStr(&status, &portRegistryPath, c0cGlobal.registryPath.Buffer, c0cGlobal.registryPath.Length);
    StrAppendStr0(&status, &portRegistryPath, L"\\Parameters\\");
    StrAppendStr0(&status, &portRegistryPath, pPortName);

    if (NT_SUCCESS(status)) {
      WCHAR portNameBuf[C0C_PORT_NAME_LEN + 1];
      UNICODE_STRING portNameTmp;
      RTL_QUERY_REGISTRY_TABLE queryTable[2];

      RtlZeroMemory(queryTable, sizeof(queryTable));

      portNameTmp.Length = 0;
      portNameTmp.MaximumLength = sizeof(portNameBuf);
      portNameTmp.Buffer = portNameBuf;

      queryTable[0].Flags        = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
      queryTable[0].Name         = L"PortName";
      queryTable[0].EntryContext = &portNameTmp;

      status = RtlQueryRegistryValues(
          RTL_REGISTRY_ABSOLUTE,
          portRegistryPath.Buffer,
          queryTable,
          NULL,
          NULL);

      if (!NT_SUCCESS(status) || !portNameTmp.Length) {
        status = STATUS_SUCCESS;
        StrAppendStr0(&status, &portName, pPortName);
      } else {
        StrAppendStr(&status, &portName, portNameTmp.Buffer, portNameTmp.Length);
        Trace00((PC0C_COMMON_EXTENSION)pPhDevExt, L"PortName set to ", portName.Buffer);
      }
    }

    emuBR = C0C_DEFAULT_EMUBR;
    emuOverrun = C0C_DEFAULT_EMUOVERRUN;
    plugInMode = C0C_DEFAULT_PLUGINMODE;
    exclusiveMode = C0C_DEFAULT_EXCLUSIVEMODE;
    pinCTS = pinDSR = pinDCD = pinRI = 0;

    if (NT_SUCCESS(status)) {
      RTL_QUERY_REGISTRY_TABLE queryTable[9];
      ULONG zero = 0;
      int i;

      RtlZeroMemory(queryTable, sizeof(queryTable));

      for (i = 0 ; i < (sizeof(queryTable)/sizeof(queryTable[0]) - 1) ; i++) {
        queryTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[i].DefaultType   = REG_DWORD;
        queryTable[i].DefaultData   = &zero;
        queryTable[i].DefaultLength = sizeof(ULONG);
      }

      i = 0;
      queryTable[i].Name          = L"EmuBR";
      queryTable[i].EntryContext  = &emuBR;
      queryTable[i].DefaultData   = &emuBR;

      i++;
      queryTable[i].Name          = L"EmuOverrun";
      queryTable[i].EntryContext  = &emuOverrun;
      queryTable[i].DefaultData   = &emuOverrun;

      i++;
      queryTable[i].Name          = L"PlugInMode";
      queryTable[i].EntryContext  = &plugInMode;
      queryTable[i].DefaultData   = &plugInMode;

      i++;
      queryTable[i].Name          = L"ExclusiveMode";
      queryTable[i].EntryContext  = &exclusiveMode;
      queryTable[i].DefaultData   = &exclusiveMode;

      i++;
      queryTable[i].Name          = L"cts";
      queryTable[i].EntryContext  = &pinCTS;

      i++;
      queryTable[i].Name          = L"dsr";
      queryTable[i].EntryContext  = &pinDSR;

      i++;
      queryTable[i].Name          = L"dcd";
      queryTable[i].EntryContext  = &pinDCD;

      i++;
      queryTable[i].Name          = L"ri";
      queryTable[i].EntryContext  = &pinRI;

      RtlQueryRegistryValues(
          RTL_REGISTRY_ABSOLUTE,
          portRegistryPath.Buffer,
          queryTable,
          NULL,
          NULL);
    }

    StrFree(&portRegistryPath);
  }

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort FAIL");
    goto clean;
  }

  status = IoCreateDevice(pDrvObj,
                          sizeof(*pDevExt),
                          NULL,
                          FILE_DEVICE_SERIAL_PORT,
                          FILE_DEVICE_SECURE_OPEN,
                          TRUE,
                          &pNewDevObj);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoCreateDevice FAIL");
    goto clean;
  }

  HALT_UNLESS(pNewDevObj);
  pDevExt = pNewDevObj->DeviceExtension;
  HALT_UNLESS(pDevExt);
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  pDevExt->pIoPortLocal = pPhDevExt->pIoPortLocal;
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_FP, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort FAIL");
    goto clean;
  }

  pDevExt->pIoPortLocal->pDevExt = pDevExt;

  if (emuBR) {
    if (NT_SUCCESS(AllocWriteDelay(pDevExt->pIoPortLocal)))
      Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Enabled baudrate emulation");
    else
      SysLog(pPhDevObj, status, L"AddFdoPort AllocWriteDelay FAIL");
  } else {
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Disabled baudrate emulation");
  }

  if (emuOverrun) {
    pDevExt->pIoPortLocal->emuOverrun = TRUE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Enabled overrun emulation");
  } else {
    pDevExt->pIoPortLocal->emuOverrun = FALSE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Disabled overrun emulation");
  }

  if (plugInMode) {
    pDevExt->pIoPortLocal->plugInMode = TRUE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Enabled plug-in mode");
  } else {
    pDevExt->pIoPortLocal->plugInMode = FALSE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Disabled plug-in mode");
  }

  if (exclusiveMode) {
    pDevExt->pIoPortLocal->exclusiveMode = TRUE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Enabled exclusive mode");
  } else {
    pDevExt->pIoPortLocal->exclusiveMode = FALSE;
    Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"Disabled exclusive mode");
  }

  AllocTimeouts(pDevExt->pIoPortLocal);

  RtlZeroMemory(&pDevExt->pIoPortLocal->specialChars, sizeof(pDevExt->pIoPortLocal->specialChars));
  pDevExt->pIoPortLocal->specialChars.XonChar      = 0x11;
  pDevExt->pIoPortLocal->specialChars.XoffChar     = 0x13;

  RtlZeroMemory(&pDevExt->pIoPortLocal->handFlow, sizeof(pDevExt->pIoPortLocal->handFlow));
  pDevExt->pIoPortLocal->handFlow.ControlHandShake = SERIAL_DTR_CONTROL;
  pDevExt->pIoPortLocal->handFlow.FlowReplace      = SERIAL_RTS_CONTROL;

  pDevExt->pIoPortLocal->lineControl.WordLength    = 7;
  pDevExt->pIoPortLocal->lineControl.Parity        = EVEN_PARITY;
  pDevExt->pIoPortLocal->lineControl.StopBits      = STOP_BIT_1;
  pDevExt->pIoPortLocal->baudRate.BaudRate         = 1200;

  SetWriteDelay(pDevExt->pIoPortLocal);

  SetTxBuffer(&pDevExt->pIoPortLocal->txBuf, 1, TRUE);

  pDevExt->pIoPortLocal->modemControl |= C0C_MCR_OUT2;

  PinMap(pDevExt->pIoPortLocal, pinCTS, pinDSR, pinDCD, pinRI);

  pDevExt->pLowDevObj = IoAttachDeviceToDeviceStack(pNewDevObj, pPhDevObj);

  if (!pDevExt->pLowDevObj) {
    status = STATUS_NO_SUCH_DEVICE;
    SysLog(pPhDevObj, status, L"AddFdoPort IoAttachDeviceToDeviceStack FAIL");
    goto clean;
  }

  pNewDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
  pNewDevObj->Flags |= DO_BUFFERED_IO;

  /* Create symbolic links to device */

  RtlInitUnicodeString(&pDevExt->ntDeviceName, NULL);
  StrAppendStr0(&status, &pDevExt->ntDeviceName, ntDeviceName.Buffer);

  RtlInitUnicodeString(&pDevExt->win32DeviceName, NULL);
  StrAppendStr0(&status, &pDevExt->win32DeviceName, C0C_PREF_WIN32_DEVICE_NAME);
  StrAppendStr0(&status, &pDevExt->win32DeviceName, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    StrFree(&pDevExt->win32DeviceName);
    StrFree(&pDevExt->ntDeviceName);

    SysLog(pPhDevObj, status, L"AddFdoPort StrAppendStr0 FAIL");
  }

  status = IoRegisterDeviceInterface(pPhDevObj,
                                     (LPGUID)&GUID_CLASS_COMPORT,
                                     NULL,
                                     &pDevExt->symbolicLinkName);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoRegisterDeviceInterface FAIL");
    pDevExt->symbolicLinkName.Buffer = NULL;
  }

  if (!pDevExt->pIoPortLocal->plugInMode || pDevExt->pIoPortLocal->pIoPortRemote->isOpen) {
    if (!ShowPort(pDevExt))
      SysLog(pDevExt->pDevObj, STATUS_UNSUCCESSFUL, L"AddFdoPort ShowPort FAIL");
  } else {
    HidePortName(pDevExt);
  }

  status = STATUS_SUCCESS;

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"AddFdoPort OK");

clean:

  if (!NT_SUCCESS(status) && pDevExt)
    RemoveFdoPort(pDevExt);

  StrFree(&ntDeviceName);
  StrFree(&portName);

  return status;
}

VOID RemovePdoPort(IN PC0C_PDOPORT_EXTENSION pDevExt)
{
  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"RemovePdoPort");

  IoDeleteDevice(pDevExt->pDevObj);
}

NTSTATUS AddPdoPort(
    IN PDRIVER_OBJECT pDrvObj,
    IN ULONG num,
    IN BOOLEAN isA,
    IN PC0C_FDOBUS_EXTENSION pBusExt,
    IN PC0C_IO_PORT pIoPortLocal,
    OUT PC0C_PDOPORT_EXTENSION *ppDevExt)
{
  NTSTATUS status;
  UNICODE_STRING portName;
  PDEVICE_OBJECT pNewDevObj;
  UNICODE_STRING ntDeviceName;
  PC0C_PDOPORT_EXTENSION pDevExt = NULL;

  status = STATUS_SUCCESS;

  RtlInitUnicodeString(&portName, NULL);
  StrAppendStr0(&status, &portName, isA ? C0C_PREF_PORT_NAME_A : C0C_PREF_PORT_NAME_B);
  StrAppendNum(&status, &portName, num, 10);

  RtlInitUnicodeString(&ntDeviceName, NULL);
  StrAppendStr0(&status, &ntDeviceName, isA ? C0C_PREF_DEVICE_NAME_A : C0C_PREF_DEVICE_NAME_B);
  StrAppendNum(&status, &ntDeviceName, num, 10);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort FAIL");
    goto clean;
  }

  status = IoCreateDevice(pDrvObj,
                          sizeof(*pDevExt),
                          &ntDeviceName,
                          FILE_DEVICE_SERIAL_PORT,
                          FILE_DEVICE_SECURE_OPEN,
                          TRUE,
                          &pNewDevObj);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort IoCreateDevice FAIL");
    goto clean;
  }

  HALT_UNLESS(pNewDevObj);
  pIoPortLocal->pPhDevObj = pNewDevObj;
  pDevExt = (pNewDevObj)->DeviceExtension;
  HALT_UNLESS(pDevExt);
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_PP, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort FAIL");
    goto clean;
  }

  pDevExt->pBusExt = pBusExt;
  pDevExt->pIoPortLocal = pIoPortLocal;

  Trace00((PC0C_COMMON_EXTENSION)pDevExt, L"AddPdoPort OK - ", ntDeviceName.Buffer);

clean:

  if (!NT_SUCCESS(status) && pDevExt) {
    RemovePdoPort(pDevExt);
    pDevExt = NULL;
  }

  StrFree(&ntDeviceName);
  StrFree(&portName);

  *ppDevExt = pDevExt;

  return status;
}

VOID RemoveFdoBus(IN PC0C_FDOBUS_EXTENSION pDevExt)
{
  int i;

  for (i = 0 ; i < 2 ; i++) {
    if (pDevExt->childs[i].pDevExt)
      RemovePdoPort(pDevExt->childs[i].pDevExt);
  }

  if (pDevExt->pLowDevObj)
    IoDetachDevice(pDevExt->pLowDevObj);

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"RemoveFdoBus");

  IoDeleteDevice(pDevExt->pDevObj);
}

ULONG AllocPortNum(IN PDRIVER_OBJECT pDrvObj, ULONG num)
{
  PDEVICE_OBJECT pDevObj;
  ULONG numNext;
  PCHAR pBusyMask;
  SIZE_T busyMaskLen;
  ULONG maskNum;
  ULONG mask;

  numNext = 0;

  for (pDevObj = pDrvObj->DeviceObject ; pDevObj ; pDevObj = pDevObj->NextDevice) {
    PC0C_FDOBUS_EXTENSION pDevExt = (PC0C_FDOBUS_EXTENSION)pDevObj->DeviceExtension;

    if (pDevExt && pDevExt->doType == C0C_DOTYPE_FB) {
      ULONG portNum = pDevExt->portNum;

      if (portNum >= numNext)
        numNext = portNum + 1;
    }
  }

  if (num == (ULONG)-1)
    num = 0;

  if (num >= numNext)
    return num;

  busyMaskLen = (numNext + (sizeof(*pBusyMask)*8 - 1))/(sizeof(*pBusyMask)*8);

  pBusyMask = C0C_ALLOCATE_POOL(PagedPool, busyMaskLen);

  if (!pBusyMask) {
    SysLog(pDrvObj, STATUS_INSUFFICIENT_RESOURCES, L"AllocPortNum C0C_ALLOCATE_POOL FAIL");
    return numNext;
  }

  RtlZeroMemory(pBusyMask, busyMaskLen);

  for (pDevObj = pDrvObj->DeviceObject ; pDevObj ; pDevObj = pDevObj->NextDevice) {
    PC0C_FDOBUS_EXTENSION pDevExt = (PC0C_FDOBUS_EXTENSION)pDevObj->DeviceExtension;

    if (pDevExt && pDevExt->doType == C0C_DOTYPE_FB) {
      ULONG portNum = pDevExt->portNum;

      maskNum = portNum/(sizeof(*pBusyMask)*8);
      mask = 1 << (portNum%(sizeof(*pBusyMask)*8));

      HALT_UNLESS3(maskNum < busyMaskLen, portNum, busyMaskLen, numNext);
      pBusyMask[maskNum] |= mask;
    }
  }

  maskNum = num/(sizeof(*pBusyMask)*8);
  mask = 1 << (num%(sizeof(*pBusyMask)*8));

  if ((pBusyMask[maskNum] & mask) != 0) {
    for (num = 0 ; num < numNext ; num++) {
      maskNum = num/(sizeof(*pBusyMask)*8);
      mask = 1 << (num%(sizeof(*pBusyMask)*8));

      HALT_UNLESS3(maskNum < busyMaskLen, num, busyMaskLen, numNext);
      if ((pBusyMask[maskNum] & mask) == 0)
        break;
    }
  }

  C0C_FREE_POOL(pBusyMask);

  return num;
}

ULONG GetPortNum(IN PDRIVER_OBJECT pDrvObj, IN PDEVICE_OBJECT pPhDevObj)
{
  ULONG num;
  ULONG numPref;
  HANDLE hKey;
  NTSTATUS status;

  numPref = (ULONG)-1;

  status = IoOpenDeviceRegistryKey(pPhDevObj,
                                   PLUGPLAY_REGKEY_DEVICE,
                                   STANDARD_RIGHTS_READ,
                                   &hKey);

  if (status == STATUS_SUCCESS) {
    UNICODE_STRING keyName;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;
    ULONG len;

    RtlInitUnicodeString(&keyName, C0C_REGSTR_VAL_PORT_NUM);

    len = sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(ULONG);

    pInfo = C0C_ALLOCATE_POOL(PagedPool, len);

    if (pInfo) {
      status = ZwQueryValueKey(hKey, &keyName, KeyValuePartialInformation, pInfo, len, &len);

      if (NT_SUCCESS(status) && pInfo->DataLength == sizeof(ULONG))
        numPref = *(PULONG)pInfo->Data;

      C0C_FREE_POOL(pInfo);
    }

    if (numPref == (ULONG)-1)
      num = AllocPortNum(pDrvObj, numPref);
    else
      num = numPref;

    if (num != numPref) {
      status = ZwSetValueKey(hKey, &keyName, 0, REG_DWORD, &num, sizeof(num));

      if (!NT_SUCCESS(status))
        SysLog(pPhDevObj, status, L"ZwSetValueKey(PortName) FAIL");
    }

    ZwClose(hKey);
  } else {
    SysLog(pPhDevObj, status, L"GetPortNum IoOpenDeviceRegistryKey(PLUGPLAY_REGKEY_DEVICE) FAIL");

    num = AllocPortNum(pDrvObj, numPref);
  }

  return num;
}

NTSTATUS AddFdoBus(IN PDRIVER_OBJECT pDrvObj, IN PDEVICE_OBJECT pPhDevObj)
{
  NTSTATUS status = STATUS_SUCCESS;
  UNICODE_STRING portName;
  UNICODE_STRING ntDeviceName;
  PDEVICE_OBJECT pNewDevObj;
  PC0C_FDOBUS_EXTENSION pDevExt = NULL;
  ULONG num;
  int i;

  num = GetPortNum(pDrvObj, pPhDevObj);

  RtlInitUnicodeString(&portName, NULL);
  StrAppendStr0(&status, &portName, C0C_PREF_BUS_NAME);
  StrAppendNum(&status, &portName, num, 10);

  RtlInitUnicodeString(&ntDeviceName, NULL);
  StrAppendStr0(&status, &ntDeviceName, C0C_PREF_NT_DEVICE_NAME);
  StrAppendStr(&status, &ntDeviceName, portName.Buffer, portName.Length);

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"AddFdoBus FAIL");
    goto clean;
  }

  status = IoCreateDevice(pDrvObj,
                          sizeof(*pDevExt),
                          &ntDeviceName,
                          FILE_DEVICE_BUS_EXTENDER,
                          0,
                          TRUE,
                          &pNewDevObj);

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"AddFdoBus IoCreateDevice FAIL");
    goto clean;
  }

  HALT_UNLESS(pNewDevObj);
  pDevExt = pNewDevObj->DeviceExtension;
  HALT_UNLESS(pDevExt);
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_FB, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"AddFdoBus InitCommonExt FAIL");
    goto clean;
  }

  pDevExt->portNum = num;
  pDevExt->pLowDevObj = IoAttachDeviceToDeviceStack(pNewDevObj, pPhDevObj);

  if (!pDevExt->pLowDevObj) {
    status = STATUS_NO_SUCH_DEVICE;
    SysLog(pNewDevObj, status, L"AddFdoBus IoAttachDeviceToDeviceStack FAIL");
    goto clean;
  }

  pNewDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
  KeInitializeSpinLock(&pDevExt->ioLock);

  for (i = 0 ; i < 2 ; i++) {
    PC0C_IO_PORT pIoPort;
    int j;

    pIoPort = &pDevExt->childs[i].ioPort;

    pIoPort->pIoLock = &pDevExt->ioLock;

    for (j = 0 ; j < C0C_QUEUE_SIZE ; j++) {
      InitializeListHead(&pIoPort->irpQueues[j].queue);
      pIoPort->irpQueues[j].pCurrent = NULL;
#if DBG
      pIoPort->irpQueues[j].started = FALSE;
#endif /* DBG */
    }

    pIoPort->pIoPortRemote = &pDevExt->childs[(i + 1) % 2].ioPort;

    status = AddPdoPort(pDrvObj,
                        num,
                        (BOOLEAN)(i ? FALSE : TRUE),
                        pDevExt,
                        pIoPort,
                        &pDevExt->childs[i].pDevExt);

    if (!NT_SUCCESS(status)) {
      SysLog(pNewDevObj, status, L"AddFdoBus AddPdoPort FAIL");
      pDevExt->childs[i].pDevExt = NULL;
      goto clean;
    }
  }

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"AddFdoBus OK");

clean:

  if (!NT_SUCCESS(status) && pDevExt)
    RemoveFdoBus(pDevExt);

  StrFree(&ntDeviceName);
  StrFree(&portName);

  return status;
}

NTSTATUS c0cAddDevice(IN PDRIVER_OBJECT pDrvObj, IN PDEVICE_OBJECT pPhDevObj)
{
  NTSTATUS status;
  UNICODE_STRING property;

  status = STATUS_SUCCESS;
  RtlInitUnicodeString(&property, NULL);

  StrAppendDeviceProperty(&status, &property, pPhDevObj, DevicePropertyHardwareID);

  if (NT_SUCCESS(status))
    Trace00(NULL, L"c0cAddDevice for ", property.Buffer);
  else {
    SysLog(pDrvObj, status, L"c0cAddDevice IoGetDeviceProperty FAIL");
    return status;
  }

  if (!_wcsicmp(C0C_PORT_DEVICE_ID, property.Buffer)) {
    StrFree(&property);
    status = AddFdoPort(pDrvObj, pPhDevObj);
  }
  else
  if (!_wcsicmp(C0C_BUS_DEVICE_ID, property.Buffer)) {
    StrFree(&property);
    status = AddFdoBus(pDrvObj, pPhDevObj);
  }
  else {
    StrFree(&property);
    status = STATUS_UNSUCCESSFUL;
    SysLog(pDrvObj, status, L"c0cAddDevice unknown HardwareID");
  }

  return status;
}
