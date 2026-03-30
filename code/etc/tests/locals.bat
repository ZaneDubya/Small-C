@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM LOCALS
ECHO === Compiling locals test ===
%BIN%\cc locals -a -p
if errorlevel 1 goto exit
%BIN%\asm locals /p
if errorlevel 1 goto exit
ECHO === Linking locals test ===
%BIN%\ylink locals.obj,%LIB%\clib.lib -e=locals.exe
if errorlevel 1 goto exit

REM CLEANUP
if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
