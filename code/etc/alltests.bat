@ECHO OFF
REM alltests.bat -- Run all Small-C tests in sequence.
REM Each test bat compiles, assembles, links, and runs its test program.
REM Artifacts (asm, obj, map) are preserved for inspection.

cd tests

ECHO ============================================================
ECHO  Running all Small-C tests
ECHO ============================================================

CALL enums.bat
IF ERRORLEVEL 1 GOTO fail

CALL locals.bat
IF ERRORLEVEL 1 GOTO fail

CALL longs.bat
IF ERRORLEVEL 1 GOTO fail

CALL mdarrays.bat
IF ERRORLEVEL 1 GOTO fail

CALL optimize.bat
IF ERRORLEVEL 1 GOTO fail

CALL print.bat
IF ERRORLEVEL 1 GOTO fail

CALL proto.bat
IF ERRORLEVEL 1 GOTO fail

CALL ptrcasts.bat
IF ERRORLEVEL 1 GOTO fail

CALL shifts.bat
IF ERRORLEVEL 1 GOTO fail

CALL statics.bat
IF ERRORLEVEL 1 GOTO fail

CALL structs.bat
IF ERRORLEVEL 1 GOTO fail

CALL typedef.bat
IF ERRORLEVEL 1 GOTO fail

CALL types.bat
IF ERRORLEVEL 1 GOTO fail

CALL unions.bat
IF ERRORLEVEL 1 GOTO fail

CALL voidptrs.bat
IF ERRORLEVEL 1 GOTO fail

ECHO ============================================================
ECHO  ALL TESTS PASSED.
ECHO ============================================================
GOTO done

:fail
ECHO ============================================================
ECHO  *** ONE OR MORE TESTS FAILED ***
ECHO ============================================================

:done
