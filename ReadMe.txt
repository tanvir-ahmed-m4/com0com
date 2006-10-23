                 =============================
                 Null-modem emulator (com0com)
                 =============================

INTRODUCTION
============

The null-modem emulator is a kernel-mode virtual serial port driver for
Windows. You can create an unlimited number of virtual COM port
pairs and use any pair to connect one application to another.
Each COM port pair provides two COM ports with default names starting
at CNCA0 and CNCB0. The output to one port is the input from other
port and vice versa.

Usually one port of the pair is used by Windows application that
requires a COM port to communicate with a device and other port is
used by device emulation program.

For example, to send/receive faxes over IP you can connect Windows Fax
application to CNCA0 port and t38modem (part of the OpenH323 project)
to CNCB0 port. In this case the t38modem is a fax modem emulation program.

The homepage for com0com project is http://com0com.sourceforge.net/.


BUILDING
========

  1. Set up the DDK environment on your machine.
  2. Run the build -wcZ command in the com0com directory to build
     com0com.sys and setup.dll.
  3. Copy com0com.inf, %CPU%\com0com.sys, %CPU%\setup.dll and setup\setup.bat
     files to a temporary directory.


INSTALLING
==========

With setup.dll
--------------

  1. Run the setup.bat in the directory with com0com.inf, com0com.sys and
     setup.dll files.
  2. Enter install command, for example:

       command> install - -

The system will create 3 new virtual devices. One of the devices has
name "com0com - bus for serial port pair emulator" and other two of
them have name "com0com - serial port emulator" and located on CNCA0
and CNCB0 ports (or CNCA1 and CNCB1 or ...).

To get more info enter help command, for example:

       command> help


Manually
--------

  1. Start the "Add/Remove Hardware" wizard in Control Panel.
  2. Click "Add/Troubleshoot a Device".
  3. Select "Add a new device" and then click Next.
  4. Select "No, I Want to Select the Hardware from a list".
  5. For the first time (if the driver is not installed yet):
       1. Select "Other Devices" and then click Next.
       2. Click "Have Disk".
       3. Enter path to the directory with com0com.inf and com0com.sys
          files and then click OK.
     For the next time (adding one more port pair) select
     "com0com - serial port emulators" and then click Next.
  6. Select "com0com - bus for serial port pair emulator" and then
     click Next.

The system will create 3 new virtual devices. One of the devices has
name "com0com - bus for serial port pair emulator" and other two of
them have name "com0com - serial port emulator" and located on CNCA0
and CNCB0 ports (or CNCA1 and CNCB1 or ...).


TESTING
=======

  1. Start the HyperTerminal on CNCA0 port.
  2. Start the HyperTerminal on CNCB0 port.
  3. The output to CNCA0 port should be the input from CNCB0 port and
     vice versa.


UNINSTALLING
============

With setup.dll
--------------

  1. Run the setup.bat in the directory with com0com.inf, com0com.sys and
     setup.dll files.
  2. Enter uninstall command:

       command> uninstall


Manually
--------

Start Device Manager this way:

  set DEVMGR_SHOW_NONPRESENT_DEVICES=1
  %SystemRoot%\system32\devmgmt.msc

Click View and select "Show hidden devices". Remove all "com0com" devices.

Remove the following subtrees from the registry:

  [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{DF799E12-3C56-421B-B298-B6D3642BC878}]
  [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com]
  [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Eventlog\System\com0com]

Delete file %SystemRoot%\system32\drivers\com0com.sys

Find in the %SystemRoot%\inf\ directory the oem{N}.inf file(s) that corresponds
to the com0com.inf file (the system renames com0com.inf to oem{N}.inf, where {N}
is a number). Delete all found oem{N}.inf and corresponding oem{N}.PNF files.


FAQ
===

Q. Is it possible to change the names CNCA0 and CNCB0 to COM2 and COM3?
A. Yes, it's possible. Add the following to the registry:

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCA0]
"PortName"="COM2"
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCB0]
"PortName"="COM3"

Q. The baud rate setting does not seem to make a difference: data is always
   transferred at the same speed. How to enable the baud rate emulation?
A. To enable baud rate emulation for transferring data from CNCA0 to CNCB0 add
   the following to the registry:

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCA0]
"EmuBR"=dword:FFFFFFFF

Q. The HyperTerminal test succeeds, but I get a failure when trying to open the
   port with CreateFile("CNCA0", ...). GetLastError() returns ERROR_FILE_NOT_FOUND.
A. You must prefix name with the special characters "\\.\". Try to open the port
   with CreateFile("\\\\.\\CNCA0", ...).

Q. My application hangs during its startup when it sends anything to one paired
   COM port. The only way to unhang it is to start HyperTerminal, which is connected
   to the other paired COM port. I didn't have this problem with physical serial
   ports.
A. Your application can hang because receive buffer overrun is disabled by
   default. You can fix the problem by enabling receive buffer overrun for the
   receiving port. Also, to prevent some flow control issues you need to enable
   baud rate emulation for the sending port. So, if your application use port CNCA0
   and other paired port is CNCB0, then add the following to the registry:

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCB0]
"EmuOverrun"=dword:FFFFFFFF
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCA0]
"EmuBR"=dword:FFFFFFFF
