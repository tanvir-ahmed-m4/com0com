/*
 * $Id$
 *
 * Copyright (c) 2004-2006 Vyacheslav Frolov
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
 * Revision 1.24  2006/02/26 08:35:55  vfrolov
 * Added check for start/stop queue matching
 *
 * Revision 1.23  2006/02/17 07:55:13  vfrolov
 * Implemented IOCTL_SERIAL_SET_BREAK_ON and IOCTL_SERIAL_SET_BREAK_OFF
 *
 * Revision 1.22  2006/01/10 10:17:23  vfrolov
 * Implemented flow control and handshaking
 * Implemented IOCTL_SERIAL_SET_XON and IOCTL_SERIAL_SET_XOFF
 * Added setting of HoldReasons, WaitForImmediate and AmountInOutQueue
 *   fields of SERIAL_STATUS for IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.21  2005/12/06 13:04:32  vfrolov
 * Fixed data types
 *
 * Revision 1.20  2005/12/05 10:54:55  vfrolov
 * Implemented IOCTL_SERIAL_IMMEDIATE_CHAR
 *
 * Revision 1.19  2005/11/30 16:04:11  vfrolov
 * Implemented IOCTL_SERIAL_GET_STATS and IOCTL_SERIAL_CLEAR_STATS
 *
 * Revision 1.18  2005/11/29 16:16:46  vfrolov
 * Removed FdoPortCancelQueue()
 *
 * Revision 1.17  2005/11/29 12:33:21  vfrolov
 * Changed SetModemStatus() to ability set and clear bits simultaneously
 *
 * Revision 1.16  2005/11/29 08:35:14  vfrolov
 * Implemented SERIAL_EV_RX80FULL
 *
 * Revision 1.15  2005/11/28 12:57:16  vfrolov
 * Moved some C0C_BUFFER code to bufutils.c
 *
 * Revision 1.14  2005/09/28 10:06:42  vfrolov
 * Implemented IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION
 *
 * Revision 1.13  2005/09/13 14:56:16  vfrolov
 * Implemented IRP_MJ_FLUSH_BUFFERS
 *
 * Revision 1.12  2005/09/06 07:23:44  vfrolov
 * Implemented overrun emulation
 *
 * Revision 1.11  2005/08/23 15:49:21  vfrolov
 * Implemented baudrate emulation
 *
 * Revision 1.10  2005/07/13 16:12:36  vfrolov
 * Added c0cGlobal struct for global driver's data
 *
 * Revision 1.9  2005/06/28 12:17:12  vfrolov
 * Added pBusExt to C0C_PDOPORT_EXTENSION
 *
 * Revision 1.8  2005/05/20 12:06:05  vfrolov
 * Improved port numbering
 *
 * Revision 1.7  2005/05/19 08:23:40  vfrolov
 * Fixed data types
 *
 * Revision 1.6  2005/05/14 17:07:02  vfrolov
 * Implemented SERIAL_LSRMST_MST insertion
 *
 * Revision 1.5  2005/05/13 16:58:03  vfrolov
 * Implemented IOCTL_SERIAL_LSRMST_INSERT
 *
 * Revision 1.4  2005/05/12 07:41:27  vfrolov
 * Added ability to change the port names
 *
 * Revision 1.3  2005/02/01 16:47:57  vfrolov
 * Implemented SERIAL_PURGE_RXCLEAR and IOCTL_SERIAL_GET_COMMSTATUS
 *
 * Revision 1.2  2005/02/01 08:37:55  vfrolov
 * Changed SetModemStatus() to set multiple bits
 *
 * Revision 1.1  2005/01/26 12:18:54  vfrolov
 * Initial revision
 *
 */

#ifndef _C0C_COM0COM_H_
#define _C0C_COM0COM_H_

#define C0C_SERIAL_DEVICEMAP        L"SERIALCOMM"
#define C0C_PREF_WIN32_DEVICE_NAME  L"\\DosDevices\\"
#define C0C_PREF_NT_DEVICE_NAME     L"\\Device\\"

#define C0C_BUS_DEVICE_ID           L"root\\com0com"
#define C0C_PORT_DEVICE_ID          L"com0com\\port"
#define C0C_PORT_HARDWARE_IDS       L"com0com\\port\0"
#define C0C_PORT_COMPATIBLE_IDS     L"\0"

#define C0C_PREF_BUS_NAME           L"CNCBUS"
#define C0C_PREF_PORT_NAME_A        L"CNCA"
#define C0C_PREF_PORT_NAME_B        L"CNCB"

#define C0C_DOTYPE_FB     1
#define C0C_DOTYPE_PP	    2
#define C0C_DOTYPE_FP     3

#define C0C_PORT_NAME_LEN 12

