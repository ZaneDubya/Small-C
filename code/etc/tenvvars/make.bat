@ECHO OFF
REM make.bat -- Build and run tenvvars.exe from tenvvars.c
REM
REM PURPOSE
REM   This batch file performs a two-level test of the new angle-bracket
REM   include path mechanism introduced in ccpre.c:
REM
REM   LEVEL 1 -- Compile-time:
REM     tenvvars.c uses  #include [stdio.h]  (angle brackets, written here
REM     with [] to avoid DOS redirection).  If the new cc.exe finds
REM     C:\BIN\STDIO.H and compiles without an "open failure" message,
REM     the angle-bracket include path is working.
REM
REM   LEVEL 2 -- Runtime:
REM     tenvvars.exe tests dos_exepath(), dirpart(), and compose_path()
REM     against the mounted filesystem.  Run from C:\ETC (one level up)
REM     so that CWD != exe directory, matching the real compiler usage.
REM
REM PREREQUISITES
REM   * cc.exe already rebuilt from the updated ccpre.c  (C:\BIN\CC.EXE)
REM   * C:\BIN\STDIO.H exists (placed there as part of the compiler fix)
REM   * tenvvars.h exists in this directory (C:\ETC\TENVVARS\TENVVARS.H)
REM
REM Run this batch file from C:\ETC\TENVVARS inside DOSBox.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === [Level 1] Compiling tenvvars with angle-bracket #include ===
ECHO     (compile success = angle-bracket includes work in the new cc.exe)
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

REM -----------------------------------------------------------------------
REM Run the test from the PARENT directory (C:\ETC), not C:\ETC\TENVVARS.
REM
REM This separates CWD from the exe directory so that:
REM   Section 3 -- compose_path(exedir,"tenvvars.h") -- PASSES (file found)
REM   Section 4 -- fopen("tenvvars.h") bare          -- INFO  (CWD miss)
REM
REM If both sections found the same file, the test would prove nothing.
REM -----------------------------------------------------------------------
if not exist tenvvars.exe goto nodone
cd ..
tenvvars\tenvvars
cd tenvvars
:nodone
