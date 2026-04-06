@ECHO OFF
REM proto.bat -- Build and run the function prototype test.
REM The -A switch sounds the alarm on errors; -P pauses after each error.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling proto ===
%BIN%\cc proto -a -p
if errorlevel 1 goto exit

ECHO === Assembling proto ===
%BIN%\asm proto /p
if errorlevel 1 goto exit

ECHO === Linking proto ===
%BIN%\ylink proto.obj,%LIB%\clib.lib -e=proto.exe
if errorlevel 1 goto exit

ECHO === Running proto ===
proto.exe

REM CLEANUP
if exist *.obj del *.obj
if exist *.map del *.map

:exit
