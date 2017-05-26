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

%BIN%\cc  link_omf -a -p
if errorlevel 1 goto exit
%BIN%\asm link_omf /p
if errorlevel 1 goto exit

%BIN%\cc  link_lib -a -p
if errorlevel 1 goto exit
%BIN%\asm link_lib /p
if errorlevel 1 goto exit

%BIN%\cc  link_lnk -a -p
if errorlevel 1 goto exit
%BIN%\asm link_lnk /p
if errorlevel 1 goto exit

ECHO === Linking ===
%BIN%\link link link_omf link_lib link_lnk,,,%LIB%\clib.lib
if errorlevel 1 goto exit

:exit
@ECHO ON
