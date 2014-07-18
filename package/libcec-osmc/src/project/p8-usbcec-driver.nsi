;p8-usbcec-driver installer
;Copyright (C) 2011-2013 Pulse-Eight Ltd.
;http://www.pulse-eight.com/

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

Name "Pulse-Eight USB-CEC Adapter"
OutFile "..\build\p8-usbcec-driver-installer.exe"

XPStyle on
InstallDir "$PROGRAMFILES\Pulse-Eight\USB-CEC Adapter\driver"
InstallDirRegKey HKLM "Software\Pulse-Eight\USB-CEC Adapter driver" ""
RequestExecutionLevel admin

!define MUI_FINISHPAGE_LINK "Visit http://www.pulse-eight.com/ for more information."
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.pulse-eight.com/"
!define MUI_ABORTWARNING  

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Pulse-Eight\USB-CEC Adapter driver" 

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

InstType "USB-CEC driver only"

Section "USB-CEC driver" SecDriver
  SetShellVarContext current
  SectionIn RO
  SectionIn 1

  ; Copy to the installation directory
  SetOutPath "$INSTDIR"
  File "..\AUTHORS"
  File "..\COPYING"

  ; Copy the driver installer and .inf file
  File "..\build\dpinst-amd64.exe"
  File "..\build\dpinst-x86.exe"
  File "..\driver\p8usb-cec.inf"
  File "..\driver\p8usb-cec.cat"

  ;Store installation folder
  WriteRegStr HKLM "Software\Pulse-Eight\USB-CEC Adapter driver" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
    
  ;add entry to add/remove programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "DisplayName" "Pulse-Eight USB-CEC Adapter driver"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "NoRepair" 1
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "Publisher" "Pulse-Eight Limited"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "HelpLink" "http://www.pulse-eight.com/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver" \
                 "URLInfoAbout" "http://www.pulse-eight.com"

  ;install driver
  ${If} ${RunningX64}
	ExecWait '"$INSTDIR\dpinst-amd64.exe" /lm /sa /sw /PATH "$INSTDIR"'
  ${Else}
	ExecWait '"$INSTDIR\dpinst-x86.exe" /lm /sa /sw /PATH "$INSTDIR"'
  ${EndIf}
SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  SetShellVarContext current

  ${If} ${RunningX64}
	ExecWait '"$INSTDIR\dpinst-amd64.exe" /u "$INSTDIR\p8usb-cec.inf"'
  ${Else}
	ExecWait '"$INSTDIR\dpinst-x64.exe" /u "$INSTDIR\p8usb-cec.inf"'
  ${EndIf}
  Delete "$INSTDIR\AUTHORS"
  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\p8usb-cec.inf"
  Delete "$INSTDIR\p8usb-cec.cat"
  Delete "$INSTDIR\dpinst-amd64.exe"
  Delete "$INSTDIR\dpinst-x86.exe"

  RMDir /r "$INSTDIR\include"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"
  RMDir "$PROGRAMFILES\Pulse-Eight\USB-CEC Adapter"
  RMDir "$PROGRAMFILES\Pulse-Eight"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pulse-Eight USB-CEC Adapter driver"
  DeleteRegKey /ifempty HKLM "Software\Pulse-Eight\USB-CEC Adapter driver"
  DeleteRegKey /ifempty HKLM "Software\Pulse-Eight"

SectionEnd