#define COMMON_EXTENSION                \
  short                   doType;       \
  PDEVICE_OBJECT          pDevObj;      \
  WCHAR                   portName[C0C_PORT_NAME_LEN]; \

#define FDO_EXTENSION                   \
  COMMON_EXTENSION                      \
  PDEVICE_OBJECT          pLowDevObj;   \

#define C0C_XCHAR_ON      1
#define C0C_XCHAR_OFF     2

typedef struct _C0C_COMMON_EXTENSION {
  COMMON_EXTENSION
} C0C_COMMON_EXTENSION, *PC0C_COMMON_EXTENSION;

typedef struct _C0C_COMMON_FDO_EXTENSION {
  FDO_EXTENSION
} C0C_COMMON_FDO_EXTENSION, *PC0C_COMMON_FDO_EXTENSION;

typedef struct _C0C_IRP_QUEUE {
  PIRP                    pCurrent;
  LIST_ENTRY              queue;
#if DBG
  BOOLEAN                 started;
#endif /* DBG */
} C0C_IRP_QUEUE, *PC0C_IRP_QUEUE;

typedef struct _C0C_RAW_DATA {
  UCHAR                   size;
  UCHAR                   data[7];
} C0C_RAW_DATA, *PC0C_RAW_DATA;

typedef struct _C0C_BUFFER {
  PUCHAR                  pBase;
  PUCHAR                  pBusy;
  PUCHAR                  pFree;
  PUCHAR                  pEnd;
  SIZE_T                  limit;
  SIZE_T                  busy;
  SIZE_T                  size80;
  BOOLEAN                 escape;
  C0C_RAW_DATA            insertData;
} C0C_BUFFER, *PC0C_BUFFER;

#define C0C_BUFFER_BUSY(pBuf) \
  ((SIZE_T)((pBuf)->busy + (pBuf)->insertData.size))

#define C0C_BUFFER_SIZE(pBuf) \
  ((SIZE_T)((pBuf)->pEnd - (pBuf)->pBase))

struct _C0C_FDOPORT_EXTENSION;
struct _C0C_ADAPTIVE_DELAY;

typedef struct _C0C_IO_PORT {

  struct _C0C_FDOPORT_EXTENSION *pDevExt;

  #define C0C_QUEUE_READ  0
  #define C0C_QUEUE_WRITE 1
  #define C0C_QUEUE_WAIT  2

  #define C0C_QUEUE_SIZE  3

  C0C_IRP_QUEUE           irpQueues[C0C_QUEUE_SIZE];

  KTIMER                  timerReadTotal;
  KDPC                    timerReadTotalDpc;

  KTIMER                  timerReadInterval;
  KDPC                    timerReadIntervalDpc;
  LARGE_INTEGER           timeoutInterval;

  KTIMER                  timerWriteTotal;
  KDPC                    timerWriteTotalDpc;

  struct _C0C_ADAPTIVE_DELAY *pWriteDelay;

  ULONG                   errors;
  ULONG                   amountInWriteQueue;
  ULONG                   waitMask;
  ULONG                   eventMask;
  UCHAR                   escapeChar;
  SERIALPERF_STATS        perfStats;

  #define C0C_MSB_CTS     0x10
  #define C0C_MSB_DSR     0x20
  #define C0C_MSB_RING    0x40
  #define C0C_MSB_RLSD    0x80

  ULONG                   modemStatus;

  C0C_BUFFER              readBuf;

  short                   sendXonXoff;
  ULONG                   writeHolding;
  BOOLEAN                 sendBreak;
  BOOLEAN                 tryWrite;
  BOOLEAN                 flipXoffLimit;

  BOOLEAN                 emuOverrun;
} C0C_IO_PORT, *PC0C_IO_PORT;

typedef struct _C0C_PDOPORT_EXTENSION {
  COMMON_EXTENSION

  struct _C0C_FDOBUS_EXTENSION *pBusExt;

  PKSPIN_LOCK             pIoLock;
  PC0C_IO_PORT            pIoPortLocal;
  PC0C_IO_PORT            pIoPortRemote;
} C0C_PDOPORT_EXTENSION, *PC0C_PDOPORT_EXTENSION;

typedef struct _C0C_FDOPORT_EXTENSION {
  FDO_EXTENSION

  PKSPIN_LOCK             pIoLock;
  PC0C_IO_PORT            pIoPortLocal;
  PC0C_IO_PORT            pIoPortRemote;

  UNICODE_STRING          ntDeviceName;
  UNICODE_STRING          win32DeviceName;
  BOOLEAN                 createdSymbolicLink;
  BOOLEAN                 mappedSerialDevice;

  LONG                    openCount;

  KSPIN_LOCK              controlLock;

  SERIAL_BAUD_RATE        baudRate;
  SERIAL_LINE_CONTROL     lineControl;
  SERIAL_CHARS            specialChars;
  SERIAL_TIMEOUTS         timeouts;
  SERIAL_HANDFLOW         handFlow;

} C0C_FDOPORT_EXTENSION, *PC0C_FDOPORT_EXTENSION;

