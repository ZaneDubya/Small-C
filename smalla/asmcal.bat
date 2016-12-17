cc  %1 -m -a -p
if errorlevel 1 goto exit
asm %1
if errorlevel 1 goto exit
del %1.asm
pause ^C to skip LINK step
link /MAP asm1 asm2 asm3 asm4 80X86,asm,asm,asm.lib \sc\clib
:exit
