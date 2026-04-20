@ECHO OFF
COPY asmlib\asmmake1.bat bld\asmmake1.bat
COPY asmlib\asmmake2.bat bld\asmmake2.bat
COPY asmlib\asmmake3.bat bld\asmmake3.bat
COPY asmlib\asmmake.lst bld\asmmake.lst
CD bld
CALL clean.bat
CALL asmmake1.bat
if errorlevel 1 goto exit
CALL asmmake2.bat
if errorlevel 1 goto exit
CALL asmmake3.bat
if errorlevel 1 goto exit

ECHO Copy asm.lib to ..\ [y/n]
CHOICE /C:YN
if errorlevel 2 goto exit
copy asm.lib ..\asm.lib

:exit
CD ..
@ECHO ON
