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
#include "timeout.h"
#include "delay.h"
#include "strutils.h"

/*
 * FILE_ID used by HALT_UNLESS to put it on BSOD
 */
#define FILE_ID 6

NTSTATUS InitCommonExt(
    PC0C_COMMON_EXTENSION pDevExt,
    IN PDEVICE_OBJECT pDevObj,
    int doType,
    PWCHAR pPortName)
{
  pDevExt->pDevObj = pDevObj;
  pDevExt->doType = doType;
  return CopyStrW(pDevExt->portName, sizeof(pDevExt->portName), pPortName);
}

VOID RemoveFdoPort(IN PC0C_FDOPORT_EXTENSION pDevExt)
{
  if (pDevExt->pIoPortLocal && pDevExt->pIoPortLocal->pDevExt) {
    pDevExt->pIoPortLocal->pDevExt = NULL;
    FreeTimeouts(pDevExt);
  }

  FreeWriteDelay(pDevExt);

  if (pDevExt->mappedSerialDevice)
    RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, C0C_SERIAL_DEVICEMAP,
                           pDevExt->ntDeviceName.Buffer);

  if (pDevExt->createdSymbolicLink)
    IoDeleteSymbolicLink(&pDevExt->win32DeviceName);

  if (pDevExt->pLowDevObj)
	  IoDetachDevice(pDevExt->pLowDevObj);

  StrFree(&pDevExt->win32DeviceName);
  StrFree(&pDevExt->ntDeviceName);

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"RemoveFdoPort");

  IoDeleteDevice(pDevExt->pDevObj);
}

