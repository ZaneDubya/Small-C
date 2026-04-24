@ECHO OFF
SET BIN=..\..\bin

REM Link all objects into ovllib.lib
%BIN%\ylink -l=ovlmake.lst -e=ovllib.lib
