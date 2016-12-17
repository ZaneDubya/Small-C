ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

REM CC1
..\bin\cc  cc1 -a -p
if errorlevel 1 goto exit
..\bin\asm cc1 /p
if errorlevel 1 goto exit

REM CC2
..\bin\cc  cc2 -a -p
if errorlevel 1 goto exit
..\bin\asm cc2 /p
if errorlevel 1 goto exit

REM CC3
..\bin\cc  cc3 -a -p
if errorlevel 1 goto exit
..\bin\asm cc3 /p
if errorlevel 1 goto exit

REM CC4
..\bin\cc  cc4 -a -p
if errorlevel 1 goto exit
..\bin\asm cc4 /p
if errorlevel 1 goto exit

REM LINK
..\bin\link cc1 cc2 cc3 cc4,cc,cc,clib.lib
if errorlevel 1 goto exit

REM CLEANUP
del *.asm
del *.obj
copy CC.EXE ..\bin\CC.EXE
del CC.EXE

:exit
ECHO ON
