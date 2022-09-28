@ECHO OFF
SET ARG1=%1
if "%ARG1%"=="tools" GOTO TOOLS

START ../build/win32.exe
GOTO LEAVE

:TOOLS
START ../build/asset_packer.exe
goto LEAVE

:LEAVE
