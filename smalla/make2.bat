SET BIN=..\bin
SET LIB=..\smalllib

%BIN%\ylink asm1.obj,asm2.obj,asm3.obj,asm4.obj,80x86.obj,%LIB%\clib.lib,%LIB%\asm.lib -d=debug.txt
:exit

