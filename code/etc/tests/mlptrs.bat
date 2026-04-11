@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM MLPTRS
ECHO === Compiling mlptrs ===
%BIN%\cc mlptrs -a -p -w
if errorlevel 1 goto exit
%BIN%\asm mlptrs /p
if errorlevel 1 goto exit
ECHO === Linking mlptrs ===
%BIN%\ylink mlptrs.obj,%LIB%\clib.lib -e=mlptrs.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist mlptrs.exe mlptrs
