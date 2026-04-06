@ECHO OFF
REM prntest.bat -- Build and run the printf smoke test.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling prntest ===
%BIN%\cc prntest -a -p
if errorlevel 1 goto exit

ECHO === Assembling prntest ===
%BIN%\asm prntest /p
if errorlevel 1 goto exit

ECHO === Linking prntest ===
%BIN%\ylink prntest.obj,%LIB%\clib.lib -e=prntest.exe
if errorlevel 1 goto exit

ECHO === Running prntest ===
prntest.exe

if exist *.obj del *.obj
if exist *.map del *.map

:exit