NTSTATUS AddFdoPort(IN PDRIVER_OBJECT pDrvObj, IN PDEVICE_OBJECT pPhDevObj)
{
  NTSTATUS status;
  UNICODE_STRING portName;
  PDEVICE_OBJECT pNewDevObj;
  PC0C_FDOPORT_EXTENSION pDevExt = NULL;
  ULONG emuBR, emuOverrun;
  WCHAR propertyBuffer[255];
  PWCHAR pPortName;
  ULONG len;
  int i;

  status = IoGetDeviceProperty(
      pPhDevObj,
      DevicePropertyPhysicalDeviceObjectName,
	    sizeof propertyBuffer,
      propertyBuffer,
      &len);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoGetDeviceProperty FAIL");
    goto clean;
  }

  Trace00((PC0C_COMMON_EXTENSION)pPhDevObj->DeviceExtension, L"AddFdoPort for ", propertyBuffer);

  for (pPortName = NULL, i = 0 ; propertyBuffer[i] ; i++)
    if (propertyBuffer[i] == L'\\')
      pPortName = &propertyBuffer[i + 1];

  if (!pPortName || !*pPortName) {
    status = STATUS_UNSUCCESSFUL;
    SysLog(pPhDevObj, status, L"AddFdoPort no port name in the property");
    goto clean;
  }

  RtlInitUnicodeString(&portName, NULL);

  {
    UNICODE_STRING portRegistryPath;

    RtlInitUnicodeString(&portRegistryPath, NULL);
    StrAppendStr(&status, &portRegistryPath, c0cGlobal.registryPath.Buffer, c0cGlobal.registryPath.Length);
    StrAppendStr0(&status, &portRegistryPath, L"\\Parameters\\");
    StrAppendStr0(&status, &portRegistryPath, pPortName);

    if (NT_SUCCESS(status)) {
      WCHAR portNameBuf[C0C_PORT_NAME_LEN];
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
        Trace00((PC0C_COMMON_EXTENSION)pPhDevObj->DeviceExtension, L"PortName set to ", portName.Buffer);
      }
    }

    emuBR = emuOverrun = 0;

    if (NT_SUCCESS(status)) {
      RTL_QUERY_REGISTRY_TABLE queryTable[3];
      ULONG zero = 0;

      RtlZeroMemory(queryTable, sizeof(queryTable));

      queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
      queryTable[0].Name          = L"EmuBR";
      queryTable[0].EntryContext  = &emuBR;
      queryTable[0].DefaultType   = REG_DWORD;
      queryTable[0].DefaultData   = &zero;
      queryTable[0].DefaultLength = sizeof(ULONG);

      queryTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
      queryTable[1].Name          = L"EmuOverrun";
      queryTable[1].EntryContext  = &emuOverrun;
      queryTable[1].DefaultType   = REG_DWORD;
      queryTable[1].DefaultData   = &zero;
      queryTable[1].DefaultLength = sizeof(ULONG);

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
                          0,
                          TRUE,
                          &pNewDevObj);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoCreateDevice FAIL");
    goto clean;
  }

  HALT_UNLESS(pNewDevObj);
  pDevExt = pNewDevObj->DeviceExtension;
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_FP, portName.Buffer);

  RtlInitUnicodeString(&pDevExt->ntDeviceName, NULL);
  StrAppendStr0(&status, &pDevExt->ntDeviceName, propertyBuffer);

  RtlInitUnicodeString(&pDevExt->win32DeviceName, NULL);
  StrAppendStr0(&status, &pDevExt->win32DeviceName, C0C_PREF_WIN32_DEVICE_NAME);
  StrAppendStr0(&status, &pDevExt->win32DeviceName, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort FAIL");
    goto clean;
  }

  pDevExt->pIoLock = ((PC0C_PDOPORT_EXTENSION)pPhDevObj->DeviceExtension)->pIoLock;
  pDevExt->pIoPortLocal = ((PC0C_PDOPORT_EXTENSION)pPhDevObj->DeviceExtension)->pIoPortLocal;
  pDevExt->pIoPortRemote = ((PC0C_PDOPORT_EXTENSION)pPhDevObj->DeviceExtension)->pIoPortRemote;

  if (emuBR) {
    if (NT_SUCCESS(AllocWriteDelay(pDevExt)))
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

  AllocTimeouts(pDevExt);

  KeInitializeSpinLock(&pDevExt->controlLock);
  pDevExt->specialChars.XonChar      = 0x11;
  pDevExt->specialChars.XoffChar     = 0x13;
  pDevExt->handFlow.ControlHandShake = SERIAL_DTR_CONTROL;
  pDevExt->handFlow.FlowReplace      = SERIAL_RTS_CONTROL;
  pDevExt->lineControl.WordLength    = 7;
  pDevExt->lineControl.Parity        = EVEN_PARITY;
  pDevExt->lineControl.StopBits      = STOP_BIT_1;
  pDevExt->baudRate.BaudRate         = 1200;

  SetWriteDelay(pDevExt);

  status = IoCreateSymbolicLink(&pDevExt->win32DeviceName, &pDevExt->ntDeviceName);

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort IoCreateSymbolicLink FAIL");
    goto clean;
  }

  pDevExt->createdSymbolicLink = TRUE;

  status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, C0C_SERIAL_DEVICEMAP,
                                 pDevExt->ntDeviceName.Buffer, REG_SZ,
                                 portName.Buffer,
                                 portName.Length + sizeof(WCHAR));

  if (!NT_SUCCESS(status)) {
    SysLog(pPhDevObj, status, L"AddFdoPort RtlWriteRegistryValue FAIL");
    goto clean;
  }

  pDevExt->mappedSerialDevice = TRUE;

  pDevExt->pLowDevObj = IoAttachDeviceToDeviceStack(pNewDevObj, pPhDevObj);

  if (!pDevExt->pLowDevObj) {
    status = STATUS_NO_SUCH_DEVICE;
    SysLog(pPhDevObj, status, L"AddFdoPort IoAttachDeviceToDeviceStack FAIL");
    goto clean;
  }

  pNewDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
  pNewDevObj->Flags |= DO_BUFFERED_IO;

  pDevExt->pIoPortLocal->pDevExt = pDevExt;

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"AddFdoPort OK");

