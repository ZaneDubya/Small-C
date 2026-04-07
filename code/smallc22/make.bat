@ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\bin
SET LIB=..\smalllib

REM CC1
ECHO === Compiling cc1.c ===
%BIN%\cc  cc1 -a -p
if errorlevel 1 goto exit
%BIN%\asm cc1 /p
if errorlevel 1 goto exit

REM CC2
ECHO === Compiling cc2.c ===
%BIN%\cc  cc2 -a -p
if errorlevel 1 goto exit
%BIN%\asm cc2 /p
if errorlevel 1 goto exit

REM CC3
ECHO === Compiling cc3.c ===
%BIN%\cc  cc3 -a -p
if errorlevel 1 goto exit
%BIN%\asm cc3 /p
if errorlevel 1 goto exit

REM CC4
ECHO === Compiling cc4.c ===
%BIN%\cc  cc4 -a -p
if errorlevel 1 goto exit
%BIN%\asm cc4 /p
if errorlevel 1 goto exit

REM CCSTRUCTS
ECHO === Compiling ccstruct.c ===
%BIN%\cc  ccstruct -a -p
if errorlevel 1 goto exit
%BIN%\asm ccstruct /p
if errorlevel 1 goto exit

REM CCENUM
ECHO === Compiling ccenum.c ===
%BIN%\cc  ccenum -a -p
if errorlevel 1 goto exit
%BIN%\asm ccenum /p
if errorlevel 1 goto exit

REM LINK
ECHO === Linking SmallC Compiler ===
REM %BIN%\link cc1 cc2 cc3 cc4,cc,cc,..\smalllib\clib.lib
%BIN%\ylink cc1.obj,cc2.obj,cc3.obj,cc4.obj,ccstruct.obj,ccenum.obj,%LIB%\clib.lib -e=cc.exe -d=debug.txt
if errorlevel 1 goto exit

ECHO Copy cc.exe to %BIN%? [y/n]
CHOICE /C:YN
if errorlevel 2 goto exit
copy cc.exe %BIN%\cc.exe

:cleanup
REM CLEANUP
REM if exist *.asm del *.asm
REM if exist *.obj del *.obj
REM if exist *.map del *.map
REM copy CC.EXE %BIN%\CC.EXE

:exit
@ECHO ON
