@ECHO OFF

SETLOCAL

REM If KODI_MIRROR is not set externally to this script, set it to the default mirror URL
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=http://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%

REM Locate the BuildDependencies directory, based on the path of this script
SET BUILD_DEPS_PATH=%~dp0
SET APP_PATH=%BUILD_DEPS_PATH%\..\..
SET TMP_PATH=%BUILD_DEPS_PATH%\scripts\tmp

REM Change to the BuildDependencies directory, if we're not there already
PUSHD %BUILD_DEPS_PATH%

REM Can't run rmdir and md back to back. access denied error otherwise.
IF EXIST lib rmdir lib /S /Q
IF EXIST include rmdir include /S /Q
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q

IF $%1$ == $$ (
  SET DL_PATH="%BUILD_DEPS_PATH%\downloads"
) ELSE (
  SET DL_PATH="%1"
)

SET WGET=%BUILD_DEPS_PATH%\bin\wget
SET ZIP=%BUILD_DEPS_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH%

md lib
md include
md %TMP_PATH%

cd scripts

SET FORMED_OK_FLAG=%TMP_PATH%\got-all-formed-packages
REM Trick to preserve console title
start /b /wait cmd.exe /c get_formed.cmd
IF NOT EXIST %FORMED_OK_FLAG% (
  ECHO ERROR: Not all formed packages are ready!
  ECHO.
  ECHO I tried to get the packages from %KODI_MIRROR%;
  ECHO if this download mirror seems to be having problems, try choosing another from
  ECHO the list on http://mirrors.kodi.tv/list.html, and setting %%KODI_MIRROR%% to
  ECHO point to it, like so:
  ECHO   C:\^> SET KODI_MIRROR=http://example.com/pub/xbmc/
  ECHO.
  ECHO Then, rerun this script.
  EXIT /B 101
)

rmdir %TMP_PATH% /S /Q

REM Restore the previous current directory
POPD

ENDLOCAL

EXIT /B 0
