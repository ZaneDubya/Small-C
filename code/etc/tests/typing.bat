@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM TYPING
ECHO === Compiling typing ===
%BIN%\cc typing -a -p
if errorlevel 1 goto exit
%BIN%\asm typing /p
if errorlevel 1 goto exit
ECHO === Linking typing ===
%BIN%\ylink typing.obj,%LIB%\clib.lib -e=typing.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist typing.exe typing
