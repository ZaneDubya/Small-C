@ECHO OFF
REM pretest.bat -- Build and run the Small-C preprocessor test suite.
REM
REM The -A switch sounds the alarm when an error is reported.
REM The -P switch pauses after each error (ENTER resumes).
REM The -W switch enables all optional compiler warnings.

SET BIN=..\..\bin
SET LIB=..\..\smalllib

ECHO === Compiling preprocessor tests ===
%BIN%\cc pretest -a -p -w
if errorlevel 1 goto exit

ECHO === Assembling preprocessor tests ===
%BIN%\asm pretest /p
if errorlevel 1 goto exit

ECHO === Linking preprocessor tests ===
%BIN%\ylink pretest.obj,%LIB%\clib.lib -e=pretest.exe
if errorlevel 1 goto exit

:exit
@ECHO ON
if exist pretest.exe pretest
