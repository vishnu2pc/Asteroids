@ECHO OFF
SET ARG1=%1
if "%ARG1%"=="tools" GOTO TOOLS

start remedybg.exe ../build/rmd_win32.rdbg
GOTO LEAVE

:TOOLS
start remedybg.exe ../build/rmd_asset_packer.rdbg
GOTO LEAVE

:LEAVE
