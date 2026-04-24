@ECHO OFF
SET BIN=..\..\bin

REM Assemble ASM to OBJ
%BIN%\asm OVLMGR.ASM /p
%BIN%\asm OVLINIT.ASM /p
%BIN%\asm OVLLOAD.ASM /p
%BIN%\asm OVLCHK.ASM /p
%BIN%\asm OVLCOPY.ASM /p
%BIN%\asm OVLGETCS.ASM /p
