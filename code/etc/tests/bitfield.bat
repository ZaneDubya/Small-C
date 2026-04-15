@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM bitfield
ECHO === Compiling bitfield test ===
%BIN%\cc bitfield -a -p
if errorlevel 1 goto exit
%BIN%\asm bitfield /p
if errorlevel 1 goto exit
ECHO === Linking bitfield test ===
%BIN%\ylink bitfield.obj,%LIB%\clib.lib -e=bitfield.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist bitfield.exe bitfield
