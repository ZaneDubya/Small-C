@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM knr
ECHO === Compiling knr ===
%BIN%\cc knr -a -p
if errorlevel 1 goto exit
%BIN%\asm knr /p
if errorlevel 1 goto exit
ECHO === Linking knr ===
%BIN%\ylink knr.obj,%LIB%\clib.lib -e=knr.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
