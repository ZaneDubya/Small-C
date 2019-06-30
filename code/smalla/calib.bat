cc %1 -m -a -p %2 %3
if errorlevel 1 goto end
masm %1;
if errorlevel 1 goto end
del %1.asm
lib asm -+%1;
del asm.bak
:end

