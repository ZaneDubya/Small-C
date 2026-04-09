@ECHO OFF
REM print.bat -- Build and run the printf smoke test.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling print ===
%BIN%\cc print -a -p -w
if errorlevel 1 goto exit

ECHO === Assembling print ===
%BIN%\asm print /p
if errorlevel 1 goto exit

ECHO === Linking print ===
%BIN%\ylink print.obj,%LIB%\clib.lib -e=print.exe
if errorlevel 1 goto exit

:exit
if exist print.exe print
