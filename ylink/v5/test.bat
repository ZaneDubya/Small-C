SET LIB=..\..\smalllib

REM link work\cc1.obj,work\cc2.obj,work\cc3.obj,work\cc4.obj,%LIB%\clib.lib
link work\hello.obj,%LIB%\clib.lib