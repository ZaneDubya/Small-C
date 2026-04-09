@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM PTRCASTS
ECHO === Compiling ptrcasts ===
%BIN%\cc ptrcasts -a -p -w
if errorlevel 1 goto exit
%BIN%\asm ptrcasts /p
if errorlevel 1 goto exit
ECHO === Linking ptrcasts ===
%BIN%\ylink ptrcasts.obj,%LIB%\clib.lib -e=ptrcasts.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist ptrcasts.exe ptrcasts
