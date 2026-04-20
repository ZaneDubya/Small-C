@ECHO OFF
REM tenvvars.bat -- Build and run tenvvars.exe from tenvvars.c
REM
REM PURPOSE
REM   This batch file performs a two-level test of the angle-bracket
REM   include path mechanism in ccpre.c:
REM
REM   LEVEL 1 -- Compile-time:
REM     tenvvars.c uses  #include <stdio.h>  (angle brackets).  If the
REM     new cc.exe finds C:\BIN\STDIO.H and compiles without an
REM     "open failure" message, the angle-bracket include path works.
REM
REM   LEVEL 2 -- Runtime:
REM     tenvvars.exe tests dos_exepath(), dirpart(), and compose_path()
REM     against the mounted filesystem.  Run from the parent directory
REM     so that CWD != exe directory, matching the real compiler usage.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === [Level 1] Compiling tenvvars with angle-bracket #include ===
%BIN%\cc tenvvars -a -p -w
if errorlevel 1 goto exit

ECHO === Assembling tenvvars ===
%BIN%\asm tenvvars /p
if errorlevel 1 goto exit

ECHO === Linking tenvvars ===
%BIN%\ylink tenvvars.obj,%LIB%\clib.lib -e=tenvvars.exe
if errorlevel 1 goto exit

:exit
@ECHO ON

REM Run from the PARENT directory (C:\ETC\TESTS\..) so CWD != exe directory.
if not exist tenvvars.exe goto nodone
cd ..
tests\tenvvars
cd tests
:nodone
