
@ECHO OFF
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

REM UNIONS
ECHO === Compiling unions test ===
%BIN%\cc unions -a -p
if errorlevel 1 goto exit
%BIN%\asm unions /p
if errorlevel 1 goto exit
ECHO === Linking unions test ===
%BIN%\ylink unions.obj,%LIB%\clib.lib -e=unions.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist unions.exe unions
