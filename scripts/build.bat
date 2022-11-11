@ECHO off
CLS
SET ARG1=%1

cd /d "%~dp0"

if not defined DevEnvDir (
    call vcvarsall.bat x64
)

SET CompilerFlags=/diagnostics:column /nologo /Od /Zo /GR- /EHa- /Gm- /Fm /Zi /FC /WX /W4 /wd4505 /wd4201 /wd4100 /wd4189 /wd4127 /wd4244 /wd4701 /wd4267
SET LinkerFlags=/incremental:no /opt:ref 

SET win32_macro_defs=/D INTERNAL 
SET game_macro_defs=/D INTERNAL 

SET build_path=..\build
SET game_path=..\src\game\
SET tools_path=..\src\tools\

SET win32_entry_libs=user32.lib winmm.lib
SET game_libs=d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib 

SET game_exports=/EXPORT:game_loop

IF NOT EXIST %build_path% MKDIR %build_path%
PUSHD %build_path%

if "%ARG1%"=="tools" GOTO TOOLS
if "%ARG1%"=="win32" GOTO WIN32
ECHO -------------------------------------------------------------------------------------------------------------

DEL *.pdb > NUL 2> NUL
ECHO WAITING FOR PDB > lock.tmp

:GAME
ECHO Compiling game
REM GAME
cl %CompilerFlags% /MTd %game_macro_defs% %game_path%game.cpp /LD /link %LinkerFlags% %game_libs% %game_exports%
DEL lock.tmp
ECHO.
GOTO END


:WIN32
ECHO -------------------------------------------------------------------------------------------------------------
ECHO Compiling win32 api
REM WIN32_API
cl %CompilerFlags% %win32_macro_defs% %game_path%win32.cpp /link /SUBSYSTEM:windows %LinkerFlags% %win32_entry_libs%
ECHO.
GOTO END

:TOOLS
ECHO ------------------------------------------------------------------------------------------------------------
ECHO Compiling Asset Packer
REM Asset Packer
cl %CompilerFlags% /wd4996 /Fe:asset_packer %tools_path%asset_packer\main.cpp /link %LinkerFlags%
ECHO.

:END
popd
ECHO -------------------------------------------------------------------------------------------------------------- 

ECHO Generating source files tags
PUSHD %tools_path%
ctags -R
cscope -R -b
POPD
PUSHD %game_path%
ctags -R
cscope -R -b
POPD

POPD
ECHO -------------------------------------------------------------------------------------------------------------- 
ECHO.

