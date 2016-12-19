@ECHO OFF
REM The -M switch lets us monitor progress by having the compiler write each function header to the screen.
REM     This also helps isolate errors to the functions containing them.
REM The -A switch causes the alarm to sound whenever an error is reported.
REM The -P switch causes the compiler to pause after reporting each error.
REM     An ENTER (carriage return) keystroke resumes execution.

ECHO === Compiling ===
..\bin\cc  ylink -a -p
if errorlevel 1 goto exit
..\bin\asm ylink /p
if errorlevel 1 goto exit

ECHO === Linking ===
..\bin\link ylink,ylink,ylink,..\smallc22\clib.lib
if errorlevel 1 goto exit

:exit
@ECHO ON
