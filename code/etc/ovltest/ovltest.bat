@ECHO OFF
REM ovltest.bat -- Build and run the overlay demo.
REM
REM Two overlay windows:
REM   Window 1 (draw): cga / ega / vga  -- one drawing mode loaded at a time
REM   Window 2 (sound): sndc4 / snde4 / sndg4 -- one OPL2 note loaded at a time

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling ===
%BIN%\cc ovltest  -a -p
if errorlevel 1 goto exit
%BIN%\cc draw_cga -a -p
if errorlevel 1 goto exit
%BIN%\cc draw_ega -a -p
if errorlevel 1 goto exit
%BIN%\cc draw_vga -a -p
if errorlevel 1 goto exit
%BIN%\cc snd_c4   -a -p
if errorlevel 1 goto exit
%BIN%\cc snd_e4   -a -p
if errorlevel 1 goto exit
%BIN%\cc snd_g4   -a -p
if errorlevel 1 goto exit

ECHO === Assembling ===
%BIN%\asm ovltest  /p
if errorlevel 1 goto exit
%BIN%\asm draw_cga /p
if errorlevel 1 goto exit
%BIN%\asm draw_ega /p
if errorlevel 1 goto exit
%BIN%\asm draw_vga /p
if errorlevel 1 goto exit
%BIN%\asm snd_c4   /p
if errorlevel 1 goto exit
%BIN%\asm snd_e4   /p
if errorlevel 1 goto exit
%BIN%\asm snd_g4   /p
if errorlevel 1 goto exit

ECHO === Linking ===
%BIN%\ylink @ovltest.rsp
if errorlevel 1 goto exit

:exit
if exist ovltest.exe ovltest
