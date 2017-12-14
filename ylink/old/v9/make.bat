@ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling ===
%BIN%\cc  link -a -p
if errorlevel 1 goto exit
%BIN%\asm link /p
if errorlevel 1 goto exit

ECHO === Linking ===
%BIN%\ylink link.obj,%LIB%\clib.lib -e=ylink.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
