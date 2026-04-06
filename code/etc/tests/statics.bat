@ECHO OFF
REM statics.bat -- Build and run statics.c
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM statics
ECHO === Compiling statics ===
%BIN%\cc statics -a -p -w
if errorlevel 1 goto exit
%BIN%\asm statics /p
if errorlevel 1 goto exit
ECHO === Linking statics ===
%BIN%\ylink statics.obj,%LIB%\clib.lib -e=statics.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist statics.exe statics
