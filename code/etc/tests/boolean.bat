@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling boolean ===
%BIN%\cc boolean -a -p
if errorlevel 1 goto exit
%BIN%\asm boolean /p
if errorlevel 1 goto exit
ECHO === Linking boolean ===
%BIN%\ylink boolean.obj,%LIB%\clib.lib -e=boolean.exe
if errorlevel 1 goto exit

:exit
if exist boolean.exe boolean