clean:

  if (!NT_SUCCESS(status) && pDevExt)
    RemoveFdoPort(pDevExt);

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
    IN PC0C_IO_PORT pIoPortRemote,
    OUT PC0C_PDOPORT_EXTENSION *ppDevExt)
{
  NTSTATUS status;
  UNICODE_STRING portName;
  PDEVICE_OBJECT pNewDevObj;
  UNICODE_STRING ntDeviceName;
  PC0C_PDOPORT_EXTENSION pDevExt = NULL;
  int i;

  status = STATUS_SUCCESS;

  RtlInitUnicodeString(&portName, NULL);
  StrAppendStr0(&status, &portName, isA ? C0C_PREF_PORT_NAME_A : C0C_PREF_PORT_NAME_B);
  StrAppendNum(&status, &portName, num, 10);

  RtlInitUnicodeString(&ntDeviceName, NULL);
  StrAppendStr0(&status, &ntDeviceName, C0C_PREF_NT_DEVICE_NAME);
  StrAppendStr(&status, &ntDeviceName, portName.Buffer, portName.Length);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort FAIL");
    goto clean;
  }

  status = IoCreateDevice(pDrvObj,
                          sizeof(*pDevExt),
                          &ntDeviceName,
                          FILE_DEVICE_BUS_EXTENDER,
                          FILE_DEVICE_SECURE_OPEN,
                          TRUE,
                          &pNewDevObj);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort IoCreateDevice FAIL");
    goto clean;
  }

  HALT_UNLESS(pNewDevObj);
  pDevExt = (pNewDevObj)->DeviceExtension;
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_PP, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pBusExt->pDevObj, status, L"AddPdoPort FAIL");
    goto clean;
  }

  pDevExt->pBusExt = pBusExt;
  pDevExt->pIoLock = &pBusExt->ioLock;
  pDevExt->pIoPortLocal = pIoPortLocal;
  pDevExt->pIoPortRemote = pIoPortRemote;

  for (i = 0 ; i < C0C_QUEUE_SIZE ; i++) {
    InitializeListHead(&pIoPortLocal->irpQueues[i].queue);
    pIoPortLocal->irpQueues[i].pCurrent = NULL;
  }

  Trace0((PC0C_COMMON_EXTENSION)pDevExt, L"AddPdoPort OK");

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

ULONG AllocPortNum(IN PDRIVER_OBJECT pDrvObj)
{
  static ULONG numNext = 0;

  PDEVICE_OBJECT pDevObj;
  ULONG num;
  PCHAR pBusyMask;
  SIZE_T busyMaskLen;

  if (!numNext)
    return numNext++;

  busyMaskLen = numNext;
  pBusyMask = ExAllocatePool(PagedPool, busyMaskLen);

  if (!pBusyMask)
    return numNext++;

  RtlZeroMemory(pBusyMask, busyMaskLen);

  for (pDevObj = pDrvObj->DeviceObject ; pDevObj ; pDevObj = pDevObj->NextDevice) {
    if (((PC0C_COMMON_EXTENSION)pDevObj->DeviceExtension)->doType == C0C_DOTYPE_FB) {
      ULONG num = ((PC0C_FDOBUS_EXTENSION)pDevObj->DeviceExtension)->portNum;

      HALT_UNLESS3(num < busyMaskLen, num, busyMaskLen, numNext);
      pBusyMask[num] = 1;
    }
  }

  for (num = 0 ; num < busyMaskLen ; num++) {
    if (!pBusyMask[num])
      break;
  }

  ExFreePool(pBusyMask);

  if (num >= busyMaskLen)
    return numNext++;

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

  num = AllocPortNum(pDrvObj);

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
  RtlZeroMemory(pDevExt, sizeof(*pDevExt));
  status = InitCommonExt((PC0C_COMMON_EXTENSION)pDevExt, pNewDevObj, C0C_DOTYPE_FB, portName.Buffer);

  if (!NT_SUCCESS(status)) {
    SysLog(pDrvObj, status, L"AddFdoBus FAIL");
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
    BOOLEAN isA = (BOOLEAN)(i ? FALSE : TRUE);

    status = AddPdoPort(pDrvObj,
                        num,
                        isA,
                        pDevExt,
                        &pDevExt->childs[i].ioPort,
                        &pDevExt->childs[(i + 1) % 2].ioPort,
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
  WCHAR propertyBuffer[255];
  ULONG len;

  status = IoGetDeviceProperty(pPhDevObj, DevicePropertyHardwareID,
	  sizeof propertyBuffer, propertyBuffer, &len);

  if (NT_SUCCESS(status))
	  Trace00(NULL, L"c0cAddDevice for ", propertyBuffer);
  else {
    SysLog(pDrvObj, status, L"c0cAddDevice IoGetDeviceProperty FAIL");
    return status;
  }

  if (!_wcsicmp(C0C_PORT_DEVICE_ID, propertyBuffer))
    status = AddFdoPort(pDrvObj, pPhDevObj);
  else
  if (!_wcsicmp(C0C_BUS_DEVICE_ID, propertyBuffer))
    status = AddFdoBus(pDrvObj, pPhDevObj);
  else {
    status = STATUS_UNSUCCESSFUL;
    SysLog(pDrvObj, status, L"c0cAddDevice unknown property");
  }

  return status;
}
