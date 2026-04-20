@ECHO OFF
REM pixels.bat -- Build and run the CGA random-pixel demo.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling pixels ===
%BIN%\cc pixels -a -p
if errorlevel 1 goto exit

ECHO === Assembling pixels ===
%BIN%\asm pixels /p
if errorlevel 1 goto exit

ECHO === Linking pixels ===
%BIN%\ylink pixels.obj,%LIB%\clib.lib -e=pixels.exe -d=link.txt
if errorlevel 1 goto exit

:exit
if exist pixels.exe pixels
