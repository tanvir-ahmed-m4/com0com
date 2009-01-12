/*
 * $Id$
 *
 * Copyright (c) 2006-2009 Vyacheslav Frolov
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
 * Revision 1.14  2009/01/12 13:16:20  vfrolov
 * Added driver updating
 *
 * Revision 1.13  2008/09/12 12:29:53  vfrolov
 * Added --silent option
 *
 * Revision 1.12  2007/11/30 09:53:37  vfrolov
 * Added license page
 *
 * Revision 1.11  2007/11/23 08:23:29  vfrolov
 * Added popup for uncompatible CPU
 *
 * Revision 1.10  2007/11/22 11:36:41  vfrolov
 * Moved output file to target CPU directory
 * Disabled moving Start Menu shortcuts to all users for Vista
 * Fixed title truncation
 * Added show ReadMe checkbox
 * Added setupg.exe
 *
 * Revision 1.9  2007/11/15 12:12:04  vfrolov
 * Removed Function LaunchSetupCommandPrompt
 * Added MUI_FINISHPAGE_LINK
 *
 * Revision 1.8  2007/10/30 15:06:14  vfrolov
 * Added changing working directory before removing $INSTDIR
 *
 * Revision 1.7  2007/10/25 14:30:27  vfrolov
 * Replaced setup.bat by setupc.exe
 *
 * Revision 1.6  2007/08/08 14:15:16  vfrolov
 * Added missing SetOutPath
 *
 * Revision 1.5  2007/01/22 17:10:32  vfrolov
 * Partially added support for non i386 CPUs
 *
 * Revision 1.4  2006/12/14 08:25:44  vfrolov
 * Added ReadMe.lnk
 *
 * Revision 1.3  2006/11/22 07:58:45  vfrolov
 * Changed uninstall keys
 *
 * Revision 1.2  2006/11/21 11:43:42  vfrolov
 * Added Modern UI
 * Added "CNCA0<->CNCB0" section
 * Added "Launch Setup Command Prompt" on finish page
 *
 * Revision 1.1  2006/10/23 12:26:02  vfrolov
 * Initial revision
 *
 */

;--------------------------------

  !include "MUI.nsh"
  !include "x64.nsh"

;--------------------------------

!if "$%BUILD_DEFAULT_TARGETS%" == "-386"
  !define TARGET_CPU i386
!else if "$%BUILD_DEFAULT_TARGETS%" == "-IA64"
  !define TARGET_CPU ia64
!else if "$%BUILD_DEFAULT_TARGETS%" == "-ia64"
  !define TARGET_CPU ia64
!else if "$%BUILD_DEFAULT_TARGETS%" == "-AMD64"
  !define TARGET_CPU amd64
!else if "$%BUILD_DEFAULT_TARGETS%" == "-amd64"
  !define TARGET_CPU amd64
!endif

!ifndef TARGET_CPU
  !define TARGET_CPU i386
  !Warning "TARGET_CPU=${TARGET_CPU}"
!endif

;--------------------------------

Function .onInit
  ${If} ${RunningX64}
    !if "${TARGET_CPU}" == "i386"
      MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
        "The 32-bit driver cannot run under 64-bit System.$\n$\nContinue?" \
        /SD IDNO IDYES end_x64
      Abort
    !endif
  ${Else}
    !if "${TARGET_CPU}" != "i386"
      MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION \
        "The 64-bit driver cannot run under 32-bit System.$\n$\nContinue?" \
        /SD IDNO IDYES end_x64
      Abort
    !endif
  ${EndIf}

end_x64:

FunctionEnd

;--------------------------------

!macro MoveFileToDetails file

  Push $0
  Push $1
  Push $2
  Push $3

  StrCpy $0 "${file}"

  FileOpen $1 $0 r
  IfErrors +9

    FileRead $1 $2
    IfErrors +7

    StrCpy $3 $2 2 -2
    StrCmp $3 "$\r$\n" 0 +2
      StrCpy $2 $2 -2

    StrCmp $2 "" +2
      DetailPrint $2

    Goto -7

  FileClose $1
  Delete $0

  Pop $3
  Pop $2
  Pop $1
  Pop $0

!macroend

