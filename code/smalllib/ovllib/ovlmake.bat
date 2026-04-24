@ECHO OFF
REM ovlmake.bat -- Build ovllib.lib from source
REM Run from the ovllib\ directory (inside DOSBox).
REM Requires CC.EXE, ASM.EXE, and YLINK.EXE in ..\..\..\bin\

SET BIN=..\..\bin

REM ---- Compile C sources to ASM ----
%BIN%\CC ovlmgr.c  >OVLMGR.ASM  -a -p -w
if errorlevel 1 goto fail
%BIN%\CC ovlinit.c >OVLINIT.ASM -a -p -w
if errorlevel 1 goto fail
%BIN%\CC ovlchk.c  >OVLCHK.ASM -a -p -w
if errorlevel 1 goto fail
%BIN%\CC ovlload.c >OVLLOAD.ASM -a -p -w
if errorlevel 1 goto fail

REM ---- Assemble all modules ----
%BIN%\ASM OVLMGR.ASM  /p
if errorlevel 1 goto fail
%BIN%\ASM OVLINIT.ASM /p
if errorlevel 1 goto fail
%BIN%\ASM OVLCHK.ASM /p
if errorlevel 1 goto fail
%BIN%\ASM OVLLOAD.ASM /p
if errorlevel 1 goto fail
%BIN%\ASM OVLCOPY.ASM /p
if errorlevel 1 goto fail
%BIN%\ASM OVLGETCS.ASM /p
if errorlevel 1 goto fail

REM ---- Link into a library ----
%BIN%\YLINK -l=ovlmake.lst -e=ovllib.lib
if errorlevel 1 goto fail

ECHO ovllib.lib built successfully.
GOTO done

:fail
ECHO Build FAILED.
:done
@ECHO ON
