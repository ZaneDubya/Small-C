@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling optimize ===
%BIN%\cc optimize -a -p
if errorlevel 1 goto exit
%BIN%\asm optimize /p
if errorlevel 1 goto exit
ECHO === Linking optimize ===
%BIN%\ylink optimize.obj,%LIB%\clib.lib -e=optimize.exe
if errorlevel 1 goto exit

:exit
if exist optimize.exe optimize