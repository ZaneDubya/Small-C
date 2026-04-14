@ECHO OFF
REM preerror.bat -- Negative-compilation test for the #error directive.
REM
REM Expected behaviour: the compiler must exit with a non-zero errorlevel
REM because preerror.c contains an unconditional #error directive.
REM
REM doerror() calls exit(ERRCODE) which correctly passes the exit code to
REM DOS INT 21h/4Ch, so the compiler errorlevel is reliable here.
REM The assembler is deliberately NOT invoked -- it is not needed.
REM
REM NOTE: EXIT /B is not used -- that is CMD.EXE-only and closes DOSBox.

SET BIN=..\..\bin

REM Clean any leftover artifacts from a previous run.
if exist preerror.asm del preerror.asm
if exist preerror.obj del preerror.obj
if exist preerror.exe del preerror.exe

ECHO === Compiling preerror (expecting compiler failure) ===
%BIN%\cc preerror
if errorlevel 1 goto pass

ECHO FAIL: compiler exited with errorlevel 0; #error did not halt compilation.
goto done

:pass
ECHO PASS: compiler exited with non-zero errorlevel as expected from #error.

:done
@ECHO ON
