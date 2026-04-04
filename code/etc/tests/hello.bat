@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM HELLO
ECHO === Compiling hello ===
%BIN%\cc hello1 -a -p
if errorlevel 1 goto exit
%BIN%\asm hello1 /p
if errorlevel 1 goto exit

REM HELLO2
ECHO === Compiling hello2 ===
%BIN%\cc hello2 -a -p
if errorlevel 1 goto exit
%BIN%\asm hello2 /p
if errorlevel 1 goto exit

ECHO === Linking hello ===
%BIN%\ylink hello1.obj,hello2.obj,%LIB%\clib.lib -e=hello.exe
if errorlevel 1 goto exit

REM CLEANUP
REM if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist hello.exe hello
