@ECHO OFF
COPY clib\cmake1.bat bld\cmake1.bat
COPY clib\cmake2.bat bld\cmake2.bat
COPY clib\cmake3.bat bld\cmake3.bat
COPY clib\cmake.lst bld\cmake.lst
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
