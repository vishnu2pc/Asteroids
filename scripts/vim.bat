@ECHO OFF
if not defined DevEnvDir (
    call vcvarsall.bat x64
)

PUSHD ..\src\game\
call gvim todo.txt
POPD
