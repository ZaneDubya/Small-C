@ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling hello.c ===
%BIN%\cc  hello -a -p
if errorlevel 1 goto exit
%BIN%\asm hello /p
if errorlevel 1 goto exit

REM LINK
ECHO === Linking ===
REM ..\bin\link hello,hello,hello,..\smalllib\clib.lib
%BIN%\ylink hello.obj,%LIB%\clib.lib -e=hello.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
