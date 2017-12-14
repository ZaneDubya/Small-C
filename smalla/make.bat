SET BIN=..\bin
SET LIB=..\smalllib

%BIN%\cmit 80x86.mit -o

SET IN=asm1
%BIN%\cc  %IN%.c -m -a -p
if errorlevel 1 goto exit
%BIN%\asm %IN%
if errorlevel 1 goto exit
del %IN%.asm

SET IN=asm2
%BIN%\cc  %IN%.c -m -a -p
if errorlevel 1 goto exit
%BIN%\asm %IN%
if errorlevel 1 goto exit
del %IN%.asm

SET IN=asm3
%BIN%\cc  %IN%.c -m -a -p
if errorlevel 1 goto exit
%BIN%\asm %IN%
if errorlevel 1 goto exit
del %IN%.asm

SET IN=asm4
%BIN%\cc  %IN%.c -m -a -p
if errorlevel 1 goto exit
%BIN%\asm %IN%
if errorlevel 1 goto exit
del %IN%.asm

%BIN%\ylink asm1.obj,asm2.obj,asm3.obj,asm4.obj,80x86.obj,%LIB%\clib.lib,asm.lib
:exit

