@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM TYPEDEFTEST
ECHO === Compiling typedef ===
%BIN%\cc typedef -a -p -w
if errorlevel 1 goto exit
%BIN%\asm typedef /p
if errorlevel 1 goto exit
ECHO === Linking typedef ===
%BIN%\ylink typedef.obj,%LIB%\clib.lib -e=typedef.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist typedef.exe typedef
