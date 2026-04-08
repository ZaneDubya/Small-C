@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM enums
ECHO === Compiling enums ===
%BIN%\cc enums -a -p -w
if errorlevel 1 goto exit
%BIN%\asm enums /p
if errorlevel 1 goto exit
ECHO === Linking enums ===
%BIN%\ylink enums.obj,%LIB%\clib.lib -e=enums.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist enums.exe enums
