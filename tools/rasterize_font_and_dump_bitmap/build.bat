cls
@echo off

set CommonCompilerFlags=/std:c++17 -nologo -WX -wd4700 -Zi /FC 
set CommonLinkerFlags= /SUBSYSTEM:CONSOLE

@mkdir bin
@pushd bin

cl %CommonCompilerFlags% ../src/main.cpp /I ../include /link %CommonLinkerFlags% 

@popd