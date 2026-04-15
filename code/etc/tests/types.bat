@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM types
ECHO === Compiling types ===
%BIN%\cc types -a -p
if errorlevel 1 goto exit
%BIN%\asm types /p
if errorlevel 1 goto exit
ECHO === Linking types ===
%BIN%\ylink types.obj,%LIB%\clib.lib -e=types.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist types.exe types
