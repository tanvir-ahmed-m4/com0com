                 =============================
                 Null-modem emulator (com0com)
                 =============================

INTRODUCTION

The null-modem emulator is a kernel-mode serial driver for
Windows. You can create an unlimited number of virtual COM port
pairs and use any pair to connect one application to another.
Each COM port pair provides two COM ports with names starting
at CNCA0 and CNCB0. The output to one port is the input from other
port and vice versa.

Usually one port of the pair is used by Windows application that
requires a COM port to communicate with a device and other port is
used by device emulation program.

For example, to send/receive faxes over IP you can connect Windows Fax
application to CNCA0 port and t38modem (part of the OpenH323 project)
to CNCB0 port.

BUILDING

  1. Set up the DDK environment on your machine.
  2. Run the build -wcZ command in the com0com directory to build
     com0com.sys.
  3. Copy com0com.inf and com0com.sys files to a temporary directory.

INSTALLING

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

  1. Start the HyperTerminal on CNCA0 port.
  2. Start the HyperTerminal on CNCB0 port.
  3. The output to CNCA0 port should be the input from CNCB0 port and
     vice versa.

FAQ

Q. Is it possible to change the names CNCA0 and CNCB0 to COM2 and COM3?
A. Yes, it's possible. Add the following to the registry:

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCA0]
"PortName"="COM2"
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\com0com\Parameters\CNCB0]
"PortName"="COM3"
