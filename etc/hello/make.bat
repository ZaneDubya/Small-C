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

ECHO === Compiling hello2.c ===
%BIN%\cc  hello2 -a -p
if errorlevel 1 goto exit
%BIN%\asm hello2 /p
if errorlevel 1 goto exit

REM LINK
ECHO === Linking ===
REM ..\bin\link hello,hello,hello,..\smalllib\clib.lib
%BIN%\ylink hello.obj,hello2.obj,%LIB%\clib.lib -e=hello.exe -d=debug.txt
if errorlevel 1 goto exit

:exit
@ECHO ON
