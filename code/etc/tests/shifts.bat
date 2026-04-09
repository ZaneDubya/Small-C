@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM SHIFTS
ECHO === Compiling shifts ===
%BIN%\cc shifts -a -p -w
if errorlevel 1 goto exit
%BIN%\asm shifts /p
if errorlevel 1 goto exit
ECHO === Linking shifts ===
%BIN%\ylink shifts.obj,%LIB%\clib.lib -e=shifts.exe
if errorlevel 1 goto exit

REM CLEANUP
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist shifts.exe shifts
