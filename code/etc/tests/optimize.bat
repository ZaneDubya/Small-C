@ECHO OFF
SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling optimize ===
%BIN%\cc optimize -a -p -w
if errorlevel 1 goto exit
%BIN%\asm optimize /p
if errorlevel 1 goto exit
ECHO === Linking optimize ===
%BIN%\ylink optimize.obj,%LIB%\clib.lib -e=optimize.exe
if errorlevel 1 goto exit

:exit
if exist optimize.exe optimize