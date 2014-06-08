@echo off

rem Check for NSIS
IF EXIST "%ProgramFiles%\NSIS\makensis.exe" (
  set NSIS="%ProgramFiles%\NSIS\makensis.exe"
) ELSE IF EXIST "%ProgramFiles(x86)%\NSIS\makensis.exe" (
  set NSIS="%ProgramFiles(x86)%\NSIS\makensis.exe"
) ELSE GOTO NONSIS

rem Check for the Windows DDK
IF NOT EXIST "C:\WinDDK\7600.16385.1" GOTO NODDK
set DDK="C:\WinDDK\7600.16385.1"

mkdir ..\build

echo. Copying driver installer
copy "%DDK%\redist\DIFx\dpinst\MultiLin\amd64\dpinst.exe" ..\build\dpinst-amd64.exe
copy "%DDK%\redist\DIFx\dpinst\MultiLin\x86\dpinst.exe" ..\build\dpinst-x86.exe

:CREATECAT
cd ..\driver
IF EXIST "..\support\private\create-cat.cmd" (
  echo. Updating the catalogue
  CALL ..\support\private\create-cat.cmd p8usb-cec.cat
)

:CREATEINSTALLER
echo. Creating the installer
cd ..\project
%NSIS% /V1 /X"SetCompressor /FINAL lzma" "p8-usbcec-driver.nsi"

IF NOT EXIST "..\build\p8-usbcec-driver-installer.exe" GOTO :ERRORCREATINGINSTALLER

rem Sign the installer if sign-binary.cmd exists
IF EXIST "..\support\private\sign-binary.cmd" (
  echo. Signing the installer binaries
  CALL ..\support\private\sign-binary.cmd ..\build\p8-usbcec-driver-installer.exe
)

echo. The installer can be found here: ..\build\p8-usbcec-driver-installer.exe

GOTO EXIT

:NOSIS
echo. NSIS could not be found on your system.
GOTO EXIT

:NODDK
echo. Windows DDK could not be found on your system
GOTO EXIT

:ERRORCREATINGINSTALLER
echo. The installer could not be created.

:EXIT
del /q /f ..\build\dpinst-amd64.exe
del /q /f ..\build\dpinst-x86.exe
cd ..\support