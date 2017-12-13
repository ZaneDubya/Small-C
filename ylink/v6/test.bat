SET LIB=..\..\smalllib

REM ylink work\cc1.obj,work\cc2.obj,work\cc3.obj,work\cc4.obj,%LIB%\clib.lib -e=y-cc.exe
REM ..\..\bin\link work\cc1.obj work\cc2.obj work\cc3.obj work\cc4.obj,cc,,%LIB%\clib.lib
ylink work\hello.obj,%LIB%\clib.lib -e=hello.exe