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

REM %BIN%\cc  link_omf -a -p
REM if errorlevel 1 goto exit
REM %BIN%\asm link_omf /p
REM if errorlevel 1 goto exit

REM %BIN%\cc  link_lib -a -p
REM if errorlevel 1 goto exit
REM %BIN%\asm link_lib /p
REM if errorlevel 1 goto exit

REM %BIN%\cc  link_lnk -a -p
REM if errorlevel 1 goto exit
REM %BIN%\asm link_lnk /p
REM if errorlevel 1 goto exit

ECHO === Linking ===
%BIN%\link link,,,%LIB%\clib.lib
if errorlevel 1 goto exit

:exit
@ECHO ON
