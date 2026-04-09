
@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM STRUCTS
ECHO === Compiling structs test ===
%BIN%\cc structs -a -p -w
if errorlevel 1 goto exit
%BIN%\asm structs /p
if errorlevel 1 goto exit
ECHO === Linking structs test ===
REM %BIN%\link cc1 cc2 cc3 cc4,cc,cc,..\smalllib\clib.lib
%BIN%\ylink structs.obj,%LIB%\clib.lib -e=structs.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist structs.exe structs
