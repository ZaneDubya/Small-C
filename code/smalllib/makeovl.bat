@ECHO OFF
COPY ovllib\ovlmake1.bat bld\ovlmake1.bat
COPY ovllib\ovlmake2.bat bld\ovlmake2.bat
COPY ovllib\ovlmake3.bat bld\ovlmake3.bat
COPY ovllib\ovlmake.lst bld\ovlmake.lst
CD bld
CALL clean.bat
CALL ovlmake1.bat
if errorlevel 1 goto exit
CALL ovlmake2.bat
if errorlevel 1 goto exit
CALL ovlmake3.bat
if errorlevel 1 goto exit

ECHO Copy ovllib.lib to ..\ [y/n]
CHOICE /C:YN
if errorlevel 2 goto exit
copy ovllib.lib ..\ovllib.lib

:exit
CD ..
@ECHO ON
