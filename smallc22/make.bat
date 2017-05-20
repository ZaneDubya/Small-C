@ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

REM CC1
ECHO === Compiling cc1.c ===
..\bin\cc  cc1 -a -p
if errorlevel 1 goto exit
..\bin\asm cc1 /p
if errorlevel 1 goto exit

REM CC2
ECHO === Compiling cc2.c ===
..\bin\cc  cc2 -a -p
if errorlevel 1 goto exit
..\bin\asm cc2 /p
if errorlevel 1 goto exit

REM CC3
ECHO === Compiling cc3.c ===
..\bin\cc  cc3 -a -p
if errorlevel 1 goto exit
..\bin\asm cc3 /p
if errorlevel 1 goto exit

REM CC4
ECHO === Compiling cc4.c ===
..\bin\cc  cc4 -a -p
if errorlevel 1 goto exit
..\bin\asm cc4 /p
if errorlevel 1 goto exit

REM LINK
ECHO === Linking SmallC Compiler ===
..\bin\link cc1 cc2 cc3 cc4,cc,cc,..\smalllib\clib.lib
if errorlevel 1 goto exit

REM CLEANUP
del *.asm
del *.obj
del *.map

copy CC.EXE ..\bin\CC.EXE
del CC.EXE

:exit
@ECHO ON
