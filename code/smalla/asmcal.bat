SET BIN=..\bin
SET LIB=..\smalllib

%BIN%\cc  %1 -m -a -p
if errorlevel 1 goto exit
%BIN%\asm %1
if errorlevel 1 goto exit
del %1.asm
pause ^C to skip LINK step
%BIN%\mslink asm1 asm2 asm3 asm4 80X86,asm,asm,asm.lib %LIB%\clib.lib
:exit