;--------------------------------

; The name of the installer
Name "Null-modem emulator (com0com)"

; The file to write
OutFile "..\${TARGET_CPU}\setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\com0com

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\com0com" "Install_Dir"

;Vista redirects $SMPROGRAMS to all users without this
RequestExecutionLevel admin

ShowInstDetails show
ShowUninstDetails show

;--------------------------------
; Pages

  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !define MUI_FINISHPAGE_TITLE_3LINES

  !define MUI_FINISHPAGE_RUN setupc.exe
  !define MUI_FINISHPAGE_RUN_TEXT "Launch Setup Command Prompt"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED

  !define MUI_FINISHPAGE_SHOWREADME ReadMe.txt
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

  !define MUI_FINISHPAGE_LINK "Visit com0com homepage"
  !define MUI_FINISHPAGE_LINK_LOCATION http://com0com.sourceforge.net/

  !define MUI_FINISHPAGE_NOAUTOCLOSE

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "..\license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !define MUI_FINISHPAGE_TITLE_3LINES
  !define MUI_UNFINISHPAGE_NOAUTOCLOSE

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

Section "com0com" sec_com0com

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put files there
  File "..\ReadMe.txt"
  File "..\com0com.inf"
  File "..\${TARGET_CPU}\com0com.sys"
  File "..\${TARGET_CPU}\setup.dll"
  File "..\${TARGET_CPU}\setupc.exe"
  File "..\setupg\Release\setupg.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\com0com "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "DisplayName" "Null-modem emulator (com0com)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "HelpLink" "http://com0com.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "URLUpdateInfo" "http://com0com.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "Readme" "$INSTDIR\ReadMe.txt"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com" "NoRepair" 1

  WriteUninstaller "uninstall.exe"

  GetTempFileName $0

  StrCpy $1 ""
  IfSilent 0 +2
  StrCpy $1 "--silent"

  ExecWait "setupc.exe $1 --output $0 update"
  !insertmacro MoveFileToDetails $0

SectionEnd

;--------------------------------

Section "Start Menu Shortcuts" sec_shortcuts

  CreateDirectory "$SMPROGRAMS\com0com"
  CreateShortCut "$SMPROGRAMS\com0com\Setup Command Prompt.lnk" "$INSTDIR\setupc.exe"
  CreateShortCut "$SMPROGRAMS\com0com\Setup.lnk" "$INSTDIR\setupg.exe"
  CreateShortCut "$SMPROGRAMS\com0com\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\com0com\ReadMe.lnk" "$INSTDIR\ReadMe.txt"

SectionEnd

;--------------------------------

Section "CNCA0<->CNCB0" sec_ports

  GetTempFileName $0

  StrCpy $1 ""
  IfSilent 0 +2
  StrCpy $1 "--silent"

  ExecWait "setupc.exe $1 --output $0 install 0 - -"
  !insertmacro MoveFileToDetails $0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  GetTempFileName $0

  StrCpy $1 ""
  IfSilent 0 +2
  StrCpy $1 "--silent"

  ExecWait "setupc.exe $1 --output $0 uninstall"
  !insertmacro MoveFileToDetails $0

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com"
  DeleteRegKey HKLM SOFTWARE\com0com

  ; Remove files and uninstaller
  Delete $INSTDIR\ReadMe.txt
  Delete $INSTDIR\com0com.inf
  Delete $INSTDIR\com0com.sys
  Delete $INSTDIR\setup.dll
  Delete $INSTDIR\setupc.exe
  Delete $INSTDIR\setupg.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\com0com\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\com0com"
  SetOutPath $TEMP
  RMDir "$INSTDIR"

SectionEnd

;--------------------------------

  ;Language strings
  LangString DESC_sec_com0com ${LANG_ENGLISH} "Install com0com files."
  LangString DESC_sec_shortcuts ${LANG_ENGLISH} "Add shortcuts to the Start Menu."
  LangString DESC_sec_ports ${LANG_ENGLISH} "Install a pair of linked ports with identifiers CNCA0 and CNCB0."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_com0com} $(DESC_sec_com0com)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_shortcuts} $(DESC_sec_shortcuts)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_ports} $(DESC_sec_ports)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
