@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM SUBSCR
ECHO === Compiling subscr ===
%BIN%\cc subscr -a -p
if errorlevel 1 goto exit
%BIN%\asm subscr /p
if errorlevel 1 goto exit
ECHO === Linking subscr ===
%BIN%\ylink subscr.obj,%LIB%\clib.lib -e=subscr.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist subscr.exe subscr
