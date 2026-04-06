@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM MDARRAYS
ECHO === Compiling mdarrays test ===
%BIN%\cc mdarrays -a -p -w
if errorlevel 1 goto exit
%BIN%\asm mdarrays /p
if errorlevel 1 goto exit
ECHO === Linking mdarrays test ===
%BIN%\ylink mdarrays.obj,%LIB%\clib.lib -e=mdarrays.exe
if errorlevel 1 goto exit

REM CLEANUP
if exist *.asm del *.asm
if exist *.obj del *.obj
if exist *.map del *.map

:exit
@ECHO ON
if exist mdarrays.exe mdarrays
