/*
 * $Id$
 *
 * Copyright (c) 2006 Vyacheslav Frolov
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
 * Revision 1.1  2006/10/23 12:26:02  vfrolov
 * Initial revision
 *
 *
 */

;--------------------------------

; The name of the installer
Name "Null-modem emulator (com0com)"

; The file to write
OutFile "setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\com0com

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\com0com" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Install com0com"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put files there
  File "..\ReadMe.txt"
  File "..\com0com.inf"
  File "..\i386\com0com.sys"
  File "..\i386\setup.dll"
  File "..\setup\setup.bat"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\com0com "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "DisplayName" "Null-modem emulator (com0com)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "URLInfoAbout" "http://com0com.sourceforge.net/"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\com0com"
  CreateShortCut "$SMPROGRAMS\com0com\setup.lnk" "$INSTDIR\setup.bat"
  CreateShortCut "$SMPROGRAMS\com0com\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ExecWait "RunDll32 setup,RunDll uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com"
  DeleteRegKey HKLM SOFTWARE\com0com

  ; Remove files and uninstaller
  Delete $INSTDIR\ReadMe.txt
  Delete $INSTDIR\com0com.inf
  Delete $INSTDIR\com0com.sys
  Delete $INSTDIR\setup.dll
  Delete $INSTDIR\setup.bat
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\com0com\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\com0com"
  RMDir "$INSTDIR"

SectionEnd
