@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM LONGTEST
ECHO === Compiling longtest ===
%BIN%\cc longtest -a -p -w
if errorlevel 1 goto exit
%BIN%\asm longtest /p
if errorlevel 1 goto exit
ECHO === Linking longtest ===
%BIN%\ylink longtest.obj,%LIB%\clib.lib -e=longtest.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist longtest.exe longtest
