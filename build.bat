cls
@echo off

set CommonCompilerFlags=/std:c++17 -nologo -WX -wd4700 -Zi /FC 
set CommonLinkerFlags=shell32.lib d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib SDL2.lib SDL2main.lib /SUBSYSTEM:CONSOLE

@mkdir bin
@pushd bin

cl %CommonCompilerFlags% ../src/main.cpp /I ../include /link %CommonLinkerFlags% /LIBPATH:../lib

@popd
