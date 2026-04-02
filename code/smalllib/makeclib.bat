@ECHO OFF
CD bld
CALL clean.bat
CALL cmake1.bat
if errorlevel 1 goto exit
CALL cmake2.bat
if errorlevel 1 goto exit
CALL cmake3.bat
if errorlevel 1 goto exit

ECHO Copy clib.lib to ..\ [y/n]
CHOICE /C:YN
if errorlevel 2 goto exit
copy clib.lib ..\clib.lib

:exit
CD ..
@ECHO ON