typedef struct _C0C_CHILD {
  PC0C_PDOPORT_EXTENSION  pDevExt;
  C0C_IO_PORT             ioPort;
} C0C_CHILD, *PC0C_CHILD;

typedef struct _C0C_FDOBUS_EXTENSION {
  FDO_EXTENSION

  KSPIN_LOCK              ioLock;
  C0C_CHILD               childs[2];
  ULONG                   portNum;
} C0C_FDOBUS_EXTENSION, *PC0C_FDOBUS_EXTENSION;

typedef struct _C0C_GLOBAL {
  PDRIVER_OBJECT pDrvObj;
  UNICODE_STRING registryPath;
} C0C_GLOBAL;

extern C0C_GLOBAL c0cGlobal;

VOID c0cUnload(IN PDRIVER_OBJECT pDrvObj);

#define DeclareMajorFunction(mfunc) \
  NTSTATUS mfunc(IN PDEVICE_OBJECT, IN PIRP)

DeclareMajorFunction(c0cOpen);
DeclareMajorFunction(c0cClose);
DeclareMajorFunction(c0cWrite);
DeclareMajorFunction(c0cRead);
DeclareMajorFunction(c0cIoControl);
DeclareMajorFunction(c0cInternalIoControl);
DeclareMajorFunction(c0cCleanup);
DeclareMajorFunction(c0cFileInformation);
DeclareMajorFunction(c0cSystemControlDispatch);

DeclareMajorFunction(c0cPnpDispatch);
DeclareMajorFunction(c0cPowerDispatch);

#undef DeclareMajorFunction

NTSTATUS c0cAddDevice(IN PDRIVER_OBJECT  pDrvObj, IN PDEVICE_OBJECT pPhDevObj);
VOID RemoveFdoPort(IN PC0C_FDOPORT_EXTENSION pDevExt);
VOID RemoveFdoBus(IN PC0C_FDOBUS_EXTENSION pDevExt);

typedef NTSTATUS (*PC0C_FDOPORT_START_ROUTINE)(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PLIST_ENTRY pQueueToComplete);

VOID ShiftQueue(PC0C_IRP_QUEUE pQueue);

NTSTATUS FdoPortStartIrp(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN UCHAR iQueue,
    IN PC0C_FDOPORT_START_ROUTINE pStartRoutine);

VOID CancelQueue(PC0C_IRP_QUEUE pQueue, PLIST_ENTRY pQueueToComplete);
VOID FdoPortCancelQueues(IN PC0C_FDOPORT_EXTENSION pDevExt);
VOID FdoPortCompleteQueue(IN PLIST_ENTRY pQueueToComplete);

NTSTATUS FdoPortImmediateChar(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

NTSTATUS FdoPortWaitOnMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

NTSTATUS FdoPortSetWaitMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

NTSTATUS FdoPortGetWaitMask(
    IN PC0C_FDOPORT_EXTENSION pDevExt,
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpStack);

VOID WaitComplete(
    IN PC0C_IO_PORT pIoPort,
    PLIST_ENTRY pQueueToComplete);

typedef struct _C0C_IRP_STATE {

#define C0C_IRP_FLAG_IN_QUEUE          0x01
#define C0C_IRP_FLAG_IS_CURRENT        0x02
#define C0C_IRP_FLAG_WAIT_ONE          0x04
#define C0C_IRP_FLAG_INTERVAL_TIMEOUT  0x08

  UCHAR                   flags;
  UCHAR                   iQueue;
} C0C_IRP_STATE, *PC0C_IRP_STATE;

PC0C_IRP_STATE GetIrpState(IN PIRP pIrp);
ULONG GetWriteLength(IN PIRP pIrp);

#define C0C_IO_TYPE_WAIT_COMPLETE      3
#define C0C_IO_TYPE_INSERT             4

NTSTATUS FdoPortIo(
    short ioType,
    PVOID pParam,
    PC0C_IO_PORT pIoPort,
    PC0C_IRP_QUEUE pQueue,
    PLIST_ENTRY pQueueToComplete);

NTSTATUS ReadWrite(
    PC0C_IO_PORT pIoPortRead,
    BOOLEAN startRead,
    PC0C_IO_PORT pIoPortWrite,
    BOOLEAN startWrite,
    PLIST_ENTRY pQueueToComplete);

VOID SetModemStatus(
    IN PC0C_IO_PORT pIoPort,
    IN ULONG bits,
    IN ULONG mask,
    PLIST_ENTRY pQueueToComplete);

#endif /* _C0C_COM0COM_H_ */
