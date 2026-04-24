@ECHO OFF

SET BIN=..\bin
SET LIB=..\smalllib

%BIN%\cmit 80x86.mit -3 -o

ECHO === Compiling asm1.c ===
SET IN=asm1
%BIN%\cc  %IN%.c -a -p -w
if errorlevel 1 goto exit
%BIN%\asm %IN% /p
if errorlevel 1 goto exit

ECHO === Compiling asm2.c ===
SET IN=asm2
%BIN%\cc  %IN%.c -a -p -w
if errorlevel 1 goto exit
%BIN%\asm %IN% /p
if errorlevel 1 goto exit

ECHO === Compiling asm3.c ===
SET IN=asm3
%BIN%\cc  %IN%.c -a -p -w
if errorlevel 1 goto exit
%BIN%\asm %IN% /p
if errorlevel 1 goto exit

ECHO === Compiling asm4.c ===
SET IN=asm4
%BIN%\cc  %IN%.c -a -p -w
if errorlevel 1 goto exit
%BIN%\asm %IN% /p
if errorlevel 1 goto exit

ECHO === Linking ===
%BIN%\ylink asm1.obj,asm2.obj,asm3.obj,asm4.obj,80x86.obj,%LIB%\asm.lib,%LIB%\clib.lib -e=asm.exe
if errorlevel 1 goto exit

ECHO Copy asm.exe to %BIN%? [y/n]
CHOICE /C:YN
if errorlevel 2 goto exit
copy asm.exe %BIN%\asm.exe

:exit
@ECHO ON

