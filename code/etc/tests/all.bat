@ECHO OFF
REM all.bat -- Run all Small-C tests in sequence.
REM Each test bat compiles, links, and runs its test program.
REM If a test bat exits with a non-zero errorlevel, execution stops.

SET TESTS_FAILED=0

ECHO ============================================================
ECHO  Running all Small-C tests
ECHO ============================================================

CALL locals.bat
if errorlevel 1 set TESTS_FAILED=1

CALL longs.bat
if errorlevel 1 set TESTS_FAILED=1

CALL mdarrays.bat
if errorlevel 1 set TESTS_FAILED=1

CALL print.bat
if errorlevel 1 set TESTS_FAILED=1

CALL proto.bat
if errorlevel 1 set TESTS_FAILED=1

CALL statics.bat
if errorlevel 1 set TESTS_FAILED=1

CALL structs.bat
if errorlevel 1 set TESTS_FAILED=1

CALL enums.bat
if errorlevel 1 set TESTS_FAILED=1

CALL types.bat
if errorlevel 1 set TESTS_FAILED=1

CALL voidptrs.bat
if errorlevel 1 set TESTS_FAILED=1

ECHO ============================================================
if "%TESTS_FAILED%"=="0" (
    ECHO  ALL TESTS PASSED.
) else (
    ECHO  *** ONE OR MORE TESTS FAILED ***
)
ECHO ============================================================
@ECHO ON
