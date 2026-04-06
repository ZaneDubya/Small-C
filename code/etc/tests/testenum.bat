@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM TESTENUM
ECHO === Compiling testenum ===
%BIN%\cc testenum -a -p -w
if errorlevel 1 goto exit
%BIN%\asm testenum /p
if errorlevel 1 goto exit
ECHO === Linking testenum ===
%BIN%\ylink testenum.obj,%LIB%\clib.lib -e=testenum.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist testenum.exe testenum
