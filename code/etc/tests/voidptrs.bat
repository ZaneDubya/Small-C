@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM VOIDPTRS
ECHO === Compiling voidptrs ===
%BIN%\cc voidptrs -a -p -w
if errorlevel 1 goto exit
%BIN%\asm voidptrs /p
if errorlevel 1 goto exit
ECHO === Linking voidptrs ===
%BIN%\ylink voidptrs.obj,%LIB%\clib.lib -e=voidptrs.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist voidptrs.exe voidptrs
