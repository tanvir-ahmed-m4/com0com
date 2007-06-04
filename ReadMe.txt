                 =============================
                 Null-modem emulator (com0com)
                 =============================

INTRODUCTION
============

The Null-modem emulator is an open source kernel-mode virtual serial
port driver for Windows, available freely under GPL license.
You can create an unlimited number of virtual COM port
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

If you'd like to build a binary from source yourself then:

  1. Set up the DDK environment on your machine.
  2. Run the build -wcZ command in the com0com directory to build
     <CPU>\com0com.sys and <CPU>\setup.dll (where <CPU> is i386, ia64
     or amd64).
  3. Create the installer (setup.exe) from NSIS\install.nsi by
     NSIS (see http://nsis.sourceforge.net/ for details)


INSTALLING
==========

Simply run the installer (setup.exe). An installation wizard will guide
you through the required steps.
If the Found New Hardware Wizard will pop up then
  - select "No, not this time" and click Next;
  - select "Install the software automatically (Recommended)" and click Next.
The one COM port pair with names CNCA0 and CNCB0 will be available on your
system after the installation.

You can add more pairs with the Setup Command Prompt:

  1. Launch the Setup Command Prompt shortcut.
  2. Enter the install command, for example:

       command> install - -

The system will create 3 new virtual devices. One of the devices has
name "com0com - bus for serial port pair emulator" and other two of
them have name "com0com - serial port emulator" and located on CNCAn
and CNCBn ports.

To get more info enter the help command, for example:

       command> help


TESTING
=======

  1. Start the HyperTerminal on CNCA0 port.
  2. Start the HyperTerminal on CNCB0 port.
  3. The output to CNCA0 port should be the input from CNCB0 port and
     vice versa.


UNINSTALLING
============

Simply launch the com0com's Uninstall shortcut in the Start Menu or remove
the "Null-modem emulator (com0com)" entry from the "Add/Remove Programs"
section in the Control Panel. An uninstallation wizard will guide
you through the required steps.

HINT: To uninstall the old version of com0com (distributed w/o installer)
install the new one and then uninstall it.


FAQ & HOWTO
===========

Q. Is it possible to change the names CNCA0 and CNCB0 to COM2 and COM3?
A. Yes, it's possible. To change the names:

  1. Launch the Setup Command Prompt shortcut.
  2. Enter the change commands, for example:

       command> change CNCA0 PortName=COM2
       command> change CNCB0 PortName=COM3

Q. The baud rate setting does not seem to make a difference: data is always
   transferred at the same speed. How to enable the baud rate emulation?
A. To enable baud rate emulation for transferring data from CNCA0 to CNCB0:

  1. Launch the Setup Command Prompt shortcut.
  2. Enter the change command, for example:

       command> change CNCA0 EmuBR=yes

Q. The HyperTerminal test succeeds, but I get a failure when trying to open the
   port with CreateFile("CNCA0", ...). GetLastError() returns ERROR_FILE_NOT_FOUND.
A. You must prefix the port name with the special characters "\\.\". Try to open
   the port with CreateFile("\\\\.\\CNCA0", ...).

Q. My application hangs during its startup when it sends anything to one paired
   COM port. The only way to unhang it is to start HyperTerminal, which is connected
   to the other paired COM port. I didn't have this problem with physical serial
   ports.
A. Your application can hang because receive buffer overrun is disabled by
   default. You can fix the problem by enabling receive buffer overrun for the
   receiving port. Also, to prevent some flow control issues you need to enable
   baud rate emulation for the sending port. So, if your application use port CNCA0
   and other paired port is CNCB0, then:

  1. Launch the Setup Command Prompt shortcut.
  2. Enter the change commands, for example:

       command> change CNCB0 EmuOverrun=yes
       command> change CNCA0 EmuBR=yes

Q. I have to write an application connected to one side of the com0com port pair,
   and I don't want users to 'see' all the virtual ports created by com0com, but
   only the really available ones.
A. if your application use port CNCB0 and other (used by users) paired port is CNCA0,
   then CNCB0 can be 'hidden' and CNCA0 can be 'shown' on opening CNCB0 by your
   application. To enable it:

  1. Launch the Setup Command Prompt shortcut.
  2. Enter the change commands:

       command> change CNCB0 ExclusiveMode=yes
       command> change CNCA0 PlugInMode=yes
